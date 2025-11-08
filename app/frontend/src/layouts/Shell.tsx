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
   		"block px-3 py-2 rounded " +
   		(isActive ? "bg-gray-100 font-medium" : "hover:bg-gray-50");


  return (
    <div className="min-h-screen flex bg-gray-50">i
      
      <TopLoader />
      <Toaster />

      <ErrorBoundary>
      {/* Sidebar */}
      <aside className="w-64 border-r bg-white">
        <div className="p-4 border-b">
          <Link to="/" className="text-lg font-semibold">WISENET</Link>
          <div className="text-xs text-gray-500">Frontend</div>
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
        <header className="h-14 border-b bg-white flex items-center justify-between px-4">
          <div className="font-medium">WISENET Console</div>
          <div className="text-sm text-gray-500">v0.1</div>
        </header>
        <main className="p-4">
		<div className="flex justify-end mb-3">
			{authed ? (
				<button onClick={doLogout} className="text-sm-border rounded px-3 py-1 bg-white hover:bg-gray-50">
					Logout
				</button>
			) : (
				<Link to="/login" className="text-sm border rounded px-3 py-1 bg-white hover:bg-gray-50">
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

