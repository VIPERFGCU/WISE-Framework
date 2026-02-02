import { Link, NavLink, Outlet, useNavigate } from "react-router-dom";
import { isAuthed, clearToken } from "../lib/auth";
import { Toaster } from "../components/Toaster";
import TopLoader from "../components/TopLoader";
import ErrorBoundary from "../components/ErrorBoundary";

export default function Shell() {
	const nav = useNavigate();
	const authed = isAuthed();
	function doLogout() {
		clearToken();
		nav("/login", {replace: true });
	}

       	// Shared sidebar link style (same for all items)
  const navLinkClass = ({ isActive }: { isActive: boolean }) =>
    "block px-3 py-2 rounded transition-colors " +
    (isActive
      ? "bg-sky-500/20 text-sky-200 border border-sky-500/40"
      : "text-slate-300 hover:bg-slate-800/60 hover:text-sky-100");


  return (
    <div className="min-h-screen flex hud">
      
      <TopLoader />
      <Toaster />

      <ErrorBoundary>
      {/* Sidebar */}
      <aside className="w-64 border-r bg-slate-950/80 backdrop-blur">
        <div className="p-4 border-b">
          <Link to="/" className="text-lg font-semibold text-sky-200">WISENET</Link>
          <div className="text-xs text-slate-400">Frontend</div>
        </div>
        <nav className="p-2 space-y-1 text-sm">
          <NavLink to="/" end className={navLinkClass}>Dashboard</NavLink>
          <NavLink to="/devices" className={navLinkClass}>Devices</NavLink>
          <NavLink to="/streams" className={navLinkClass}>Streams</NavLink>
	  <NavLink to={isAuthed() ? "/admin" : "/login"} className={navLinkClass}>Admin</NavLink>
	  <NavLink to="/status" className={navLinkClass}>Status</NavLink>
        </nav>
      </aside>

      {/* Main */}
      <div className="flex-1 flex flex-col">
        <header className="h-14 border-b bg-slate-950/80 backdrop-blur flex items-center justify-between px-4">
          <div className="font-medium text-sky-100">WISENET Console</div>
          <div className="text-xs text-slate-400">v0.1</div>
        </header>
        <main className="p-4">
		<div className="flex justify-end mb-3">
			{authed ? (
				<button onClick={doLogout} className="text-sm border rounded px-3 py-1 bg-slate-900/80 hover:bg-slate-800/80 text-slate-100">
					Logout
				</button>
			) : (
				<Link to="/login" className="text-sm border rounded px-3 py-1 bg-slate-900/80 hover:bg-slate-800/80 text-slate-100">
					Login
				</Link>
			)}
		</div>
          <Outlet />
        </main>
      </div>
      </ErrorBoundary>
    </div>
  );
}

