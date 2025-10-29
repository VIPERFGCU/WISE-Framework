import type { Device, DeviceStatus } from "../types/device";

// Try to interpret status from various shapes
function normalizeStatus(v: any): DeviceStatus {
  if (v === "on" || v === "off" || v === "updating") return v;
  // common alternatives: true/false/online/offline/updating
  if (v === true || v === "online" || v === 1) return "on";
  if (v === false || v === "offline" || v === 0) return "off";
  return "updating";
}

export function normalizeDevice(raw: any): Device {
  const sensor_id =
    raw?.sensor_id ??
    raw?.id ??
    raw?.device_id ??
    raw?.name ??
    raw?.uid ??
    "unknown";

  return {
    sensor_id: String(sensor_id),
    status: normalizeStatus(raw?.status ?? raw?.online ?? raw?.state),
    recording: Boolean(raw?.recording ?? raw?.is_recording ?? raw?.rec),
    sensing: Boolean(raw?.sensing ?? raw?.is_sensing ?? raw?.sense),
    uptime_seconds: Number(raw?.uptime_seconds ?? raw?.uptime ?? 0),
    updated_at: raw?.updated_at ?? raw?.last_seen ?? undefined,
  };
}

