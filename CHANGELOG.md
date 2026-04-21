## [Unreleased]
### Added

- Snapshot-only `window_probe` CLI with `list` and `active` commands for KDE Plasma Wayland window inspection

### Fixed

- Nothing

### Changed

- HTTP responses now use a consistent `{ "payload": ..., "error": ... }` envelope across snapshot and control endpoints
- WebSocket messages now use `{ "type": "...", "payload": ..., "error": ... }`, and the audio protocol version is bumped to `2`
- HTTP control failures now return structured `error.details` instead of success-shaped bodies with `status`
- Shared common types and hermetic test coverage now include window snapshot formatting and the new window probe runner

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
