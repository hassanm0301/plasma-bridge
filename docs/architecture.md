# Architecture

Plasma Remote Toolbar is organized as a monorepo with one implemented backend, one initial web client, and one planned mobile client.

## Monorepo Shape

- `backend/`: Qt 6 C++ service for KDE Plasma, plus backend tests, probe tools, and backend-specific docs
- `clients/web/`: React web client
- `clients/app/`: planned Flutter mobile app
- `specs/`: OpenAPI and AsyncAPI contracts shared by backend and clients
- `docs/`: project-wide documentation

## Current Runtime

The implemented runtime is `plasma_bridge`, a standalone service that runs inside a KDE Plasma user session. It reads local audio and window state, exposes HTTP snapshot and control endpoints, and publishes live updates over a WebSocket state stream.

The backend binds to localhost by default:

- HTTP on `127.0.0.1:8080`
- WebSocket on `ws://127.0.0.1:8081/ws`

## Contract Boundary

The checked-in specs under `specs/` are the contract between the backend and clients. Backend changes that alter HTTP or WebSocket behavior should update those specs in the same change.

## Web Client

The web client is a Vite, React, and TypeScript app. Its first screen validates that the browser can reach the backend HTTP docs spec and the WebSocket state stream. It keeps its own version and changelog under `clients/web/`.

## Documentation Boundary

Project-wide docs stay in this folder. Backend-only development and runtime details live in `backend/docs/`. Client-specific setup and development notes live under the relevant client folder.
