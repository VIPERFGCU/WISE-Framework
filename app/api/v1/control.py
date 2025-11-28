# app/api/v1/control.py
from fastapi import APIRouter, Depends, HTTPException, status
from pydantic import BaseModel
from app import deps
import os, json, paho.mqtt.client as mqtt


router = APIRouter(prefix="/api/devices", tags=["control"])

MQTT_HOST = os.getenv("MQTT_HOST", "mosquitto")
MQTT_PORT = int(os.getenv("MQTT_PORT", "1883"))

class RateUpdate(BaseModel):
    rate_hz: int

class BatchUpdate(BaseModel):
    batch_size: int

def _publish_control(device_id: str, payload: dict) -> None:
    """
    Publish a control message to the device's control topic.
    Topic: devices/<device_id>/control
    QoS 1, non-retained.
    """
    topic = f"devices/{device_id}/control"

    cli = mqtt.Client(client_id="backend-control-pub", clean_session=True)
    # If your broker requires auth, set it here:
    # cli.username_pw_set(os.getenv("MQTT_USER",""), os.getenv("MQTT_PASS",""))

    cli.connect(MQTT_HOST, MQTT_PORT, keepalive=20)
    cli.loop_start()
    try:
        payload_str = json.dumps(payload)
        # Publish with QoS 1 so ESP32 definitely gets it even with brief Wi-Fi jitter
        info = cli.publish(topic, payload_str, qos=1, retain=False)
        info.wait_for_publish(timeout=2.0)
    finally:
        cli.loop_stop()
        cli.disconnect()

@router.post(
    "/{device_id}/start",
    dependencies=[Depends(deps.require_api_key_or_role("admin"))],
)
async def start_device(device_id: str, body: dict = None):
    rate_hz = (body or {}).get("rate_hz", 10)
    try:
        _publish_control(device_id, {"cmd": "START", "rate_hz": int(rate_hz)})
        return {"ok": True}
    except Exception as e:
        raise HTTPException(status_code=status.HTTP_502_BAD_GATEWAY,
                            detail=f"MQTT publish failed: {type(e).__name__}: {e}")

@router.post(
    "/{device_id}/stop",
    dependencies=[Depends(deps.require_api_key_or_role("admin"))],
)
async def stop_device(device_id: str):
    try:
        _publish_control(device_id, {"cmd": "STOP"})
        return {"ok": True}
    except Exception as e:
        raise HTTPException(status_code=status.HTTP_502_BAD_GATEWAY,
                            detail=f"MQTT publish failed: {type(e).__name__}: {e}")

@router.post(
        "/{device_id}/rate",
        dependencies=[Depends(deps.require_api_key_or_role("admin"))],
)
async def set_device_rate(device_id: str, body: RateUpdate):
    """
    Update the sampling frequency (Hz) for a single device.
    Sends: {"cmd": "SET_RATE", "rate_hz": <int>}
    """
    try:
        _publish_control(device_id, {"cmd": "SET_RATE", "rate_hz": int(body.rate_hz)})
        return {"ok": True}
    except Exception as e:
        raise HTTPException(
                status_code=status.HTTP_502_BAD_GATEWAY,
                detail=f"MQTT publish failed: {type(e).__name__}: {e}",
            )

@router.post(
        "/{device_id}/batch",
        dependencies=[Depends(deps.require_api_key_or_role("admin"))],
)
async def set_device_batch(device_id: str, body: BatchUpdate):
    """
    Update the batch size (samples per publish) for a single device.
    Sends: {"cmd": "SET_BATCH", "batch_size": <int>}
    """
    try:
        _publish_control(device_id, {"cmd": "SET_BATCH", "batch_size": int(body.batch_size)})
        return {"ok": True}
    except Exception as e:
        raise HTTPException(
                status_code=status.HTTP_502_BAD_GATEWAY,
                detail=f"MQTT publish failed: {type(e).__name__}: {e}",
            )
