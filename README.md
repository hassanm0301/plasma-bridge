# Plasma Remote Toolbar

Plasma Remote Toolbar is a monorepo for a KDE Plasma backend service and client applications.

The current implementation includes the backend service, `plasma_bridge`, and a React web dashboard. The Flutter mobile app is planned but not implemented yet.

## Repository Layout

- [backend](backend/): Qt 6 C++ service, probe tools, tests, and backend-specific docs
- [clients](clients/): React web dashboard and future Flutter mobile app
- [docs](docs/): project-wide usage and architecture docs
- [specs](specs/): checked-in OpenAPI and AsyncAPI contracts
- [VERSION](VERSION): single project version for backend and clients
- [CHANGELOG.md](CHANGELOG.md): single release history for backend and clients

## Quick Links

- [Project Getting Started](docs/getting-started.md)
- [Project Architecture](docs/architecture.md)
- [Backend](backend/README.md)
- [Clients](clients/README.md)
- [API Specs](specs/README.md)

Default backend endpoints:

- HTTP API: `http://127.0.0.1:8080`
- WebSocket state stream: `ws://127.0.0.1:8081/ws`
- Hosted docs: `http://127.0.0.1:8080/docs/`

The backend currently serves audio snapshots and controls, current media-session snapshots and transport controls, and Plasma window snapshots plus activation. The React dashboard renders those as a taskbar-style windows strip, a current-media card with artwork, large transport icons, draggable progress, and elapsed/total time, plus sink/source controls.

These defaults are localhost-only. To expose the backend on your LAN, start `plasma_bridge` with a non-loopback `--host`. If a browser app on another machine should call the backend, also pass one or more `--allow-origin` values for those web origins.
