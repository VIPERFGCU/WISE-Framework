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
    log.debug(f"[Preview] Request for device={device_id}, window={window_s}s")
    
    # Convert window_s to Flux range string (e.g., 60s -> "1m")
    if window_s < 60:
        range_str = f"{window_s}s"
    elif window_s < 3600:
        range_str = f"{window_s // 60}m"
    else:
        range_str = f"{window_s // 3600}h"
    
    # Query both accel and heartbeat series in parallel
    accel_task = read_accel_series(device_id, range_str)
    heartbeat_task = read_heartbeat_series(device_id, range_str)
    
    accel_series, heartbeat_series = await asyncio.gather(accel_task, heartbeat_task)
    
    return {
        "device_id": device_id,
        "accel": {"series": accel_series.series} if accel_series.series else None,
        "heartbeat": {"series": heartbeat_series.series} if heartbeat_series.series else None,
    }

