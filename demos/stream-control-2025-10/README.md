# Stream Control Demo (2025-10)

End-to-end demo for start/stop stream control using ESP32, MQTT, FastAPI, and InfluxDB.

## What This Demo Covers

- Device command/control path
- Telemetry ingest path
- Dashboard validation flow

## Key Artifacts

- [contracts.md](contracts.md): command and payload contracts
- [demo_script.md](demo_script.md): presenter script and timing
- [compose/docker-compose.yml](compose/docker-compose.yml): local demo stack
- [simulators/fake_device.py](simulators/fake_device.py): device simulator

## Run

### 1) Start demo infrastructure

```bash
cd demos/stream-control-2025-10/compose
docker compose up -d
```

### 2) Start simulator (if not already included in compose)

```bash
cd demos/stream-control-2025-10/simulators
pip install -r requirements.txt
python fake_device.py
```

### 3) Validate basic behavior

- Open backend health endpoint and confirm `ok`.
- Start a device stream with control endpoint.
- Confirm telemetry points appear in preview/query endpoints.

### 4) Presenter flow

Use [demo_script.md](demo_script.md) as your speaking guide.

## Troubleshooting

- If control commands are not received, verify broker config in [compose/mosquitto.conf](compose/mosquitto.conf).
- If telemetry is missing, verify simulator dependencies in [simulators/requirements.txt](simulators/requirements.txt).
- If API is up but no time-series data appears, verify Influx settings and token.
- If dashboard is blank, validate frontend API base URL and CORS settings.

## Related Docs

- [demos/README.md](../README.md)
- [docs/README.md](../../docs/README.md)
