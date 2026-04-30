# Getting Started

## Requirements

Build from the monorepo root.

- CMake 3.25 or newer
- Ninja
- Qt 6 development packages for `Core`, `DBus`, `Network`, and `WebSockets`
- `KF6PulseAudioQt` development package

## Build

```bash
cmake -S backend -B backend/build -G Ninja
cmake --build backend/build --target plasma_bridge
```

The first build downloads the pinned Swagger UI and AsyncAPI browser assets into the build tree. Later builds reuse that local cache.

The optional probe tools can be built from the same tree:

```bash
cmake --build backend/build --target audio_probe audio_control_probe window_probe
```

## Test

The repository includes automated unit tests and feature tests built with Qt Test and run through CTest.
The default test suite is hermetic:

- no live KDE Plasma session is required
- no real PulseAudio audio device changes are made
- HTTP and WebSocket feature tests bind only to localhost

Configure and build with testing enabled:

```bash
cmake -S backend -B backend/build -G Ninja -DBUILD_TESTING=ON
cmake --build backend/build
```

Run the full suite:

```bash
ctest --test-dir backend/build --output-on-failure
```

Run one test executable by name:

```bash
ctest --test-dir backend/build --output-on-failure -R test_state_websocket_server
```

Current coverage includes:

- unit tests for common audio-state serialization and formatting
- unit tests for common window-state serialization and formatting
- unit tests for volume, device-control, and window activation result formatting
- unit tests for audio state store behavior
- unit tests for the `audio_probe` and `audio_control_probe` runner helpers
- unit tests for the `window_probe` runner helper
- feature tests for the HTTP snapshot and control server
- feature tests for the WebSocket server
- CLI coverage for `plasma_bridge`, `audio_probe`, `audio_control_probe`, and `window_probe`

## Run

Start the service from a KDE Plasma user session. Window snapshots use the KWin script helper backend; audio endpoints remain available when window state is not ready.

```bash
./backend/build/src/app/plasma_bridge
```

Default addresses:

- HTTP: `http://127.0.0.1:8080`
- WebSocket: `ws://127.0.0.1:8081/ws`

Supported flags:

- `-h`, `--help`: print available command-line flags
- `--host`: bind address, default `127.0.0.1`
- `--port`: HTTP port, default `8080`
- `--ws-port`: WebSocket port, default `8081`
- `--allow-origin`: repeatable browser origin allowlist entry for CORS, default loopback-only

Example:

```bash
./backend/build/src/app/plasma_bridge --host 127.0.0.1 --port 8080 --ws-port 8081
```

LAN browser example:

```bash
./backend/build/src/app/plasma_bridge --host 0.0.0.0 --allow-origin http://192.168.x.y:5173
```

`--host` controls which network interfaces accept TCP connections. `--allow-origin` controls which browser origins receive CORS permission on HTTP responses. Non-browser clients such as `curl` only need reachability; browser dashboards need both.

On startup, the service prints one HTTP listener, one WebSocket listener, and the hosted docs URLs.

To list all available flags:

```bash
./backend/build/src/app/plasma_bridge -h
```

Optional window probe backend inspection:

```bash
./backend/build/tools/probes/window_probe/window_probe setup
./backend/build/tools/probes/window_probe/window_probe status
./backend/build/tools/probes/window_probe/window_probe --json list
./backend/build/tools/probes/window_probe/window_probe --json active
./backend/build/tools/probes/window_probe/window_probe --json activate window-editor
```

`plasma_bridge` auto-installs and enables the same KWin script helper backend on startup.
The standalone `window_probe` tool uses that shared backend too; run `setup` from a KDE Plasma session before using `window_probe list` or `active` directly.

## Verify

Check the HTTP endpoints:

```bash
SINK_ID='alsa_output.usb-default.analog-stereo'
SOURCE_ID='alsa_input.usb-default.analog-stereo'
WINDOW_ID='window-editor'

curl http://127.0.0.1:8080/snapshot/audio/sinks
curl http://127.0.0.1:8080/snapshot/audio/default-sink
curl http://127.0.0.1:8080/snapshot/audio/sources
curl http://127.0.0.1:8080/snapshot/audio/default-source
curl http://127.0.0.1:8080/snapshot/windows
curl http://127.0.0.1:8080/snapshot/windows/active
curl -X POST http://127.0.0.1:8080/control/audio/sinks/${SINK_ID}/default
curl -X POST http://127.0.0.1:8080/control/audio/sources/${SOURCE_ID}/default
curl -X POST http://127.0.0.1:8080/control/audio/sinks/${SINK_ID}/mute \
  -H 'Content-Type: application/json' \
  -d '{"muted":true}'
curl -X POST http://127.0.0.1:8080/control/audio/sources/${SOURCE_ID}/mute \
  -H 'Content-Type: application/json' \
  -d '{"muted":false}'
curl -X POST http://127.0.0.1:8080/control/audio/sinks/${SINK_ID}/volume \
  -H 'Content-Type: application/json' \
  -d '{"value":0.55}'
curl -X POST http://127.0.0.1:8080/control/audio/sinks/${SINK_ID}/volume/increment \
  -H 'Content-Type: application/json' \
  -d '{"value":0.10}'
curl -X POST http://127.0.0.1:8080/control/audio/sinks/${SINK_ID}/volume/decrement \
  -H 'Content-Type: application/json' \
  -d '{"value":0.15}'
curl -X POST http://127.0.0.1:8080/control/windows/${WINDOW_ID}/active
```

HTTP JSON responses now use a consistent envelope:

```json
{ "payload": { ... }, "error": null }
```

or, on failure:

```json
{ "payload": null, "error": { "code": "bad_request", "message": "..." } }
```

Hosted docs and runtime-served specs:

- `http://127.0.0.1:8080/docs/`
- `http://127.0.0.1:8080/docs/http`
- `http://127.0.0.1:8080/docs/ws`
- `http://127.0.0.1:8080/docs/openapi.yaml`
- `http://127.0.0.1:8080/docs/asyncapi.yaml`

When the service is exposed on the LAN, replace `127.0.0.1` with the backend machine's address. Remote browser dashboards still need their own origin listed with `--allow-origin`.

For WebSocket monitoring, connect to `ws://127.0.0.1:8081/ws` and send:

```json
{
  "type": "hello",
  "payload": { "protocolVersion": 2 },
  "error": null
}
```

Server messages use the same explicit contract:

```json
{ "type": "fullState", "payload": { ... }, "error": null }
```

You should receive one `fullState` message followed by `patch` messages as sink, source, or window state changes.
Local HTTP default, mute, volume-control, and window activation writes will converge back through the same WebSocket state stream.
The `fullState` payload includes `audio` and `windowState` once both configured stores are ready.
The `audio` object includes `sinks`, `selectedSinkId`, `sources`, and `selectedSourceId`.
The `windowState` object includes `activeWindowId`, `activeWindow`, and `windows`.
Patch payloads use one stable shape with `reason`, `sinkId`, `sourceId`, `windowId`, and `changes`; IDs unrelated to a given patch are `null`.
Connections to `/` and other unknown WebSocket paths are rejected.

Repo API specs:

- [OpenAPI](../../specs/openapi.yaml)
- [AsyncAPI](../../specs/asyncapi.yaml)
