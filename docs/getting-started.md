# Getting Started

Build and run commands currently target the backend service because the React web client and Flutter mobile app are placeholders.

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

## Clients

Client implementation is pending.

- [React web client placeholder](../clients/web/)
- [Flutter mobile app placeholder](../clients/app/)

When the clients are added, their setup and development commands should live in their respective folders.

## Specs

The checked-in API contracts live in [specs](../specs/). The backend also serves runtime-adjusted copies from `/docs/openapi.yaml` and `/docs/asyncapi.yaml` while running.
