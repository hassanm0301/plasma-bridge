# Web Client

React web client for Plasma Remote Toolbar.

## Version

The web client uses the repository-wide project version from [../../VERSION](../../VERSION).
Release notes live in [../../CHANGELOG.md](../../CHANGELOG.md).

## Getting Started

```bash
npm install
npm run dev
```

The app opens a dashboard and auto-connects to the backend with:

- HTTP base URL, default `http://127.0.0.1:8080`
- WebSocket URL, default `ws://127.0.0.1:8081/ws`

Use Settings to change endpoints, reconnect, or switch between light and dark mode. The dashboard shows live windows, audio sinks, and audio sources. Inactive windows in the taskbar strip can be clicked to request focus. Sink volume plus sink/source mute controls use HTTP control endpoints; source volume is read-only until the backend supports writes for sources.

## Docs

- [Usage](docs/usage.md)
- [Development](docs/development.md)
