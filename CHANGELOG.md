## [Unreleased]
### Added

- `plasma_bridge` now accepts repeatable `--allow-origin` flags to grant CORS access to additional browser origins when the backend is exposed beyond localhost.
- Current MPRIS media-session snapshots on HTTP and WebSocket, including player metadata, app icon URLs, and best-effort artwork URLs.
- HTTP media transport control endpoints for play, pause, play-pause, next, previous, and absolute seek.
- `media_probe` for local current-session inspection and transport control.
- A current-media section in the React dashboard between windows and sinks/sources, including artwork or app-icon rendering plus previous, play/pause, and next controls.
- Media playback progress reporting and seek support, including draggable web timeline controls with elapsed and total track time.

### Fixed

- Configured non-loopback browser origins now receive CORS headers for backend responses and HTTP preflight requests.
- Media resume from the web dashboard now uses the toggle-style play/pause action for better MPRIS compatibility across players.

### Changed

- The web client's Vite dev and preview servers now bind on all interfaces to simplify LAN testing.
- Backend and web docs now explain localhost-only defaults, LAN exposure, and remote browser setup requirements.
- The WebSocket state protocol version is now `3` to include top-level media state and `playerId` patch metadata.
- Media state now includes playback position and seek capability, and the backend refreshes the selected player's position while it is playing.
- The dashboard media controls now use centered icon-only transport buttons with a larger visual emphasis.

### Removed

- Nothing

## [0.4.0] - 2026-04-27
### Added

- Window snapshots now include app icon URLs that clients can render.
- Inactive taskbar windows can now be clicked to request focus through the backend window activation endpoint.
- Taskbar window items now render backend-provided app icons when available.

### Fixed

- Window activation now reloads the KWin script and helper cleanly so focus requests are consumed by the current script instance.

### Changed

- Web usage docs now describe window activation from the taskbar strip.

### Removed

- Nothing

## [0.3.2] - 2026-04-27
### Added

- HTTP endpoint to activate a Plasma window by id.
- `window_probe activate <window-id>` command backed by the shared KWin script helper.
- Initial React web client scaffold with Vite and TypeScript.
- Endpoint validation helpers for HTTP and WebSocket reachability.
- Breeze-inspired light and dark themes with persisted theme selection.
- Web development and usage docs.
- Auto-connecting dashboard backed by the WebSocket state stream.
- Taskbar-style window list sorted with the active window first.
- Audio sink and source sections sorted with the selected device first.
- Sink volume control and sink/source mute controls through HTTP control endpoints.
- Settings popup for endpoints, connection status, reconnect, and theme mode.

### Fixed

- Nothing

### Changed

- API docs and backend getting-started examples now document window activation control.
- Moved endpoint fields and the light/dark theme control out of the main screen and into Settings.
- Source volume sliders are shown read-only until the backend exposes source volume writes.

### Removed

- Connection-first screen.

## [0.3.1] - 2026-04-27
### Added

- CORS headers for loopback web-client origins so browser clients can read backend HTTP endpoints during local development.
- HTTP `OPTIONS` preflight handling for backend endpoints.

### Fixed

- Browser requests from local Vite dev-server origins are no longer blocked when reading `/docs/openapi.yaml`.

### Changed

- Nothing

### Removed

- Nothing

## [0.3.0] - 2026-04-27
### Added

- Snapshot-only `window_probe` CLI with `setup`, `status`, `teardown`, `list`, and `active` commands backed by a KWin script helper.
- HTTP endpoints to list Plasma windows and read the active window.
- WebSocket window-state coverage on the unified `/ws` state stream.

### Fixed

- Nothing

### Changed

- HTTP responses now use a consistent `{ "payload": ..., "error": ... }` envelope across snapshot and control endpoints.
- WebSocket messages now use `{ "type": "...", "payload": ..., "error": ... }`, and the state protocol version is bumped to `2`.
- `plasma_bridge` now reports one WebSocket listener, `ws://127.0.0.1:8081/ws`, instead of separate audio and window WebSocket endpoints.
- HTTP control failures now return structured `error.details` instead of success-shaped bodies with `status`.
- Shared common types and hermetic test coverage now include window snapshot formatting and the new window probe runner.
- `window_probe` and the long-running service now share the KWin script helper backend for window snapshots.

### Removed

- Nothing

## [0.2.0] - 2026-04-20
### Added

- HTTP endpoint to list available audio sources.
- HTTP endpoint to list the default audio source.
- HTTP endpoints to set the default sink and default source.
- HTTP endpoints to mute and unmute sinks and sources.
- WebSocket source-state coverage on `/ws/audio`, including source-aware patch metadata.

### Fixed

- Nothing

### Changed

- Shared audio state and observer flow now track both sinks and sources.
- API docs and getting-started examples now document audio source snapshots plus sink/source default and mute controls.
- Automated tests now cover source HTTP snapshots, sink/source control endpoints, and source WebSocket updates.

### Removed

- Nothing

## [0.1.0] - 2025-04-19
### Added

- Web server to expose endpoints.
- HTTP and WS endpoint to list available audio sinks.
- HTTP and WS endpoint to list default audio sink.
- Swagger and AsyncAPI docs.

### Fixed

- Nothing

### Changed

- Nothing

### Removed

- Nothing
