from fastapi import APIRouter, Query, Depends
from app import deps
from app.schemas.sensor import AccelSeries, AccelPoint
from app.services.influx import read_accel_series

router = APIRouter(prefix="/api/v1/query", tags=["query"])

@router.get(
        "/accel",
        response_model=AccelSeries, 
        summary="Get time series accelerometer data",
        dependencies=[Depends(deps.require_role("viewer"))],
)
async def get_accel(
    device_id: str = Query(..., description="Device id tag"),
    range: str = Query("15m", description="Flux-style duration like 15m,1h,24h"),
    query_api = Depends(deps.get_influx_query_api),
) -> AccelSeries:
    series = await read_accel_series(device_id=device_id, range=range)
    return series

