import axios from "axios";
import { emitError } from "../components/Toaster";

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
	return config;
});
api.interceptors.response.use(
	(res) => {
		loadingStop();
		return res;
	},
	(err) => {
		loadingStop();
		const msg = 
			err?.response?.data?.detail ||
			err?.message ||
			"Request failed";
		emitError(String(msg));
		return Promise.reject(err);
	}
);
