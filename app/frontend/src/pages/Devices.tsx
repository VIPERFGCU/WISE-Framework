import { useEffect, useMemo, useState } from "react";
import { listDevices, setSensing, setFrequency, setBatchSize } from "../services/devices";
import type { Device, DeviceStatus } from "../types/device";
import StatusBadge from "../components/StatusBadge";
import BoolPill from "../components/BoolPill";
import { formatUptime, parseIsoMs, timeAgo } from "../lib/format";
import { emitError } from "../components/Toaster";
import DeviceDrawer from "../components/DeviceDrawer";

type Tri = "all" | "yes" | "no";
type SortKey = "uptime_asc" | "uptime_desc" | "status";

const STALE_SEC = 120; // mark devices stale if no update within 2 minutes
const LEGACY_SENSOR_IDS = new Set(["bridge-esp32-001", "esp32-test-01", "sim-device-001"]);

export default function Devices() {
  const [devices, setDevices] = useState<Device[]>([]);
  const [drawerFor, setDrawerFor] = useState<Device | null>(null);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);

  const btn = [
	"px-3 py-1.5 text-xs rounded border border-blue-500 text-blue-600 bg-blue-50",
  	"hover:bg-blue-100 hover:text-blue-700 hover:shadow-sm active:bg-blue-200",
  	"disabled:opacity-60 whitespace-nowrap w-24 text-center font-medium",
  	"transition-colors duration-150",
  ].join(" ");

	// per-row action loading states
	const [senseBusy, setSenseBusy] = useState<Record<string, boolean>>({});

	// per-row freq/batch input values and busy states
	const [freqInputs, setFreqInputs] = useState<Record<string, string>>({});
	const [batchInputs, setBatchInputs] = useState<Record<string, string>>({});
	const [freqBusy, setFreqBusy] = useState<Record<string, boolean>>({});
	const [batchBusy, setBatchBusy] = useState<Record<string, boolean>>({});

	// controls
	const [q, setQ] = useState("");
	const [status, setStatus] = useState<DeviceStatus | "all">("all");
	const [sense, setSense] = useState<Tri>("all");
  const [sortKey, setSortKey] = useState<SortKey>("uptime_desc");
  const [globalHz, setGlobalHz] = useState<string>("");
  const [globalBatch, setGlobalBatch] = useState<string>("");
  const [globalBusy, setGlobalBusy] = useState(false);


  // page-level "last loaded" clock
  const [lastLoadAt, setLastLoadAt] = useState<number | null>(null);

  async function load() {
    try {
      setError(null);
      const data = await listDevices();
      setDevices(data ?? []);
      setLastLoadAt(Date.now());
    } catch (e: any) {
      setError(e?.message ?? "Failed to fetch devices");
    } finally {
      setLoading(false);
    }
  }

  useEffect(() => {
    load();
    const id = setInterval(load, 10_000);
    return () => clearInterval(id);
  }, []);

  // Initialize per-row input values from device list when devices change
  useEffect(() => {
    const f: Record<string, string> = {};
    const b: Record<string, string> = {};
    for (const d of devices) {
      if (d.sample_hz != null) f[d.sensor_id] = String(d.sample_hz);
      if (d.batch_size != null) b[d.sensor_id] = String(d.batch_size);
    }
    setFreqInputs((prev) => ({ ...f, ...prev }));
    setBatchInputs((prev) => ({ ...b, ...prev }));
  }, [devices]);

  // Compute filtered + sorted rows
  //filter
  const rows = useMemo(() => {
		let out = devices.filter((d) => {
			if (LEGACY_SENSOR_IDS.has(d.sensor_id)) return false;
			if (status !== "all" && d.status !== status) return false;
			if (sense !== "all" && d.sensing !== (sense === "yes")) return false;
			if (q && !d.sensor_id.toLowerCase().includes(q.toLowerCase())) return false;
			return true;
		});

    // sort
    if (sortKey === "uptime_asc") {
      out = out.slice().sort((a, b) => a.uptime_seconds - b.uptime_seconds);
    } else if (sortKey === "uptime_desc") {
      out = out.slice().sort((a, b) => b.uptime_seconds - a.uptime_seconds);
    } else if (sortKey === "status") {
      const order: Record<DeviceStatus, number> = { on: 0, updating: 1, off: 2 };
      out = out.slice().sort((a, b) => order[a.status] - order[b.status]);
    }

    return out;
	}, [devices, q, status, sense, sortKey]);

  // helpers
  function isStale(d: Device): boolean {
	  const t = parseIsoMs(d.updated_at);
	  if (!Number.isFinite(t)) return false; //Unknown -> don't publish
	  return Date.now() - t > STALE_SEC * 1000;
  }

  

  const toggleSensing = async (d: Device) => {
	  const next = !d.sensing;
	  setSenseBusy((m) => ({ ...m, [d.sensor_id]: true }));
	  setDevices((list) => list.map((x) => (x.sensor_id === d.sensor_id ? { ...x, sensing: next } : x)));
	  try {
		  await setSensing(d.sensor_id, next);
	  } catch (e: any) {
		  setDevices((list) => list.map((x) => (x.sensor_id === d.sensor_id ? { ...x, sensing: !next } : x)));
		  emitError(e?.message ?? "Failed to update sensing");
	  } finally {
		  setSenseBusy((m) => ({ ...m, [d.sensor_id]: false }));
	  }
  };

	const applyFrequency = async (d: Device) => {
		const hz = Number(freqInputs[d.sensor_id]);
		if (!Number.isFinite(hz) || hz <= 0) {
			emitError("Enter a positive number for frequency.");
			return;
		}
		setFreqBusy((m) => ({ ...m, [d.sensor_id]: true }));
		try {
			await setFrequency(d.sensor_id, hz);
			await load();
		} catch (e: any) {
			emitError(e?.message ?? "Failed to set frequency");
		} finally {
			setFreqBusy((m) => ({ ...m, [d.sensor_id]: false }));
		}
	};

	const applyBatch = async (d: Device) => {
		const bs = Number(batchInputs[d.sensor_id]);
		if (!Number.isFinite(bs) || bs <= 0) {
			emitError("Enter a positive integer for batch size.");
			return;
		}
		setBatchBusy((m) => ({ ...m, [d.sensor_id]: true }));
		try {
			await setBatchSize(d.sensor_id, bs);
			await load();
		} catch (e: any) {
			emitError(e?.message ?? "Failed to set batch size");
		} finally {
			setBatchBusy((m) => ({ ...m, [d.sensor_id]: false }));
		}
	};


  return (
    <div className="space-y-4">
      <div className="flex items-center justify-between gap-3 flex-wrap">
        <h2 className="text-lg font-semibold">Devices</h2>
        <div className="flex items-center gap-4 text-sm text-gray-600">
	   {lastLoadAt && <span>Last refresh: {timeAgo(Date.now() - lastLoadAt)}</span>}
          <button
            onClick={load}
            className="px-3 py-1.5 text-sm border rounded bg-white hover:bg-gray-50"
            disabled={loading}
            title="Refresh"
          >
            Refresh
          </button>
        </div>
      </div>

      {/* Global controls */}
      <div className="bg-white border rounded p-3 grid gap-3 sm:grid-cols-2 lg:grid-cols-[1fr_1fr_auto]">
      	<div>
	   <label className="block text-xs text-gray-600 mb-1">
	   	All sensors: Frequency (Hz)
	   </label>
	   <input
	      value={globalHz}
	      onChange={(e) => setGlobalHz(e.target.value)}
	      inputMode="numeric"
	      className="border rounded px-3 py-2 text-sm w-full"
	      placeholder="e.g. 50"
	   />
	</div>
	<div>
	   <label className="block text-xs text-gray-600 mb-1">
	   	All sensors: Batch size
	   </label>
	   <input
	      value={globalBatch}
	      onChange={(e) => setGlobalBatch(e.target.value)}
	      inputMode="numeric"
	      className="border rounded px-3 py-2 text-sm w-full"
	      placeholder="e.g. 100"
	   />
	</div>
	<button
	   disabled={globalBusy}
	   onClick={async () => {
		   try {
			   setGlobalBusy(true);
			   const hz = Number(globalHz);
			   const bs = Number(globalBatch);

			   const tasks: Promise<void>[] = [];
			   for (const d of devices) {
				   if (Number.isFinite(hz) && hz > 0) {
					   tasks.push(setFrequency(d.sensor_id, hz));
				   }
				   if (Number.isFinite(bs) && bs > 0) {
					   tasks.push(setBatchSize(d.sensor_id, bs));
				   }
			   }
			   if (tasks.length === 0) {
				   emitError("Enter at least one valid value before applying.");
			   } else {
				   await Promise.all(tasks);
				   await load(); //refresh devices list
			   }
		   } catch (e: any) {
			   emitError(e?.message ?? "Failed to update all devices");
		   } finally {
			   setGlobalBusy(false);
		   }
	   }}
	   className="px-3 py-2 text-sm rounded bg-black text-white disabled:opacity-60 sm:w-fit w-full"
	 >
	   {globalBusy ? "Applying..." : "Apply to All"}
	 </button>
       </div>
      	      
	{/* Controls */}
	<div className="bg-white border rounded p-3 grid gap-2 grid-cols-1 sm:grid-cols-2 lg:grid-cols-5">
        <input
          value={q}
          onChange={(e) => setQ(e.target.value)}
          placeholder="Search sensor id…"
		  className="border rounded px-3 py-2 text-sm sm:col-span-2 lg:col-span-2"
        />
        <select
          value={status}
          onChange={(e) => setStatus(e.target.value as any)}
          className="border rounded px-3 py-2 text-sm"
          title="Status"
        >
          <option value="all">Status: All</option>
          <option value="on">Status: On</option>
          <option value="updating">Status: Updating</option>
          <option value="off">Status: Off</option>
        </select>
				{/* Recording filter removed (redundant with Sensing) */}
        <select
          value={sense}
          onChange={(e) => setSense(e.target.value as Tri)}
          className="border rounded px-3 py-2 text-sm"
          title="Sensing"
        >
          <option value="all">Sensing: All</option>
          <option value="yes">Sensing: Y</option>
          <option value="no">Sensing: N</option>
        </select>
        <select
          value={sortKey}
          onChange={(e) => setSortKey(e.target.value as SortKey)}
					className="border rounded px-3 py-2 text-sm sm:col-span-2 lg:col-span-2"
          title="Sort"
        >
          <option value="uptime_desc">Sort: Uptime ↓</option>
          <option value="uptime_asc">Sort: Uptime ↑</option>
          <option value="status">Sort: Status</option>
        </select>
      </div>

      {/* Table */}
      {loading && <div className="text-gray-600">Loading devices…</div>}
      {error && <div className="text-red-600">Error: {error}</div>}
      {!loading && !error && rows.length === 0 && (
        <div className="text-gray-600">No devices match your filters.</div>
      )}

			{!loading && !error && rows.length > 0 && (
				<div className="space-y-3 md:hidden">
					{rows.map((d) => {
						const t = parseIsoMs(d.updated_at);
						const stale = isStale(d);
						return (
							<section
								key={d.sensor_id}
								className={`rounded border p-3 bg-white ${stale ? "opacity-80 bg-gray-50" : ""}`}
								title={Number.isFinite(t) ? new Date(t).toLocaleString() : "unknown"}
							>
								<div className="flex items-start justify-between gap-3 mb-2">
									<div>
										<div className="font-mono text-sm break-all">{d.sensor_id}</div>
										<div className="text-xs text-gray-500">{d.label ?? "-"}</div>
									</div>
									<StatusBadge status={d.status} />
								</div>

								<div className="grid grid-cols-2 gap-2 text-sm mb-3">
									<div>
										<div className="text-xs text-gray-500">Sensing</div>
										<BoolPill value={d.sensing} />
									</div>
									<div>
										<div className="text-xs text-gray-500">Up time</div>
										<div>{formatUptime(d.uptime_seconds)}</div>
									</div>
									<div className="col-span-2">
										<div className="text-xs text-gray-500">Last Updated</div>
										<div>
											{Number.isFinite(t) ? timeAgo(Date.now() - t) : "-"}
											{stale && <span className="ml-2 inline-block h-2 w-2 rounded-full bg-red-500 align-middle" />}
										</div>
									</div>
								</div>

								<div className="space-y-2 mb-3">
									<div className="flex items-center gap-2">
										<input
											value={freqInputs[d.sensor_id] ?? (d.sample_hz != null ? String(d.sample_hz) : "")}
											onChange={(e) => setFreqInputs((m) => ({ ...m, [d.sensor_id]: e.target.value }))}
											inputMode="numeric"
											className="border rounded px-2 py-1 text-sm flex-1"
											placeholder="Hz"
										/>
										<button onClick={() => applyFrequency(d)} className={btn}>
											{freqBusy[d.sensor_id] ? "..." : "Apply"}
										</button>
									</div>
									<div className="flex items-center gap-2">
										<input
											value={batchInputs[d.sensor_id] ?? (d.batch_size != null ? String(d.batch_size) : "")}
											onChange={(e) => setBatchInputs((m) => ({ ...m, [d.sensor_id]: e.target.value }))}
											inputMode="numeric"
											className="border rounded px-2 py-1 text-sm flex-1"
											placeholder="batch"
										/>
										<button onClick={() => applyBatch(d)} className={btn}>
											{batchBusy[d.sensor_id] ? "..." : "Apply"}
										</button>
									</div>
								</div>

								<div className="flex items-center gap-2">
									<button
										onClick={() => toggleSensing(d)}
										disabled={!!senseBusy[d.sensor_id]}
										className={btn}
										title={d.sensing ? "Stop sensing" : "Start sensing"}
									>
										{senseBusy[d.sensor_id] ? "..." : d.sensing ? "Stop Sense" : "Start Sense"}
									</button>
									<button onClick={() => setDrawerFor(d)} className={btn} title="View details">
										Details
									</button>
								</div>
							</section>
						);
					})}

					<div className="text-xs text-gray-500">
						Devices marked light gray are considered <span className="font-medium">stale</span> (no update in &gt; {STALE_SEC}s).
					</div>
				</div>
			)}

			{!loading && !error && rows.length > 0 && (
				<div className="hidden md:block overflow-x-auto">
          <table className="min-w-full bg-white border rounded">
            <thead className="text-left text-sm text-gray-600 border-b">
              <tr>
                <th className="px-3 py-2">Sensor ID</th>
								<th className="px-3 py-2">Label</th>
                <th className="px-3 py-2">Status</th>
                <th className="px-3 py-2">Sensing</th>
		<th className="px-3 py-2">Freq (Hz)</th>
		<th className="px-3 py-2">Batch</th>
                <th className="px-3 py-2">Up time</th>
		<th className="px-3 py-2">Last Updated</th>
		<th className="px-3 py-2">Actions</th>
              </tr>
            </thead>
            <tbody className="text-sm">
              {rows.map((d) => {
		const t = parseIsoMs(d.updated_at);
		const stale = isStale(d);
		return (
                 <tr 
			key={d.sensor_id} 
			className={`border-t ${stale ? "bg-gray-50 opacity-80": ""}`}
			title={Number.isFinite(t) ? new Date(t).toLocaleString() : "unknown"}
		 >
                  <td className="px-3 py-2 font-mono">{d.sensor_id}</td>
				  <td className="px-3 py-2">{d.label ?? "-"}</td>
                  <td className="px-3 py-2"><StatusBadge status={d.status} /></td>
				  <td className="px-3 py-2"><BoolPill value={d.sensing} /></td>
		  <td className="px-3 py-2">
		  	{/* per-row freq input */}
			<div className="flex items-center gap-2">
			  <input
				value={freqInputs[d.sensor_id] ?? (d.sample_hz != null ? String(d.sample_hz) : "")}
				onChange={(e) => setFreqInputs((m) => ({ ...m, [d.sensor_id]: e.target.value }))}
				inputMode="numeric"
				className="border rounded px-2 py-1 text-sm w-24"
				placeholder="Hz"
			  />
			  <button
				onClick={() => applyFrequency(d)}
				className={btn}
			  >
				{freqBusy[d.sensor_id] ? "..." : "Apply"}
			  </button>
			</div>
		  </td>
		  <td className="px-3 py-2">
		     {/* per-row batch input */}
			<div className="flex items-center gap-2">
			  <input
				value={batchInputs[d.sensor_id] ?? (d.batch_size != null ? String(d.batch_size) : "")}
				onChange={(e) => setBatchInputs((m) => ({ ...m, [d.sensor_id]: e.target.value }))}
				inputMode="numeric"
				className="border rounded px-2 py-1 text-sm w-24"
				placeholder="batch"
			  />
			  <button
				onClick={() => applyBatch(d)}
				className={btn}
			  >
				{batchBusy[d.sensor_id] ? "..." : "Apply"}
			  </button>
			</div>
		  </td>
		  <td className="px-3 py-2">{formatUptime(d.uptime_seconds)}</td>
		  <td className="px-3 py-2">
		     {Number.isFinite(t) ? timeAgo(Date.now() - t) : "-"}
		     {stale && (
			     <span className="ml-2 inline-block h-2 w-2 rounded-full bg-red-500 align-middle" />
		     )}
		  </td>
		  <td className="px-3 py-2">
		     <div className="flex items-center gap-2">
			<button
			   onClick={() => toggleSensing(d)}
			   disabled={!!senseBusy[d.sensor_id]}
			   className={btn}
			   title={d.sensing ? "Stop sensing" : "Start sensing"}
			>
			   {senseBusy[d.sensor_id] ? "..." : d.sensing ? "Stop Sense" : "Start Sense"}
			</button>
			<button
				onClick={() => setDrawerFor(d)}
				className={btn}
				title="View details"
			>
				Details
			</button>
		     </div>
		  </td>
                </tr>
              );
	      })}
            </tbody>
          </table>

          <div className="text-xs text-gray-500 mt-2">
	  	Devices marked light gray are considered <span className="font-medium">stale</span> (no update in &gt; {STALE_SEC}s).
	  </div>

        </div> //Closing div for overflow-auto-x
      )}

      {/* Drawer mounted at page root */}
      <DeviceDrawer device={drawerFor} onClose={() => setDrawerFor(null)} />

    </div> //Closing wrapper div
  );
}

