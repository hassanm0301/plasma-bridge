# Plasma Bridge Backend

`plasma_bridge` is a Qt 6 service for KDE Plasma that exposes desktop state plus local audio and media controls to localhost clients by default, with optional LAN access when explicitly configured.

The current implementation exposes local audio, media, and window state:

- list all KDE-visible output sinks
- list all KDE-visible input sources
- list Plasma windows through the KWin script helper backend
- track the currently selected MPRIS media session
- report the active Plasma window
- report the current default output sink
- report the current default input source
- report current media metadata, transport capabilities, playback position, track length, app icon URL, and artwork URL when available
- report per-device volume and mute state
- serve audio snapshots over HTTP
- serve current media snapshots over HTTP
- serve window snapshots over HTTP
- set sink and source defaults over HTTP
- set sink and source mute state over HTTP
- set, increment, and decrement sink volume over HTTP
- request play, pause, play-pause, next-track, previous-track, and absolute seek over HTTP
- activate Plasma windows over HTTP
- publish live sink, source, media, and window updates over WebSocket

The current media payload reports one selected MPRIS session at a time and includes title, artists, album, playback status, capability flags, backend-relative app icon URLs, best-effort artwork URLs, current playback position, and total track length. While playback is active, the backend refreshes the selected player's position so WebSocket clients can keep progress displays in sync.

Optional snapshot probe tools are also available for local inspection:

- `audio_probe`
- `audio_control_probe`
- `media_probe`
- `window_probe` for shared KWin-script-backed window listing, active-window lookup, activation, and backend setup/status

Documentation:

- [Getting Started](docs/getting-started.md)
- [Architecture](docs/architecture.md)
- [Development Guide](docs/development.md)
- [API Specs](../specs/README.md)

Default local endpoints:

- HTTP API: `http://127.0.0.1:8080`
- WebSocket state stream: `ws://127.0.0.1:8081/ws`
- Hosted docs: `http://127.0.0.1:8080/docs/`

To allow another machine on the same network to reach the backend, start it with a non-loopback `--host`. For browser-based dashboards on other machines, also add one or more `--allow-origin` values that match the dashboard's web origin.

Automated tests use Qt Test and CTest. See [Getting Started](docs/getting-started.md) for how to build and run the unit and feature suites from the monorepo root.
