## [Unreleased]
### Added

- Nothing

### Fixed

- Nothing

### Changed

- Nothing

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
