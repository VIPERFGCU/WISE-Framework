import { useEffect, useMemo, useState } from "react";
import { useParams } from "react-router-dom";
import { listDevices } from "../services/devices";
import type { Device } from "../types/device";
import StatusBadge from "../components/StatusBadge";
import BoolPill from "../components/BoolPill";
import { formatUptime, parseIsoMs, timeAgo } from "../lib/format";

export default function SensorProfile() {
  const { sensorId } = useParams<{ sensorId: string }>();
  const [device, setDevice] = useState<Device | null>(null);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);

  const decodedSensorId = useMemo(() => {
    try {
      return sensorId ? decodeURIComponent(sensorId) : "";
    } catch {
      return sensorId ?? "";
    }
  }, [sensorId]);

  useEffect(() => {
    async function load() {
      if (!decodedSensorId) {
        setError("Missing sensor id");
        setLoading(false);
        return;
      }

      try {
        setError(null);
        const devices = await listDevices();
        const match = devices.find((d) => d.sensor_id === decodedSensorId) ?? null;
        setDevice(match);
      } catch (e: any) {
        setError(e?.message ?? "Failed to load sensor profile");
      } finally {
        setLoading(false);
      }
    }

    load();
    const id = setInterval(load, 15000);
    return () => clearInterval(id);
  }, [decodedSensorId]);

  const t = parseIsoMs(device?.updated_at);

  return (
    <div className="min-h-screen bg-slate-950 text-slate-100 p-4 sm:p-6">
      <div className="mx-auto max-w-xl">
        <div className="rounded-lg border border-slate-700 bg-slate-900/70 p-4 sm:p-6">
          <h1 className="text-xl font-semibold mb-1">Sensor Profile</h1>
          <p className="text-sm text-slate-400 mb-4">WISENET device information</p>

          {loading && <div className="text-slate-300">Loading…</div>}

          {!loading && error && (
            <div className="text-rose-300">{error}</div>
          )}

          {!loading && !error && !device && (
            <div className="text-slate-300">No device found for id: <span className="font-mono">{decodedSensorId}</span></div>
          )}

          {!loading && !error && device && (
            <div className="space-y-4">
              <section className="rounded border border-slate-700 p-3 bg-slate-950/60">
                <div className="text-xs text-slate-400">Sensor ID</div>
                <div className="font-mono text-base break-all">{device.sensor_id}</div>
                <div className="text-xs text-slate-400 mt-2">Label</div>
                <div>{device.label ?? "-"}</div>
              </section>

              <section className="rounded border border-slate-700 p-3 bg-slate-950/60 grid grid-cols-2 gap-3">
                <div>
                  <div className="text-xs text-slate-400 mb-1">Status</div>
                  <StatusBadge status={device.status} />
                </div>
                <div>
                  <div className="text-xs text-slate-400 mb-1">Uptime</div>
                  <div>{formatUptime(device.uptime_seconds)}</div>
                </div>
                <div>
                  <div className="text-xs text-slate-400 mb-1">Sensing</div>
                  <BoolPill value={device.sensing} />
                </div>
                <div>
                  <div className="text-xs text-slate-400 mb-1">Recording</div>
                  <BoolPill value={device.recording} />
                </div>
              </section>

              <section className="rounded border border-slate-700 p-3 bg-slate-950/60">
                <div className="text-xs text-slate-400">Last Updated</div>
                <div>
                  {Number.isFinite(t)
                    ? `${timeAgo(Date.now() - t)} (${new Date(t).toLocaleString()})`
                    : "-"}
                </div>
              </section>
            </div>
          )}
        </div>
      </div>
    </div>
  );
}
