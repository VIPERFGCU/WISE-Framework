# Contracts

## MQTT topics (per device_id like `bridge-esp32-001`)
- Control (retained): `devices/<id>/control` → `{"cmd":"START"|"STOP","rate_hz":<int>}`
- Status (retained):  `devices/<id>/status`  → `{"state":"online"|"offline"|"streaming"|"stopped","rate_hz":<int>,"ts":"<iso8601>"}`
- Heartbeat:          `devices/<id>/heartbeat` → `{"rssi":-57,"uptime_s":123,"fw":"x.y.z","ts":"<iso8601>"}`
- Data:               `devices/<id>/data` → `{"ts":"<iso8601>","x":<float>,"y":<float>,"z":<float>}`
- LWT: broker publishes `{"state":"offline"}` to `devices/<id>/status` on disconnect.

## HTTP (UI → backend)
- POST `/api/devices/<id>/start` `{ "rate_hz": 10 }`
- POST `/api/devices/<id>/stop`
- GET  `/api/devices`
- GET  `/api/devices/<id>/status`
- GET  `/api/preview/<id>?window_s=10`

