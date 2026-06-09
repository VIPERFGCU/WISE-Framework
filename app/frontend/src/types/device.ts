export type DeviceStatus = "on" | "off" | "updating";

export interface Device {
	sensor_id: string;
	label?: string;
	status: DeviceStatus;
	recording: boolean;
	sensing: boolean;
	uptime_seconds: number;
	updated_at?: string; // ISO string from backend
	// Telemetry controls
	sample_hz?: number; // e.g. 50 = 50 samples per second
	batch_size?: number; // e.g. number of samples per publish
}
