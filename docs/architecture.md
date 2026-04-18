# Architecture

## Runtime Shape

The backend is planned as a standalone Qt 6 service running in the user's Plasma session. It is not planned as a Plasma plugin-first project.

Core runtime choices:

- C++
- Qt 6
- Qt DBus
- Qt Network
- Qt WebSockets
- `KF6PulseAudioQt` for the first audio integration path
- `libpulse` only if the Qt wrapper proves insufficient
- a minimal in-repo HTTP server for the first transport slice
- `QWebSocketServer` for the first live audio monitoring slice
- a more complete HTTP surface later in the project
- systemd user-service packaging later in the project

## Dependency Policy

- Plasma-specific libraries are acceptable when they provide the cleanest and most reliable integration path.
- Audio should start with `KF6PulseAudioQt`, with `libpulse` as a fallback only if the Qt wrapper is insufficient.
- Window discovery should start with Plasma or KWin-friendly integrations suitable for Plasma 6 on Wayland.
- Media integration is deferred until after the V1 state model is working.

## Module Boundaries

The backend should be split into clear layers so desktop integrations do not leak into transport code. The first checked-in transport slice should keep the runtime narrow: audio observation, shared state, HTTP snapshots, and WebSocket live monitoring.

- `app`: startup and bootstrap code for the runtime executable
- `core`: later service lifecycle, config, and logging
- `adapters`: audio and window integrations
- `state`: canonical backend state and lookup helpers
- `api`: HTTP and WebSocket transport surfaces
- `actions`: later infrequent control operations
- `common`: shared types, ids, and error helpers

## Repository Layout

```text
docs/
  README.md
  roadmap.md
  architecture.md
  interfaces.md
  decisions/
    0001-foundation.md

src/
  adapters/
    audio/
  common/
  app/
  core/
  state/
  api/
  actions/

tools/
  probes/
    audio_probe/
  clients/

tests/
  unit/
  integration/
  fixtures/

packaging/
  systemd/
  cmake/
```

## Implementation Notes

- `tools/probes/` should exist before the real backend so feasibility work can be done without polluting the service runtime.
- The first probe should validate output-sink enumeration, selected default sink tracking, and live sink updates.
- The first checked-in runtime should expose read-only HTTP snapshot endpoints and a WebSocket live feed for audio sinks, and bind to localhost by default.
- The public protocol should be backed by one canonical state model, not by raw KDE or Pulse events.
- Window terminology should stay consistent with the product choice: V1 exposes individual usable windows, not grouped applications.
- HTTP and WebSocket should share the same underlying state and action services rather than duplicating logic.

## Deferred Areas

- X11 compatibility work
- media adapters
- stronger authentication and pairing
- broader desktop integration outside KDE Plasma
