import { useEffect, useState } from "react";
import { api } from "../api/client";

type Point = { t: string; x: number; y: number; z: number };

function Sparkline({ data, color }: { data: number[]; color: string }) {
  const w = 600, h = 120, pad = 6;
  if (!data || data.length === 0) return <div className="text-sm text-gray-500">No data</div>;
  const min = Math.min(...data), max = Math.max(...data);
  const scaleX = (i: number) => pad + (i / Math.max(1, data.length - 1)) * (w - pad * 2);
  const scaleY = (v: number) => {
    if (max === min) return h / 2;
    return pad + (1 - (v - min) / (max - min)) * (h - pad * 2);
  };
  const d = data.map((v, i) => `${i === 0 ? 'M' : 'L'} ${scaleX(i)} ${scaleY(v)}`).join(' ');
  return (
    <svg viewBox={`0 0 ${w} ${h}`} width="100%" height="120">
      <path d={d} fill="none" stroke={color} strokeWidth={2} strokeLinejoin="round" strokeLinecap="round" />
    </svg>
  );
}

export default function Streams() {
  const [device, setDevice] = useState<string>("bridge-esp32-001");
  const [windowS, setWindowS] = useState<number>(60);
  const [points, setPoints] = useState<Point[]>([]);
  const [loading, setLoading] = useState(false);

  const load = async () => {
    try {
      setLoading(true);
      const res = await api.get(`/api/preview/${encodeURIComponent(device)}?window_s=${windowS}`);
      const data = res.data?.series ?? [];
      // convert to expected shape
      const pts = data.map((p: any) => ({ t: p.t, x: p.x, y: p.y, z: p.z }));
      setPoints(pts);
    } catch (e) {
      setPoints([]);
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => { load(); const id = setInterval(load, 5000); return () => clearInterval(id); }, [device, windowS]);

  return (
    <div className="flex flex-col h-full space-y-4 p-4">
      <h1 className="text-2xl font-bold text-gray-800">Sensor Streams</h1>

      <div className="flex items-center gap-3">
        <label className="text-sm">Device ID</label>
        <input value={device} onChange={(e) => setDevice(e.target.value)} className="border rounded px-2 py-1" />
        <label className="text-sm">Window (s)</label>
        <input type="number" value={windowS} onChange={(e) => setWindowS(Number(e.target.value))} className="border rounded px-2 py-1 w-24" />
        <button onClick={load} className="px-3 py-1 rounded bg-blue-600 text-white">Refresh</button>
      </div>

      <div className="bg-white border rounded p-3">
        {loading && <div className="text-sm text-gray-500">Loading…</div>}
        {!loading && points.length === 0 && <div className="text-sm text-gray-500">No data for this device/window.</div>}
        {!loading && points.length > 0 && (
          <div>
            <div className="text-sm text-gray-600">X axis</div>
            <Sparkline data={points.map(p => p.x)} color="#ef4444" />
            <div className="text-sm text-gray-600 mt-2">Y axis</div>
            <Sparkline data={points.map(p => p.y)} color="#06b6d4" />
            <div className="text-sm text-gray-600 mt-2">Z axis</div>
            <Sparkline data={points.map(p => p.z)} color="#10b981" />
          </div>
        )}
      </div>
    </div>
  );
}
