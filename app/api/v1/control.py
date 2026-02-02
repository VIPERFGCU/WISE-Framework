# app/api/v1/control.py
from fastapi import APIRouter, Depends, HTTPException, status, Body
from pydantic import BaseModel
from app import deps
from app.mqtt import publish_control 
import os, json, paho.mqtt.client as mqtt


router = APIRouter(prefix="/api/v1/devices", tags=["control"])


class RateUpdate(BaseModel):
    rate_hz: int

class BatchUpdate(BaseModel):
    batch_size: int

@router.post("/{device_id}/start")
async def start_device(device_id: str, body: dict = Body(None)):
    rate_hz = (body or {}).get("rate_hz", 10)
    if publish_control(device_id, {"cmd": "START", "rate_hz": int(rate_hz)}):
        return {"ok": True}
    raise HTTPException(status_code=502, detail="MQTT publish failed")

@router.post("/{device_id}/stop")
async def stop_device(device_id: str):
    if publish_control(device_id, {"cmd": "STOP"}):
        return {"ok": True}
    raise HTTPException(status_code=502, detail="MQTT publish failed")

@router.post("/{device_id}/rate")
async def set_device_rate(device_id: str, body: RateUpdate):
    """
    Update the sampling frequency (Hz) for a single device.
    """
    if publish_control(device_id, {"cmd": "SET_RATE", "rate_hz": int(body.rate_hz)}):
        return {"ok": True}
    raise HTTPException(status_code=502, detail="MQTT publish failed")

@router.post("/{device_id}/batch")
async def set_device_batch(device_id: str, body: BatchUpdate):
    """
    Update the batch size (samples per publish) for a single device.
    """
    if publish_control(device_id, {"cmd": "SET_BATCH", "batch_size": int(body.batch_size)}):
        return {"ok": True}
    raise HTTPException(status_code=502, detail="MQTT publish failed")
