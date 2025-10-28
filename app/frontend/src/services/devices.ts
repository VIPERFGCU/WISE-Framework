import { api } from "../api/client";
import type { Device } from "../types/device";

export async function listDevices(): Promise<Device[]> {
	const { data } = await api.get<Device[]>("/api/v1/devices");
	return data;
}
