# App Overview

This folder contains the backend service implementation for WISENET.

## What This Section Owns

- API entrypoint and app lifecycle
- Dependency wiring and shared config
- MQTT integration hooks
- Domain services for device and time-series operations

## High-Level Flow

1. The API boots in [app/main.py](main.py).
2. HTTP endpoints are registered from [app/api/v1](api/v1).
3. Incoming telemetry is validated with schemas in [app/schemas](schemas).
4. Data is written to InfluxDB through [app/services/influx.py](services/influx.py).
5. Device status and control flows through MQTT via [app/mqtt.py](mqtt.py).
6. Live updates are broadcast to the frontend over the WebSocket endpoint at `/api/v1/stream`.

## Key Directories

- [app/api](api): HTTP API routes
- [app/core](core): config and security foundations
- [app/services](services): business and data logic
- [app/schemas](schemas): data models and validation

## Important Files

| File | Why It Matters |
| --- | --- |
| [app/main.py](main.py) | App startup, middleware, router wiring, WebSocket stream |
| [app/deps.py](deps.py) | Auth dependencies, API key/JWT guards, shared providers |
| [app/core/config.py](core/config.py) | Environment-driven settings |
| [app/services/influx.py](services/influx.py) | Read/write logic for time-series and heartbeat data |
| [app/services/devices.py](services/devices.py) | Device registry and status helpers |

## Environment Variables Used Most Often

- `INFLUX_URL`, `INFLUX_ORG`, `INFLUX_BUCKET`, `INFLUX_TOKEN`
- `API_KEY`
- `ADMIN_USERNAME`, `ADMIN_PASSWORD`
- `JWT_SECRET`
- `CORS_ORIGINS`
- `MQTT_HOST`, `MQTT_PORT`

## Local Run

Use Docker (recommended):

```bash
cp .env.example .env
docker compose up --build
```

Run backend only (without Docker):

```bash
python -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
uvicorn app.main:app --reload --host 0.0.0.0 --port 8000
```

Open API docs:

- Swagger UI: `http://localhost:8000/docs`
- OpenAPI JSON: `http://localhost:8000/openapi.json`

## Quick Sanity Checks

```bash
curl http://localhost:8000/health
curl http://localhost:8000/health/version
```

## Common Pitfalls

- API returns but no query data: verify Influx credentials in `.env`.
- Device start/stop returns 502: MQTT broker may be unreachable.
- Browser auth errors: check `CORS_ORIGINS` and frontend API base URL.

## Related Docs

- [app/api/README.md](api/README.md)
- [docs/README.md](../docs/README.md)
