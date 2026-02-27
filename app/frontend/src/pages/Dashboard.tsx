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

type SwflSiteKey = "fort_myers" | "cape_coral" | "naples" | "bonita_springs" | "lehigh_acres" | "punta_gorda";

const SWFL_SITE_ANCHORS: Record<SwflSiteKey, { x: number; y: number; label: string }> = {
	fort_myers: { x: 54, y: 34, label: "Fort Myers" },
	cape_coral: { x: 49, y: 31, label: "Cape Coral" },
	naples: { x: 43, y: 60, label: "Naples" },
	bonita_springs: { x: 48, y: 49, label: "Bonita Springs" },
	lehigh_acres: { x: 61, y: 41, label: "Lehigh Acres" },
	punta_gorda: { x: 60, y: 22, label: "Punta Gorda" },
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

function hashToSwflSite(sensorId: string): SwflSiteKey {
	let hash = 0;
	for (let index = 0; index < sensorId.length; index++) {
		hash = (hash * 31 + sensorId.charCodeAt(index)) >>> 0;
	}
	const sites: SwflSiteKey[] = ["fort_myers", "cape_coral", "naples", "bonita_springs", "lehigh_acres", "punta_gorda"];
	return sites[hash % sites.length];
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

	const siteCounts = devices.reduce<Record<SwflSiteKey, number>>((acc, d) => {
		const site = hashToSwflSite(d.sensor_id);
		acc[site] = (acc[site] ?? 0) + 1;
		return acc;
	}, { fort_myers: 0, cape_coral: 0, naples: 0, bonita_springs: 0, lehigh_acres: 0, punta_gorda: 0 });

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
						<div className="text-xs uppercase tracking-wide text-slate-400">Sensor Locations</div>
						<div className="text-xs text-slate-400">{total} sensors mapped</div>
					</div>
					<div className="relative h-[280px] rounded border border-slate-700 bg-slate-950/70 overflow-hidden">
						<div className="absolute inset-0 opacity-70">
							<svg viewBox="0 0 100 100" width="100%" height="100%" preserveAspectRatio="none">
								<rect x="0" y="0" width="100" height="100" fill="rgba(2,6,23,0.85)" />
								<path d="M38,12 C42,15 45,20 45,25 C45,30 43,35 45,42 C47,50 53,56 56,64 C59,72 58,81 53,90 C49,97 45,98 40,94 C35,90 35,83 36,77 C37,69 35,63 31,57 C26,49 24,41 25,34 C26,25 30,16 38,12 Z" fill="rgba(30,41,59,0.92)" stroke="rgba(148,163,184,0.28)" strokeWidth="0.6" />
								<path d="M27,36 C23,40 20,46 21,51 C22,58 28,61 34,62" fill="none" stroke="rgba(125,211,252,0.35)" strokeWidth="0.8" strokeDasharray="1.2 1.2" />
								<path d="M44,26 C49,31 54,31 59,28" fill="none" stroke="rgba(125,211,252,0.35)" strokeWidth="0.8" strokeDasharray="1.2 1.2" />
								<text x="10" y="20" fill="rgba(148,163,184,0.5)" fontSize="3.4">Gulf of Mexico</text>
								<text x="64" y="28" fill="rgba(148,163,184,0.55)" fontSize="3.4">SW Florida</text>
							</svg>
						</div>
						{(Object.keys(SWFL_SITE_ANCHORS) as SwflSiteKey[]).map((site) => {
							const anchor = SWFL_SITE_ANCHORS[site];
							const count = siteCounts[site] ?? 0;
							const size = count > 0 ? 18 + Math.min(24, count * 2) : 14;
							return (
								<div
									key={site}
									className="absolute -translate-x-1/2 -translate-y-1/2"
									style={{ left: `${anchor.x}%`, top: `${anchor.y}%` }}
									title={`${anchor.label}: ${count}`}
								>
									<div
										className="rounded-full bg-sky-500/80 border border-sky-300/80 text-[11px] text-white font-semibold flex items-center justify-center shadow-[0_0_0_4px_rgba(56,189,248,0.15)]"
										style={{ width: `${size}px`, height: `${size}px` }}
									>
										{count}
									</div>
									<div className="mt-1 text-[10px] text-slate-300 text-center whitespace-nowrap">{anchor.label}</div>
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

