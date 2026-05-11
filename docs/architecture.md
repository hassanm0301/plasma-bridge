# Architecture

Plasma Bridge is organized as a monorepo with one implemented backend plus web and mobile clients.

## Monorepo Shape

- `backend/`: Qt 6 C++ service for KDE Plasma, plus backend tests, probe tools, and backend-specific docs
- `clients/web/`: React web client
- `clients/app/`: Flutter mobile app, Android-first
- `specs/`: OpenAPI and AsyncAPI contracts shared by backend and clients
- `docs/`: project-wide documentation
- `VERSION` and `CHANGELOG.md`: the single project version and release history for backend and clients

## Current Runtime

The implemented runtime is `plasma_bridge`, a standalone service that runs inside a KDE Plasma user session. It reads local audio, media-session, and window state, exposes HTTP snapshot and control endpoints including media transport and seek control plus window activation, and publishes live updates over a WebSocket state stream.

The backend binds to localhost by default:

- HTTP on `127.0.0.1:8080`
- WebSocket on `ws://127.0.0.1:8081/ws`

Operators can expose the service on a LAN by starting `plasma_bridge` with a non-loopback `--host`. Browser-based clients on other machines also need an explicit `--allow-origin` entry because the backend only grants CORS access to loopback browser origins by default.

## Contract Boundary

The checked-in specs under `specs/` are the contract between the backend and clients. Backend changes that alter HTTP or WebSocket behavior should update those specs in the same change.

## Web Client

The web client is a Vite, React, and TypeScript app. It auto-connects to the backend WebSocket state stream, renders live window, media, and audio state, and uses HTTP control endpoints for window activation, media transport and seek, and sink volume plus sink/source mute writes. Endpoint and theme settings live in a Settings popup. The web client uses the repository-wide version and changelog instead of client-specific release metadata.

## Mobile Client

The mobile client is a Flutter app with Android as the first supported target. It implements the same checked-in HTTP and WebSocket contracts as the web client, but uses a compact Breeze-inspired dashboard layout and a first-run endpoint setup flow instead of defaulting to localhost. The UI is optimized for landscape tablet use: connection status stays in the app bar, windows and current media occupy full-width sections, and audio controls are denser than the web layout. Theme and endpoint settings are persisted locally, and the app reconnects when settings change or the app returns to the foreground.

## Documentation Boundary

Project-wide docs stay in this folder. Backend-only development and runtime details live in `backend/docs/`. Client-specific setup and development notes live under the relevant client folder.
