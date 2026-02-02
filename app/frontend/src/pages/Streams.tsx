import { useEffect, useState } from "react";
import { api } from "../api/client";
import { useRef } from "react";

type Point = { t: string; x: number; y: number; z: number };
type HeartbeatPoint = { t: string; rssi: number };

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

function MultiSparkline({ series }: { series: { data: number[]; color: string; label?: string }[] }) {
  const w = 600, h = 140, pad = 6;
  if (!series || series.length === 0) return <div className="text-sm text-gray-500">No data</div>;
  const lengths = series.map(s => s.data.length);
  const maxLen = Math.max(...lengths, 1);
  // Flatten values to compute global min/max
  const values = series.flatMap(s => s.data);
  if (values.length === 0) return <div className="text-sm text-gray-500">No data</div>;
  const min = Math.min(...values), max = Math.max(...values);
  const scaleX = (i: number) => pad + (i / Math.max(1, maxLen - 1)) * (w - pad * 2);
  const scaleY = (v: number) => {
    if (max === min) return h / 2;
    return pad + (1 - (v - min) / (max - min)) * (h - pad * 2);
  };

  const paths = series.map(s => {
    const d = s.data.map((v, i) => `${i === 0 ? 'M' : 'L'} ${scaleX(i)} ${scaleY(v)}`).join(' ');
    return <path key={s.color} d={d} fill="none" stroke={s.color} strokeWidth={2} strokeLinejoin="round" strokeLinecap="round" />;
  });

  return (
    <div>
      <div className="flex items-center gap-3 text-sm text-gray-600 mb-1">
        {series.map(s => (
          <div key={s.color} className="flex items-center gap-2">
            <span style={{ width: 12, height: 12, background: s.color, display: 'inline-block', borderRadius: 4 }} />
            <span>{s.label ?? ''}</span>
          </div>
        ))}
      </div>
      <svg viewBox={`0 0 ${w} ${h}`} width="100%" height={h}>
        {paths}
      </svg>
    </div>
  );
}

export default function Streams() {
  const [device, setDevice] = useState<string>("bridge-esp32-001");
  const [windowS, setWindowS] = useState<number>(60);
  const [points, setPoints] = useState<Point[]>([]);
  const [heartbeatPoints, setHeartbeatPoints] = useState<HeartbeatPoint[]>([]);
  const [loading, setLoading] = useState(false);
  const [refreshing, setRefreshing] = useState(false);
  const [devices, setDevices] = useState<{ device_id: string; label?: string }[]>([]);
  // Live MQTT stream via backend WebSocket
  const [livePoints, setLivePoints] = useState<Point[]>([]);
  const [liveHeartbeat, setLiveHeartbeat] = useState<HeartbeatPoint | null>(null);
  const wsRef = useRef<WebSocket | null>(null);

  useEffect(() => {
    try {
      const scheme = window.location.protocol === "https:" ? "wss" : "ws";
      const ws = new WebSocket(`${scheme}://${window.location.host}/api/v1/stream`);
      wsRef.current = ws;
      ws.onopen = () => console.info("WS open");
      ws.onclose = () => console.info("WS closed");
      ws.onerror = (e) => console.warn("WS error", e);
      ws.onmessage = (ev) => {
        try {
          const msg = JSON.parse(ev.data);
          if (msg.type === "data") {
            const p: Point = { t: msg.ts, x: msg.x, y: msg.y, z: msg.z };
            setLivePoints(prev => {
              const next = [...prev, p].slice(-200);
              return next;
            });
          } else if (msg.type === "heartbeat") {
            const hb: HeartbeatPoint = { t: msg.ts, rssi: msg.rssi };
            setLiveHeartbeat(hb);
          }
        } catch (e) {
          // ignore malformed
        }
      };
      return () => {
        try { ws.close(); } catch (e) {}
      };
    } catch (e) {
      // ignore
    }
  }, []);

  const load = async (silent: boolean = false) => {
    try {
      if (silent) {
        setRefreshing(true);
      } else {
        setLoading(true);
      }
      const res = await api.get(`/api/preview/${encodeURIComponent(device)}?window_s=${windowS}`);
      
      // Parse accelerometer data
      if (res.data?.accel?.series) {
        const pts = res.data.accel.series.map((p: any) => ({ t: p.t, x: p.x, y: p.y, z: p.z }));
        setPoints(pts);
      } else if (!silent) {
        setPoints([]);
      }

      // Parse heartbeat data
      if (res.data?.heartbeat?.series) {
        const hbPts = res.data.heartbeat.series.map((p: any) => ({ t: p.t, rssi: p.rssi }));
        setHeartbeatPoints(hbPts);
      } else if (!silent) {
        setHeartbeatPoints([]);
      }
    } catch (e) {
      if (!silent) {
        setPoints([]);
        setHeartbeatPoints([]);
      }
    } finally {
      setLoading(false);
      setRefreshing(false);
    }
  };

  const loadDevices = async () => {
    try {
      const res = await api.get("/api/v1/devices");
      if (Array.isArray(res.data)) {
        setDevices(res.data.map((d: any) => ({ device_id: d.device_id, label: d.label })));
      }
    } catch (e) {
      setDevices([]);
    }
  };

  useEffect(() => {
    loadDevices();
  }, []);

  useEffect(() => {
    setPoints([]);
    setHeartbeatPoints([]);
    load(false);
    const id = setInterval(() => load(true), 5000);
    return () => clearInterval(id);
  }, [device, windowS]);

  return (
    <div className="flex flex-col h-full space-y-4 p-4">
      <h1 className="text-2xl font-bold text-gray-800">Sensor Streams</h1>

      <div className="flex items-center gap-3">
        <label className="text-sm">Device ID</label>
        <select value={device} onChange={(e) => setDevice(e.target.value)} className="border rounded px-2 py-1">
          {devices.length === 0 && (
            <option value={device}>{device}</option>
          )}
          {devices.map((d) => (
            <option key={d.device_id} value={d.device_id}>
              {d.label ? `${d.label} (${d.device_id})` : d.device_id}
            </option>
          ))}
        </select>
        <label className="text-sm">Window (s)</label>
        <input type="number" value={windowS} onChange={(e) => setWindowS(Number(e.target.value))} className="border rounded px-2 py-1 w-24" />
        <button onClick={() => load(false)} className="px-3 py-1 rounded bg-blue-600 text-white">Refresh</button>
      </div>

      <div className="bg-white border rounded p-3">
        {/* Live stream panel */}
        <div className="mb-4 p-2 border rounded bg-gray-50">
          <div className="flex justify-between items-center mb-2">
            <div className="text-sm text-gray-700 font-semibold">Live Stream</div>
            <div className="text-xs text-gray-500">WebSocket</div>
          </div>
          <div className="flex items-center gap-4">
            <div className="text-sm text-gray-600">Latest RSSI:</div>
            <div className="text-sm font-mono">{liveHeartbeat ? liveHeartbeat.rssi : "—"}</div>
            <div className="ml-6 text-sm text-gray-600">Latest Accel:</div>
            <div className="text-sm font-mono">{livePoints.length ? `${livePoints[livePoints.length-1].x.toFixed(3)}, ${livePoints[livePoints.length-1].y.toFixed(3)}, ${livePoints[livePoints.length-1].z.toFixed(3)}` : "—"}</div>
          </div>
          <div className="mt-3">
            {livePoints.length > 0 && (
              <MultiSparkline series={[
                { data: livePoints.map(p => p.x), color: '#ef4444', label: 'X' },
                { data: livePoints.map(p => p.y), color: '#06b6d4', label: 'Y' },
                { data: livePoints.map(p => p.z), color: '#10b981', label: 'Z' }
              ]} />
            )}
          </div>
        </div>
        {(loading || refreshing) && (
          <div className="text-xs text-gray-500 mb-2">
            {loading ? "Loading…" : "Refreshing…"}
          </div>
        )}
        {!loading && points.length === 0 && heartbeatPoints.length === 0 && (
          <div className="text-sm text-gray-500">
            No data available. Graphs will appear here when sensor data is available.
          </div>
        )}
        
        {/* Heartbeat Graph */}
        {heartbeatPoints.length > 0 && (
          <div className="mb-6">
            <div className="text-sm text-gray-600 font-semibold mb-2">Signal Strength (RSSI)</div>
            <Sparkline data={heartbeatPoints.map(p => p.rssi)} color="#8b5cf6" />
          </div>
        )}

        {/* Accelerometer Graphs */}
        {points.length > 0 && (
          <div>
            <div className="text-sm text-gray-600 font-semibold mb-2">Combined X / Y / Z</div>
            <MultiSparkline series={[
              { data: points.map(p => p.x), color: '#ef4444', label: 'X' },
              { data: points.map(p => p.y), color: '#06b6d4', label: 'Y' },
              { data: points.map(p => p.z), color: '#10b981', label: 'Z' }
            ]} />
            <div className="h-4" />
            <div className="text-sm text-gray-600 font-semibold mb-2">X axis</div>
            <Sparkline data={points.map(p => p.x)} color="#ef4444" />
            <div className="text-sm text-gray-600 font-semibold mt-4 mb-2">Y axis</div>
            <Sparkline data={points.map(p => p.y)} color="#06b6d4" />
            <div className="text-sm text-gray-600 font-semibold mt-4 mb-2">Z axis</div>
            <Sparkline data={points.map(p => p.z)} color="#10b981" />
          </div>
        )}
      </div>
    </div>
  );
}
