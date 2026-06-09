# API Overview

This folder contains versioned API routes and endpoint handlers.

## Routing Structure

- [app/api/v1](v1): version 1 endpoints currently used by the app

## Typical Request Path

1. Client calls a route in [app/api/v1](v1).
2. Request payload is validated by schemas.
3. Handler calls service-layer logic.
4. Response is normalized and returned.

## Endpoint Map

### Health

- `GET /health`
- `GET /health/version`
- `GET /health/secure` (JWT viewer role)
- `GET /health/secure-key` (API key)
- `GET /health/secure-either` (JWT or API key)

### Auth

- `POST /api/v1/auth/login`

### Ingest and Query

- `POST /api/v1/ingest/accel`
- `GET /api/v1/query/accel?device_id=<id>&range=15m`

### Device Management

- `POST /api/v1/devices/register`
- `GET /api/v1/devices`
- `GET /api/v1/devices/{device_id}`
- `GET /api/v1/devices/{device_id}/status`

### Device Control (MQTT-backed)

- `POST /api/v1/devices/{device_id}/start`
- `POST /api/v1/devices/{device_id}/stop`
- `POST /api/v1/devices/{device_id}/rate`
- `POST /api/v1/devices/{device_id}/batch`

### Preview

- `GET /api/preview/{device_id}?window_s=60`
- `GET /api/preview/{device_id}/csv?window_s=60`

### Streaming

- `WS /api/v1/stream`

## Authentication Model

- API key and JWT are both supported.
- Some endpoints are public, some require `viewer`, `device`, or `admin` role.
- Frontend login uses `POST /api/v1/auth/login` and sends `Authorization: Bearer <token>` on later requests.

## Example Requests

```bash
curl http://localhost:8000/health
```

```bash
curl -X POST http://localhost:8000/api/v1/auth/login \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"admin"}'
```

```bash
curl -X POST http://localhost:8000/api/v1/ingest/accel \
  -H "Content-Type: application/json" \
  -H "x-api-key: devkey" \
  -d '{"device_id":"dev-sensor-001","x":0.1,"y":-0.2,"z":0.3}'
```

## Documentation Rules

- Any new endpoint must include:
  - Purpose and expected caller
  - Example request/response
  - Auth requirements
  - Error behavior

## Note

There is an older route file at [app/api/v1/data.py](v1/data.py). Current app wiring in [app/main.py](../main.py) uses `ingest.py` and `query.py` for telemetry ingestion/query paths.

## Related Docs

- [app/README.md](../README.md)
- [docs/README.md](../../docs/README.md)
