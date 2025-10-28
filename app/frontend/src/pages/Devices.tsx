import { useEffect, useMemo, useState } from "react";
import { listDevices } from "../services/devices";
import type { Device } from "../types/device";
import StatusBadge from "../components/StatusBadge";
import BoolPill from "../components/BoolPill";
import { formatUptime } from "../lib/format";

export default function Devices() {
  const [devices, setDevices] = useState<Device[]>([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);

  async function load() {
    try {
      setError(null);
      const data = await listDevices();
      setDevices(data ?? []);
    } catch (e: any) {
      setError(e?.message ?? "Failed to fetch devices");
    } finally {
      setLoading(false);
    }
  }

  useEffect(() => {
    load();
    const id = setInterval(load, 10_000); // poll every 10s
    return () => clearInterval(id);
  }, []);

  const rows = useMemo(() => devices, [devices]);

  return (
    <div className="space-y-4">
      <div className="flex items-center justify-between">
        <h2 className="text-lg font-semibold">Devices</h2>
        <button
          onClick={load}
          className="px-3 py-1.5 text-sm border rounded bg-white hover:bg-gray-50"
          disabled={loading}
          title="Refresh"
        >
          Refresh
        </button>
      </div>

      {loading && (
        <div className="text-gray-600">Loading devices…</div>
      )}

      {error && (
        <div className="text-red-600">Error: {error}</div>
      )}

      {!loading && !error && rows.length === 0 && (
        <div className="text-gray-600">No devices found.</div>
      )}

      {!loading && !error && rows.length > 0 && (
        <div className="overflow-x-auto">
          <table className="min-w-full bg-white border rounded">
            <thead className="text-left text-sm text-gray-600 border-b">
              <tr>
                <th className="px-3 py-2">Sensor ID</th>
                <th className="px-3 py-2">Status</th>
                <th className="px-3 py-2">Recording</th>
                <th className="px-3 py-2">Sensing</th>
                <th className="px-3 py-2">Up time</th>
              </tr>
            </thead>
            <tbody className="text-sm">
              {rows.map((d) => (
                <tr key={d.sensor_id} className="border-t">
                  <td className="px-3 py-2 font-mono">{d.sensor_id}</td>
                  <td className="px-3 py-2"><StatusBadge status={d.status} /></td>
                  <td className="px-3 py-2"><BoolPill value={d.recording} /></td>
                  <td className="px-3 py-2"><BoolPill value={d.sensing} /></td>
                  <td className="px-3 py-2">{formatUptime(d.uptime_seconds)}</td>
                </tr>
              ))}
            </tbody>
          </table>
          <div className="text-xs text-gray-500 mt-2">
            Auto-refreshing every 10s
          </div>
        </div>
      )}
    </div>
  );
}

