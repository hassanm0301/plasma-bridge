## [0.3.1] - 2026-04-27
### Added

- CORS headers for loopback web-client origins so browser clients can read backend HTTP endpoints during local development
- HTTP `OPTIONS` preflight handling for backend endpoints

### Fixed

- Browser requests from local Vite dev-server origins are no longer blocked when reading `/docs/openapi.yaml`

### Changed

- Nothing

### Removed

- Nothing

## [0.3.0] - 2026-04-27
### Added

- Snapshot-only `window_probe` CLI with `setup`, `status`, `teardown`, `list`, and `active` commands backed by a KWin script helper
- HTTP endpoints to list Plasma windows and read the active window
- WebSocket window-state coverage on the unified `/ws` state stream

### Fixed

- Nothing

### Changed

- HTTP responses now use a consistent `{ "payload": ..., "error": ... }` envelope across snapshot and control endpoints
- WebSocket messages now use `{ "type": "...", "payload": ..., "error": ... }`, and the state protocol version is bumped to `2`
- `plasma_bridge` now reports one WebSocket listener, `ws://127.0.0.1:8081/ws`, instead of separate audio and window WebSocket endpoints
- HTTP control failures now return structured `error.details` instead of success-shaped bodies with `status`
- Shared common types and hermetic test coverage now include window snapshot formatting and the new window probe runner
- `window_probe` and the long-running service now share the KWin script helper backend for window snapshots

### Removed

- Nothing

## [0.2.0] - 2026-04-20
### Added

- HTTP endpoint to list available audio sources
- HTTP endpoint to list the default audio source
- HTTP endpoints to set the default sink and default source
- HTTP endpoints to mute and unmute sinks and sources
- WebSocket source-state coverage on `/ws/audio`, including source-aware patch metadata

### Fixed

- Nothing

### Changed

- Shared audio state and observer flow now track both sinks and sources
- API docs and getting-started examples now document audio source snapshots plus sink/source default and mute controls
- Automated tests now cover source HTTP snapshots, sink/source control endpoints, and source WebSocket updates

### Removed

- Nothing

## [0.1.0] - 2025-04-19
### Added

- Web server to expose endpoints
- HTTP and WS endpoint to list available audio sinks
- HTTP and WS endpoint to list default audio sink
- Swagger and AsyncAPI docs

### Fixed

- Nothing

### Changed

- Nothing

### Removed

- Nothing
