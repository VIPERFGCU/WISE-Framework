import { useEffect, useState } from "react";
import { pingHealth, type HealthResponse } from "../services/health";

export default function Status() {
  const [data, setData] = useState<HealthResponse | null>(null);
  const [error, setError] = useState<string | null>(null);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    pingHealth()
      .then(setData)
      .catch((e) => setError(e?.message ?? "Unknown error"))
      .finally(() => setLoading(false));
  }, []);

  if (loading) return <div>Checking API…</div>;
  if (error) return <div className="text-red-600">API Error: {error}</div>;

  return (
    <div className="space-y-2">
      <div className="text-lg font-semibold">Backend Health</div>
      <pre className="bg-white border rounded p-3 text-sm overflow-x-auto">
        {JSON.stringify(data, null, 2)}
      </pre>
    </div>
  );
}

