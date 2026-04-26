# Getting Started

Build and run commands currently target the backend service and React web client. The Flutter mobile app is still a placeholder.

## Backend

From the monorepo root:

```bash
cmake -S backend -B backend/build -G Ninja
cmake --build backend/build --target plasma_bridge
```

Run the service from a KDE Plasma user session:

```bash
./backend/build/src/app/plasma_bridge
```

Default backend endpoints:

- HTTP: `http://127.0.0.1:8080`
- WebSocket: `ws://127.0.0.1:8081/ws`
- Hosted docs: `http://127.0.0.1:8080/docs/`

For full backend setup, verification, and test instructions, see [backend/docs/getting-started.md](../backend/docs/getting-started.md).

## Web Client

From the monorepo root:

```bash
cd clients/web
npm install
npm run dev
```

Open the Vite dev server URL and use the default backend endpoints:

- HTTP: `http://127.0.0.1:8080`
- WebSocket: `ws://127.0.0.1:8081/ws`

See [clients/web](../clients/web/) for web-specific docs.

## Mobile App

The Flutter mobile app is pending. Its setup and development commands should live in [clients/app](../clients/app/) when the project is generated.

## Specs

The checked-in API contracts live in [specs](../specs/). The backend also serves runtime-adjusted copies from `/docs/openapi.yaml` and `/docs/asyncapi.yaml` while running.
