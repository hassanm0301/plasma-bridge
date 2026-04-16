# Architecture

## Runtime Shape

The backend is planned as a standalone Qt 6 service running in the user's Plasma session. It is not planned as a Plasma plugin-first project.

Core runtime choices:

- C++
- Qt 6
- Qt DBus
- QWebSocketServer
- an HTTP server compatible with Qt 6
- systemd user-service packaging later in the project

## Dependency Policy

- Plasma-specific libraries are acceptable when they provide the cleanest and most reliable integration path.
- Audio should start with `PulseAudioQt`, with `libpulse` as a fallback only if the Qt wrapper is insufficient.
- Window discovery should start with Plasma or KWin-friendly integrations suitable for Plasma 6 on Wayland.
- Media integration is deferred until after the V1 state model is working.

## Module Boundaries

The backend should be split into clear layers so desktop integrations do not leak into transport code.

- `app`: startup and bootstrap
- `core`: service lifecycle, config, logging
- `adapters`: audio and window integrations
- `state`: canonical backend state and patch generation
- `api`: WebSocket and HTTP transport surfaces
- `actions`: infrequent control operations added after read-only stability
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
  app/
  core/
  adapters/
    audio/
    windows/
  state/
  api/
  actions/
  common/

tools/
  probes/
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
- The public protocol should be backed by one canonical state model, not by raw KDE or Pulse events.
- Window terminology should stay consistent with the product choice: V1 exposes individual usable windows, not grouped applications.
- HTTP and WebSocket should share the same underlying state and action services rather than duplicating logic.

## Deferred Areas

- X11 compatibility work
- media adapters
- stronger authentication and pairing
- broader desktop integration outside KDE Plasma
