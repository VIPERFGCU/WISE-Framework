import { useEffect, useState } from "react";

export default function TopLoader() {
	const [active, setActive] = useState(false);

	useEffect(() => {
		const onStart = () => setActive(true);
		const onStop = () => setActive(false);
		window.addEventListener("app:loadstart", onStart);
		window.addEventListener("app:loadstop", onStop);
		return () => {
			window.removeEventListener("app:loadstart", onStart);
			window.removeEventListener("app:loadstop", onStop);
		};
	}, []);

	return active ? (
		<div className="fixed top-0 left-0 right-0 h-1 bg-black animate-pulse z-50"/>
	) : null;
}
