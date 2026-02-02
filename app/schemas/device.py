from pydantic import BaseModel
from datetime import datetime
from typing import Optional

class DeviceCreate(BaseModel):
    device_id: str
    label: Optional[str] = None
    notes: Optional[str] = None
    
class DeviceOut(BaseModel):
    device_id: str
    label: Optional[str] = None
    last_seen: Optional[datetime] = None
    status: str # "on" | "off" | "updating" | "streaming" | "stopped" | "unknown"
    
    # MQTT-derived telemetry state
    sensing: bool = False  # Is device currently streaming data?
    sample_hz: Optional[int] = None  # Current sampling frequency (Hz)
    batch_size: Optional[int] = None  # Current batch size
    uptime_seconds: int = 0  # Device uptime in seconds
