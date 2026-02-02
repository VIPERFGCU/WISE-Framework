from __future__ import annotations
from datetime import datetime
from typing import List, Optional

from influxdb_client import InfluxDBClient, Point, WritePrecision
from influxdb_client.client.write_api import SYNCHRONOUS, WriteApi
from influxdb_client.client.query_api import QueryApi

from app.core.config import settings
from app.schemas.sensor import SensorReading, AccelSeries, AccelPoint


# helpers

_client = InfluxDBClient(
            url=settings.influx_url,
            token=settings.influx_token,
            org=settings.influx_org,
            timeout=10_000
        )

# Persistent Global Write API
_write_api = _client.write_api(write_options=SYNCHRONOUS)
# writes

async def write_accel_point(reading: SensorReading, ts: datetime) -> datetime:
    """
    Write a single accelerometer point using the persistent global Write API.
    """
    try:
        p = (
            Point("accel")
            .tag("device_id", reading.device_id)
            .field("x", float(reading.x))
            .field("y", float(reading.y))
            .field("z", float(reading.z))
            .time(ts, WritePrecision.NS)
        )
        # Use the global persistent API
        _write_api.write(bucket=settings.influx_bucket, record=p)
        return ts
    except Exception as e:
        # If the persistent write fails, it is usually a network/auth issue
        raise e

def write_accel_point_sync(reading: SensorReading, ts: datetime) -> datetime:
    """Synchronous wrapper for writing a single accel point.
    Useful when calls must be offloaded to a thread.
    """
    try:
        p = (
            Point("accel")
            .tag("device_id", reading.device_id)
            .field("x", float(reading.x))
            .field("y", float(reading.y))
            .field("z", float(reading.z))
            .time(ts, WritePrecision.NS)
        )
        _write_api.write(bucket=settings.influx_bucket, record=p)
        return ts
    except Exception as e:
        raise e

def write_heartbeat_sync(device_id: str, rssi: int, uptime_s: int, fw: Optional[str], ts: datetime) -> datetime:
    """Synchronous write for device heartbeat metrics (RSSI, uptime)."""
    try:
        p = (
            Point("heartbeat")
            .tag("device_id", device_id)
            .field("rssi", int(rssi))
            .field("uptime_s", int(uptime_s))
        )
        if fw:
            p.tag("fw", fw)
        p.time(ts, WritePrecision.NS)
        _write_api.write(bucket=settings.influx_bucket, record=p)
        return ts
    except Exception as e:
        raise e
# queries

async def read_accel_series(
        device_id: str, 
        range: str,
        query_api: Optional[QueryApi] = None,
) -> AccelSeries:
    """
    Read a time series of accelerometer data over a Flux-style range string (e.g., "15m", "1hr").
    """
    close_client = False
    if query_api is None:
        client = _mk_client()
        query_api = client.query_api()
        close_client = True

    flux = f"""
from(bucket: "{settings.influx_bucket}")
  |> range(start: -{range})
  |> filter(fn: (r) => r._measurement == "accel")
  |> filter(fn: (r) => r.device_id == "{device_id}")
  |> pivot(rowKey:["_time"], columnKey: ["_field"], valueColumn: "_value")
  |> keep(columns: ["_time","x","y","z"])
  |> sort(columns: ["_time"])
"""

    points: List[AccelPoint] = []

    try:
        tables = query_api.query(org=settings.influx_org, query=flux)
        # tables: List[FluxTable]; each has records with .values dict
        for table in tables:
            for record in table.records:
                 v = record.values
                 t = v.get("_time")
                 # Fields may be absent if not written; guard with get()
                 x = v.get("x")
                 y = v.get("y")
                 z = v.get("z")
                 # Only append rows that have all three componenets
                 if t is not None and x is not None and y is not None and z is not None:
                    points.append(AccelPoint(t=t, x=float(x), y=float(y), z=float(z)))
    finally:
        if close_client and client is not None:
            try:
                 client.close() # type: ignore[name-defined]
            except Exception:
                 pass

    return AccelSeries(device_id=device_id, series=points)

async def read_heartbeat_series(
        device_id: str, 
        range: str,
        query_api: Optional[QueryApi] = None,
) -> "HeartbeatSeries":
    """
    Read a time series of heartbeat (RSSI) data over a Flux-style range string.
    """
    from app.schemas.sensor import HeartbeatPoint, HeartbeatSeries
    
    close_client = False
    if query_api is None:
        client = _mk_client()
        query_api = client.query_api()
        close_client = True

    flux = f"""
from(bucket: "{settings.influx_bucket}")
  |> range(start: -{range})
  |> filter(fn: (r) => r._measurement == "heartbeat")
  |> filter(fn: (r) => r.device_id == "{device_id}")
  |> filter(fn: (r) => r._field == "rssi")
  |> sort(columns: ["_time"])
"""

    points: List[HeartbeatPoint] = []

    try:
        tables = query_api.query(org=settings.influx_org, query=flux)
        for table in tables:
            for record in table.records:
                 v = record.values
                 t = v.get("_time")
                 rssi = v.get("_value")
                 if t is not None and rssi is not None:
                    points.append(HeartbeatPoint(t=t, rssi=int(rssi)))
    finally:
        if close_client and client is not None:
            try:
                 client.close()
            except Exception:
                 pass

    return HeartbeatSeries(device_id=device_id, series=points)
