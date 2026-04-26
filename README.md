# Plasma Remote Toolbar

Plasma Remote Toolbar is a monorepo for a KDE Plasma backend service and future client applications.

The current implementation is the backend service, `plasma_bridge`, which exposes local audio and window state over HTTP and WebSocket APIs. The React web client and Flutter mobile app are planned but not implemented yet.

## Repository Layout

- [backend](backend/): Qt 6 C++ service, probe tools, tests, and backend-specific docs
- [clients](clients/): future React web and Flutter mobile clients
- [docs](docs/): project-wide usage and architecture docs
- [specs](specs/): checked-in OpenAPI and AsyncAPI contracts

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
