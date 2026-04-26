# Development Guide

This guide is for contributors who want to understand where behavior lives before changing it.

## Source Map

- `src/app/`: application entry point, CLI flags, service wiring, and startup output
- `src/api/`: HTTP server, WebSocket server, JSON envelope helpers, hosted docs assets
- `src/adapters/audio/`: PulseAudioQt observers and controllers
- `src/adapters/window/`: KWin script helper backend and window observer interface
- `src/state/`: in-memory stores that hold the canonical audio and window snapshots
- `src/common/`: shared state models, JSON serialization, formatting, and generated build config
- `tools/probes/`: standalone inspection and control tools used for local debugging
- `tests/`: Qt Test unit and feature coverage plus fake probe helpers
- `docs/api/`: checked-in OpenAPI and AsyncAPI source files

## Runtime Shape

The app wires adapters into state stores, then serves those stores through HTTP and WebSocket transports.
HTTP reads and local audio controls are handled by `SnapshotHttpServer`.
Live updates are handled by `StateWebSocketServer` on one endpoint, `/ws`.

The WebSocket flow is:

1. Client connects to `ws://127.0.0.1:8081/ws`.
2. Client sends a `hello` envelope with the supported `protocolVersion`.
3. Server sends one `fullState` envelope after configured stores are ready.
4. Server sends `patch` envelopes whenever audio or window state changes.

All public HTTP and WebSocket messages use explicit envelopes. Keep that convention for new API behavior:

```json
{ "payload": { "...": "..." }, "error": null }
```

```json
{ "type": "patch", "payload": { "...": "..." }, "error": null }
```

## Making Changes

Prefer changing the shared state model first when adding a field. Then update serialization, stores, adapters, API responses, tests, and docs in that order.

For HTTP changes, update `docs/api/openapi.yaml` with the same response envelopes that the server returns.
For WebSocket changes, update `docs/api/asyncapi.yaml` and keep the checked-in placeholders such as `@PLASMA_BRIDGE_WS_PATH@`; the build rewrites them for the hosted runtime spec.

The hosted docs pages under `src/api/docs_assets/` are intentionally small static shells. The checked-in OpenAPI and AsyncAPI files remain the source of truth.

## Testing

Use CTest from the build directory:

```bash
ctest --test-dir build --output-on-failure
```

Focused tests are usually faster while iterating:

```bash
ctest --test-dir build --output-on-failure -R test_state_websocket_server
ctest --test-dir build --output-on-failure -R test_snapshot_http_server
ctest --test-dir build --output-on-failure -R test_cli_binaries
```

The default automated suite is hermetic: it does not require a live KDE Plasma session and does not mutate real audio devices.

## Local Runtime Checks

From a Plasma user session, build and run:

```bash
cmake --build build --target plasma_bridge
./build/src/app/plasma_bridge
```

Use the hosted docs at `http://127.0.0.1:8080/docs/` for manual API inspection. Use `window_probe setup` and `window_probe status` when diagnosing the shared KWin script helper backend directly.
