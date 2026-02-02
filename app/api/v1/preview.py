from fastapi import APIRouter, Query, Depends, HTTPException
from typing import Optional
from app.services.influx import read_accel_series, read_heartbeat_series
from app.deps import get_influx_query_api
from pydantic import BaseModel

router = APIRouter(prefix="/api", tags=["preview"])


class PreviewResponse(BaseModel):
    device_id: str
    accel: Optional[dict] = None
    heartbeat: Optional[dict] = None


@router.get("/preview/{device_id}")
async def preview(device_id: str, window_s: int = Query(60, ge=1, le=3600), query_api=Depends(get_influx_query_api)):
    """Return accelerometer and heartbeat series for the given device over the last `window_s` seconds."""
    # Convert seconds to a Flux range shorthand
    if window_s < 60:
        rng = f"{window_s}s"
    elif window_s % 60 == 0:
        rng = f"{window_s//60}m"
    else:
        rng = f"{window_s}s"

    accel_data = None
    heartbeat_data = None

    try:
        accel_series = await read_accel_series(device_id=device_id, range=rng, query_api=query_api)
        accel_data = accel_series.model_dump()
    except Exception as e:
        pass  # Heartbeat may still be available even if accel fails

    try:
        heartbeat_series = await read_heartbeat_series(device_id=device_id, range=rng, query_api=query_api)
        heartbeat_data = heartbeat_series.model_dump()
    except Exception as e:
        pass  # Accel may still be available even if heartbeat fails

    if not accel_data and not heartbeat_data:
        raise HTTPException(status_code=500, detail="No data available for device")

    return {
        "device_id": device_id,
        "accel": accel_data,
        "heartbeat": heartbeat_data,
    }

