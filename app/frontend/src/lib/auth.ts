const KEY = "auth_token";

export function getToken(): string | null {
	try { return localStorage.getItem(KEY); } catch { return null; }
}

export function setToken(tok: string) {
	try { localStorage.setItem(KEY, tok); } catch {}
}

export function clearToken() {
	try { localStorage.removeItem(KEY); } catch {}
}

export function isAuthed(): boolean {
	return !!getToken();
}
