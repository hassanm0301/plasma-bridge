# Web Usage

Start the backend first, then run the web client.

Default endpoints:

- HTTP: `http://127.0.0.1:8080`
- WebSocket: `ws://127.0.0.1:8081/ws`

When you open the dashboard from another machine on the same network, those defaults still point to the viewer's own localhost. Use Settings to replace them with the backend machine's address, and make sure the backend was started with a matching `--allow-origin http://<web-host>:5173` or equivalent web origin.

The dashboard auto-connects on load. It opens the WebSocket state stream, sends the protocol `hello` message with version `3`, stores `fullState`, and applies top-level `patch` messages for `audio`, `media`, and `windowState`.

Use Settings to edit endpoints, reconnect, view connection status, and switch between light and dark mode. Endpoint and theme choices are persisted in `localStorage`.

The main dashboard shows:

- Windows in a taskbar-style strip, with the active window first
- The current media session between windows and audio, with artwork or app icon when available
- Audio sinks with volume sliders and mute/unmute buttons
- Audio sources with read-only volume sliders and active mute/unmute buttons

Inactive taskbar windows can be clicked to request focus through `POST /control/windows/{windowId}/active`. The active window changes after the backend publishes the resulting WebSocket state update.

The media section follows the backend-selected current player only. It shows title, artists, album, playback status, and source identity, then exposes previous, play/pause, and next controls through centered icon-only buttons. The center button uses the backend `play-pause` toggle route for reliable resume and pause behavior across players.

When position and duration are available, the media card also shows elapsed time, a draggable seek bar, and total track length. The seek slider advances smoothly while playback is active, resyncs from backend media patches, and commits drag updates through `POST /control/media/current/seek`. If the backend cannot seek or timing data is missing, the slider stays disabled.

Sink volume writes use `POST /control/audio/sinks/{sinkId}/volume`. Sink and source mute writes use `POST /control/audio/{sinks|sources}/{id}/mute`. Source volume is display-only until the backend exposes a source volume write endpoint.
