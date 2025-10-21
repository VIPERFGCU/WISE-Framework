
from fastapi import APIRouter, Depends, HTTPException
from influxdb_client import Point, WritePrecision
from influxdb_client.client.write_api import SYNCHRONOUS
from datetime import datetime, timezone
from typing import Any
import os

from ..schemas import Ingest, QueryParams
from ..deps import get_influx_client

router = APIRouter(prefix="/api/v1", tags=["data"])

@router.post("/ingest")
def ingest(payload: Ingest):
    # Default ts to server time
    ts = payload.ts or datetime.now(timezone.utc)

    client = get_influx_client()
    if client is None:
        # Accept request even without Influx configured (no-op write)
        return {"status": "accepted", "note": "No INFLUX_* env set; skipping DB write."}

    bucket = os.getenv("INFLUX_BUCKET", "sensors")
    write_api = client.write_api(write_options=SYNCHRONOUS)
    p = (
        Point("sensor_data")
        .tag("device_id", payload.device_id)
        .field("x", float(payload.x))
        .field("y", float(payload.y))
        .field("z", float(payload.z))
        .time(ts, WritePrecision.NS)
    )
    write_api.write(bucket=bucket, record=p)
    return {"status": "ok"}

@router.get("/data")
def get_data(device_id: str, range: str = "1h", limit: int = 1000) -> list[dict[str, Any]]:
    client = get_influx_client()
    if client is None:
        raise HTTPException(status_code=501, detail="InfluxDB not configured (set INFLUX_* envs)")

    bucket = os.getenv("INFLUX_BUCKET", "sensors")
    org = os.getenv("INFLUX_ORG", "my-org")
    query = f'''
from(bucket: "{bucket}")
  |> range(start: -{range})
  |> filter(fn: (r) => r["_measurement"] == "sensor_data")
  |> filter(fn: (r) => r["device_id"] == "{device_id}")
  |> filter(fn: (r) => r["_field"] == "x" or r["_field"] == "y" or r["_field"] == "z")
  |> pivot(rowKey:["_time"], columnKey: ["_field"], valueColumn: "_value")
  |> sort(columns: ["_time"], desc: false)
  |> limit(n:{limit})
'''
    tables = client.query_api().query(query=query, org=org)
    out: list[dict[str, Any]] = []
    for table in tables:
        for rec in table.records:
            vals = rec.values  # dict
            t = vals.get("_time")
            time_str = t.isoformat() if hasattr(t, "isoformat") else (str(t) if t is not None else None)
            out.append({
                "time": time_str,
                "device_id": vals.get("device_id"),
                "x": vals.get("x"),
                "y": vals.get("y"),
                "z": vals.get("z"),
        })
    return out

