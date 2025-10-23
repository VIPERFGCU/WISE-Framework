export type DeviceStatus = "on" | "off" | "updating";

export interface Device {
	sensor_id: string;
	status: DeviceStatus;
	recording: boolean;
	sensing: boolean;
	uptime_seconds: number;
	updated_at?: string; // ISO string from backend
}
