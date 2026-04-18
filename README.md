# Plasma Remote Toolbar Backend

`plasma_bridge` is a Qt 6 service for KDE Plasma that exposes desktop state to local clients.

The current implementation is audio-first and read-only:

- list all KDE-visible output sinks
- report the current default output sink
- report per-sink volume and mute state
- serve audio snapshots over HTTP
- publish live audio updates over WebSocket

Documentation:

- [Getting Started](docs/getting-started.md)
- [Architecture](docs/architecture.md)
- [API Reference](docs/api/README.md)
