import { Routes, Route } from "react-router-dom";
import Shell from "./layouts/Shell";
import Dashboard from "./pages/Dashboard";
import Devices from "./pages/Devices";
import Streams from "./pages/Streams";
import Admin from "./pages/Admin";
import Status from "./pages/Status";
import NotFound from "./pages/NotFound";

export default function App() {
  return (
    <Routes>
      <Route element={<Shell />}>
        <Route index element={<Dashboard />} />
        <Route path="devices" element={<Devices />} />
        <Route path="streams" element={<Streams />} />
        <Route path="admin" element={<Admin />} />
        <Route path="status" element={<Status />} />
	<Route path="*" element={<NotFound />} />
      </Route>
    </Routes>
  );
}

