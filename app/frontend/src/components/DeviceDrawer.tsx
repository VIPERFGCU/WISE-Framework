import { useEffect, useRef } from "react";
import type { Device } from "../types/device";
import StatusBadge from "./StatusBadge";
import BoolPill from "./BoolPill";
import { formatUptime, parseIsoMs, timeAgo } from "../lib/format";
import { QRCodeSVG } from "qrcode.react";

export default function DeviceDrawer({
  device,
  onClose,
}: {
  device: Device | null;
  onClose: () => void;
}) {
  const qrWrapperRef = useRef<HTMLDivElement | null>(null);

  useEffect(() => {
    function onEsc(e: KeyboardEvent) {
      if (e.key === "Escape") onClose();
    }
    if (device) document.addEventListener("keydown", onEsc);
    return () => document.removeEventListener("keydown", onEsc);
  }, [device, onClose]);

  if (!device) return null;
  const t = parseIsoMs(device.updated_at);

  const qrPayload = JSON.stringify(
    {
      sensor_id: device.sensor_id,
      label: device.label ?? null,
      status: device.status,
      sensing: device.sensing,
      recording: device.recording,
      uptime_seconds: device.uptime_seconds,
      updated_at: device.updated_at ?? null,
      sample_hz: device.sample_hz ?? null,
      batch_size: device.batch_size ?? null,
      generated_at: new Date().toISOString(),
    },
    null,
    2,
  );

  const onPrintQr = () => {
    const svg = qrWrapperRef.current?.querySelector("svg")?.outerHTML;
    if (!svg) return;

    const win = window.open("", "_blank", "width=600,height=800");
    if (!win) return;

    const escapedId = device.sensor_id.replace(/</g, "&lt;").replace(/>/g, "&gt;");
    const escapedLabel = (device.label ?? "-").replace(/</g, "&lt;").replace(/>/g, "&gt;");
    const escapedStatus = device.status.replace(/</g, "&lt;").replace(/>/g, "&gt;");

    win.document.write(`
      <html>
        <head>
          <title>Sensor QR - ${escapedId}</title>
          <style>
            body { font-family: Arial, sans-serif; margin: 24px; }
            .card { border: 1px solid #ddd; border-radius: 8px; padding: 16px; max-width: 420px; }
            .meta { margin-top: 12px; line-height: 1.5; font-size: 14px; }
            .meta b { display: inline-block; width: 90px; }
          </style>
        </head>
        <body>
          <div class="card">
            <h2>Sensor QR</h2>
            ${svg}
            <div class="meta">
              <div><b>ID:</b> ${escapedId}</div>
              <div><b>Label:</b> ${escapedLabel}</div>
              <div><b>Status:</b> ${escapedStatus}</div>
            </div>
          </div>
          <script>
            window.onload = function() { window.print(); };
          </script>
        </body>
      </html>
    `);
    win.document.close();
  };

  return (
    <div className="fixed inset-0 z-50">
      {/* Backdrop */}
      <div className="absolute inset-0 bg-black/30" onClick={onClose} />
      {/* Panel */}
      <div className="absolute top-0 right-0 h-full w-full max-w-md bg-white shadow-xl border-l p-4 overflow-y-auto">
        <div className="flex items-center justify-between mb-4">
          <div className="font-semibold text-lg">Device Details</div>
          <button onClick={onClose} className="text-sm border rounded px-2 py-1 bg-white hover:bg-gray-50">
            Close (Esc)
          </button>
        </div>

        <div className="space-y-4">
          <section className="border rounded p-3 bg-gray-50">
            <div className="text-xs text-gray-500">Sensor ID</div>
            <div className="font-mono">{device.sensor_id}</div>
          </section>

          <section className="border rounded p-3 bg-white">
            <div className="grid grid-cols-2 gap-3">
              <div>
                <div className="text-xs text-gray-500 mb-1">Status</div>
                <StatusBadge status={device.status} />
              </div>
              <div>
                <div className="text-xs text-gray-500 mb-1">Uptime</div>
                <div>{formatUptime(device.uptime_seconds)}</div>
              </div>
              <div>
                <div className="text-xs text-gray-500 mb-1">Recording</div>
                <BoolPill value={device.recording} />
              </div>
              <div>
                <div className="text-xs text-gray-500 mb-1">Sensing</div>
                <BoolPill value={device.sensing} />
              </div>
            </div>
          </section>

          <section className="border rounded p-3 bg-white">
            <div className="text-xs text-gray-500 mb-1">Last Updated</div>
            <div>
              {Number.isFinite(t) ? (
                <>
                  <span className="mr-2">{timeAgo(Date.now() - t)}</span>
                  <span className="text-xs text-gray-500">({new Date(t).toLocaleString()})</span>
                </>
              ) : (
                "—"
              )}
            </div>
          </section>

          <section className="border rounded p-3 bg-white">
            <div className="flex items-center justify-between mb-2">
              <div className="text-xs text-gray-500">Sensor QR Code</div>
              <button
                onClick={onPrintQr}
                className="text-sm border rounded px-2 py-1 bg-white hover:bg-gray-50"
              >
                Print QR
              </button>
            </div>
            <div className="flex items-center justify-center border rounded p-3 bg-gray-50" ref={qrWrapperRef}>
              <QRCodeSVG value={qrPayload} size={180} includeMargin />
            </div>
            <div className="text-xs text-gray-500 mt-2 break-all">
              Encoded data: sensor id, label, status, sensing, recording, uptime, last update, sample rate, batch size.
            </div>
          </section>

          {/* Placeholder for future charts/logs */}
          <section className="border rounded p-3 bg-white">
            <div className="text-xs text-gray-500 mb-1">Notes</div>
            <div className="text-sm text-gray-600">
              Future: recent readings, error logs, location, firmware, etc.
            </div>
          </section>
        </div>
      </div>
    </div>
  );
}

