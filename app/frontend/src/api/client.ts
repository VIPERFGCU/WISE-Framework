import axios from "axios";
import { emitError } from "../components/Toaster";
import { getToken, clearToken } from "../lib/auth";

const baseURL = import.meta.env.VITE_API_BASE_URL ?? "/";

export const api = axios.create({
	baseURL,
	timeout: 15000,
});

let inflight = 0;
function loadingStart() {
	if (inflight++ === 0) window.dispatchEvent(new Event("app:loadstart"));
}

function loadingStop() {
	if (inflight > 0 && --inflight === 0) window.dispatchEvent(new Event("app:loadstop"));
}

api.interceptors.request.use((config) => {
	loadingStart();
	const tok = getToken();
	if (tok) {
		config.headers = config.headers ?? {};
		(config.headers as any).Authorization = `Bearer ${tok}`;
		(config.withCredentials as any) = false; //using token header for now
	}
	return config;
});
api.interceptors.response.use(
	(res) => {
		loadingStop();
		return res;
	},
	(err) => {
		loadingStop();
		if (err?.response?.status === 401) {
			clearToken();
			if (location.pathname !== "/login") {
				location.assign("/login");
			}
		}
		const msg = 
			err?.response?.data?.detail ||
			err?.message ||
			"Request failed";
		emitError(String(msg));
		return Promise.reject(err);
	}
);
