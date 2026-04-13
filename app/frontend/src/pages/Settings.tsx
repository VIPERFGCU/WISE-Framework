import { useTheme } from "../lib/theme";

export default function Settings() {
  const { mode, setMode } = useTheme();

  const buttonClass = (value: "light" | "dark") =>
    "rounded border px-3 py-2 text-sm transition-colors " +
    (mode === value
      ? "bg-sky-500/20 text-sky-100 border-sky-500/50"
      : "bg-slate-900/50 text-slate-300 border-slate-700 hover:bg-slate-800/60");

  return (
    <section className="max-w-xl rounded border border-slate-700 bg-slate-900/40 p-4 sm:p-5">
      <h1 className="text-xl font-semibold text-slate-100">Settings</h1>
      <p className="mt-1 text-sm text-slate-400">Appearance</p>

      <div className="mt-4">
        <div className="text-sm text-slate-300">Theme</div>
        <div className="mt-2 flex gap-2">
          <button type="button" className={buttonClass("light")} onClick={() => setMode("light")}>
            Light
          </button>
          <button type="button" className={buttonClass("dark")} onClick={() => setMode("dark")}>
            Dark
          </button>
        </div>
      </div>
    </section>
  );
}
