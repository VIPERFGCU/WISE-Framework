from fastapi import APIRouter, Query
import os, time
from datetime import datetime, timezone, timedelta
from typing import List, Dict, Any
# import your existing Influx client helper here

router = APIRouter(prefix="/api", tags=["preview"])

@router.get("/preview/{device_id}")
def preview(device_id: str, window_s: int = Query(10, ge=1, le=120)):
    # Replace with your actual Influx query helper
    # Return list of {ts,x,y,z}
    return []  # TEMP stub so the UI doesn't break if Influx is not wired yet

