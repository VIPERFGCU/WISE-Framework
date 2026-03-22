export type StreamPoint = { t: string; x: number; y: number; z: number };
export type HeartbeatPoint = { t: string; rssi: number };
export type SpectrumBin = { f_hz: number; amplitude: number };

export function Sparkline({
  data,
  color,
  noDataClassName = "text-sm text-gray-500",
}: {
  data: number[];
  color: string;
  noDataClassName?: string;
}) {
  const w = 600, h = 120, pad = 6;
  if (!data || data.length === 0) return <div className={noDataClassName}>No data</div>;

  const min = Math.min(...data), max = Math.max(...data);
  const scaleX = (i: number) => pad + (i / Math.max(1, data.length - 1)) * (w - pad * 2);
  const scaleY = (v: number) => {
    if (max === min) return h / 2;
    return pad + (1 - (v - min) / (max - min)) * (h - pad * 2);
  };
  const d = data.map((v, i) => `${i === 0 ? "M" : "L"} ${scaleX(i)} ${scaleY(v)}`).join(" ");

  return (
    <svg viewBox={`0 0 ${w} ${h}`} width="100%" height="120">
      <path d={d} fill="none" stroke={color} strokeWidth={2} strokeLinejoin="round" strokeLinecap="round" />
    </svg>
  );
}

export function MultiSparkline({
  series,
  legendClassName = "flex items-center gap-3 text-sm text-gray-600 mb-1",
  noDataClassName = "text-sm text-gray-500",
}: {
  series: { data: number[]; color: string; label?: string }[];
  legendClassName?: string;
  noDataClassName?: string;
}) {
  const w = 600, h = 140, pad = 6;
  if (!series || series.length === 0) return <div className={noDataClassName}>No data</div>;

  const lengths = series.map((s) => s.data.length);
  const maxLen = Math.max(...lengths, 1);
  const values = series.flatMap((s) => s.data);
  if (values.length === 0) return <div className={noDataClassName}>No data</div>;

  const min = Math.min(...values), max = Math.max(...values);
  const scaleX = (i: number) => pad + (i / Math.max(1, maxLen - 1)) * (w - pad * 2);
  const scaleY = (v: number) => {
    if (max === min) return h / 2;
    return pad + (1 - (v - min) / (max - min)) * (h - pad * 2);
  };

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
      <svg viewBox={`0 0 ${w} ${h}`} width="100%" height={h}>
        {series.map((s) => {
          const d = s.data.map((v, i) => `${i === 0 ? "M" : "L"} ${scaleX(i)} ${scaleY(v)}`).join(" ");
          return <path key={s.label ?? s.color} d={d} fill="none" stroke={s.color} strokeWidth={2} strokeLinejoin="round" strokeLinecap="round" />;
        })}
      </svg>
    </div>
  );
}

export function SpectrumBars({
  bins,
  color,
  noDataClassName = "text-sm text-gray-500",
}: {
  bins: SpectrumBin[];
  color: string;
  noDataClassName?: string;
}) {
  const w = 600;
  const h = 140;
  const pad = 8;

  if (!bins || bins.length === 0) return <div className={noDataClassName}>No spectrum data</div>;

  const maxAmp = Math.max(...bins.map((b) => b.amplitude), 1e-9);
  const barW = (w - pad * 2) / bins.length;

  return (
    <svg viewBox={`0 0 ${w} ${h}`} width="100%" height={h}>
      {bins.map((b, i) => {
        const x = pad + i * barW;
        const barH = ((h - pad * 2) * b.amplitude) / maxAmp;
        const y = h - pad - barH;
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
    </svg>
  );
}