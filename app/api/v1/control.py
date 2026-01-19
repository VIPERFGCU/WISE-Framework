# app/api/v1/control.py
from fastapi import APIRouter, Depends, HTTPException, status
from pydantic import BaseModel
from app import deps
from app.main import mqtt_client # Importing the global client
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

    # Checking if global client from main.py is connected
    if not mqtt_client.is_connected():
        raise Exception("Global MQTT client is not connected to broker")
    
    try:
        payload_str = json.dumps(payload)
        # Use the persistent global client to publish
        info = mqtt_client.publish(topic, payload_str, qos=1)
        # Confirm it actually left the buffer
        info.wait_for_publish(timeout=1.0)
    except Exception as e:
        raise e

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
