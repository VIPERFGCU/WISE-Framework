import { api } from "../api/client";

export type HealthResponse = {
	status?: string;
	version?: string;
	timestamp?: string;
	[k: string]: unknown;
};

export async function pingHealth(): Promise<HealthResponse> {
	//Uses relative path so dev proxy forwards to http://localhost:8000
	const { data } = await api.get<HealthResponse>("/health");
	return data;
}
