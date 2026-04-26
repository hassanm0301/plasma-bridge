# Plasma Bridge - Server
`plasma_bridge` is a Qt 6 service for KDE Plasma that exposes desktop state and local audio controls to local clients.

The current implementation exposes local audio and window state:

- list all KDE-visible output sinks
- list all KDE-visible input sources
- list Plasma windows through the KWin script helper backend
- report the active Plasma window
- report the current default output sink
- report the current default input source
- report per-device volume and mute state
- serve audio snapshots over HTTP
- serve window snapshots over HTTP
- set sink and source defaults over HTTP
- set sink and source mute state over HTTP
- set, increment, and decrement sink volume over HTTP
- publish live sink, source, and window updates over WebSocket

Optional snapshot probe tools are also available for local inspection:

- `audio_probe`
- `audio_control_probe`
- `window_probe` for shared KWin-script-backed window listing, active-window lookup, and backend setup/status

Documentation:

- [Getting Started](docs/getting-started.md)
- [Architecture](docs/architecture.md)
- [Development Guide](docs/development.md)
- [API Reference](docs/api/README.md)

Default local endpoints:

- HTTP API: `http://127.0.0.1:8080`
- WebSocket state stream: `ws://127.0.0.1:8081/ws`
- Hosted docs: `http://127.0.0.1:8080/docs/`

Automated tests use Qt Test and CTest. See [Getting Started](docs/getting-started.md) for how to build and run the unit and feature suites.
