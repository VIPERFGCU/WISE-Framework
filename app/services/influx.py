from __future__ import annotations
from datetime import datetime
from typing import List, Optional

from influxdb_client import InfluxDBClient, Point, WritePrecision
from influxdb_client.client.write_api import WriteApi
from influxdb_client.client.query_api import QueryApi

from app.core.config import settings
from app.schemas.sensor import SensorReading, AccelSeries, AccelPoint


# helpers

def _mk_client() -> InfluxDBClient:
    return InfluxDBClient(
            url=settings.influx_url,
            token=settings.influx_token,
            org=settings.influx_org,
            timeout=10_000
        )


# writes

async def write_accel_point(
        reading: SensorReading, 
        ts: datetime,
        write_api: Optional[WriteAPI] = None,        
) -> datetime:
    """
    Write a single accelerometer point. If write_api is not provided, create a short-lived client.
    Returns the timestamp stored.
    """
    close_client = False
    if write_api is None:
        client = _mk_client()
        write_api = client.write_api()
        close_client = True
    try:
        p = (
                Point("accel")
                .tag("device_id", reading.device_id)
                .field("x", float(reading.x))
                .field("y", float(reading.y))
                .field("z", float(reading.z))
                .time(ts, WritePrecision.NS) # high precision; Influx stores in ns
        )
        write_api.write(bucket=settings.influx_bucket, record=p)
        return ts
    finally:
        if close_client:
            # Make sure to close only if we created it here
            write_api.close() # closes underlying client worker
            try:
                client.close() # type: ignore[name-defined]
            except Exception:
                pass

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
