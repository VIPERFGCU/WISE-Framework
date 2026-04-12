import { useEffect, useMemo, useState } from "react";
import { parseTelemetryTimestampMs } from "../lib/time";

export type StreamPoint = { t: string; x: number; y: number; z: number };
export type HeartbeatPoint = { t: string; rssi: number };
export type SpectrumBin = { f_hz: number; amplitude: number };

type ChartGeometry = {
  w: number;
  h: number;
  left: number;
  right: number;
  top: number;
  bottom: number;
};

const defaultGeom: ChartGeometry = {
  w: 640,
  h: 180,
  left: 46,
  right: 14,
  top: 12,
  bottom: 32,
};

function clampIndex(value: number, max: number): number {
  return Math.max(0, Math.min(max, value));
}

function safeFixed(value: unknown, digits: number): string {
  return typeof value === "number" && Number.isFinite(value) ? value.toFixed(digits) : "-";
}

function axisStroke() {
  return "#94a3b8";
}

function gridStroke() {
  return "rgba(148, 163, 184, 0.25)";
}

function formatOffset(minutesEast: number): string {
  const sign = minutesEast >= 0 ? "+" : "-";
  const abs = Math.abs(minutesEast);
  const hh = String(Math.floor(abs / 60)).padStart(2, "0");
  const mm = String(abs % 60).padStart(2, "0");
  return `${sign}${hh}:${mm}`;
}

function defaultXFormatter(v: string | number): string {
  if (typeof v === "number") return String(v);
  const t = parseTelemetryTimestampMs(v);
  if (t !== null) {
    const d = new Date(t);
    const yyyy = d.getFullYear();
    const mm = String(d.getMonth() + 1).padStart(2, "0");
    const dd = String(d.getDate()).padStart(2, "0");
    const hh = String(d.getHours()).padStart(2, "0");
    const mi = String(d.getMinutes()).padStart(2, "0");
    const ss = String(d.getSeconds()).padStart(2, "0");
    const ms = String(d.getMilliseconds()).padStart(3, "0");
    const tz = formatOffset(-d.getTimezoneOffset());
    return `${yyyy}-${mm}-${dd} ${hh}:${mi}:${ss}.${ms} UTC${tz}`;
  }
  return v;
}

function pointerIndexFromEvent(
  event: React.PointerEvent<SVGSVGElement>,
  length: number,
  geom: ChartGeometry,
): number {
  const rect = event.currentTarget.getBoundingClientRect();
  const leftPx = rect.width * (geom.left / geom.w);
  const rightPx = rect.width * (geom.right / geom.w);
  const usable = Math.max(1, rect.width - leftPx - rightPx);
  const raw = (event.clientX - rect.left - leftPx) / usable;
  const clamped = Math.max(0, Math.min(1, raw));
  const boosted = Math.max(0, Math.min(1, (clamped - 0.5) * 1.12 + 0.5));
  return clampIndex(Math.round(boosted * (length - 1)), length - 1);
}

export function Sparkline({
  data,
  color,
  noDataClassName = "text-sm text-gray-500",
  xLabel = "Sample",
  yLabel = "Value",
  xValues,
  xValueFormatter,
}: {
  data: number[];
  color: string;
  noDataClassName?: string;
  xLabel?: string;
  yLabel?: string;
  xValues?: Array<string | number>;
  xValueFormatter?: (v: string | number) => string;
}) {
  if (!data || data.length === 0) return <div className={noDataClassName}>No data</div>;

  const g = defaultGeom;
  const [cursor, setCursor] = useState<number>(data.length - 1);
  useEffect(() => {
    setCursor((prev) => clampIndex(prev, data.length - 1));
  }, [data.length]);

  const min = Math.min(...data);
  const max = Math.max(...data);
  const safeCursor = clampIndex(cursor, data.length - 1);
  const scaleX = (i: number) => g.left + (i / Math.max(1, data.length - 1)) * (g.w - g.left - g.right);
  const scaleY = (v: number) => {
    if (max === min) return (g.top + g.h - g.bottom) / 2;
    return g.top + (1 - (v - min) / (max - min)) * (g.h - g.top - g.bottom);
  };
  const pathD = data.map((v, i) => `${i === 0 ? "M" : "L"} ${scaleX(i)} ${scaleY(v)}`).join(" ");

  const cursorX = scaleX(safeCursor);
  const cursorY = scaleY(data[safeCursor]);

  const onPointerMove = (event: React.PointerEvent<SVGSVGElement>) => {
    setCursor(pointerIndexFromEvent(event, data.length, g));
  };

  const onPointerDown = (event: React.PointerEvent<SVGSVGElement>) => {
    event.currentTarget.setPointerCapture(event.pointerId);
    setCursor(pointerIndexFromEvent(event, data.length, g));
  };

  const tipLeft = Math.min(g.w - 130, Math.max(g.left + 8, cursorX + 8));
  const tipTop = Math.max(g.top + 4, cursorY - 38);
  const scrubY = g.h - 22;
  const xDisplay =
    xValues && xValues[safeCursor] !== undefined
      ? (xValueFormatter ?? defaultXFormatter)(xValues[safeCursor])
      : `idx ${safeCursor}`;

  return (
    <div className="relative">
      <svg
        viewBox={`0 0 ${g.w} ${g.h}`}
        width="100%"
        height={g.h}
        style={{ touchAction: "none" }}
        onPointerDown={onPointerDown}
        onPointerMove={onPointerMove}
      >
        <line x1={g.left} y1={g.h - g.bottom} x2={g.w - g.right} y2={g.h - g.bottom} stroke={axisStroke()} strokeWidth={1} />
        <line x1={g.left} y1={g.top} x2={g.left} y2={g.h - g.bottom} stroke={axisStroke()} strokeWidth={1} />
        <line x1={g.left} y1={g.top} x2={g.w - g.right} y2={g.top} stroke={gridStroke()} strokeWidth={1} />
        <line x1={g.left} y1={(g.top + g.h - g.bottom) / 2} x2={g.w - g.right} y2={(g.top + g.h - g.bottom) / 2} stroke={gridStroke()} strokeWidth={1} />
        <path d={pathD} fill="none" stroke={color} strokeWidth={2} strokeLinejoin="round" strokeLinecap="round" />
        <line x1={cursorX} y1={g.top} x2={cursorX} y2={g.h - g.bottom} stroke="rgba(56,189,248,0.95)" strokeDasharray="6 4" strokeWidth={1.8} />
        <line x1={g.left} y1={scrubY} x2={g.w - g.right} y2={scrubY} stroke="rgba(148,163,184,0.5)" strokeWidth={1.5} />
        <line x1={cursorX} y1={g.h - g.bottom} x2={cursorX} y2={scrubY} stroke="rgba(148,163,184,0.45)" strokeWidth={1} />
        <circle cx={cursorX} cy={cursorY} r={5.4} fill="none" stroke="rgba(255,255,255,0.5)" strokeWidth={1.8} />
        <circle cx={cursorX} cy={cursorY} r={4.2} fill="#ffffff" stroke={color} strokeWidth={2.4} />
        <circle cx={cursorX} cy={scrubY} r={5} fill="#ffffff" stroke={color} strokeWidth={2} />
        <text x={g.left - 4} y={g.top + 4} textAnchor="end" fontSize="11" fill="#64748b">{max.toFixed(2)}</text>
        <text x={g.left - 4} y={g.h - g.bottom + 4} textAnchor="end" fontSize="11" fill="#64748b">{min.toFixed(2)}</text>
        <text x={(g.left + g.w - g.right) / 2} y={g.h - 8} textAnchor="middle" fontSize="11" fill="#64748b">{xLabel}</text>
        <text x={12} y={(g.top + g.h - g.bottom) / 2} textAnchor="middle" fontSize="11" fill="#64748b" transform={`rotate(-90 12 ${(g.top + g.h - g.bottom) / 2})`}>{yLabel}</text>
      </svg>
      <div className="pointer-events-none absolute rounded border border-slate-300 bg-white/95 px-2 py-1 text-xs text-slate-700 shadow" style={{ left: `${(tipLeft / g.w) * 100}%`, top: `${(tipTop / g.h) * 100}%` }}>
        <div>x: {xDisplay}</div>
        <div>value: {safeFixed(data[safeCursor], 3)}</div>
      </div>
    </div>
  );
}

export function MultiSparkline({
  series,
  legendClassName = "flex items-center gap-3 text-sm text-gray-600 mb-1",
  noDataClassName = "text-sm text-gray-500",
  xLabel = "Sample",
  yLabel = "Value",
  xValues,
  xValueFormatter,
}: {
  series: { data: number[]; color: string; label?: string }[];
  legendClassName?: string;
  noDataClassName?: string;
  xLabel?: string;
  yLabel?: string;
  xValues?: Array<string | number>;
  xValueFormatter?: (v: string | number) => string;
}) {
  if (!series || series.length === 0) return <div className={noDataClassName}>No data</div>;

  const g = useMemo(() => ({ ...defaultGeom, h: 190 }), []);

  const lengths = series.map((s) => s.data.length);
  const maxLen = Math.max(...lengths, 1);
  const values = series.flatMap((s) => s.data);
  if (values.length === 0) return <div className={noDataClassName}>No data</div>;

  const [cursor, setCursor] = useState<number>(maxLen - 1);
  useEffect(() => {
    setCursor((prev) => clampIndex(prev, maxLen - 1));
  }, [maxLen]);

  const safeCursor = clampIndex(cursor, maxLen - 1);
  const min = Math.min(...values), max = Math.max(...values);
  const scaleX = (i: number) => g.left + (i / Math.max(1, maxLen - 1)) * (g.w - g.left - g.right);
  const scaleY = (v: number) => {
    if (max === min) return (g.top + g.h - g.bottom) / 2;
    return g.top + (1 - (v - min) / (max - min)) * (g.h - g.top - g.bottom);
  };

  const onPointerMove = (event: React.PointerEvent<SVGSVGElement>) => {
    setCursor(pointerIndexFromEvent(event, maxLen, g));
  };

  const onPointerDown = (event: React.PointerEvent<SVGSVGElement>) => {
    event.currentTarget.setPointerCapture(event.pointerId);
    setCursor(pointerIndexFromEvent(event, maxLen, g));
  };

  const cursorX = scaleX(safeCursor);
  const scrubY = g.h - 22;
  const xDisplay =
    xValues && xValues[safeCursor] !== undefined
      ? (xValueFormatter ?? defaultXFormatter)(xValues[safeCursor])
      : `idx ${safeCursor}`;
  const tooltipRows = series
    .filter((s) => s.data.length > 0)
    .map((s) => {
      const idx = clampIndex(safeCursor, s.data.length - 1);
      return {
        label: s.label ?? "series",
        color: s.color,
        value: s.data[idx],
      };
    });

  return (
    <div>
      <div className={legendClassName}>
        {series.map((s) => (
          <div key={s.label ?? s.color} className="flex items-center gap-2">
            <span style={{ width: 12, height: 12, background: s.color, display: "inline-block", borderRadius: 4 }} />
            <span>{s.label ?? ""}</span>
          </div>
        ))}
      </div>
      <div className="relative">
        <svg
          viewBox={`0 0 ${g.w} ${g.h}`}
          width="100%"
          height={g.h}
          style={{ touchAction: "none" }}
          onPointerDown={onPointerDown}
          onPointerMove={onPointerMove}
        >
          <line x1={g.left} y1={g.h - g.bottom} x2={g.w - g.right} y2={g.h - g.bottom} stroke={axisStroke()} strokeWidth={1} />
          <line x1={g.left} y1={g.top} x2={g.left} y2={g.h - g.bottom} stroke={axisStroke()} strokeWidth={1} />
          <line x1={g.left} y1={g.top} x2={g.w - g.right} y2={g.top} stroke={gridStroke()} strokeWidth={1} />
          <line x1={g.left} y1={(g.top + g.h - g.bottom) / 2} x2={g.w - g.right} y2={(g.top + g.h - g.bottom) / 2} stroke={gridStroke()} strokeWidth={1} />
          {series.map((s) => {
            const d = s.data.map((v, i) => `${i === 0 ? "M" : "L"} ${scaleX(i)} ${scaleY(v)}`).join(" ");
            return <path key={s.label ?? s.color} d={d} fill="none" stroke={s.color} strokeWidth={2} strokeLinejoin="round" strokeLinecap="round" />;
          })}
          <line x1={cursorX} y1={g.top} x2={cursorX} y2={g.h - g.bottom} stroke="rgba(56,189,248,0.95)" strokeDasharray="6 4" strokeWidth={1.8} />
          {series.map((s) => {
            if (s.data.length === 0) return null;
            const idx = clampIndex(safeCursor, s.data.length - 1);
            const y = scaleY(s.data[idx]);
            return (
              <g key={`${s.label ?? s.color}-cursor`}>
                <circle cx={cursorX} cy={y} r={5.4} fill="none" stroke="rgba(255,255,255,0.5)" strokeWidth={1.8} />
                <circle cx={cursorX} cy={y} r={4.2} fill="#ffffff" stroke={s.color} strokeWidth={2.4} />
              </g>
            );
          })}
          <line x1={g.left} y1={scrubY} x2={g.w - g.right} y2={scrubY} stroke="rgba(148,163,184,0.5)" strokeWidth={1.5} />
          <line x1={cursorX} y1={g.h - g.bottom} x2={cursorX} y2={scrubY} stroke="rgba(148,163,184,0.45)" strokeWidth={1} />
          <circle cx={cursorX} cy={scrubY} r={5} fill="#ffffff" stroke="#334155" strokeWidth={2} />
          <text x={g.left - 4} y={g.top + 4} textAnchor="end" fontSize="11" fill="#64748b">{max.toFixed(2)}</text>
          <text x={g.left - 4} y={g.h - g.bottom + 4} textAnchor="end" fontSize="11" fill="#64748b">{min.toFixed(2)}</text>
          <text x={(g.left + g.w - g.right) / 2} y={g.h - 8} textAnchor="middle" fontSize="11" fill="#64748b">{xLabel}</text>
          <text x={12} y={(g.top + g.h - g.bottom) / 2} textAnchor="middle" fontSize="11" fill="#64748b" transform={`rotate(-90 12 ${(g.top + g.h - g.bottom) / 2})`}>{yLabel}</text>
        </svg>
        <div className="pointer-events-none absolute rounded border border-slate-300 bg-white/95 px-2 py-1 text-xs text-slate-700 shadow" style={{ left: `${Math.min(82, Math.max(2, (cursorX / g.w) * 100))}%`, top: "8%" }}>
          <div>x: {xDisplay}</div>
          {tooltipRows.map((row) => (
            <div key={row.label} className="flex items-center gap-1">
              <span style={{ width: 8, height: 8, borderRadius: 99, background: row.color, display: "inline-block" }} />
              <span>{row.label}: {safeFixed(row.value, 3)}</span>
            </div>
          ))}
        </div>
      </div>
    </div>
  );
}

export function SpectrumBars({
  bins,
  color,
  noDataClassName = "text-sm text-gray-500",
  xLabel = "Frequency (Hz)",
  yLabel = "Amplitude (g)",
}: {
  bins: SpectrumBin[];
  color: string;
  noDataClassName?: string;
  xLabel?: string;
  yLabel?: string;
}) {
  if (!bins || bins.length === 0) return <div className={noDataClassName}>No spectrum data</div>;

  const g = useMemo(() => ({ ...defaultGeom, h: 190 }), []);
  const [cursor, setCursor] = useState<number>(bins.length - 1);
  useEffect(() => {
    setCursor((prev) => clampIndex(prev, bins.length - 1));
  }, [bins.length]);

  const safeCursor = clampIndex(cursor, bins.length - 1);
  const maxAmp = Math.max(...bins.map((b) => b.amplitude), 1e-9);
  const barW = (g.w - g.left - g.right) / bins.length;
  const scrubY = g.h - 22;

  const onPointerMove = (event: React.PointerEvent<SVGSVGElement>) => {
    setCursor(pointerIndexFromEvent(event, bins.length, g));
  };

  const onPointerDown = (event: React.PointerEvent<SVGSVGElement>) => {
    event.currentTarget.setPointerCapture(event.pointerId);
    setCursor(pointerIndexFromEvent(event, bins.length, g));
  };

  const cursorX = g.left + safeCursor * barW + barW / 2;
  const tipLeft = Math.min(82, Math.max(2, (cursorX / g.w) * 100));
  const selected = bins[safeCursor];

  return (
    <div className="relative">
      <svg
        viewBox={`0 0 ${g.w} ${g.h}`}
        width="100%"
        height={g.h}
        style={{ touchAction: "none" }}
        onPointerDown={onPointerDown}
        onPointerMove={onPointerMove}
      >
        <line x1={g.left} y1={g.h - g.bottom} x2={g.w - g.right} y2={g.h - g.bottom} stroke={axisStroke()} strokeWidth={1} />
        <line x1={g.left} y1={g.top} x2={g.left} y2={g.h - g.bottom} stroke={axisStroke()} strokeWidth={1} />
        <line x1={g.left} y1={g.top} x2={g.w - g.right} y2={g.top} stroke={gridStroke()} strokeWidth={1} />
        <line x1={g.left} y1={(g.top + g.h - g.bottom) / 2} x2={g.w - g.right} y2={(g.top + g.h - g.bottom) / 2} stroke={gridStroke()} strokeWidth={1} />
        {bins.map((b, i) => {
          const x = g.left + i * barW;
          const barH = ((g.h - g.top - g.bottom) * b.amplitude) / maxAmp;
          const y = g.h - g.bottom - barH;
          return (
            <rect
              key={`${b.f_hz}-${i}`}
              x={x}
              y={y}
              width={Math.max(1, barW - 1)}
              height={Math.max(1, barH)}
              fill={color}
              opacity={0.85}
            />
          );
        })}
        <line x1={cursorX} y1={g.top} x2={cursorX} y2={g.h - g.bottom} stroke="rgba(56,189,248,0.95)" strokeDasharray="6 4" strokeWidth={1.8} />
        <line x1={g.left} y1={scrubY} x2={g.w - g.right} y2={scrubY} stroke="rgba(148,163,184,0.5)" strokeWidth={1.5} />
        <line x1={cursorX} y1={g.h - g.bottom} x2={cursorX} y2={scrubY} stroke="rgba(148,163,184,0.45)" strokeWidth={1} />
        <circle cx={cursorX} cy={scrubY} r={5} fill="#ffffff" stroke="#334155" strokeWidth={2} />
        <text x={g.left - 4} y={g.top + 4} textAnchor="end" fontSize="11" fill="#64748b">{maxAmp.toFixed(3)}</text>
        <text x={g.left - 4} y={g.h - g.bottom + 4} textAnchor="end" fontSize="11" fill="#64748b">0.000</text>
        <text x={(g.left + g.w - g.right) / 2} y={g.h - 8} textAnchor="middle" fontSize="11" fill="#64748b">{xLabel}</text>
        <text x={12} y={(g.top + g.h - g.bottom) / 2} textAnchor="middle" fontSize="11" fill="#64748b" transform={`rotate(-90 12 ${(g.top + g.h - g.bottom) / 2})`}>{yLabel}</text>
      </svg>
      <div className="pointer-events-none absolute rounded border border-slate-300 bg-white/95 px-2 py-1 text-xs text-slate-700 shadow" style={{ left: `${tipLeft}%`, top: "8%" }}>
        <div>bin: {safeCursor}</div>
        <div>f: {safeFixed(selected?.f_hz, 2)} Hz</div>
        <div>a: {safeFixed(selected?.amplitude, 4)}</div>
      </div>
    </div>
  );
}