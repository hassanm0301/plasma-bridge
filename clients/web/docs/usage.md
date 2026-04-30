# Web Usage

Start the backend first, then run the web client.

Default endpoints:

- HTTP: `http://127.0.0.1:8080`
- WebSocket: `ws://127.0.0.1:8081/ws`

When you open the dashboard from another machine on the same network, those defaults still point to the viewer's own localhost. Use Settings to replace them with the backend machine's address, and make sure the backend was started with a matching `--allow-origin http://<web-host>:5173` or equivalent web origin.

The dashboard auto-connects on load. It opens the WebSocket state stream, sends the protocol `hello` message, stores `fullState`, and applies top-level `patch` messages for `audio` and `windowState`.

Use Settings to edit endpoints, reconnect, view connection status, and switch between light and dark mode. Endpoint and theme choices are persisted in `localStorage`.

The main dashboard shows:

- Windows in a taskbar-style strip, with the active window first
- Audio sinks with volume sliders and mute/unmute buttons
- Audio sources with read-only volume sliders and active mute/unmute buttons

Inactive taskbar windows can be clicked to request focus through `POST /control/windows/{windowId}/active`. The active window changes after the backend publishes the resulting WebSocket state update.

Sink volume writes use `POST /control/audio/sinks/{sinkId}/volume`. Sink and source mute writes use `POST /control/audio/{sinks|sources}/{id}/mute`. Source volume is display-only until the backend exposes a source volume write endpoint.
