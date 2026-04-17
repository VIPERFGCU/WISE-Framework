import os, json, asyncio, logging, threading, time
from typing import Set
from datetime import datetime, timezone
from queue import Queue, Empty

from fastapi import FastAPI, WebSocket, WebSocketDisconnect, Body, Request, Query
from fastapi.middleware.cors import CORSMiddleware
from starlette.responses import Response

from influxdb_client import Point, WritePrecision
from influxdb_client.client.write_api import SYNCHRONOUS
from paho.mqtt import client as paho

from app.api.v1 import devices, ingest, query, health, control, preview
from app.api.v1 import auth
from app.core.config import settings
from app.deps import get_influx_client
from app.services.influx import write_accel_point, write_heartbeat_sync
from app.schemas.sensor import SensorReading, DeviceHeartbeat
from app.mqtt import mqtt_client, publish_control, set_connection_event
from app.services import devices as devices_svc

# -------------------------------------------------------------------
# App setup
# -------------------------------------------------------------------
app = FastAPI(
    title="Sensor Backend", 
    version="0.2.0",
    docs_urls="/docs",
    redoc_url="/redoc",
    openapi_url="/openapi.json",
)

# Logging
LOG_LEVEL = settings.api_log_level.upper()
logging.basicConfig(level=LOG_LEVEL)
log = logging.getLogger("sensor-backend")

# -------------------------------------------------------------------
# CORS (explicitly allow x-api-key and use configured origins)
# -------------------------------------------------------------------
raw_origins = settings.cors_allow_origins
if isinstance(raw_origins, str):
    # support comma-separated env like: "http://wise-net.io:5173,http://127.0.0.1:5173"
    origins = [o.strip() for o in raw_origins.split(",") if o.strip()]
elif isinstance(raw_origins, (list, tuple)):
    origins = list(raw_origins)
else:
    origins = ["http://wise-net.io:5173", "http://127.0.0.1:5173"]

app.add_middleware(
    CORSMiddleware,
    allow_origins=origins,                        # <-- use parsed origins
    allow_credentials=True,
    allow_methods=["*"],
    # include wildcard and explicit casings to satisfy some browsers/proxies
    allow_headers=["*", "x-api-key", "X-API-Key", "Authorization", "Content-Type", "Accept", "Origin"],
    expose_headers=["*"],
    max_age=600,
)

# Catch-all OPTIONS so preflights never hit route dependencies
@app.options("/{rest_of_path:path}")
async def cors_preflight_ok(rest_of_path: str) -> Response:
    # CORSMiddleware will add Access-Control-Allow-* headers
    return Response(status_code=200)


@app.get("/api/v1/debug/cors")
async def debug_cors(request: Request, origin: str | None = Query(default=None)):
    requested_origin = origin or request.headers.get("origin")
    wildcard = "*" in origins
    is_allowed = bool(wildcard or (requested_origin and requested_origin in origins))
    return {
        "requested_origin": requested_origin,
        "allowed": is_allowed,
        "wildcard": wildcard,
        "configured_origins": origins,
    }

# -------------------------------------------------------------------
# Routers
# -------------------------------------------------------------------
app.include_router(health.router)
app.include_router(ingest.router)
app.include_router(query.router)
app.include_router(devices.router)
app.include_router(control.router)
app.include_router(preview.router)
app.include_router(auth.router)

# -------------------------------------------------------------------
# Debug / Manual Control
# -------------------------------------------------------------------
@app.post("/api/v1/debug/start")
async def debug_start_sensor(device_id: str = "dev-sensor-001"):
    """
    Manually triggers the sensor to start via the backend's MQTT logic.
    This effectively tells the ESP32 to set 'streaming = true'.
    """
    if publish_control(device_id, {"cmd": "START", "rate_hz": 10}):
        return {"status": "command sent"}
    return {"status": "error", "message": "MQTT publish failed"}

# -------------------------------------------------------------------
# WebSocket broadcast hub (simple in-memory)
# -------------------------------------------------------------------
active_clients: Set[WebSocket] = set()

def _ws_count() -> int:
    return len(active_clients)

@app.websocket("/api/v1/stream")
async def stream(ws: WebSocket):
    await ws.accept()
    active_clients.add(ws)
    log.info(f"[WS] client connected; total={_ws_count()}")
    try:
        # Keep-alive loop; we don't expect inbound messages from the UI
        while True:
            # Wait for ping or ignore incoming text (optional)
            try:
                _ = await ws.receive_text()
            except Exception:
                await asyncio.sleep(0.1)
    except WebSocketDisconnect:
        pass
    finally:
        active_clients.discard(ws)
        log.info(f"[WS] client disconnected; total={_ws_count()}")


# -------------------------------------------------------------------
# MQTT Consumer -> InfluxDB + WebSocket fan-out
# -------------------------------------------------------------------
MQTT_HOST = os.getenv("MQTT_HOST", "mosquitto")
MQTT_PORT = int(os.getenv("MQTT_PORT", "1883"))
MQTT_TOPIC = os.getenv("MQTT_TOPIC", "devices/+/data")
INFLUX_BUCKET = settings.influx_bucket

# thread-safe queue to hand off messages to the asyncio loop
_mqtt_queue: Queue[str] = Queue()
ENABLE_MQTT = os.getenv("ENABLE_MQTT", "true").lower() == "true"

def _paho_on_connect(client, userdata, flags, reason_code, properties=None):
    if reason_code == 0:
        log.info(f"[MQTT] Connected successfully, subscribing to devices/# and mesh/#")
        client.subscribe("devices/#")
        client.subscribe("mesh/#")
        # Signal that we're connected
        if hasattr(_paho_on_connect, "_event"):
            _paho_on_connect._event.set()
    else:
        log.error(f"[MQTT] Connection failed with code {reason_code}")

def _paho_on_message(client, userdata, msg):
    try:
        # Standard Paho msg objects work similarly, but wrap in try/except for safety
        payload = msg.payload.decode()
        log.info(f"[MQTT TRACE] Topic: {msg.topic} | Payload: {payload}")
        _mqtt_queue.put_nowait((msg.topic, payload))
    except Exception as e:
        log.warning(f"[MQTT] Processing failed: {e}")

def _mqtt_thread():
    # Create an event for signaling connection
    connection_event = threading.Event()
    _paho_on_connect._event = connection_event  # type: ignore
    set_connection_event(connection_event)

    mqtt_client.on_connect = _paho_on_connect
    mqtt_client.on_message = _paho_on_message

    # Attempt connect and use a non-blocking loop so FastAPI is not blocked
    try:
        log.info(f"[MQTT] Attempting connection to {MQTT_HOST}:{MQTT_PORT}")
        mqtt_client.connect_async(MQTT_HOST, MQTT_PORT, keepalive=60)
        # Use loop_start() to run network loop in background thread
        mqtt_client.loop_start()
    except Exception as e:
        log.warning(f"[MQTT] Initial connect failed: {e}")

    # Keep the thread alive (loop_start handles MQTT I/O)
    while True:
        time.sleep(1)

# -------------------------------------------------------------------
# WebSocket and Mesh Protocol Helpers
# -------------------------------------------------------------------
async def _safe_send(ws: WebSocket, msg: dict):
    """Safely send a WebSocket message, handling closed connections."""
    try:
        await ws.send_json(msg)
    except Exception as e:
        log.debug(f"[WS] Send failed: {e}")

def _process_mesh_data_message(data: dict, device_mac: str) -> list[SensorReading]:
    """
    Convert mesh protocol batched data to individual SensorReading objects.
    
    Input format (from firmware):
    {
        "id": "device_mac_address",
        "type": "data",
        "t_start": 1707432851000000,  # microseconds since epoch
        "interval": 20000,             # microseconds between samples
        "vals": [[x1,y1,z1], [x2,y2,z2], ...]
    }
    
    Output: List of SensorReading objects with individual timestamps
    """
    readings = []
    try:
        t_start_us = data.get("t_start", 0)
        interval_us = data.get("interval", 0)
        vals = data.get("vals", [])
        device_id = data.get("device_id") or data.get("id", device_mac)
        # Normalize to string to prevent duplicates (e.g., 1 vs "1")
        device_id = str(device_id)
        
        for idx, (x, y, z) in enumerate(vals):
            # Calculate timestamp for this sample
            ts_us = t_start_us + (idx * interval_us)
            ts = datetime.fromtimestamp(ts_us / 1_000_000, tz=timezone.utc)
            
            reading = SensorReading(
                device_id=device_id,
                x=float(x),
                y=float(y),
                z=float(z),
                ts=ts
            )
            readings.append(reading)
    except Exception as e:
        log.error(f"[Mesh] Failed to process data message: {e}")
    
    return readings

def _process_mesh_assignment(data: dict) -> str:
    """
    Handle device assignment message from mesh protocol.
    
    Input format:
    {
        "id": "device_mac_address",
        "type": "client_assignment"
    }
    
    Returns: The device_id (MAC address)
    """
    device_id = data.get("device_id") or data.get("id")
    if device_id:
        # Normalize to string to prevent duplicates (e.g., 1 vs "1")
        device_id = str(device_id)
        # Auto-register the device
        try:
            devices_svc.register(device_id, label=device_id, notes="auto-registered-mesh-protocol")
            log.info(f"[Mesh] Auto-registered device: {device_id}")
        except Exception as e:
            log.error(f"[Mesh] Failed to register device {device_id}: {e}")
    return device_id

# -------------------------------------------------------------------
async def _drain_mqtt_queue():
    """Consume messages placed on the thread-safe queue by the Paho callbacks.
    Processes both devices/* (standard protocol) and mesh/* (batch protocol) topics.
    """
    while True:
        try:
            topic, payload_raw = _mqtt_queue.get_nowait()
        except Empty:
            await asyncio.sleep(0.05)
            continue

        try:
            data = json.loads(payload_raw)
            parts = topic.split("/")
            device_id = parts[1] if len(parts) >= 2 else "unknown"

            # ===== DEVICES/* PROTOCOL =====
            # Handle sensor data messages (devices/{id}/data)
            if topic.startswith("devices/") and topic.endswith("/data"):
                data["device_id"] = str(data.get("device_id") or device_id)
                ts_str = data.get("ts")
                ts = (datetime.fromisoformat(ts_str.replace("Z", "+00:00"))
                      if ts_str else datetime.now(timezone.utc))
                reading = SensorReading(**data)

                try:
                    from app.services.influx import write_accel_point_sync
                    await asyncio.to_thread(write_accel_point_sync, reading, ts)
                    log.info(f"[Influx] Write Success: {reading.device_id} at {ts}")
                    # Keep device registry fresh so /api/v1/devices shows all active sensors
                    devices_svc.touch_last_seen(reading.device_id, ts)
                    # Broadcast to websocket clients
                    try:
                        msg = {
                            "type": "data",
                            "topic": topic,
                            "device_id": reading.device_id,
                            "ts": ts.isoformat(),
                            "x": reading.x,
                            "y": reading.y,
                            "z": reading.z,
                        }
                        for ws in list(active_clients):
                            asyncio.create_task(_safe_send(ws, msg))
                    except Exception:
                        pass
                except Exception as e:
                    log.error(f"[Influx] Write failed for {reading.device_id}: {e}")

            # Handle heartbeat messages (devices/{id}/heartbeat)
            elif topic.startswith("devices/") and topic.endswith("/heartbeat"):
                data["device_id"] = str(data.get("device_id") or device_id)
                ts_str = data.get("ts")
                ts = (datetime.fromisoformat(ts_str.replace("Z", "+00:00"))
                      if ts_str else datetime.now(timezone.utc))
                hb = DeviceHeartbeat(**data)

                try:
                    await asyncio.to_thread(
                        write_heartbeat_sync,
                        hb.device_id or device_id,
                        hb.rssi,
                        hb.uptime_s,
                        hb.fw,
                        ts
                    )
                    log.info(f"[Influx] Heartbeat Write Success: {device_id} (rssi={hb.rssi})")
                    # Refresh registry presence and recency for standard protocol devices
                    devices_svc.touch_last_seen(hb.device_id or device_id, ts)
                    # Broadcast heartbeat to websocket clients
                    try:
                        msg = {
                            "type": "heartbeat",
                            "topic": topic,
                            "device_id": hb.device_id or device_id,
                            "ts": ts.isoformat(),
                            "rssi": hb.rssi,
                            "uptime_s": hb.uptime_s,
                            "fw": hb.fw,
                        }
                        for ws in list(active_clients):
                            asyncio.create_task(_safe_send(ws, msg))
                    except Exception:
                        pass
                except Exception as e:
                    log.error(f"[Influx] Heartbeat Write failed for {device_id}: {e}")

            # ===== MESH/* PROTOCOL =====
            # Handle mesh protocol client assignment (device registration)
            elif topic.startswith("mesh/"):
                msg_type = data.get("type")
                
                if msg_type == "client_assignment":
                    # Register the device
                    registered_device_id = _process_mesh_assignment(data)
                    # Touch last seen time
                    if registered_device_id:
                        devices_svc.touch_last_seen(registered_device_id)
                    log.info(f"[Mesh] Device assignment processed: {registered_device_id}")
                
                elif msg_type == "data":
                    # Convert mesh batched format to individual readings
                    device_mac = data.get("id", device_id)
                    readings = _process_mesh_data_message(data, device_mac)
                    
                    if readings:
                        from app.services.influx import write_accel_point_sync
                        for reading in readings:
                            try:
                                await asyncio.to_thread(write_accel_point_sync, reading, reading.ts)
                                log.info(f"[Influx] Mesh Write Success: {reading.device_id} at {reading.ts}")
                                
                                # Broadcast to websocket clients
                                try:
                                    msg = {
                                        "type": "data",
                                        "topic": f"mesh/{device_mac}/data",
                                        "device_id": reading.device_id,
                                        "ts": reading.ts.isoformat(),
                                        "x": reading.x,
                                        "y": reading.y,
                                        "z": reading.z,
                                    }
                                    for ws in list(active_clients):
                                        asyncio.create_task(_safe_send(ws, msg))
                                except Exception:
                                    pass
                            except Exception as e:
                                log.error(f"[Influx] Mesh Write failed for {reading.device_id}: {e}")
                        
                        # Touch last seen for this device
                        if readings:
                            devices_svc.touch_last_seen(readings[-1].device_id, readings[-1].ts)
                else:
                    log.debug(f"[Mesh] Ignoring unknown message type: {msg_type} on {topic}")
            
            else:
                log.debug(f"[MQTT] Ignoring message on {topic}")

        except Exception as e:
            log.error(f"[Drain Error] Failed to process {topic}: {e}")
            continue

@app.on_event("startup")
async def start_background_tasks():
    if ENABLE_MQTT:
        threading.Thread(target=_mqtt_thread, daemon=True).start()
        asyncio.create_task(_drain_mqtt_queue())

@app.on_event("shutdown")
async def shutdown():
    client = get_influx_client()
    if client:
        client.close()

@app.get("/healthz")
def healthz():
    return {"ok": True}

