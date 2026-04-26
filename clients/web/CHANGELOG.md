## [Unreleased]
### Added

- Initial React web client scaffold with Vite, TypeScript, and independent web versioning
- Endpoint validation helpers for HTTP and WebSocket reachability
- Breeze-inspired light and dark themes with persisted theme selection
- Web development and usage docs
- Auto-connecting dashboard backed by the WebSocket state stream
- Taskbar-style window list sorted with the active window first
- Audio sink and source sections sorted with the selected device first
- Sink volume control and sink/source mute controls through HTTP control endpoints
- Settings popup for endpoints, connection status, reconnect, and theme mode

### Fixed

- Nothing

### Changed

- Moved endpoint fields and the light/dark theme control out of the main screen and into Settings
- Source volume sliders are shown read-only until the backend exposes source volume writes

### Removed

- Connection-first screen
