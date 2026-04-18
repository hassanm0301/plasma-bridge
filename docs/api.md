# API Reference

## Overview

The current public API is read-only and focused on audio sink state.

Default addresses:

- HTTP: `http://127.0.0.1:8080`
- WebSocket: `ws://127.0.0.1:8081`

## Audio Model

Both transports expose the same audio data:

```json
{
  "audio": {
    "sinks": [
      {
        "id": "alsa_output.usb-example.analog-stereo",
        "label": "USB Audio",
        "volume": 0.75,
        "muted": false,
        "available": true,
        "isDefault": true,
        "isVirtual": false,
        "backendApi": "alsa"
      }
    ],
    "selectedSinkId": "alsa_output.usb-example.analog-stereo"
  }
}
```

Field meanings:

- `audio.sinks`: all output sinks currently visible to KDE
- `audio.selectedSinkId`: the current default output sink id, or `null`
- `id`: sink identifier
- `label`: human-readable sink name
- `volume`: normalized sink volume
- `muted`: whether the sink is muted
- `available`: whether the sink is available
- `isDefault`: whether this sink is the current default sink
- `isVirtual`: whether this is a virtual sink
- `backendApi`: backend API reported by the sink, such as `alsa`

## HTTP

Use HTTP for snapshot reads.

### `GET /snapshot/audio/sinks`

Returns:

```json
{
  "sinks": [],
  "selectedSinkId": null
}
```

### `GET /snapshot/audio/default-sink`

Returns the current default sink object.

If no default sink exists, the server returns `404` with:

```json
{ "ok": false, "code": "not_found", "message": "No default audio sink available." }
```

### HTTP Error Behavior

- known audio endpoints return `503` with `code: "not_ready"` before the initial audio state is available
- non-`GET` requests to known endpoints return `405` and `Allow: GET`
- unknown paths return `404`
- responses use `application/json; charset=utf-8`

## WebSocket

Use WebSocket for continuous monitoring.

### Handshake

After connecting, the client must send:

```json
{ "type": "hello", "protocolVersion": 1 }
```

### `fullState`

Sent after a successful handshake once the audio state is ready:

```json
{
  "type": "fullState",
  "state": {
    "audio": {
      "sinks": [],
      "selectedSinkId": null
    }
  }
}
```

### `patch`

Sent whenever the sink list or default sink changes:

```json
{
  "type": "patch",
  "reason": "sink-updated",
  "sinkId": "alsa_output.example",
  "changes": [
    {
      "path": "audio",
      "value": {
        "sinks": [],
        "selectedSinkId": null
      }
    }
  ]
}
```

Each patch replaces the full `audio` subtree.

### `error`

If the client sends an invalid or unsupported message, the server replies with:

```json
{ "type": "error", "code": "not_ready", "message": "..." }
```

### WebSocket Rules

- the client must send `hello` before any state is streamed
- unsupported protocol versions are rejected
- invalid JSON or unsupported message types return an error and close the socket
- `audio.selectedSinkId` is the canonical default-sink identifier for clients
