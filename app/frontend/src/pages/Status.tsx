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

  if (loading) return <div className="p-2 sm:p-3">Checking API…</div>;
  if (error) return <div className="p-2 sm:p-3 text-red-600">API Error: {error}</div>;

  return (
    <div className="space-y-2 mx-auto w-full max-w-3xl p-2 sm:p-3 md:p-4">
      <div className="text-lg font-semibold">Backend Health</div>
      <pre className="bg-white border rounded p-3 text-xs sm:text-sm overflow-x-auto whitespace-pre-wrap break-words">
        {JSON.stringify(data, null, 2)}
      </pre>
    </div>
  );
}

