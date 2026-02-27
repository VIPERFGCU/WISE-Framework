import type { Device, DeviceStatus } from "../types/device";

// Try to interpret status from various shapes
function normalizeStatus(v: any, raw: any): DeviceStatus {
  if (v === "on" || v === "off" || v === "updating") return v;

  if (v === true || v === 1) return "on";
  if (v === false || v === 0) return "off";

  const value = typeof v === "string" ? v.toLowerCase().trim() : "";
  if (["online", "streaming", "running", "active", "connected", "stopped", "idle"].includes(value)) {
    return "on";
  }
  if (["offline", "disconnected", "down", "unreachable"].includes(value)) {
    return "off";
  }
  if (["updating", "booting", "starting", "restarting", "pending"].includes(value)) {
    return "updating";
  }

  const seen = raw?.updated_at ?? raw?.last_seen;
  if (seen) {
    const ts = Date.parse(String(seen));
    if (Number.isFinite(ts)) {
      // if recently seen, treat as online; otherwise offline
      return Date.now() - ts < 180_000 ? "on" : "off";
    }
  }

  if (raw?.sensing === true || raw?.is_sensing === true || raw?.sense === true) {
    return "on";
  }

  return "off";
}

export function normalizeDevice(raw: any): Device {
  const sensor_id =
    raw?.sensor_id ??
    raw?.id ??
    raw?.device_id ??
    raw?.name ??
    raw?.uid ??
    "unknown";
  const label = raw?.label ?? raw?.name ?? raw?.label_text ?? undefined;

  return {
    sensor_id: String(sensor_id),
    label: label ? String(label) : undefined,
    status: normalizeStatus(raw?.status ?? raw?.online ?? raw?.state, raw),
    recording: Boolean(raw?.recording ?? raw?.is_recording ?? raw?.rec),
    sensing: Boolean(raw?.sensing ?? raw?.is_sensing ?? raw?.sense),
    uptime_seconds: Number(raw?.uptime_seconds ?? raw?.uptime ?? 0),
    updated_at: raw?.updated_at ?? raw?.last_seen ?? undefined,
    sample_hz:
	    raw?.sample_hz ??
	    raw?.rate_hz ??
	    raw?.frequency_hz ??
	    raw?.freq ??
	    undefined,
    batch_size:
	    raw?.batch_size ??
	    raw?.batch ??
	    undefined,
  };
}

