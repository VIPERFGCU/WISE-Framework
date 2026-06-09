import { useState } from "react";
import { api } from "../api/client";
import { isAuthed } from "../lib/auth";

export default function Admin() {
  const [deviceId, setDeviceId] = useState("");
  const [label, setLabel] = useState("");
  const [notes, setNotes] = useState("");
  const [result, setResult] = useState<any>(null);
  const [loading, setLoading] = useState(false);

  if (!isAuthed()) {
    return <div className="p-4">Please sign in to access admin features.</div>;
  }

  const submit = async (e: any) => {
    e.preventDefault();
    setLoading(true);
    setResult(null);
    try {
      const res = await api.post(`/api/v1/devices/register`, { device_id: deviceId, label, notes });
      setResult(res.data);
    } catch (err) {
      // error handled by interceptor/toaster
    } finally {
      setLoading(false);
    }
  };

  return (
    <div className="mx-auto w-full max-w-2xl p-2 sm:p-3 md:p-4">
      <h1 className="text-xl sm:text-2xl font-bold mb-4">Admin</h1>
      <form onSubmit={submit} className="space-y-3">
        <div>
          <label className="block text-sm text-gray-700">Device ID</label>
          <input value={deviceId} onChange={(e) => setDeviceId(e.target.value)} className="w-full border rounded px-3 py-2" />
        </div>
        <div>
          <label className="block text-sm text-gray-700">Label (optional)</label>
          <input value={label} onChange={(e) => setLabel(e.target.value)} className="w-full border rounded px-3 py-2" />
        </div>
        <div>
          <label className="block text-sm text-gray-700">Notes (optional)</label>
          <input value={notes} onChange={(e) => setNotes(e.target.value)} className="w-full border rounded px-3 py-2" />
        </div>
        <div>
          <button disabled={loading} className="w-full sm:w-auto px-3 py-2 rounded bg-blue-600 text-white">{loading ? 'Saving...' : 'Register Device'}</button>
        </div>
      </form>

      {result && (
        <div className="mt-6 bg-white border rounded p-3">
          <div className="text-sm text-gray-700">Device registered:</div>
          <div className="font-mono mt-2 break-all">{result.device_id}</div>
          {result.label && <div className="text-sm text-gray-500">Label: {result.label}</div>}
          {result.mqtt_key && (
            <div className="mt-3">
              <div className="text-sm text-gray-700">MQTT Key (copy for device):</div>
              <div className="font-mono bg-gray-100 p-2 rounded mt-1 break-all">{result.mqtt_key}</div>
              <div className="text-xs text-gray-500 mt-1">Use this secret as the MQTT password for the device (username = device id), and configure your broker to accept it.</div>
            </div>
          )}
        </div>
      )}
    </div>
  );
}

