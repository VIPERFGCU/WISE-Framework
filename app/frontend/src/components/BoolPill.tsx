export default function BoolPill({ value, yes="Y", no="N" }: { value: boolean; yes?: string; no?: string }) {
  const cls = value
    ? "bg-green-100 text-green-700 border-green-200"
    : "bg-red-100 text-red-700 border-red-200";
  return (
    <span className={`inline-flex items-center px-2 py-0.5 rounded text-xs border ${cls}`}>
      {value ? yes : no}
    </span>
  );
}

