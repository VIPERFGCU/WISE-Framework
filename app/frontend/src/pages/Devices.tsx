import StatusBadge from "../components/StatusBadge";
import BoolPill from "../components/BoolPill";
import { formatUptime } from "../lib/format";

export default function Devices() {
  return (
    <div className="space-y-4">
      <h2 className="text-lg font-semibold">Devices</h2>
      <table className="min-w-full bg-white border rounded">
        <thead className="text-left text-sm text-gray-600 border-b">
          <tr>
            <th className="px-3 py-2">Sensor ID</th>
            <th className="px-3 py-2">Status</th>
            <th className="px-3 py-2">Recording</th>
            <th className="px-3 py-2">Sensing</th>
            <th className="px-3 py-2">Up time</th>
          </tr>
        </thead>
        <tbody className="text-sm">
          <tr className="border-t">
            <td className="px-3 py-2 font-mono">bridge-esp32-001</td>
            <td className="px-3 py-2"><StatusBadge status="updating" /></td>
            <td className="px-3 py-2"><BoolPill value={true} /></td>
            <td className="px-3 py-2"><BoolPill value={false} /></td>
            <td className="px-3 py-2">{formatUptime(98765)}</td>
          </tr>
        </tbody>
      </table>
    </div>
  );
}

