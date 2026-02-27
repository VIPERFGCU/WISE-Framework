from fastapi import APIRouter, Query, HTTPException
from typing import Optional
import asyncio
import logging
import csv
from io import StringIO
from datetime import datetime, timezone
from starlette.responses import Response
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


@router.get("/preview/{device_id}/csv")
async def preview_csv(
    device_id: str,
    window_s: int = Query(60, ge=1, le=3600),
    start_ts: Optional[str] = Query(default=None, description="ISO timestamp start"),
    end_ts: Optional[str] = Query(default=None, description="ISO timestamp end"),
):
    """Download accelerometer and heartbeat data as CSV for the given device/window."""
    now_utc = datetime.now(timezone.utc)

    start_dt: Optional[datetime] = None
    end_dt: Optional[datetime] = None
    if start_ts or end_ts:
        if not start_ts or not end_ts:
            raise HTTPException(status_code=400, detail="Both start_ts and end_ts are required for date-range export")
        try:
            start_dt = datetime.fromisoformat(start_ts.replace("Z", "+00:00"))
            end_dt = datetime.fromisoformat(end_ts.replace("Z", "+00:00"))
        except ValueError:
            raise HTTPException(status_code=400, detail="Invalid start_ts/end_ts. Use ISO format.")

        if start_dt.tzinfo is None:
            start_dt = start_dt.replace(tzinfo=timezone.utc)
        else:
            start_dt = start_dt.astimezone(timezone.utc)

        if end_dt.tzinfo is None:
            end_dt = end_dt.replace(tzinfo=timezone.utc)
        else:
            end_dt = end_dt.astimezone(timezone.utc)

        if start_dt >= end_dt:
            raise HTTPException(status_code=400, detail="start_ts must be before end_ts")

        range_seconds = max(1, int((now_utc - start_dt).total_seconds()))
    else:
        range_seconds = window_s

    if range_seconds < 60:
        range_str = f"{range_seconds}s"
    elif range_seconds < 3600:
        range_str = f"{range_seconds // 60}m"
    else:
        range_str = f"{range_seconds // 3600}h"

    accel_task = read_accel_series(device_id, range_str)
    heartbeat_task = read_heartbeat_series(device_id, range_str)
    accel_series, heartbeat_series = await asyncio.gather(accel_task, heartbeat_task)

    output = StringIO()
    writer = csv.writer(output)
    writer.writerow(["timestamp", "measurement", "x", "y", "z", "rssi"])

    for point in accel_series.series:
        if start_dt and end_dt and not (start_dt <= point.t <= end_dt):
            continue
        writer.writerow([point.t.isoformat(), "accel", point.x, point.y, point.z, ""])

    for point in heartbeat_series.series:
        if start_dt and end_dt and not (start_dt <= point.t <= end_dt):
            continue
        writer.writerow([point.t.isoformat(), "heartbeat", "", "", "", point.rssi])

    ts = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")
    safe_device_id = "".join(c if c.isalnum() or c in ("-", "_") else "_" for c in device_id)
    filename = f"{safe_device_id}_streams_{ts}.csv"

    return Response(
        content=output.getvalue(),
        media_type="text/csv",
        headers={
            "Content-Disposition": f'attachment; filename="{filename}"'
        },
    )

