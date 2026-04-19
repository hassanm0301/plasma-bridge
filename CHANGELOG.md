## [Unreleased]
### Added

- HTTP endpoint to list available audio sources
- HTTP endpoint to list the default audio source
- WebSocket source-state coverage on `/ws/audio`, including source-aware patch metadata

### Fixed

- Nothing

### Changed

- Shared audio state and observer flow now track both sinks and sources
- API docs and getting-started examples now document audio source snapshots and the expanded WebSocket payload
- Automated tests now cover source HTTP snapshots and source WebSocket updates

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
