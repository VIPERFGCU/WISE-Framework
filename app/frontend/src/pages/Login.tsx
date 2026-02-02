import { type FormEvent, useState } from "react";
import { api } from "../api/client";
import { setToken } from "../lib/auth";

const FAKE = import.meta.env.VITE_DEV_FAKE_AUTH === "true";

export default function Login() {
  const [email, setEmail] = useState("");
  const [password, setPassword] = useState("");
  const [err, setErr] = useState<string | null>(null);
  const [busy, setBusy] = useState(false);

  async function onSubmit(e: FormEvent) {
    e.preventDefault();
    setErr(null);
    setBusy(true);
    try {
      if (FAKE) {
        // simulate successful login
        await new Promise((r) => setTimeout(r, 400));
        setToken("dev-fake-token");
      } else {
        // adjust to your FastAPI route/shape
        const { data } = await api.post("/api/v1/auth/login", { email, password });
        const token = data?.access_token ?? data?.token ?? "";
        if (!token) throw new Error("No token in response");
        setToken(token);
      }
      location.assign("/admin");
    } catch (e: any) {
      setErr(e?.message ?? "Login failed");
    } finally {
      setBusy(false);
    }
  }

  return (
    <div className="min-h-[60vh] grid place-items-center">
      <form onSubmit={onSubmit} className="w-full max-w-sm border rounded p-4 bg-white">
        <h1 className="text-lg font-semibold mb-3">Sign in</h1>
        {FAKE && (
          <div className="mb-3 text-xs text-gray-600">
            Dev mode: no backend required. Submitting will set a dummy token.
          </div>
        )}
        {err && <div className="mb-3 text-sm text-red-600">{err}</div>}
        <label className="block mb-2 text-sm">
          Email
          <input
            type="email"
            value={email}
            onChange={(e) => setEmail(e.target.value)}
            className="mt-1 w-full border rounded px-3 py-2"
            placeholder="you@example.com"
            required={!FAKE}
          />
        </label>
        <label className="block mb-4 text-sm">
          Password
          <input
            type="password"
            value={password}
            onChange={(e) => setPassword(e.target.value)}
            className="mt-1 w-full border rounded px-3 py-2"
            placeholder="••••••••"
            required={!FAKE}
          />
        </label>
        <button
          type="submit"
          disabled={busy}
          className="w-full px-3 py-2 text-sm rounded bg-black text-white disabled:opacity-60"
        >
          {busy ? "Signing in…" : "Sign in"}
        </button>
      </form>
    </div>
  );
}

