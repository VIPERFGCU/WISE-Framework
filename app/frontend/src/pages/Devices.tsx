import { useEffect, useMemo, useState } from "react";
import { listDevices } from "../services/devices";
import type { Device, DeviceStatus } from "../types/device";
import StatusBadge from "../components/StatusBadge";
import BoolPill from "../components/BoolPill";
import { formatUptime } from "../lib/format";

type Tri = "all" | "yes" | "no";
type SortKey = "uptime_asc" | "uptime_desc" | "status";

export default function Devices() {
  const [devices, setDevices] = useState<Device[]>([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);

  // controls
  const [q, setQ] = useState("");
  const [status, setStatus] = useState<DeviceStatus | "all">("all");
  const [rec, setRec] = useState<Tri>("all");
  const [sense, setSense] = useState<Tri>("all");
  const [sortKey, setSortKey] = useState<SortKey>("uptime_desc");

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
    const id = setInterval(load, 10_000);
    return () => clearInterval(id);
  }, []);

  const rows = useMemo(() => {
    // filter
    let out = devices.filter((d) => {
      if (status !== "all" && d.status !== status) return false;
      if (rec !== "all" && d.recording !== (rec === "yes")) return false;
      if (sense !== "all" && d.sensing !== (sense === "yes")) return false;
      if (q && !d.sensor_id.toLowerCase().includes(q.toLowerCase())) return false;
      return true;
    });

    // sort
    if (sortKey === "uptime_asc") {
      out = out.slice().sort((a, b) => a.uptime_seconds - b.uptime_seconds);
    } else if (sortKey === "uptime_desc") {
      out = out.slice().sort((a, b) => b.uptime_seconds - a.uptime_seconds);
    } else if (sortKey === "status") {
      const order: Record<DeviceStatus, number> = { on: 0, updating: 1, off: 2 };
      out = out.slice().sort((a, b) => order[a.status] - order[b.status]);
    }

    return out;
  }, [devices, q, status, rec, sense, sortKey]);

  return (
    <div className="space-y-4">
      <div className="flex items-center justify-between gap-3 flex-wrap">
        <h2 className="text-lg font-semibold">Devices</h2>
        <div className="flex items-center gap-2">
          <button
            onClick={load}
            className="px-3 py-1.5 text-sm border rounded bg-white hover:bg-gray-50"
            disabled={loading}
            title="Refresh"
          >
            Refresh
          </button>
        </div>
      </div>

      {/* Controls */}
      <div className="bg-white border rounded p-3 grid gap-2 md:grid-cols-5">
        <input
          value={q}
          onChange={(e) => setQ(e.target.value)}
          placeholder="Search sensor id…"
          className="border rounded px-3 py-2 text-sm md:col-span-2"
        />
        <select
          value={status}
          onChange={(e) => setStatus(e.target.value as any)}
          className="border rounded px-3 py-2 text-sm"
          title="Status"
        >
          <option value="all">Status: All</option>
          <option value="on">Status: On</option>
          <option value="updating">Status: Updating</option>
          <option value="off">Status: Off</option>
        </select>
        <select
          value={rec}
          onChange={(e) => setRec(e.target.value as Tri)}
          className="border rounded px-3 py-2 text-sm"
          title="Recording"
        >
          <option value="all">Recording: All</option>
          <option value="yes">Recording: Y</option>
          <option value="no">Recording: N</option>
        </select>
        <select
          value={sense}
          onChange={(e) => setSense(e.target.value as Tri)}
          className="border rounded px-3 py-2 text-sm"
          title="Sensing"
        >
          <option value="all">Sensing: All</option>
          <option value="yes">Sensing: Y</option>
          <option value="no">Sensing: N</option>
        </select>
        <select
          value={sortKey}
          onChange={(e) => setSortKey(e.target.value as SortKey)}
          className="border rounded px-3 py-2 text-sm md:col-span-2"
          title="Sort"
        >
          <option value="uptime_desc">Sort: Uptime ↓</option>
          <option value="uptime_asc">Sort: Uptime ↑</option>
          <option value="status">Sort: Status</option>
        </select>
      </div>

      {/* Table */}
      {loading && <div className="text-gray-600">Loading devices…</div>}
      {error && <div className="text-red-600">Error: {error}</div>}
      {!loading && !error && rows.length === 0 && (
        <div className="text-gray-600">No devices match your filters.</div>
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
          <div className="text-xs text-gray-500 mt-2">Auto-refreshing every 10s</div>
        </div>
      )}
    </div>
  );
}

