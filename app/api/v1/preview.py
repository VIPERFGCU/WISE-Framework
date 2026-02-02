from fastapi import APIRouter, Query, Depends, HTTPException
from typing import Optional
from app.services.influx import read_accel_series
from app.deps import get_influx_query_api

router = APIRouter(prefix="/api", tags=["preview"])


@router.get("/preview/{device_id}")
async def preview(device_id: str, window_s: int = Query(60, ge=1, le=3600), query_api=Depends(get_influx_query_api)):
    """Return accelerometer series for the given device over the last `window_s` seconds.

    Uses the existing `read_accel_series` helper which expects a Flux-style range (e.g. "5m").
    """
    # Convert seconds to a Flux range shorthand (seconds -> "{N}s", minutes if large)
    if window_s < 60:
        rng = f"{window_s}s"
    elif window_s % 60 == 0:
        rng = f"{window_s//60}m"
    else:
        rng = f"{window_s}s"

    try:
        series = await read_accel_series(device_id=device_id, range=rng, query_api=query_api)
        return series
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))

