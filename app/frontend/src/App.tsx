import { Routes, Route } from "react-router-dom";
import Shell from "./layouts/Shell";
import Dashboard from "./pages/Dashboard";
import Devices from "./pages/Devices";
import Streams from "./pages/Streams";
import Admin from "./pages/Admin";
import NotFound from "./pages/NotFound";
import ProtectedRoute from "./auth/ProtectedRoute";
import Login from "./pages/Login";
import SensorProfile from "./pages/SensorProfile";

export default function App() {
  return (
    <Routes>
      <Route path="sensor/:sensorId" element={<SensorProfile />} />
      <Route element={<Shell />}>
        <Route index element={<Dashboard />} />
        <Route path="devices" element={<Devices />} />
        <Route path="streams" element={<Streams />} />
        <Route path="admin" element={<Admin />} />
	<Route path="login" element={<Login />} />
	<Route element={<ProtectedRoute />}>
		<Route path="admin" element={<Admin />} />
	</Route>
	<Route path="*" element={<NotFound />} />
      </Route>
    </Routes>
  );
}

