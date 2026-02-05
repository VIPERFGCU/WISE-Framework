import os, json, asyncio, logging, threading, time
from typing import Set
from datetime import datetime, timezone
from queue import Queue, Empty

from fastapi import FastAPI, WebSocket, WebSocketDisconnect, Body
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
async def debug_start_sensor(device_id: str = "bridge-esp32-001"):
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
        log.info(f"[MQTT] Connected successfully, subscribing to devices/#")
        client.subscribe("devices/#")
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
# MQTT Consumer -> InfluxDB + WebSocket fan-out
# -------------------------------------------------------------------
async def _drain_mqtt_queue():
    """Consume messages placed on the thread-safe queue by the Paho callbacks.
    Processes both /data (sensor readings) and /heartbeat topics.
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

            # Handle sensor data messages
            if topic.endswith("/data"):
                data["device_id"] = data.get("device_id") or device_id
                ts_str = data.get("ts")
                ts = (datetime.fromisoformat(ts_str.replace("Z", "+00:00"))
                      if ts_str else datetime.now(timezone.utc))
                reading = SensorReading(**data)

                try:
                    from app.services.influx import write_accel_point_sync
                    await asyncio.to_thread(write_accel_point_sync, reading, ts)
                    log.info(f"[Influx] Write Success: {reading.device_id} at {ts}")
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

            # Handle heartbeat messages
            elif topic.endswith("/heartbeat"):
                data["device_id"] = data.get("device_id") or device_id
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

            else:
                log.debug(f"[MQTT] Ignoring non-data/heartbeat message on {topic}")

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

