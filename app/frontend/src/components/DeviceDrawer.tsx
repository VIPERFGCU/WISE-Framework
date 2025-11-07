import { useEffect } from "react";
import type { Device } from "../types/device";
import StatusBadge from "./StatusBadge";
import BoolPill from "./BoolPill";
import { formatUptime, parseIsoMs, timeAgo } from "../lib/format";

export default function DeviceDrawer({
  device,
  onClose,
}: {
  device: Device | null;
  onClose: () => void;
}) {
  useEffect(() => {
    function onEsc(e: KeyboardEvent) {
      if (e.key === "Escape") onClose();
    }
    if (device) document.addEventListener("keydown", onEsc);
    return () => document.removeEventListener("keydown", onEsc);
  }, [device, onClose]);

  if (!device) return null;
  const t = parseIsoMs(device.updated_at);

  return (
    <div className="fixed inset-0 z-50">
      {/* Backdrop */}
      <div className="absolute inset-0 bg-black/30" onClick={onClose} />
      {/* Panel */}
      <div className="absolute top-0 right-0 h-full w-full max-w-md bg-white shadow-xl border-l p-4 overflow-y-auto">
        <div className="flex items-center justify-between mb-4">
          <div className="font-semibold text-lg">Device Details</div>
          <button onClick={onClose} className="text-sm border rounded px-2 py-1 bg-white hover:bg-gray-50">
            Close (Esc)
          </button>
        </div>

        <div className="space-y-4">
          <section className="border rounded p-3 bg-gray-50">
            <div className="text-xs text-gray-500">Sensor ID</div>
            <div className="font-mono">{device.sensor_id}</div>
          </section>

          <section className="border rounded p-3 bg-white">
            <div className="grid grid-cols-2 gap-3">
              <div>
                <div className="text-xs text-gray-500 mb-1">Status</div>
                <StatusBadge status={device.status} />
              </div>
              <div>
                <div className="text-xs text-gray-500 mb-1">Uptime</div>
                <div>{formatUptime(device.uptime_seconds)}</div>
              </div>
              <div>
                <div className="text-xs text-gray-500 mb-1">Recording</div>
                <BoolPill value={device.recording} />
              </div>
              <div>
                <div className="text-xs text-gray-500 mb-1">Sensing</div>
                <BoolPill value={device.sensing} />
              </div>
            </div>
          </section>

          <section className="border rounded p-3 bg-white">
            <div className="text-xs text-gray-500 mb-1">Last Updated</div>
            <div>
              {Number.isFinite(t) ? (
                <>
                  <span className="mr-2">{timeAgo(Date.now() - t)}</span>
                  <span className="text-xs text-gray-500">({new Date(t).toLocaleString()})</span>
                </>
              ) : (
                "—"
              )}
            </div>
          </section>

          {/* Placeholder for future charts/logs */}
          <section className="border rounded p-3 bg-white">
            <div className="text-xs text-gray-500 mb-1">Notes</div>
            <div className="text-sm text-gray-600">
              Future: recent readings, error logs, location, firmware, etc.
            </div>
          </section>
        </div>
      </div>
    </div>
  );
}

