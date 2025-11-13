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
	if (FAKE) { await sleep(300); return; }
	// Try PUT first (405 earlier suggests POST may not exist)
	await api.put(`/api/v1/devices/${encodeURIComponent(sensorId)}/recording`, { recording });
}

export async function setSensing(sensorId: string, sensing: boolean): Promise<void> {
  if (!sensorId || sensorId === "unknown") throw new Error("Missing sensor id");
  if (FAKE) { await sleep(400); return; }
  await api.put(`/api/v1/devices/${encodeURIComponent(sensorId)}/sensing`, { sensing });
}

export async function setFrequency(deviceId: string, rateHz: number): Promise<void> {
  if (!deviceId || deviceId === "unknown") throw new Error("Missing device id");
  if (!Number.isFinite(rateHz) || rateHz <= 0) throw new Error("Invalid rateHz");
  if (FAKE) { await sleep(300); return; }

  await api.post(`/api/devices/${encodeURIComponent(deviceId)}/rate`, {
    rate_hz: rateHz,
  });
}

export async function setBatchSize(deviceId: string, batchSize: number): Promise<void> {
  if (!deviceId || deviceId === "unknown") throw new Error("Missing device id");
  if (!Number.isFinite(batchSize) || batchSize <= 0) throw new Error("Invalid batch size");
  if (FAKE) { await sleep(300); return; }

  await api.post(`/api/devices/${encodeURIComponent(deviceId)}/batch`, {
    batch_size: batchSize,
  });
}

