
import { useEffect, useState } from "react";
import { listDevices } from "../services/devices";
import type { Device } from "../types/device";
import { parseIsoMs } from "../lib/format";

const STALE_SEC = 120;

function isStale(d: Device) {
	const t = parseIsoMs(d.updated_at);
	if (!Number.isFinite(t)) return false;
	return Date.now() - t > STALE_SEC * 1000;
}

export default function Dashboard() {
	const [devices, setDevices] = useState<Device[]>([]);
	const [loading, setLoading] = useState(true);

	async function load() {
		try {
			const data = await listDevices();
			setDevices(data ?? []);
		} finally {
			setLoading(false);
		}
	}

	useEffect(() => {
		load();
		const id = setInterval(load, 10000);
		return () => clearInterval(id);
	}, []);

	const total = devices.length;
	const on = devices.filter(d => d.status === "on").length;
	const off = devices.filter(d => d.status === "off").length;
	const updating = devices.filter(d => d.status === "updating").length;
	const recYes = devices.filter(d => d.recording).length;
	const senseYes = devices.filter(d => d.sensing).length;
	const stale = devices.filter(isStale).length;

	const Card = ({ label, value }: { label : string; value: number | string }) => (
		<div className="p-4 border rounded bg-white">
			<div className="text-2x1 font-semibold">{value}</div>
			<div className="text-sm text-gray-600">{label}</div>
		</div>
	);

	if (loading) return <div>Loading overview...</div>;

	return (
		<div className="space-y-4">
			<h2 className="text-lg font-semibold">Overview</h2>
			<div className="grid gap-3 sm:grid-cols-2 lg:grid-cols-4">
				<Card label="Total Sensors" value={total} />
				<Card label="Online" value={on} />
				<Card label="Updating" value={updating} />
				<Card label="Offline" value={off} />
				<Card label="Recording (Y)" value={recYes} />
				<Card label="Sensing (Y)" value={senseYes} />
				<Card label={`Stale (> ${STALE_SEC}s)`} value={stale} />
			</div>
		</div>
	);
}

