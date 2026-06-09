export function formatUptime(seconds: number): string {
	if (!Number.isFinite(seconds) || seconds < 0) return "-";
	const d = Math.floor(seconds / 86400);
	seconds %= 86400;
	const h = Math.floor(seconds / 3600);
	seconds %= 3600;
	const m = Math.floor(seconds / 60);
	const s = Math.floor(seconds % 60);
	const pad = (n: number) => n.toString().padStart(2, "0");
	return d > 0 ? `${d}d ${pad(h)}:${pad(m)}:${pad(s)}` : `${pad(h)}:${pad(m)}:${pad(s)}`;
}

// Safe ISO parse → number (ms) or NaN
export function parseIsoMs(v?: string): number {
	if (!v) return NaN;
	const t = Date.parse(v);
	return Number.isFinite(t) ? t: NaN;
}

export function timeAgo(ms: number): string {
	if (!Number.isFinite(ms)) return "-";
	const s = Math.max(0, Math.floor(ms / 1000));
	if (s < 60) return `${s}s ago`;
	const m = Math.floor(s / 60);
	if (m < 60) return `${m}m ago`;
	const h = Math.floor(m / 60);
	if (h < 24) return `${h}h ago`;
	const d = Math.floor(h / 24);
	return `${d}d ago`;
}
