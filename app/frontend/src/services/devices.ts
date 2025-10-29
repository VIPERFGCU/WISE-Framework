import { api } from "../api/client";
import type { Device } from "../types/device";
import { normalizeDevice } from "../lib/normalize";

const FAKE = import.meta.env.VITE_DEV_FAKE_ACTIONS === "true";

// Simulating latency
function sleep(ms: number) { return new Promise(res => setTimeout(res, ms)); }

export async function listDevices(): Promise<Device[]> {
	const { data } = await api.get<Device[]>("/api/v1/devices");
	return Array.isArray(data) ? data.map(normalizeDevice) : [];
}

export async function setRecording(sensorId: string, recording: boolean): Promise<void> {
	if (!sensorId || sensorId === "unknown") throw new Error("Missing sensor id");
	if (FAKE) { await sleep(400); return; } //pretend success
	// Try PUT first (405 earlier suggests POST may not exist)
	await api.put(`/api/v1/devices/${encodeURIComponent(sensorId)}/recording`, { recording });
}

export async function setSensing(sensorId: string, sensing: boolean): Promise<void> {
  if (!sensorId || sensorId === "unknown") throw new Error("Missing sensor id");
  if (FAKE) { await sleep(400); return; }
  await api.put(`/api/v1/devices/${encodeURIComponent(sensorId)}/sensing`, { sensing });
}
