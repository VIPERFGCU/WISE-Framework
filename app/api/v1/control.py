# app/api/v1/control.py
from fastapi import APIRouter, Depends, HTTPException, status, Body
from pydantic import BaseModel
from app import deps
from app.mqtt import publish_control 
from app.services import devices as devices_svc
import os, json, paho.mqtt.client as mqtt


router = APIRouter(prefix="/api/v1/devices", tags=["control"])


class RateUpdate(BaseModel):
    rate_hz: int

class BatchUpdate(BaseModel):
    batch_size: int

@router.post("/{device_id}/start")
async def start_device(device_id: str, body: dict = Body(None)):
    rec = devices_svc.get(device_id)
    rate_hz = (body or {}).get("rate_hz", getattr(rec, "preferred_rate_hz", None) or 10)
    batch_size = (body or {}).get("batch_size", getattr(rec, "preferred_batch_size", None) or 1)
    devices_svc.set_rate(device_id, int(rate_hz))
    devices_svc.set_batch(device_id, int(batch_size))
    if publish_control(device_id, {"cmd": "START", "rate_hz": int(rate_hz), "batch_size": int(batch_size)}):
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
    devices_svc.set_rate(device_id, int(body.rate_hz))
    if publish_control(device_id, {"cmd": "SET_RATE", "rate_hz": int(body.rate_hz)}):
        return {"ok": True}
    raise HTTPException(status_code=502, detail="MQTT publish failed")

@router.post("/{device_id}/batch")
async def set_device_batch(device_id: str, body: BatchUpdate):
    """
    Update the batch size (samples per publish) for a single device.
    """
    devices_svc.set_batch(device_id, int(body.batch_size))
    if publish_control(device_id, {"cmd": "SET_BATCH", "batch_size": int(body.batch_size)}):
        return {"ok": True}
    raise HTTPException(status_code=502, detail="MQTT publish failed")
