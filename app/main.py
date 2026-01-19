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
from app.core.config import settings
from app.deps import get_influx_client
from app.services.influx import write_accel_point
from app.schemas.sensor import SensorReading

# -------------------------------------------------------------------
# App setup
# -------------------------------------------------------------------
app = FastAPI(title="Sensor Backend", version="0.2.0")

# Logging
LOG_LEVEL = settings.api_log_level.upper()
logging.basicConfig(level=LOG_LEVEL)
log = logging.getLogger("sensor-backend")

# -------------------------------------------------------------------
# CORS (explicitly allow x-api-key and use configured origins)
# -------------------------------------------------------------------
raw_origins = settings.cors_allow_origins
if isinstance(raw_origins, str):
    # support comma-separated env like: "http://localhost:5173,http://127.0.0.1:5173"
    origins = [o.strip() for o in raw_origins.split(",") if o.strip()]
elif isinstance(raw_origins, (list, tuple)):
    origins = list(raw_origins)
else:
    origins = ["http://localhost:5173", "http://127.0.0.1:5173"]

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

# -------------------------------------------------------------------
# Debug / Manual Control
# -------------------------------------------------------------------
@app.post("api/v1/debug/start")
async def debug_start_sensor(device_id: str = "bridge-esp32-001"):
    """
    Manually triggers the sensor to start via the backend's MQTT logic.
    This effectively tells the ESP32 to set 'streaming = true'.
    """
    payload = json.dumps({"cmd": "START", "rate_hz": 10})
    topic = f"devices/{device_id}/control"

    try:
        # Connecting a temporary client to publish the message
        temp_client = paho.Client(paho.CallbackAPIVersion.VERSION2)
        temp_client.connect(MQTT_HOST, MQTT_PORT)
        temp_client.publish(topic, payload)
        temp_client.disconnect()
        log.info(f"[Debug] Sent START command to {topic}")
        return {"status": "command send", "topic": topic}
    except Exception as e:
        log.error(f"[Debug] Failed to send START command: {type(e).__name__}: {e}")
        return {"status": "error", "message": str(e)}

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
    log.info(f"[MQTT] Connected (rc={reason_code}), subscribing to {MQTT_TOPIC}")
    client.subscribe(MQTT_TOPIC)

def _paho_on_message(client, userdata, msg):
    try:
        _mqtt_queue.put_nowait((msg.topic, msg.payload.decode()))
    except Exception as e:
        log.warning(f"[MQTT] Queue put failed: {type(e).__name__}: {e}")

def _mqtt_thread():
    while True:
        try:
            client = paho.Client(paho.CallbackAPIVersion.VERSION2)
            client.on_connect = _paho_on_connect
            client.on_message = _paho_on_message
            client.connect(MQTT_HOST, MQTT_PORT, keepalive=30)
            client.loop_forever()
        except Exception as e:
            log.warning(f"[MQTT] Thread error: {type(e).__name__}: {e}; retrying in 2s")
            time.sleep(2)

# -------------------------------------------------------------------
# MQTT Consumer -> InfluxDB + WebSocket fan-out
# -------------------------------------------------------------------
async def _drain_mqtt_queue():
    while True:
        try:
            topic, payload_raw = _mqtt_queue.get_nowait()
        except Empty:
            await asyncio.sleep(0.05)
            continue

        # ---- parse & normalize ----
        try:
            data = json.loads(payload_raw)
        except Exception as e:
            log.warning(f"[MQTT] bad JSON: {type(e).__name__}: {e}; payload={payload_raw!r}")
            continue

        parts = topic.split("/")
        device_from_topic = parts[1] if len(parts) >= 3 else None
        device_id = data.get("device_id") or device_from_topic or "unknown"

        ts_str = data.get("ts")
        ts = (datetime.fromisoformat(ts_str.replace("Z", "+00:00"))
              if ts_str else datetime.now(timezone.utc))

        # ---- try Influx, but never block WS on failure ----
        try:
            # 1 . Creating the schema object required by write_accel_point
            reading = SensorReading(
                device_id=device_id,
                x=float(data.get("x", 0)),
                y=float(data.get("y", 0)),
                z=float(data.get("z", 0)),
            )

            # 2. Calling async service function
            await write_accel_point(reading, ts)
            log.info(f"[Influx] write succeeded: device_id={device_id} ts={ts.isoformat()}")
        except Exception as e:
            log.warning(f"[Influx] write failed: {type(e).__name__}: {e}")

        # ---- ALWAYS broadcast to WS ----
        out = {
            "device_id": device_id,
            "x": data.get("x"),
            "y": data.get("y"),
            "z": data.get("z"),
            "ts": ts.isoformat(),
            "topic": topic,
        }
        msg = json.dumps(out)
        sent = 0
        for ws in list(active_clients):
            try:
                await ws.send_text(msg)
                sent += 1
            except Exception:
                pass
        log.info(f"[WS] broadcast to {sent} client(s): {out}")

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

