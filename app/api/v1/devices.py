from typing import List, Dict, Any
from fastapi import APIRouter, Depends, HTTPException, status
from fastapi.responses import Response
import os, json, threading
import paho.mqtt.client as mqtt
from datetime import datetime

from app import deps
from app.schemas.device import DeviceCreate, DeviceOut, DeviceOutAdmin
from app.services import devices as svc

router = APIRouter(prefix="/api/v1/devices", tags=["devices"])

# ----------------------------
# MQTT-backed live status cache
# ----------------------------
# Cache format per device_id:
# {
#   "state": "online"|"offline"|"streaming"|"stopped"|"unknown",
#   "rate_hz": int|None,
#   "ts": ISO8601 string (device-published),
# }
_mqtt_status_cache: Dict[str, Dict[str, Any]] = {}
_cache_lock = threading.Lock()

MQTT_HOST = os.getenv("MQTT_HOST", "localhost")
MQTT_PORT = int(os.getenv("MQTT_PORT", "1883"))
_STATUS_SUB = "devices/+/status"

def _deviceout_from_cache(device_id: str, rec=None) -> DeviceOut:
    """Build a DeviceOut using the MQTT cache snapshot (non-blocking).

    If a service `rec` is provided, prefer its `label` and `last_seen` values
    while still sourcing live telemetry fields from the MQTT cache.
    """
    with _cache_lock:
        c = _mqtt_status_cache.get(device_id)

    sensing = (c.get("state") == "streaming") if c else False
    label = (rec.label if rec and getattr(rec, "label", None) else device_id)
    last_seen = None
    # Prefer service-observed last_seen when available, else use cache ts
    if rec and getattr(rec, "last_seen", None):
        last_seen = rec.last_seen
    else:
        last_seen = c.get("ts") if c else None

    return DeviceOut(
        device_id=device_id,
        label=label,
        last_seen=last_seen,
        status=(c.get("state") if c else "unknown"),
        sensing=sensing,
        sample_hz=(c.get("rate_hz") if c else None),
        batch_size=(c.get("batch_size") if c else None),
        uptime_seconds=(c.get("uptime_s", 0) if c else 0),
    )


def _on_status_msg(client, userdata, msg):
    # topic: devices/<id>/status
    try:
        parts = msg.topic.split("/")
        device_id = parts[1] if len(parts) >= 3 else None
        payload = json.loads(msg.payload.decode())
    except Exception:
        return
    if not device_id:
        return
    with _cache_lock:
        _mqtt_status_cache[device_id] = {
            "state": payload.get("state", "unknown"),
            "rate_hz": payload.get("rate_hz"),
            "batch_size": payload.get("batch_size"),
            "uptime_s": payload.get("uptime_s"),
            "ts": payload.get("ts"),
        }

def _ensure_mqtt_started():
    # Start a single background MQTT client (idempotent)
    if getattr(_ensure_mqtt_started, "_started", False):
        return
    cli = mqtt.Client(client_id="backend-status-cache", clean_session=True)
    cli.on_message = _on_status_msg
    try:
        cli.connect(MQTT_HOST, MQTT_PORT, keepalive=30)
        cli.subscribe(_STATUS_SUB, qos=1)
        cli.loop_start()
        _ensure_mqtt_started._started = True  # type: ignore[attr-defined]
    except Exception:
        # Non-fatal: API still works; status will fall back to svc.status
        pass

_ensure_mqtt_started()

def _merged_status(device_id: str, fallback: str) -> str:
    """Prefer live MQTT status if available; else your service's status string."""
    with _cache_lock:
        cached = _mqtt_status_cache.get(device_id)
    return (cached.get("state") if cached else None) or fallback or "unknown"

def _cached_status_detail(device_id: str) -> Dict[str, Any]:
    with _cache_lock:
        cached = _mqtt_status_cache.get(device_id)
    if not cached:
        return {"id": device_id, "state": "unknown"}
    return {"id": device_id, **cached}

# ----------------------------
# Existing endpoints (unchanged behavior, but enriched with live status)
# ----------------------------

@router.post(
    "/register",
    response_model=DeviceOutAdmin,
    status_code=status.HTTP_201_CREATED,
    summary="Register or update a device",
    dependencies=[Depends(deps.require_api_key_or_role("admin"))],
)
async def register_device(payload: DeviceCreate) -> DeviceOut:
    rec = svc.register(device_id=payload.device_id, label=payload.label, notes=payload.notes)
    # Build the enriched DeviceOut and attach the mqtt_key for admin consumers
    out = _deviceout_from_cache(rec.device_id, rec)
    # attach mqtt key if present
    try:
        setattr(out, "mqtt_key", getattr(rec, "mqtt_key", None))
    except Exception:
        pass
    return out

@router.get(
    "",
    response_model=List[DeviceOut],
    summary="List devices with status",
)
async def list_devices() -> List[DeviceOut]:
    _ensure_mqtt_started()  # make sure cache is live
    
    out: List[DeviceOut] = []
    for rec in svc.all_devices():
        out.append(_deviceout_from_cache(rec.device_id, rec))
    out.sort(key=lambda d: d.device_id)
    return out

@router.get(
    "/{device_id}",
    response_model=DeviceOut,
    summary="Get single device",
    dependencies=[Depends(deps.require_api_key_or_role("viewer"))],
)
async def get_device(device_id: str) -> DeviceOut:
    rec = svc.get(device_id)
    if not rec:
        raise HTTPException(status_code=404, detail="Device not found")
    return _deviceout_from_cache(rec.device_id, rec)

# ----------------------------
# New: explicit live status endpoint for the UI
# ----------------------------
@router.get(
    "/{device_id}/status",
    summary="Get live device status (MQTT cache)",
    dependencies=[Depends(deps.require_api_key_or_role("viewer"))],
)
async def device_status(device_id: str) -> Dict[str, Any]:
    """
    Returns a minimal status object for the UI:
    {
      "id": "<device_id>",
      "state": "streaming|stopped|online|offline|unknown",
      "rate_hz": <int|null>,
      "ts": "ISO8601 string or null"
    }
    """
    # Touch MQTT init just in case
    _ensure_mqtt_started()
    detail = _cached_status_detail(device_id)

    # If not in cache but the device exists, derive a best-effort state
    if detail.get("state") == "unknown":
        rec = svc.get(device_id)
        if rec:
            detail["state"] = svc.status(rec)
    # Optionally add server-observed last_seen as convenience
    rec = svc.get(device_id)
    if rec:
        # Make this a string to keep JSON consistent
        try:
            detail["last_seen"] = rec.last_seen.isoformat() if hasattr(rec.last_seen, "isoformat") else str(rec.last_seen)
        except Exception:
            detail["last_seen"] = str(rec.last_seen)
    return detail

@router.options("", include_in_schema=False)
async def _options_devices() -> Response:
    # Let CORSMiddleware add the CORS headers; just return 200
    return Response(status_code=200)

@router.options("/{device_id}", include_in_schema=False)
async def _options_device(device_id: str) -> Response:
    return Response(status_code=200)
