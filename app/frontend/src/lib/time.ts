const MIN_TS_MS = Date.UTC(2020, 0, 1);
const MAX_FUTURE_DRIFT_MS = 10 * 60 * 1000; // allow small clock drift

export function parseTelemetryTimestampMs(value: unknown): number | null {
  if (typeof value !== "string" || value.trim().length === 0) {
    return null;
  }

  const ts = Date.parse(value);
  if (!Number.isFinite(ts)) {
    return null;
  }

  const maxTs = Date.now() + MAX_FUTURE_DRIFT_MS;
  if (ts < MIN_TS_MS || ts > maxTs) {
    return null;
  }

  return ts;
}

export function isTelemetryTimestampSane(value: unknown): boolean {
  return parseTelemetryTimestampMs(value) !== null;
}
