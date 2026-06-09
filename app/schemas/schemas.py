
from pydantic import BaseModel, Field
from typing import Optional
from datetime import datetime

class Ingest(BaseModel):
    device_id: str = Field(..., examples=["esp32-1"])
    ts: Optional[datetime] = Field(None, description="ISO8601 timestamp; if omitted, server time will be used")
    x: float
    y: float
    z: float

class QueryParams(BaseModel):
    device_id: str
    range: str = Field("1h", description="Influx range window, e.g., 15m, 1h, 24h")
    limit: int = Field(1000, ge=1, le=10000)
