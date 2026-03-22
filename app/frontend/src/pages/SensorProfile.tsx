import { useEffect, useMemo, useState } from "react";
import { useParams } from "react-router-dom";
import { api } from "../api/client";
import { listDevices } from "../services/devices";
import type { Device } from "../types/device";
import StatusBadge from "../components/StatusBadge";
import BoolPill from "../components/BoolPill";
import { MultiSparkline, Sparkline, type HeartbeatPoint, type StreamPoint } from "../components/StreamCharts";
import { formatUptime, parseIsoMs, timeAgo } from "../lib/format";

export default function SensorProfile() {
  const { sensorId } = useParams<{ sensorId: string }>();
  const [device, setDevice] = useState<Device | null>(null);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);
  const [windowS, setWindowS] = useState<number>(60);
  const [points, setPoints] = useState<StreamPoint[]>([]);
  const [heartbeatPoints, setHeartbeatPoints] = useState<HeartbeatPoint[]>([]);
  const [loadingGraph, setLoadingGraph] = useState(false);
  const [graphError, setGraphError] = useState<string | null>(null);

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

  useEffect(() => {
    let cancelled = false;

    async function loadGraph() {
      if (!decodedSensorId) {
        setPoints([]);
        setHeartbeatPoints([]);
        setGraphError("Missing sensor id");
        return;
      }

      try {
        setGraphError(null);
        setLoadingGraph(true);
        const res = await api.get(`/api/preview/${encodeURIComponent(decodedSensorId)}?window_s=${windowS}`);
        const accelSeries = Array.isArray(res.data?.accel?.series) ? res.data.accel.series : [];
        const hbSeries = Array.isArray(res.data?.heartbeat?.series) ? res.data.heartbeat.series : [];
        if (!cancelled) {
          setPoints(accelSeries.map((p: any) => ({ t: p.t, x: p.x, y: p.y, z: p.z })));
          setHeartbeatPoints(hbSeries.map((p: any) => ({ t: p.t, rssi: p.rssi })));
        }
      } catch (e: any) {
        if (!cancelled) {
          setPoints([]);
          setHeartbeatPoints([]);
          setGraphError(e?.message ?? "Failed to load sensor stream data");
        }
      } finally {
        if (!cancelled) {
          setLoadingGraph(false);
        }
      }
    }

    loadGraph();
    const id = setInterval(loadGraph, 5000);
    return () => {
      cancelled = true;
      clearInterval(id);
    };
  }, [decodedSensorId, windowS]);

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

              <section className="rounded border border-slate-700 p-3 bg-slate-950/60">
                <div className="flex items-center justify-between gap-3 flex-wrap mb-3">
                  <div>
                    <div className="text-sm text-slate-200 font-semibold">Sensor Stream</div>
                    <div className="text-xs text-slate-400">Recent accelerometer and signal data for this sensor</div>
                  </div>
                  <div className="flex items-center gap-2">
                    <label className="text-xs text-slate-400">Window (s)</label>
                    <input
                      type="number"
                      min={5}
                      step={5}
                      value={windowS}
                      onChange={(e) => setWindowS(Number(e.target.value) || 60)}
                      className="w-24 rounded border border-slate-600 bg-slate-900 px-2 py-1 text-sm"
                    />
                  </div>
                </div>

                {loadingGraph && <div className="text-xs text-slate-400 mb-2">Loading stream data…</div>}
                {!loadingGraph && graphError && <div className="text-rose-300 text-sm">{graphError}</div>}
                {!loadingGraph && !graphError && points.length === 0 && heartbeatPoints.length === 0 && (
                  <div className="text-sm text-slate-400">No stream data available for this sensor yet.</div>
                )}

                {heartbeatPoints.length > 0 && (
                  <div className="mb-6">
                    <div className="text-sm text-slate-300 font-semibold mb-2">Signal Strength (RSSI)</div>
                    <Sparkline
                      data={heartbeatPoints.map((p) => p.rssi)}
                      color="#8b5cf6"
                      xValues={heartbeatPoints.map((p) => p.t)}
                      xLabel="Time"
                      yLabel="RSSI"
                      noDataClassName="text-sm text-slate-400"
                    />
                  </div>
                )}

                {points.length > 0 && (
                  <div>
                    <div className="text-sm text-slate-300 font-semibold mb-2">Combined X / Y / Z</div>
                    <MultiSparkline
                      legendClassName="flex items-center gap-3 text-xs text-slate-400 mb-1"
                      noDataClassName="text-sm text-slate-400"
                      xValues={points.map((p) => p.t)}
                      xLabel="Time"
                      yLabel="Accel"
                      series={[
                        { data: points.map((p) => p.x), color: "#ef4444", label: "X" },
                        { data: points.map((p) => p.y), color: "#06b6d4", label: "Y" },
                        { data: points.map((p) => p.z), color: "#10b981", label: "Z" },
                      ]}
                    />

                    <div className="h-4" />
                    <div className="text-sm text-slate-300 font-semibold mb-2">X axis</div>
                    <Sparkline data={points.map((p) => p.x)} color="#ef4444" xValues={points.map((p) => p.t)} xLabel="Time" yLabel="X" noDataClassName="text-sm text-slate-400" />

                    <div className="text-sm text-slate-300 font-semibold mt-4 mb-2">Y axis</div>
                    <Sparkline data={points.map((p) => p.y)} color="#06b6d4" xValues={points.map((p) => p.t)} xLabel="Time" yLabel="Y" noDataClassName="text-sm text-slate-400" />

                    <div className="text-sm text-slate-300 font-semibold mt-4 mb-2">Z axis</div>
                    <Sparkline data={points.map((p) => p.z)} color="#10b981" xValues={points.map((p) => p.t)} xLabel="Time" yLabel="Z" noDataClassName="text-sm text-slate-400" />
                  </div>
                )}
              </section>
            </div>
          )}
        </div>
      </div>
    </div>
  );
}
