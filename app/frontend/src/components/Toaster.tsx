import { useEffect, useState } from "react";

type Toast = { id: number; message: string };

export function Toaster() {
  const [toasts, setToasts] = useState<Toast[]>([]);

  useEffect(() => {
    function onError(e: Event) {
      const evt = e as CustomEvent<string>;
      const id = Date.now();
      const message = typeof evt.detail === "string" ? evt.detail : "Unknown error";
      setToasts((t) => [...t, { id, message }]);
      setTimeout(() => setToasts((t) => t.filter((x) => x.id !== id)), 3000);
    }
    window.addEventListener("app:error", onError);
    return () => window.removeEventListener("app:error", onError);
  }, []);

  return (
    <div className="fixed top-3 right-3 space-y-2 z-50">
      {toasts.map((t) => (
        <div key={t.id} className="px-3 py-2 rounded bg-red-600 text-white shadow">
          {t.message}
        </div>
      ))}
    </div>
  );
}

// 👇 make sure the class name is exactly CustomEvent (not CustomeEvent)
export function emitError(message: string) {
  window.dispatchEvent(new CustomEvent<string>("app:error", { detail: message }));
}

