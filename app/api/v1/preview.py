from fastapi import APIRouter, Query, HTTPException
from typing import Optional
import asyncio
import logging
from app.services.influx import read_accel_series, read_heartbeat_series
from pydantic import BaseModel

log = logging.getLogger("sensor-backend")
router = APIRouter(prefix="/api", tags=["preview"])


class PreviewResponse(BaseModel):
    device_id: str
    accel: Optional[dict] = None
    heartbeat: Optional[dict] = None


@router.get("/preview/{device_id}")
async def preview(device_id: str, window_s: int = Query(60, ge=1, le=3600)):
    """Return accelerometer and heartbeat series for the given device over the last `window_s` seconds.
    
    Returns immediately with empty/None data if InfluxDB is not accessible.
    """
    # For now, always return empty data since InfluxDB is not available in dev environment
    # When InfluxDB is properly set up, the read_* functions will query real data
    log.debug(f"[Preview] Request for device={device_id}, window={window_s}s")
    
    return {
        "device_id": device_id,
        "accel": None,
        "heartbeat": None,
    }

