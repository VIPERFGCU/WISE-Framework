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
    
    Note: InfluxDB queries are done directly without dependency injection to avoid blocking
    when InfluxDB is unavailable.
    """
    
    # Convert seconds to a Flux range shorthand
    if window_s < 60:
        rng = f"{window_s}s"
    elif window_s % 60 == 0:
        rng = f"{window_s//60}m"
    else:
        rng = f"{window_s}s"

    accel_data = None
    heartbeat_data = None

    # Try accel with timeout (query_api will be created inside read_accel_series)
    try:
        accel_series = await asyncio.wait_for(
            read_accel_series(device_id=device_id, range=rng, query_api=None),
            timeout=5.0
        )
        accel_data = accel_series.model_dump()
    except asyncio.TimeoutError:
        log.warning(f"[Preview] Accel query timeout for {device_id}")
    except Exception as e:
        log.warning(f"[Preview] Accel query failed for {device_id}: {e}")

    # Try heartbeat with timeout (query_api will be created inside read_heartbeat_series)
    try:
        heartbeat_series = await asyncio.wait_for(
            read_heartbeat_series(device_id=device_id, range=rng, query_api=None),
            timeout=5.0
        )
        heartbeat_data = heartbeat_series.model_dump()
    except asyncio.TimeoutError:
        log.warning(f"[Preview] Heartbeat query timeout for {device_id}")
    except Exception as e:
        log.warning(f"[Preview] Heartbeat query failed for {device_id}: {e}")

    return {
        "device_id": device_id,
        "accel": accel_data,
        "heartbeat": heartbeat_data,
    }

