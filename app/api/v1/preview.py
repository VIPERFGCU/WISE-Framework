from fastapi import APIRouter, Query, HTTPException
from typing import Optional
import asyncio
import logging
import csv
from io import StringIO
from datetime import datetime, timezone
from starlette.responses import Response
from app.services.influx import read_accel_series, read_heartbeat_series, read_recent_device_ids
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
    
    all_selector = {"__all__", "all", "*"}
    if device_id in all_selector:
        device_ids = await read_recent_device_ids(range=range_str)
        if not device_ids:
            return {
                "device_id": "__all__",
                "accel": None,
                "heartbeat": None,
            }

        accel_tasks = [read_accel_series(did, range_str) for did in device_ids]
        heartbeat_tasks = [read_heartbeat_series(did, range_str) for did in device_ids]
        accel_results, heartbeat_results = await asyncio.gather(
            asyncio.gather(*accel_tasks),
            asyncio.gather(*heartbeat_tasks),
        )

        accel_points = [
            {
                "device_id": series.device_id,
                "t": point.t,
                "x": point.x,
                "y": point.y,
                "z": point.z,
            }
            for series in accel_results
            for point in series.series
        ]
        accel_points.sort(key=lambda p: p["t"])

        heartbeat_points = [
            {
                "device_id": series.device_id,
                "t": point.t,
                "rssi": point.rssi,
            }
            for series in heartbeat_results
            for point in series.series
        ]
        heartbeat_points.sort(key=lambda p: p["t"])

        return {
            "device_id": "__all__",
            "accel": {"series": accel_points} if accel_points else None,
            "heartbeat": {"series": heartbeat_points} if heartbeat_points else None,
        }

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

    all_selector = {"__all__", "all", "*"}
    if device_id in all_selector:
        device_ids = await read_recent_device_ids(range=range_str)
    else:
        device_ids = [device_id]

    accel_tasks = [read_accel_series(did, range_str) for did in device_ids]
    heartbeat_tasks = [read_heartbeat_series(did, range_str) for did in device_ids]
    accel_results, heartbeat_results = await asyncio.gather(
        asyncio.gather(*accel_tasks),
        asyncio.gather(*heartbeat_tasks),
    )

    output = StringIO()
    writer = csv.writer(output)
    writer.writerow(["timestamp", "device_id", "measurement", "x", "y", "z", "rssi"])

    for series in accel_results:
        for point in series.series:
            if start_dt and end_dt and not (start_dt <= point.t <= end_dt):
                continue
            writer.writerow([point.t.isoformat(), series.device_id, "accel", point.x, point.y, point.z, ""])

    for series in heartbeat_results:
        for point in series.series:
            if start_dt and end_dt and not (start_dt <= point.t <= end_dt):
                continue
            writer.writerow([point.t.isoformat(), series.device_id, "heartbeat", "", "", "", point.rssi])

    ts = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")
    safe_device_id = "all_sensors" if device_id in all_selector else "".join(c if c.isalnum() or c in ("-", "_") else "_" for c in device_id)
    filename = f"{safe_device_id}_streams_{ts}.csv"

    return Response(
        content=output.getvalue(),
        media_type="text/csv",
        headers={
            "Content-Disposition": f'attachment; filename="{filename}"'
        },
    )

