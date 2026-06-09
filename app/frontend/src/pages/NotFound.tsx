import { Link } from "react-router-dom";

export default function NotFound() {
	return (
		<div className="h-[60vh] grid place-items-center text-center">
			<div>
				<h1 className="text-3xl front-bold">404</h1>
				<p className="text-gray-600 mt-2">Page not found.</p>
				<Link to="/" className="inline-block mt-4 px-4 py-2 rounded bg-black text-white">
					Go Home
				</Link>
			</div>
		</div>
	);
}
