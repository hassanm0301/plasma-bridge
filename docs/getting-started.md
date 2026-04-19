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

The optional audio probe can be built from the same tree:

```bash
cmake --build build --target audio_probe
```

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
curl http://127.0.0.1:8080/snapshot/audio/sinks
curl http://127.0.0.1:8080/snapshot/audio/default-sink
```

Hosted docs and runtime-served specs:

- `http://127.0.0.1:8080/docs/`
- `http://127.0.0.1:8080/docs/http`
- `http://127.0.0.1:8080/docs/ws`
- `http://127.0.0.1:8080/docs/openapi.yaml`
- `http://127.0.0.1:8080/docs/asyncapi.yaml`

For WebSocket monitoring, connect to `ws://127.0.0.1:8081/ws/audio` and send:

```json
{ "type": "hello", "protocolVersion": 1 }
```

You should receive one `fullState` message followed by `patch` messages as sink state changes.
Connections to `/` and other non-audio WebSocket paths are rejected.

Repo API specs:

- [OpenAPI](api/openapi.yaml)
- [AsyncAPI](api/asyncapi.yaml)
