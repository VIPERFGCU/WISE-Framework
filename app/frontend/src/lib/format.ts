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
