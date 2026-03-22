import { useEffect, useState } from "react";
import { api } from "../api/client";
import { useRef } from "react";
import { MultiSparkline, Sparkline, SpectrumBars, type HeartbeatPoint, type StreamPoint, type SpectrumBin } from "../components/StreamCharts";

const ALL_SENSORS_VALUE = "__all__";

type AxisSpectrum = {
  axis: string;
  sample_rate_hz: number;
  window_samples: number;
  dominant_frequency_hz: number | null;
  bins: SpectrumBin[];
};

export default function Streams() {
  const [device, setDevice] = useState<string>("");
  const [windowS, setWindowS] = useState<number>(60);
  const [viewMode, setViewMode] = useState<"time" | "frequency">("time");
  const [points, setPoints] = useState<StreamPoint[]>([]);
  const [heartbeatPoints, setHeartbeatPoints] = useState<HeartbeatPoint[]>([]);
  const [spectra, setSpectra] = useState<AxisSpectrum[]>([]);
  const [spectrumLoading, setSpectrumLoading] = useState(false);
  const [loading, setLoading] = useState(false);
  const [refreshing, setRefreshing] = useState(false);
  const [downloadingCsv, setDownloadingCsv] = useState(false);
  const [csvFrom, setCsvFrom] = useState<string>("");
  const [csvTo, setCsvTo] = useState<string>("");
  const [devices, setDevices] = useState<{ device_id: string; label?: string }[]>([]);
  // Live MQTT stream via backend WebSocket
  const [livePoints, setLivePoints] = useState<StreamPoint[]>([]);
  const [liveHeartbeat, setLiveHeartbeat] = useState<HeartbeatPoint | null>(null);
  const wsRef = useRef<WebSocket | null>(null);

  function buildStreamWsUrl(): string {
    const apiBase = import.meta.env.VITE_API_BASE_URL as string | undefined;
    const base = apiBase && apiBase !== "/" ? apiBase : window.location.origin;
    const apiUrl = new URL(base, window.location.origin);
    const wsUrl = new URL("/api/v1/stream", apiUrl);
    wsUrl.protocol = apiUrl.protocol === "https:" ? "wss:" : "ws:";
    return wsUrl.toString();
  }

  useEffect(() => {
    try {
      const ws = new WebSocket(buildStreamWsUrl());
      wsRef.current = ws;
      ws.onopen = () => console.info("WS open");
      ws.onclose = () => console.info("WS closed");
      ws.onerror = (e) => console.warn("WS error", e);
      ws.onmessage = (ev) => {
        try {
          const msg = JSON.parse(ev.data);
          if (msg.type === "data") {
            const p: StreamPoint = { t: msg.ts, x: msg.x, y: msg.y, z: msg.z };
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

  const windowToRange = (seconds: number): string => {
    if (seconds < 60) return `${Math.max(1, seconds)}s`;
    if (seconds < 3600) return `${Math.max(1, Math.floor(seconds / 60))}m`;
    return `${Math.max(1, Math.floor(seconds / 3600))}h`;
  };

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
        const items = res.data.map((d: any) => ({ device_id: d.device_id, label: d.label }));
        setDevices(items);
        if (items.length > 0) {
          setDevice((prev) => {
            if (prev === ALL_SENSORS_VALUE) return prev;
            return prev && items.some((d) => d.device_id === prev) ? prev : items[0].device_id;
          });
        }
      }
    } catch (e) {
      setDevices([]);
    }
  };

  const loadSpectrum = async () => {
    if (!device || device === ALL_SENSORS_VALUE) {
      setSpectra([]);
      return;
    }
    try {
      setSpectrumLoading(true);
      const range = windowToRange(windowS);
      const res = await api.get(`/api/v1/query/accel/spectrum?device_id=${encodeURIComponent(device)}&range=${range}&axes=x,y,z,mag&max_bins=192`);
      if (Array.isArray(res.data?.spectra)) {
        setSpectra(res.data.spectra);
      } else {
        setSpectra([]);
      }
    } catch (e) {
      setSpectra([]);
    } finally {
      setSpectrumLoading(false);
    }
  };

  useEffect(() => {
    loadDevices();
    const id = setInterval(loadDevices, 10_000);
    return () => clearInterval(id);
  }, []);

  useEffect(() => {
    if (!device) return;
    if (viewMode === "time") {
      setPoints([]);
      setHeartbeatPoints([]);
      load(false);
      const id = setInterval(() => load(true), 5000);
      return () => clearInterval(id);
    }

    setSpectra([]);
    loadSpectrum();
    const id = setInterval(() => loadSpectrum(), 5000);
    return () => clearInterval(id);
  }, [device, windowS, viewMode]);

  const downloadCsv = async () => {
    if (!device) return;
    try {
      setDownloadingCsv(true);
      const params = new URLSearchParams();
      if (csvFrom && csvTo) {
        params.set("start_ts", new Date(csvFrom).toISOString());
        params.set("end_ts", new Date(csvTo).toISOString());
      } else {
        params.set("window_s", String(windowS));
      }

      const res = await api.get(`/api/preview/${encodeURIComponent(device)}/csv?${params.toString()}`, {
        responseType: "blob",
      });

      const contentDisposition = res.headers["content-disposition"] as string | undefined;
      const filenameMatch = contentDisposition?.match(/filename="?([^";]+)"?/i);
      const filename = filenameMatch?.[1] || `${device}_streams.csv`;

      const url = window.URL.createObjectURL(res.data);
      const a = document.createElement("a");
      a.href = url;
      a.download = filename;
      document.body.appendChild(a);
      a.click();
      a.remove();
      window.URL.revokeObjectURL(url);
    } finally {
      setDownloadingCsv(false);
    }
  };

  return (
    <div className="flex flex-col h-full space-y-4 p-2 sm:p-3 md:p-4">
      <div className="flex items-center justify-between gap-3 flex-wrap">
        <h1 className="text-2xl font-bold text-gray-800">Sensor Streams</h1>
        <div className="grid grid-cols-1 sm:grid-cols-[auto_1fr_auto_1fr_auto] gap-2 w-full lg:w-auto lg:items-center">
          <label className="text-xs text-gray-600 self-center">From</label>
          <input
            type="datetime-local"
            value={csvFrom}
            onChange={(e) => setCsvFrom(e.target.value)}
            className="border rounded px-2 py-1 text-sm min-w-0"
          />
          <label className="text-xs text-gray-600 self-center">To</label>
          <input
            type="datetime-local"
            value={csvTo}
            onChange={(e) => setCsvTo(e.target.value)}
            className="border rounded px-2 py-1 text-sm min-w-0"
          />
          <button
            onClick={downloadCsv}
            disabled={!device || downloadingCsv}
            className="px-3 py-1 rounded bg-blue-600 text-white disabled:opacity-60 disabled:cursor-not-allowed w-full sm:w-auto"
          >
            {downloadingCsv ? "Downloading CSV…" : "Download CSV"}
          </button>
        </div>
      </div>

      <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-[auto_1fr_auto_auto_auto] gap-2 lg:items-center">
        <label className="text-sm self-center">Device ID</label>
        <select value={device} onChange={(e) => setDevice(e.target.value)} className="border rounded px-2 py-1 min-w-0">
          <option value={ALL_SENSORS_VALUE}>All Sensors</option>
          {devices.length === 0 && (
            <option value={device}>{device}</option>
          )}
          {devices.map((d) => (
            <option key={d.device_id} value={d.device_id}>
              {d.label ? `${d.label} (${d.device_id})` : d.device_id}
            </option>
          ))}
        </select>
        <label className="text-sm self-center">View</label>
        <select value={viewMode} onChange={(e) => setViewMode(e.target.value as "time" | "frequency")} className="border rounded px-2 py-1 w-full sm:w-32">
          <option value="time">Time</option>
          <option value="frequency">Frequency</option>
        </select>
        <label className="text-sm self-center">Window (s)</label>
        <input type="number" value={windowS} onChange={(e) => setWindowS(Number(e.target.value))} className="border rounded px-2 py-1 w-full sm:w-24" />
        <button onClick={() => viewMode === "time" ? load(false) : loadSpectrum()} className="px-3 py-1 rounded bg-blue-600 text-white w-full sm:w-auto">Refresh</button>
      </div>

      <div className="bg-white border rounded p-2 sm:p-3">
        {viewMode === "frequency" && device === ALL_SENSORS_VALUE && (
          <div className="text-sm text-amber-700 bg-amber-50 border border-amber-200 rounded p-2 mb-3">
            Frequency view currently supports one device at a time. Select a specific device to compute FFT.
          </div>
        )}
        {/* Live stream panel */}
        {viewMode === "time" && (
        <div className="mb-4 p-2 border rounded bg-gray-50">
          <div className="flex justify-between items-center mb-2">
            <div className="text-sm text-gray-700 font-semibold">Live Stream</div>
            <div className="text-xs text-gray-500">WebSocket</div>
          </div>
          <div className="grid grid-cols-1 sm:grid-cols-2 gap-2 sm:gap-4">
            <div className="text-sm">
              <span className="text-gray-600">Latest RSSI: </span>
              <span className="font-mono">{liveHeartbeat ? liveHeartbeat.rssi : "—"}</span>
            </div>
            <div className="text-sm min-w-0">
              <span className="text-gray-600">Latest Accel: </span>
              <span className="font-mono break-all">{livePoints.length ? `${livePoints[livePoints.length-1].x.toFixed(3)}, ${livePoints[livePoints.length-1].y.toFixed(3)}, ${livePoints[livePoints.length-1].z.toFixed(3)}` : "—"}</span>
            </div>
          </div>
          <div className="mt-3">
            {livePoints.length > 0 && (
              <MultiSparkline series={[
                { data: livePoints.map(p => p.x), color: '#ef4444', label: 'X' },
                { data: livePoints.map(p => p.y), color: '#06b6d4', label: 'Y' },
                { data: livePoints.map(p => p.z), color: '#10b981', label: 'Z' }
              ]} xValues={livePoints.map((p) => p.t)} xLabel="Time" yLabel="Accel" />
            )}
          </div>
        </div>
        )}
        {(loading || refreshing) && (
          <div className="text-xs text-gray-500 mb-2">
            {loading ? "Loading…" : "Refreshing…"}
          </div>
        )}
        {viewMode === "frequency" && spectrumLoading && (
          <div className="text-xs text-gray-500 mb-2">Computing spectrum…</div>
        )}
        {!loading && points.length === 0 && heartbeatPoints.length === 0 && (
          <>
          {viewMode === "time" && (
          <div className="text-sm text-gray-500">
            No data available. Graphs will appear here when sensor data is available.
          </div>
          )}
          </>
        )}
        
        {/* Heartbeat Graph */}
        {heartbeatPoints.length > 0 && (
          <div className="mb-6">
            <div className="text-sm text-gray-600 font-semibold mb-2">Signal Strength (RSSI)</div>
            <Sparkline
              data={heartbeatPoints.map(p => p.rssi)}
              color="#8b5cf6"
              xValues={heartbeatPoints.map((p) => p.t)}
              xLabel="Time"
              yLabel="RSSI"
            />
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
            ]} xValues={points.map((p) => p.t)} xLabel="Time" yLabel="Accel" />
            <div className="h-4" />
            <div className="text-sm text-gray-600 font-semibold mb-2">X axis</div>
            <Sparkline data={points.map(p => p.x)} color="#ef4444" xValues={points.map((p) => p.t)} xLabel="Time" yLabel="X" />
            <div className="text-sm text-gray-600 font-semibold mt-4 mb-2">Y axis</div>
            <Sparkline data={points.map(p => p.y)} color="#06b6d4" xValues={points.map((p) => p.t)} xLabel="Time" yLabel="Y" />
            <div className="text-sm text-gray-600 font-semibold mt-4 mb-2">Z axis</div>
            <Sparkline data={points.map(p => p.z)} color="#10b981" xValues={points.map((p) => p.t)} xLabel="Time" yLabel="Z" />
          </div>
        )}

        {viewMode === "frequency" && spectra.length > 0 && (
          <div className="space-y-6">
            {spectra.map((s) => (
              <div key={s.axis}>
                <div className="text-sm text-gray-700 font-semibold mb-1">{s.axis.toUpperCase()} spectrum</div>
                <div className="text-xs text-gray-500 mb-2">
                  Fs={s.sample_rate_hz.toFixed(2)} Hz, N={s.window_samples}, dominant={s.dominant_frequency_hz ? `${s.dominant_frequency_hz.toFixed(2)} Hz` : "n/a"}
                </div>
                <SpectrumBars bins={s.bins} color={s.axis === "x" ? "#ef4444" : s.axis === "y" ? "#06b6d4" : s.axis === "z" ? "#10b981" : "#0f172a"} />
              </div>
            ))}
          </div>
        )}
      </div>
    </div>
  );
}
