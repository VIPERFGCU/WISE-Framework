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
    status: str
