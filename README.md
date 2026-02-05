
# FastAPI Backend (Docker, InfluxDB, Mosquitto)

A minimal backend interface for sensor ingest and querying, built with FastAPI. Ships with Docker Compose that brings up:
- **api**: FastAPI + Uvicorn
- **influxdb**: time-series database (v2)
- **mosquitto**: MQTT broker

> Grafana is intentionally omitted. We can add it later.

## Quick start

```bash
# 1) Copy .env.example to .env and fill in InfluxDB token after first boot
cp .env.example .env

# 2) Start the stack
docker compose up --build

# 3) Initialize InfluxDB at http://wise-net.io:8086 (set org, bucket, and generate a token)
#    Update the .env file with the token and (optionally) restart the stack.
#    You can also use docker exec to run influx setup non-interactively.

# 4) Test health
curl http://wise-net.io:8000/health

# 5) Send a sample ingest
curl -X POST http://wise-net.io:8000/api/v1/ingest   -H "Content-Type: application/json"   -d '{"device_id":"esp32-1","ts":"2025-09-25T12:00:00Z","x":1.23,"y":-0.4,"z":0.77}'

# 6) Query recent data (last 1h)
curl "http://wise-net.io:8000/api/v1/data?device_id=esp32-1&range=1h"
```

### Dev tips
- The `api` container watches files by default (uvicorn `--reload`) when using a bind mount. Edit code locally and it hot-reloads.
- Mosquitto is included. An MQTT consumer can be enabled later (see TODO in `main.py`).

## Project layout
```
fastapi-backend/
  ├─ api/
  │  ├─ Dockerfile
  │  ├─ requirements.txt
  │  └─ app/
  │     ├─ main.py
  │     ├─ deps.py
  │     ├─ schemas.py
  │     └─ routers/
  │        └─ data.py
  ├─ mosquitto/
  │  └─ mosquitto.conf
  ├─ .env.example
  ├─ docker-compose.yml
  └─ README.md
```
