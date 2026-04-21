# Getting Started

## Requirements

Build from the repository root.

- CMake 3.25 or newer
- Ninja
- Qt 6 development packages for `Core`, `Network`, and `WebSockets`
- `KF6PulseAudioQt` development package

## Build

```bash
cmake -S . -B build -G Ninja
cmake --build build --target plasma_bridge
```

The first build downloads the pinned Swagger UI and AsyncAPI browser assets into the build tree. Later builds reuse that local cache.

The optional probe tools can be built from the same tree:

```bash
cmake --build build --target audio_probe audio_control_probe
```

## Test

The repository includes automated unit tests and feature tests built with Qt Test and run through CTest.
The default test suite is hermetic:

- no live KDE Plasma session is required
- no real PulseAudio audio device changes are made
- HTTP and WebSocket feature tests bind only to localhost

Configure and build with testing enabled:

```bash
cmake -S . -B build -G Ninja -DBUILD_TESTING=ON
cmake --build build
```

Run the full suite:

```bash
ctest --test-dir build --output-on-failure
```

Run one test executable by name:

```bash
ctest --test-dir build --output-on-failure -R test_audio_websocket_server
```

Current coverage includes:

- unit tests for common audio-state serialization and formatting
- unit tests for volume and device-control result formatting
- unit tests for audio state store behavior
- unit tests for the `audio_probe` and `audio_control_probe` runner helpers
- feature tests for the HTTP snapshot and control server
- feature tests for the WebSocket server
- CLI coverage for `plasma_bridge`, `audio_probe`, and `audio_control_probe`

## Run

Start the service from a KDE Plasma user session:

```bash
./build/src/app/plasma_bridge
```

Default addresses:

- HTTP: `http://127.0.0.1:8080`
- WebSocket: `ws://127.0.0.1:8081/ws/audio`

Supported flags:

- `-h`, `--help`: print available command-line flags
- `--host`: bind address, default `127.0.0.1`
- `--port`: HTTP port, default `8080`
- `--ws-port`: WebSocket port, default `8081`

Example:

```bash
./build/src/app/plasma_bridge --host 127.0.0.1 --port 8080 --ws-port 8081
```

To list all available flags:

```bash
./build/src/app/plasma_bridge -h
```

## Verify

Check the HTTP endpoints:

```bash
SINK_ID='alsa_output.usb-default.analog-stereo'
SOURCE_ID='alsa_input.usb-default.analog-stereo'

curl http://127.0.0.1:8080/snapshot/audio/sinks
curl http://127.0.0.1:8080/snapshot/audio/default-sink
curl http://127.0.0.1:8080/snapshot/audio/sources
curl http://127.0.0.1:8080/snapshot/audio/default-source
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

For WebSocket monitoring, connect to `ws://127.0.0.1:8081/ws/audio` and send:

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

You should receive one `fullState` message followed by `patch` messages as sink or source state changes.
Local HTTP default, mute, and volume-control writes will converge back through the same WebSocket state stream.
The `state.audio` object now includes `sinks`, `selectedSinkId`, `sources`, and `selectedSourceId`.
Connections to `/` and other non-audio WebSocket paths are rejected.

Repo API specs:

- [OpenAPI](api/openapi.yaml)
- [AsyncAPI](api/asyncapi.yaml)
