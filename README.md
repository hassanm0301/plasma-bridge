# Plasma Bridge - Server
`plasma_bridge` is a Qt 6 service for KDE Plasma that exposes desktop state and local audio controls to local clients.

The current implementation is audio-first:

- list all KDE-visible output sinks
- list all KDE-visible input sources
- report the current default output sink
- report the current default input source
- report per-device volume and mute state
- serve audio snapshots over HTTP
- set sink and source defaults over HTTP
- set sink and source mute state over HTTP
- set, increment, and decrement sink volume over HTTP
- publish live sink and source updates over WebSocket

Documentation:

- [Getting Started](docs/getting-started.md)
- [Architecture](docs/architecture.md)
- [API Reference](docs/api/README.md)

Automated tests use Qt Test and CTest. See [Getting Started](docs/getting-started.md) for how to build and run the unit and feature suites.
