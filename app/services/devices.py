from __future__ import annotations
from dataclasses import dataclass, asdict
from datetime import datetime, timezone
from pathlib import Path
import json
from typing import Optional, Dict, List

_REGISTRY_PATH = Path("./data/devices.json")
_REGISTRY_PATH.parent.mkdir(parents=True, exist_ok=True)

@dataclass
class DeviceRecord:
    device_id: str
    label: Optional[str] = None
    notes: Optional[str] = None
    last_seen: Optional[str] = None  # ISO8601 string

def _load() -> Dict[str, DeviceRecord]:
    if not _REGISTRY_PATH.exists():
        return {}
    with _REGISTRY_PATH.open("r", encoding="utf-8") as f:
        raw = json.load(f)
    return {k: DeviceRecord(**v) for k, v in raw.items()}

def _save(state: Dict[str, DeviceRecord]) -> None:
    with _REGISTRY_PATH.open("w", encoding="utf-8") as f:
        json.dump({k: asdict(v) for k, v in state.items()}, f, indent=2)

def register(device_id: str, label: Optional[str] = None, notes: Optional[str] = None) -> DeviceRecord:
    state = _load()
    rec = state.get(device_id) or DeviceRecord(device_id=device_id)
    if label is not None:
        rec.label = label
    if notes is not None:
        rec.notes = notes
    state[device_id] = rec
    _save(state)
    return rec

def all_devices() -> List[DeviceRecord]:
    return list(_load().values())

def get(device_id: str) -> Optional[DeviceRecord]:
    return _load().get(device_id)

def touch_last_seen(device_id: str, ts: Optional[datetime] = None) -> None:
    state = _load()
    rec = state.get(device_id) or DeviceRecord(device_id=device_id)
    iso = (ts or datetime.now(timezone.utc)).isoformat()
    rec.last_seen = iso
    state[device_id] = rec
    _save(state)

def status(rec: DeviceRecord, offline_after_seconds: int = 60) -> str:
    if not rec.last_seen:
        return "unknown"
    try:
        last = datetime.fromisoformat(rec.last_seen)
    except Exception:
        return "unknown"
    delta = datetime.now(timezone.utc) - last
    return "online" if delta.total_seconds() < offline_after_seconds else "offline"

