import type { DeviceStatus } from "../types/device";

const styles: Record<DeviceStatus, string> = {
  on: "bg-green-100 text-green-700 border-green-200",
  off: "bg-gray-100 text-gray-700 border-gray-200",
  updating: "bg-yellow-100 text-yellow-700 border-yellow-200",
};

export default function StatusBadge({ status }: { status: DeviceStatus }) {
  return (
    <span className={`inline-flex items-center px-2 py-0.5 rounded text-xs border ${styles[status]}`}>
      {status === "on" ? "On" : status === "off" ? "Off" : "Updating"}
    </span>
  );
}

