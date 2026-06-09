# Frontend Overview

This folder contains the WISENET web UI built with React, TypeScript, and Vite.

## What This Section Owns

- Authentication and protected routes
- Dashboard and stream visualizations
- Device and sensor profile experiences
- Frontend API client and service integrations

## Key Structure

- [app/frontend/src/pages](src/pages): page-level features and routes
- [app/frontend/src/components](src/components): reusable UI components
- [app/frontend/src/services](src/services): frontend-facing service calls
- [app/frontend/src/api](src/api): API client setup
- [app/frontend/src/lib](src/lib): shared utilities and normalization

## Route Map

- `/` dashboard
- `/devices` device table and status
- `/streams` live stream charts
- `/status` system status page
- `/sensor/:sensorId` sensor profile page
- `/login` sign-in page
- `/admin` admin page (protected)

## API Integration Basics

- API client is configured in [app/frontend/src/api/client.ts](src/api/client.ts).
- Base URL comes from `VITE_API_BASE_URL` and defaults to `/`.
- JWT token is stored in local storage and attached as `Authorization: Bearer <token>`.
- On `401`, token is cleared and the app redirects to `/login`.

## Environment Variables

- `VITE_API_BASE_URL` example: `http://localhost:8000` for local API dev
- `VITE_DEV_FAKE_AUTH=true` can be used during UI-only development

## Local Run

```bash
cd app/frontend
npm install
npm run dev
```

Open: `http://localhost:5173`

## Build

```bash
cd app/frontend
npm run build
```

## Quick Smoke Test

1. Open `/login` and sign in.
2. Confirm redirect to `/admin`.
3. Go to `/devices` and verify list data loads.
4. Go to `/streams` and verify charts render when data exists.

## Documentation Rules

- New page or route: update this README with ownership and data dependencies.
- New API integration: document service file and expected response shape.
- UI behavior changes: add before/after notes in the related feature docs.

## Related Docs

- [docs/README.md](../../docs/README.md)
- [README.md](../../README.md)
