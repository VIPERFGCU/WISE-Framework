import { useEffect, useState } from "react";
import { listDevices } from "../services/devices";
import { pingHealth } from "../services/health";
import type { Device } from "../types/device";
import { parseIsoMs } from "../lib/format";
import { api } from "../api/client";

const STALE_SEC = 120;
const HISTORY_POINTS = 30;

type HealthState = "ok" | "degraded";

type Snapshot = {
	ts: number;
	online: number;
	streaming: number;
	stale: number;
	messagesPerMin: number;
};

type RegionKey = "NA" | "SA" | "EU" | "AF" | "AS" | "OC";

const REGION_ANCHORS: Record<RegionKey, { x: number; y: number; label: string }> = {
	NA: { x: 18, y: 35, label: "North America" },
	SA: { x: 30, y: 67, label: "South America" },
	EU: { x: 51, y: 30, label: "Europe" },
	AF: { x: 53, y: 58, label: "Africa" },
	AS: { x: 74, y: 43, label: "Asia" },
	OC: { x: 86, y: 73, label: "Oceania" },
};

function isStale(d: Device) {
	const t = parseIsoMs(d.updated_at);
	if (!Number.isFinite(t)) return false;
	return Date.now() - t > STALE_SEC * 1000;
}

function MiniSparkline({ data, color }: { data: number[]; color: string }) {
	const width = 120;
	const height = 34;
	const pad = 3;
	if (!data.length) return <div className="h-[34px]" />;
	const min = Math.min(...data);
	const max = Math.max(...data);
	const scaleX = (index: number) => pad + (index / Math.max(1, data.length - 1)) * (width - pad * 2);
	const scaleY = (value: number) => {
		if (max === min) return height / 2;
		return pad + (1 - (value - min) / (max - min)) * (height - pad * 2);
	};
	const d = data.map((value, index) => `${index === 0 ? "M" : "L"} ${scaleX(index)} ${scaleY(value)}`).join(" ");

	return (
		<svg viewBox={`0 0 ${width} ${height}`} width="100%" height={height}>
			<path d={d} fill="none" stroke={color} strokeWidth={2} strokeLinecap="round" strokeLinejoin="round" />
		</svg>
	);
}

function AreaOperationsChart({ data }: { data: Snapshot[] }) {
	const width = 920;
	const height = 260;
	const padX = 26;
	const padY = 24;

	if (!data.length) {
		return <div className="h-[260px] rounded border border-slate-700 bg-slate-900/40 p-3 text-sm text-slate-400">No operations data yet</div>;
	}

	const values = data.map((d) => d.messagesPerMin);
	const minVal = Math.min(...values, 0);
	const maxVal = Math.max(...values, 1);

	const scaleX = (i: number) => padX + (i / Math.max(1, data.length - 1)) * (width - padX * 2);
	const scaleY = (v: number) => {
		if (maxVal === minVal) return height / 2;
		return padY + (1 - (v - minVal) / (maxVal - minVal)) * (height - padY * 2);
	};

	const lineD = data.map((d, i) => `${i === 0 ? "M" : "L"} ${scaleX(i)} ${scaleY(d.messagesPerMin)}`).join(" ");
	const areaD = `${lineD} L ${scaleX(data.length - 1)} ${height - padY} L ${scaleX(0)} ${height - padY} Z`;

	const horizontalGuides = [0.2, 0.4, 0.6, 0.8].map((r) => {
		const y = padY + r * (height - padY * 2);
		return <line key={r} x1={padX} y1={y} x2={width - padX} y2={y} stroke="rgba(148,163,184,0.2)" strokeWidth="1" />;
	});

	return (
		<div className="rounded border border-slate-700 bg-slate-900/40 p-3">
			<div className="mb-2 flex items-center justify-between">
				<div>
					<div className="text-xs uppercase tracking-wide text-slate-400">Operations Trend</div>
					<div className="text-sm text-slate-200">Messages per minute (last {data.length} samples)</div>
				</div>
				<div className="text-xs text-slate-400">Live feed</div>
			</div>
			<svg viewBox={`0 0 ${width} ${height}`} width="100%" height={260}>
				{horizontalGuides}
				<path d={areaD} fill="url(#opsArea)" />
				<path d={lineD} fill="none" stroke="#38bdf8" strokeWidth={3} strokeLinecap="round" strokeLinejoin="round" />
				<defs>
					<linearGradient id="opsArea" x1="0" y1="0" x2="0" y2="1">
						<stop offset="0%" stopColor="rgba(56,189,248,0.45)" />
						<stop offset="100%" stopColor="rgba(56,189,248,0.02)" />
					</linearGradient>
				</defs>
			</svg>
		</div>
	);
}

function hashToRegion(sensorId: string): RegionKey {
	let hash = 0;
	for (let index = 0; index < sensorId.length; index++) {
		hash = (hash * 31 + sensorId.charCodeAt(index)) >>> 0;
	}
	const regions: RegionKey[] = ["NA", "SA", "EU", "AF", "AS", "OC"];
	return regions[hash % regions.length];
}

function DeltaChip({ delta }: { delta: number }) {
	const cls = delta > 0
		? "text-emerald-300 bg-emerald-500/20"
		: delta < 0
			? "text-rose-300 bg-rose-500/20"
			: "text-slate-300 bg-slate-500/20";
	const text = delta > 0 ? `+${delta}` : `${delta}`;
	return <span className={`inline-flex rounded px-1.5 py-0.5 text-xs ${cls}`}>{text} vs last</span>;
}

function HealthPill({ label, state, latencyMs }: { label: string; state: HealthState; latencyMs?: number }) {
	const cls = state === "ok"
		? "border-emerald-500/40 bg-emerald-500/15 text-emerald-200"
		: "border-amber-500/40 bg-amber-500/15 text-amber-200";
	return (
		<div className={`rounded border px-3 py-2 text-sm ${cls}`}>
			<div className="font-medium">{label}</div>
			<div className="text-xs opacity-90">{state === "ok" ? "Healthy" : "Degraded"}{typeof latencyMs === "number" ? ` • ${Math.round(latencyMs)}ms` : ""}</div>
		</div>
	);
}

function UniformMetricCard({ label, value, tone = "neutral" }: { label: string; value: number | string; tone?: "neutral" | "good" | "warn" }) {
	const toneClass = tone === "good"
		? "border-emerald-500/30"
		: tone === "warn"
			? "border-amber-500/30"
			: "border-slate-700";
	return (
		<div className={`rounded border ${toneClass} bg-slate-900/40 p-4 min-h-[92px] flex flex-col justify-between`}>
			<div className="text-xs uppercase tracking-wide text-slate-400">{label}</div>
			<div className="text-3xl font-semibold text-slate-100 leading-none">{value}</div>
		</div>
	);
}

export default function Dashboard() {
	const [devices, setDevices] = useState<Device[]>([]);
	const [loading, setLoading] = useState(true);
	const [apiLatencyMs, setApiLatencyMs] = useState<number | undefined>(undefined);
	const [influxLatencyMs, setInfluxLatencyMs] = useState<number | undefined>(undefined);
	const [devicesLatencyMs, setDevicesLatencyMs] = useState<number | undefined>(undefined);
	const [apiOk, setApiOk] = useState(false);
	const [influxOk, setInfluxOk] = useState(false);
	const [wsOpen, setWsOpen] = useState(false);
	const [history, setHistory] = useState<Snapshot[]>([]);
	const [messageTimes, setMessageTimes] = useState<number[]>([]);

	async function load() {
		const devicesStart = performance.now();
		try {
			const data = await listDevices();
			setDevices(data ?? []);
			setDevicesLatencyMs(performance.now() - devicesStart);
		} finally {
			setLoading(false);
		}
	}

	async function loadApiHealth() {
		const started = performance.now();
		try {
			await pingHealth();
			setApiOk(true);
			setApiLatencyMs(performance.now() - started);
		} catch {
			setApiOk(false);
			setApiLatencyMs(undefined);
		}
	}

	async function loadInfluxHealth() {
		const started = performance.now();
		try {
			await api.get("/api/preview/__all__?window_s=60");
			setInfluxOk(true);
			setInfluxLatencyMs(performance.now() - started);
		} catch {
			setInfluxOk(false);
			setInfluxLatencyMs(undefined);
		}
	}

	function buildStreamWsUrl(): string {
		const apiBase = import.meta.env.VITE_API_BASE_URL as string | undefined;
		const base = apiBase && apiBase !== "/" ? apiBase : window.location.origin;
		const apiUrl = new URL(base, window.location.origin);
		const wsUrl = new URL("/api/v1/stream", apiUrl);
		wsUrl.protocol = apiUrl.protocol === "https:" ? "wss:" : "ws:";
		return wsUrl.toString();
	}

	useEffect(() => {
		load();
		loadApiHealth();
		loadInfluxHealth();

		const id = setInterval(() => {
			load();
			loadApiHealth();
			loadInfluxHealth();
		}, 10000);
		return () => clearInterval(id);
	}, []);

	useEffect(() => {
		const now = Date.now();
		setMessageTimes((prev) => prev.filter((ts) => now - ts <= 60_000));
	}, [devices]);

	useEffect(() => {
		const ws = new WebSocket(buildStreamWsUrl());
		ws.onopen = () => setWsOpen(true);
		ws.onclose = () => setWsOpen(false);
		ws.onerror = () => setWsOpen(false);
		ws.onmessage = (event) => {
			try {
				const msg = JSON.parse(event.data);
				if (msg.type === "data") {
					const now = Date.now();
					setMessageTimes((prev) => [...prev, now].filter((ts) => now - ts <= 60_000));
				}
			} catch {
				// ignore malformed data
			}
		};

		return () => {
			try {
				ws.close();
			} catch {
				// ignore
			}
		};
	}, []);

	const total = devices.length;
	const on = devices.filter(d => d.status === "on").length;
	const off = devices.filter(d => d.status === "off").length;
	const updating = devices.filter(d => d.status === "updating").length;
	const recYes = devices.filter(d => d.recording).length;
	const senseYes = devices.filter(d => d.sensing).length;
	const stale = devices.filter(isStale).length;
	const messagesPerMin = messageTimes.length;

	useEffect(() => {
		if (loading) return;
		const snapshot: Snapshot = {
			ts: Date.now(),
			online: on,
			streaming: senseYes,
			stale,
			messagesPerMin,
		};
		setHistory((prev) => [...prev, snapshot].slice(-HISTORY_POINTS));
	}, [loading, on, senseYes, stale, messagesPerMin]);

	const prev = history.length > 1 ? history[history.length - 2] : undefined;
	const onlineDelta = prev ? on - prev.online : 0;
	const streamingDelta = prev ? senseYes - prev.streaming : 0;
	const staleDelta = prev ? stale - prev.stale : 0;
	const messagesDelta = prev ? messagesPerMin - prev.messagesPerMin : 0;

	const onlineSeries = history.map((h) => h.online);
	const streamingSeries = history.map((h) => h.streaming);
	const staleSeries = history.map((h) => h.stale);
	const messagesSeries = history.map((h) => h.messagesPerMin);
	const operationsSeries = history.slice(-20);

	const regionCounts = devices.reduce<Record<RegionKey, number>>((acc, d) => {
		const region = hashToRegion(d.sensor_id);
		acc[region] = (acc[region] ?? 0) + 1;
		return acc;
	}, { NA: 0, SA: 0, EU: 0, AF: 0, AS: 0, OC: 0 });

	const topSensors = devices
		.slice()
		.sort((a, b) => (b.sample_hz ?? 0) - (a.sample_hz ?? 0))
		.slice(0, 3);

	const activityItems = devices
		.slice()
		.sort((a, b) => parseIsoMs(b.updated_at) - parseIsoMs(a.updated_at))
		.slice(0, 5)
		.map((d) => ({
			sensorId: d.sensor_id,
			status: d.status,
			updatedAt: d.updated_at,
		}));

	const Card = ({
		label,
		value,
		delta,
		series,
		color,
	}: {
		label: string;
		value: number | string;
		delta: number;
		series: number[];
		color: string;
	}) => (
		<div className="rounded border border-slate-700 bg-slate-900/60 p-3">
			<div className="mb-1 flex items-center justify-between">
				<div className="text-xs text-slate-300">{label}</div>
				<DeltaChip delta={delta} />
			</div>
			<div className="text-2xl font-semibold text-slate-100">{value}</div>
			<div className="mt-2">
				<MiniSparkline data={series} color={color} />
			</div>
		</div>
	);

	if (loading) return <div>Loading overview...</div>;

	return (
		<div className="space-y-4">
			<div className="flex items-center justify-between">
				<h2 className="text-lg font-semibold">Operations Overview</h2>
				<div className="text-xs text-slate-400">Live command center</div>
			</div>

			<div className="grid gap-2 sm:grid-cols-2 xl:grid-cols-4">
				<HealthPill label="API" state={apiOk ? "ok" : "degraded"} latencyMs={apiLatencyMs} />
				<HealthPill label="Influx Query" state={influxOk ? "ok" : "degraded"} latencyMs={influxLatencyMs} />
				<HealthPill label="Device Poll" state={typeof devicesLatencyMs === "number" ? "ok" : "degraded"} latencyMs={devicesLatencyMs} />
				<HealthPill label="MQTT Stream" state={wsOpen ? "ok" : "degraded"} />
			</div>

			<div className="grid gap-3 sm:grid-cols-2 xl:grid-cols-4">
				<Card label="Online" value={on} delta={onlineDelta} series={onlineSeries} color="#22c55e" />
				<Card label="Sensing (Y)" value={senseYes} delta={streamingDelta} series={streamingSeries} color="#06b6d4" />
				<Card label={`Stale (> ${STALE_SEC}s)`} value={stale} delta={staleDelta} series={staleSeries} color="#f59e0b" />
				<Card label="Messages / min" value={messagesPerMin} delta={messagesDelta} series={messagesSeries} color="#a78bfa" />
			</div>

			<div className="grid gap-3 grid-cols-2 xl:grid-cols-4">
				<UniformMetricCard label="Total Sensors" value={total} />
				<UniformMetricCard label="Updating" value={updating} />
				<UniformMetricCard label="Offline" value={off} tone={off > 0 ? "warn" : "good"} />
				<UniformMetricCard label="Recording" value={recYes} tone={recYes > 0 ? "good" : "neutral"} />
			</div>

			<div className="grid gap-3 xl:grid-cols-12">
				<div className="xl:col-span-8">
					<AreaOperationsChart data={operationsSeries} />
				</div>
				<div className="xl:col-span-4 rounded border border-slate-700 bg-slate-900/40 p-4">
					<div className="text-xs uppercase tracking-wide text-slate-400 mb-3">Sensor Spotlights</div>
					<div className="space-y-3">
						{topSensors.length === 0 && <div className="text-sm text-slate-400">No sensors discovered yet</div>}
						{topSensors.map((sensor) => (
							<div key={sensor.sensor_id} className="rounded border border-slate-700 bg-slate-950/60 p-3">
								<div className="text-sm font-medium text-slate-100 truncate">{sensor.label ?? sensor.sensor_id}</div>
								<div className="text-xs text-slate-400 mt-1">{sensor.sensor_id}</div>
								<div className="mt-2 grid grid-cols-2 gap-2 text-xs text-slate-300">
									<div>Rate: <span className="text-slate-100">{sensor.sample_hz ?? 0}Hz</span></div>
									<div>Batch: <span className="text-slate-100">{sensor.batch_size ?? 0}</span></div>
								</div>
							</div>
						))}
					</div>
				</div>
			</div>

			<div className="grid gap-3 xl:grid-cols-12">
				<div className="xl:col-span-8 rounded border border-slate-700 bg-slate-900/40 p-4">
					<div className="mb-3 flex items-center justify-between">
						<div className="text-xs uppercase tracking-wide text-slate-400">Potential Sensor Locations</div>
						<div className="text-xs text-slate-400">{total} sensors mapped</div>
					</div>
					<div className="relative h-[280px] rounded border border-slate-700 bg-slate-950/70 overflow-hidden">
						<div className="absolute inset-0 opacity-40">
							<svg viewBox="0 0 100 100" width="100%" height="100%" preserveAspectRatio="none">
								<path d="M7,40 C18,25 32,22 38,31 C44,39 34,44 30,49 C26,54 24,64 18,66 C11,67 6,59 5,51 C4,45 5,42 7,40 Z" fill="rgba(51,65,85,0.7)" />
								<path d="M28,67 C34,61 41,62 44,68 C46,72 44,78 41,83 C38,86 33,85 30,80 C26,74 25,70 28,67 Z" fill="rgba(51,65,85,0.7)" />
								<path d="M45,35 C50,30 57,29 61,33 C65,37 66,44 63,48 C59,52 52,52 47,49 C44,45 43,39 45,35 Z" fill="rgba(51,65,85,0.7)" />
								<path d="M49,56 C53,53 57,55 60,60 C62,65 61,74 57,80 C54,84 49,83 46,76 C43,69 45,60 49,56 Z" fill="rgba(51,65,85,0.7)" />
								<path d="M63,36 C70,31 79,34 84,40 C89,46 87,53 80,58 C72,64 66,63 62,57 C59,50 58,41 63,36 Z" fill="rgba(51,65,85,0.7)" />
								<path d="M82,72 C85,70 89,71 91,74 C93,77 91,82 88,84 C84,86 80,84 79,80 C78,77 79,73 82,72 Z" fill="rgba(51,65,85,0.7)" />
							</svg>
						</div>
						{(Object.keys(REGION_ANCHORS) as RegionKey[]).map((region) => {
							const anchor = REGION_ANCHORS[region];
							const count = regionCounts[region] ?? 0;
							const size = count > 0 ? 18 + Math.min(24, count * 2) : 14;
							return (
								<div
									key={region}
									className="absolute -translate-x-1/2 -translate-y-1/2"
									style={{ left: `${anchor.x}%`, top: `${anchor.y}%` }}
									title={`${anchor.label}: ${count}`}
								>
									<div
										className="rounded-full bg-sky-500/70 border border-sky-300/70 text-[11px] text-white font-semibold flex items-center justify-center"
										style={{ width: `${size}px`, height: `${size}px` }}
									>
										{count}
									</div>
								</div>
							);
						})}
					</div>
				</div>

				<div className="xl:col-span-4 rounded border border-slate-700 bg-slate-900/40 p-4">
					<div className="text-xs uppercase tracking-wide text-slate-400 mb-3">Recent Sensor Activity</div>
					<div className="space-y-3">
						{activityItems.length === 0 && <div className="text-sm text-slate-400">No recent activity</div>}
						{activityItems.map((item) => {
							const ts = parseIsoMs(item.updatedAt);
							return (
								<div key={item.sensorId} className="pb-2 border-b border-slate-700/60 last:border-b-0">
									<div className="text-sm font-medium text-slate-100 truncate">{item.sensorId}</div>
									<div className="text-xs text-slate-400 mt-0.5">{item.status.toUpperCase()}</div>
									<div className="text-xs text-slate-500 mt-1">{Number.isFinite(ts) ? new Date(ts).toLocaleString() : "Unknown update time"}</div>
								</div>
							);
						})}
					</div>
				</div>
			</div>
		</div>
	);
}

