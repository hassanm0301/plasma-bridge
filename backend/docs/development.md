# Development Guide

This guide is for contributors who want to understand where behavior lives before changing it.

## Source Map

- `backend/src/app/`: application entry point, CLI flags, service wiring, and startup output
- `backend/src/api/`: HTTP server, WebSocket server, JSON envelope helpers, hosted docs assets
- `backend/src/adapters/audio/`: PulseAudioQt observers and controllers
- `backend/src/adapters/media/`: MPRIS observer and media controller
- `backend/src/adapters/window/`: KWin script helper backend and window observer interface
- `backend/src/state/`: in-memory stores that hold the canonical audio, media, and window snapshots
- `backend/src/common/`: shared state models, JSON serialization, formatting, and generated build config
- `backend/tools/probes/`: standalone inspection and control tools used for local debugging
- `backend/tests/`: Qt Test unit and feature coverage plus fake probe helpers
- `specs/`: checked-in OpenAPI and AsyncAPI source files

## Runtime Shape

The app wires adapters into state stores, then serves those stores through HTTP and WebSocket transports.
HTTP reads, local audio controls, media transport and seek controls, and window activation controls are handled by `SnapshotHttpServer`.
Live updates are handled by `StateWebSocketServer` on one endpoint, `/ws`.

The WebSocket flow is:

1. Client connects to `ws://127.0.0.1:8081/ws`.
2. Client sends a `hello` envelope with the supported `protocolVersion`.
3. Server sends one `fullState` envelope after configured stores are ready.
4. Server sends `patch` envelopes whenever audio, media, or window state changes.

All public HTTP and WebSocket messages use explicit envelopes. Keep that convention for new API behavior:

```json
{ "payload": { "...": "..." }, "error": null }
```

```json
{ "type": "patch", "payload": { "...": "..." }, "error": null }
```

## Making Changes

Prefer changing the shared state model first when adding a field. Then update serialization, stores, adapters, API responses, tests, and docs in that order.

For HTTP changes, update `specs/openapi.yaml` with the same response envelopes that the server returns.
For WebSocket changes, update `specs/asyncapi.yaml` and keep the checked-in placeholders such as `@PLASMA_BRIDGE_WS_PATH@`; the build rewrites them for the hosted runtime spec.

The hosted docs pages under `backend/src/api/docs_assets/` are intentionally small static shells. The checked-in OpenAPI and AsyncAPI files remain the source of truth.

## Testing

Use CTest from the build directory:

```bash
ctest --test-dir backend/build --output-on-failure
```

Focused tests are usually faster while iterating:

```bash
ctest --test-dir backend/build --output-on-failure -R test_state_websocket_server
ctest --test-dir backend/build --output-on-failure -R test_snapshot_http_server
ctest --test-dir backend/build --output-on-failure -R test_cli_binaries
```

The default automated suite is hermetic: it does not require a live KDE Plasma session and does not mutate real audio devices.

## Local Runtime Checks

From a Plasma user session, build and run:

```bash
cmake --build backend/build --target plasma_bridge
./backend/build/src/app/plasma_bridge
```

Use the hosted docs at `http://127.0.0.1:8080/docs/` for manual API inspection. Use `media_probe --json current` and `media_probe --json play-pause` when diagnosing MPRIS media behavior, and `window_probe setup`, `window_probe status`, and `window_probe activate <window-id>` when diagnosing the shared KWin script helper backend directly.
