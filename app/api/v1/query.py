from fastapi import APIRouter, Query, Depends
from typing import List
from app import deps
from app.schemas.sensor import AccelSeries, AccelSpectrum
from app.services.influx import read_accel_series, compute_accel_spectrum

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


@router.get(
        "/accel/spectrum",
        response_model=AccelSpectrum,
    summary="Get accelerometer frequency-domain spectrum",
)
async def get_accel_spectrum(
    device_id: str = Query(..., description="Device id tag"),
    range: str = Query("15m", description="Flux-style duration like 15m,1h,24h"),
    axes: str = Query("x,y,z,mag", description="Comma-separated axes: x,y,z,mag"),
    sample_rate_hz: float | None = Query(None, gt=0, description="Optional sampling rate override in Hz"),
    max_bins: int = Query(256, ge=16, le=4096),
    query_api = Depends(deps.get_influx_query_api),
) -> AccelSpectrum:
    accel = await read_accel_series(device_id=device_id, range=range, query_api=query_api)
    raw_axes: List[str] = [a.strip().lower() for a in axes.split(",") if a.strip()]
    allowed = {"x", "y", "z", "mag"}
    selected_axes = [a for a in raw_axes if a in allowed]
    if not selected_axes:
        selected_axes = ["x", "y", "z", "mag"]

    spectra = [
        compute_accel_spectrum(
            points=accel.series,
            axis=axis,
            sample_rate_hz=sample_rate_hz,
            max_bins=max_bins,
        )
        for axis in selected_axes
    ]

    return AccelSpectrum(
        device_id=device_id,
        range=range,
        spectra=spectra,
    )

