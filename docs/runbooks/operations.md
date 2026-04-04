# Operations Runbook

This runbook is the first-response guide for WISENET runtime issues.

## Service Check Order

1. API health endpoint
2. MQTT broker availability
3. InfluxDB health and write access
4. Frontend connectivity to API

## Quick Validation Commands

```bash
docker compose ps
curl http://localhost:8000/health
curl http://localhost:8000/health/version
curl http://localhost:8086/health
```

## First 10 Minutes Checklist

1. Confirm containers are running and healthy.
2. Confirm API health endpoint responds.
3. Confirm InfluxDB health endpoint responds.
4. Confirm MQTT broker container is up.
5. Confirm frontend can reach API.

## Service Restart Order

When stack components are unstable, restart in this order:

1. `mosquitto`
2. `influxdb`
3. `api`
4. `frontend`

Example:

```bash
docker compose restart mosquitto influxdb api frontend
```

## Common Incident Patterns

- Ingest endpoint responds but no data in queries: check Influx token and bucket config.
- Device control appears delayed: inspect MQTT broker load and topic mapping.
- Frontend empty states: verify API base URL and auth token behavior.

## Useful Logs

```bash
docker compose logs -f api
docker compose logs -f mosquitto
docker compose logs -f influxdb
```

## Escalation

- Primary: Backend Lead
- Secondary: Platform/DevOps

## Related Docs

- [README.md](../../README.md)
- [docs/README.md](../README.md)
- [docs/templates/runbook-template.md](../templates/runbook-template.md)
