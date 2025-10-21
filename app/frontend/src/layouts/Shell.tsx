import { Link, NavLink, Outlet } from "react-router-dom";
import { Toaster } from "../components/Toaster";
import TopLoader from "../components/TopLoader";

export default function Shell() {
  return (
    <div className="min-h-screen flex bg-gray-50">i
      
      <TopLoader />
      <Toaster />

      {/* Sidebar */}
      <aside className="w-64 border-r bg-white">
        <div className="p-4 border-b">
          <Link to="/" className="text-lg font-semibold">WISENET</Link>
          <div className="text-xs text-gray-500">Frontend</div>
        </div>
        <nav className="p-2 space-y-1 text-sm">
          <NavLink to="/" end className={({isActive}) =>
            `block px-3 py-2 rounded ${isActive ? "bg-gray-100 font-medium" : "hover:bg-gray-50"}`
          }>Dashboard</NavLink>
          <NavLink to="/devices" className={({isActive}) =>
            `block px-3 py-2 rounded ${isActive ? "bg-gray-100 font-medium" : "hover:bg-gray-50"}`
          }>Devices</NavLink>
          <NavLink to="/streams" className={({isActive}) =>
            `block px-3 py-2 rounded ${isActive ? "bg-gray-100 font-medium" : "hover:bg-gray-50"}`
          }>Streams</NavLink>
          <NavLink to="/admin" className={({isActive}) =>
            `block px-3 py-2 rounded ${isActive ? "bg-gray-100 font-medium" : "hover:bg-gray-50"}`
          }>Admin</NavLink>
          <NavLink to="/status" className={({isActive}) =>
            `block px-3 py-2 rounded ${isActive ? "bg-gray-100 font-medium" : "hover:bg-gray-50"}`
          }>Status</NavLink>
        </nav>
      </aside>

      {/* Main */}
      <div className="flex-1 flex flex-col">
        <header className="h-14 border-b bg-white flex items-center justify-between px-4">
          <div className="font-medium">WISENET Console</div>
          <div className="text-sm text-gray-500">v0.1</div>
        </header>
        <main className="p-4">
          <Outlet />
        </main>
      </div>
    </div>
  );
}

