import type { DeviceStatus } from "../types/device";

const styles: Record<DeviceStatus, string> = {
  on: "bg-emerald-500/20 text-emerald-200 border-emerald-500/40",
  off: "bg-slate-500/20 text-slate-200 border-slate-500/40",
  updating: "bg-amber-500/20 text-amber-200 border-amber-500/40",
};

export default function StatusBadge({ status }: { status: DeviceStatus }) {
  return (
    <span className={`inline-flex items-center px-2 py-0.5 rounded text-xs border ${styles[status]}`}>
      {status === "on" ? "On" : status === "off" ? "Off" : "Updating"}
    </span>
  );
}

