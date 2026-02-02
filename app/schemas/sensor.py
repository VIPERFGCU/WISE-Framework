from pydantic import BaseModel, Field
from typing import Optional, List
from datetime import datetime

class SensorReading(BaseModel):
    device_id: str = Field(..., min_length=1)
    x: float = Field(..., ge=-20.0, le=20.0)
    y: float = Field(..., ge=-20.0, le=20.0)
    z: float = Field(..., ge=-20.0, le=20.0)
    ts: Optional[datetime] = None

    class Config:
        json_schema_extra = {
            "example": {
                "device_id": "esp32-bridge-01",
                "x": -0.0123, "y": 0.9811, "z": 0.0456,
                "ts": "2025-09-25T13:22:11.702Z"
            }
        }

class IngestAck(BaseModel):
    status: str
    device_id: str
    stored_at: datetime

class AccelPoint(BaseModel):
    t: datetime
    x: float
    y: float
    z: float

    class Config:
        json_schema_extra = {
            "example": {"t": "2025-09-25T13:22:11.702Z", "x": 0.01, "y": -0.04, "z":0.99}
        }

class AccelSeries(BaseModel):
    device_id: str
    series: List[AccelPoint]

    class Config:
        json_schema_extra = {
                "example": {
                    "device_id": "esp32-bridge-01",
                    "series": [
                        {"t": "2025-09-25T13:22:11.702Z", "x": 0.01, "y": -0.04, "z":0.99},
                        {"t": "2025-09-25T13:22:12.702Z", "x": 0.02, "y": -0.03, "z":1.00}
                    ]
                }
            }

class DeviceHeartbeat(BaseModel):
    device_id: Optional[str] = None
    rssi: int = Field(..., description="Signal strength in dBm")
    uptime_s: int = Field(..., description="Uptime in seconds")
    fw: Optional[str] = None
    ts: Optional[str] = None

    class Config:
        json_schema_extra = {
            "example": {
                "device_id": "esp32-bridge-01",
                "rssi": -45,
                "uptime_s": 3600,
                "fw": "esp32-demo-1.0",
                "ts": "2025-09-25T13:22:11.702Z"
            }
        }

class HeartbeatPoint(BaseModel):
    t: datetime
    rssi: int

    class Config:
        json_schema_extra = {
            "example": {"t": "2025-09-25T13:22:11.702Z", "rssi": -45}
        }

class HeartbeatSeries(BaseModel):
    device_id: str
    series: List[HeartbeatPoint]

    class Config:
        json_schema_extra = {
            "example": {
                "device_id": "esp32-bridge-01",
                "series": [
                    {"t": "2025-09-25T13:22:11.702Z", "rssi": -45},
                    {"t": "2025-09-25T13:22:12.702Z", "rssi": -44}
                ]
            }
        }
