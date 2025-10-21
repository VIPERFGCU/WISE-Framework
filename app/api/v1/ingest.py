from datetime import datetime, timezone
from fastapi import APIRouter, Depends, status

from app import deps
from app.schemas.sensor import SensorReading, IngestAck
from app.services.influx import write_accel_point
from app.services import devices as devices_svc

router = APIRouter(prefix="/api/v1/ingest", tags=["ingest"])

@router.post(
    "/accel",
    response_model=IngestAck,
    status_code=status.HTTP_202_ACCEPTED,
    summary="Ingest accelerometer reading",
    # Accepting either x-api-key or JWT with 'device' role
    dependencies=[Depends(deps.require_api_key_or_role("device"))],
)
async def ingest_accel(
        reading: SensorReading,
        write_api = Depends(deps.get_influx_write_api),
) -> IngestAck:
    # Ensure server timestamp if missing
    ts = reading.ts or datetime.now(timezone.utc)
    
    # Write to influx (await if client is async; keep as-is otherwise)
    stored_at = await write_accel_point(reading, ts=ts)
    
    # Mark device as seen
    devices_svc.touch_last_seen(reading.device_id, ts=stored_at)
    
    return IngestAck(status="accepted", device_id=reading.device_id, stored_at=stored_at)

