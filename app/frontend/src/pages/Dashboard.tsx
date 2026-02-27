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
			<h2 className="text-lg font-semibold">Overview</h2>
			<div className="grid gap-2 sm:grid-cols-2 lg:grid-cols-4">
				<HealthPill label="API" state={apiOk ? "ok" : "degraded"} latencyMs={apiLatencyMs} />
				<HealthPill label="Influx Query" state={influxOk ? "ok" : "degraded"} latencyMs={influxLatencyMs} />
				<HealthPill label="Device Poll" state={typeof devicesLatencyMs === "number" ? "ok" : "degraded"} latencyMs={devicesLatencyMs} />
				<HealthPill label="MQTT Stream" state={wsOpen ? "ok" : "degraded"} />
			</div>
			<div className="grid gap-3 sm:grid-cols-2 lg:grid-cols-4">
				<Card label="Online" value={on} delta={onlineDelta} series={onlineSeries} color="#22c55e" />
				<Card label="Sensing (Y)" value={senseYes} delta={streamingDelta} series={streamingSeries} color="#06b6d4" />
				<Card label={`Stale (> ${STALE_SEC}s)`} value={stale} delta={staleDelta} series={staleSeries} color="#f59e0b" />
				<Card label="Messages / min" value={messagesPerMin} delta={messagesDelta} series={messagesSeries} color="#a78bfa" />
			</div>
			<div className="grid gap-3 sm:grid-cols-2 lg:grid-cols-3">
				<div className="rounded border border-slate-700 bg-slate-900/40 p-3 text-sm text-slate-200">Total Sensors: <span className="font-semibold">{total}</span></div>
				<div className="rounded border border-slate-700 bg-slate-900/40 p-3 text-sm text-slate-200">Updating: <span className="font-semibold">{updating}</span></div>
				<div className="rounded border border-slate-700 bg-slate-900/40 p-3 text-sm text-slate-200">Offline: <span className="font-semibold">{off}</span></div>
				<div className="rounded border border-slate-700 bg-slate-900/40 p-3 text-sm text-slate-200">Recording (Y): <span className="font-semibold">{recYes}</span></div>
			</div>
		</div>
	);
}

