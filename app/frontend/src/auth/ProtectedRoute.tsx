import { Navigate, Outlet } from "react-router-dom";
import { isAuthed } from "../lib/auth";

export default function ProtectedRoute() {
	return isAuthed() ? <Outlet /> : <Navigate to="/login" replace />;
}

