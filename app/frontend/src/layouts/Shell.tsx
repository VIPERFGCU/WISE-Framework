import { Link, NavLink, Outlet, useNavigate } from "react-router-dom";
import { useState } from "react";
import { isAuthed, clearToken } from "../lib/auth";
import { Toaster } from "../components/Toaster";
import TopLoader from "../components/TopLoader";
import ErrorBoundary from "../components/ErrorBoundary";

export default function Shell() {
	const nav = useNavigate();
	const authed = isAuthed();
  const [sidebarOpen, setSidebarOpen] = useState(true);
  const [mobileNavOpen, setMobileNavOpen] = useState(false);

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

  const navItems = (
    <>
      <NavLink to="/" end className={navLinkClass} onClick={() => setMobileNavOpen(false)}>Dashboard</NavLink>
      <NavLink to="/devices" className={navLinkClass} onClick={() => setMobileNavOpen(false)}>Devices</NavLink>
      <NavLink to="/streams" className={navLinkClass} onClick={() => setMobileNavOpen(false)}>Streams</NavLink>
      <NavLink to={isAuthed() ? "/admin" : "/login"} className={navLinkClass} onClick={() => setMobileNavOpen(false)}>Admin</NavLink>
    </>
  );


  return (
    <div className="min-h-screen hud">
      
      <TopLoader />
      <Toaster />

      <ErrorBoundary>
      <div className="min-h-screen flex">
      {/* Sidebar (desktop) */}
      {sidebarOpen && (
        <aside className="hidden md:block w-64 border-r bg-slate-950/80 backdrop-blur">
          <div className="p-4 border-b">
            <Link to="/" className="text-lg font-semibold text-sky-200">WISENET</Link>
            <div className="text-xs text-slate-400">Frontend</div>
          </div>
          <nav className="p-2 space-y-1 text-sm">{navItems}</nav>
        </aside>
      )}

      {/* Sidebar (mobile drawer) */}
      {mobileNavOpen && (
        <div className="md:hidden fixed inset-0 z-40">
          <button
            type="button"
            className="absolute inset-0 bg-black/50"
            aria-label="Close navigation menu"
            onClick={() => setMobileNavOpen(false)}
          />
          <aside className="relative h-full w-72 max-w-[85vw] border-r bg-slate-950/95 backdrop-blur">
            <div className="p-4 border-b flex items-center justify-between">
              <div>
                <Link to="/" className="text-lg font-semibold text-sky-200" onClick={() => setMobileNavOpen(false)}>WISENET</Link>
                <div className="text-xs text-slate-400">Frontend</div>
              </div>
              <button
                type="button"
                className="text-sm border rounded px-2 py-1 bg-slate-900/80 hover:bg-slate-800/80 text-slate-100"
                onClick={() => setMobileNavOpen(false)}
              >
                Close
              </button>
            </div>
            <nav className="p-2 space-y-1 text-sm">{navItems}</nav>
          </aside>
        </div>
      )}

      {/* Main */}
      <div className="flex-1 min-w-0 flex flex-col">
        <header className="h-14 border-b bg-slate-950/80 backdrop-blur flex items-center justify-between px-3 sm:px-4">
          <div className="flex items-center gap-2 sm:gap-3">
            <button
              type="button"
              className="md:hidden text-sm border rounded px-2 py-1 bg-slate-900/80 hover:bg-slate-800/80 text-slate-100"
              onClick={() => setMobileNavOpen(true)}
            >
              Menu
            </button>
            <button
              type="button"
              className="hidden md:inline-block text-sm border rounded px-2 py-1 bg-slate-900/80 hover:bg-slate-800/80 text-slate-100"
              onClick={() => setSidebarOpen((v) => !v)}
              title={sidebarOpen ? "Collapse sidebar" : "Expand sidebar"}
            >
              {sidebarOpen ? "Collapse" : "Expand"}
            </button>
            <div className="font-medium text-sky-100">WISENET Console</div>
          </div>
          <div className="text-xs text-slate-400">v0.1</div>
        </header>
        <main className="p-3 sm:p-4 md:p-5 lg:p-6 overflow-x-hidden">
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
      </div>
      </ErrorBoundary>
    </div>
  );
}

