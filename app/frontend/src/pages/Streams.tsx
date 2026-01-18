export default function Streams() {
  // Grafana iframe URL
  const grafanaUrl = "http://localhost:4000/d-solo/adszfxv/wisenet-dashboard?orgId=1&timezone=browser&tab=queries&panelId=1&__feature.dashboardSceneSolo=true&kiosk";


return (
  <div className="flex flex-col h-full space-y-4 p-4">
    <h1 className="text-2xl font-bold text-gray-800">Sensor Streams</h1>
  
    {/* Container for the Grafana Iframe */}
    <div className="w-full flex-grow border-2 border-dashed border-gray-200 rounded-xl overflow-hidden shadow-lg bg-white">
      <iframe
        src={grafanaUrl}
        width="100%"
        height="100%"
        frameBorder="0"
        title="WISE-NET Sensor Dashboard"
        className="rounded-lg"
      ></iframe>
    </div>

    <p className="text-sm text-gray-500">
      Showing last 5 minutes of telemetry data. 
    </p>
  </div>
)
}
