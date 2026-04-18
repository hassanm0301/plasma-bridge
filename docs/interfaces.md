# Interfaces

## Purpose

This document defines the contributor-facing backend contract summary: what state exists, how the transports are divided, and which interface areas are deferred.

## Canonical State Model

The backend owns one canonical audio-first state model for the first V1 slice:

- `audio.sinks`
- `audio.selectedSinkId`

### Audio

The backend should expose all KDE-visible output sinks rather than only a single default device snapshot.

`audio.sinks[]` entries should provide:

- `id`
- `label`
- `volume`
- `muted`
- `available`
- `isDefault`
- `isVirtual`
- `backendApi`

`audio.selectedSinkId` should identify the current default output sink and should be `null` when no default sink is available.

### Windows

Window state is still planned, but it is not part of the first checked-in audio milestone.

The `windows` collection represents individual usable windows, not grouped apps.

Filtering rule for V1:

- include normal user-meaningful app or task windows
- exclude panels, popups, menus, notifications, tooltips, splash screens, internal desktop surfaces, and similar non-toolbar targets by default

`activeWindowId` should point at the currently active usable window when one exists.

## Transport Split

The current milestone exposes both HTTP snapshots and WebSocket live monitoring for audio sinks.

### WebSocket

WebSocket is the primary live-state interface.

The current checked-in WebSocket surface is:

- default URL: `ws://127.0.0.1:8081`

Client hello:

```json
{ "type": "hello", "protocolVersion": 1 }
```

Server full snapshot:

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

Server patch:

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

Server error:

```json
{ "type": "error", "code": "not_ready", "message": "..." }
```

WebSocket rules for this slice:

- the client must send `hello` before any state is streamed
- a successful hello returns one `fullState`
- later sink and default-sink changes are sent as coarse-grained `patch` messages replacing the full `audio` subtree
- `audio.selectedSinkId` is the canonical way to identify the current default output sink
- invalid JSON, unsupported protocol versions, or unsupported client messages return an error and close the socket

Use WebSocket for:

- initial client handshake
- full-state snapshot after connection
- live state updates for audio and windows
- any other feature that depends on continuous observation

WebSocket should be favored whenever the client would otherwise need to poll.

### HTTP

HTTP is the infrequent request or operational interface.

The current checked-in endpoints are:

- `GET /snapshot/audio/sinks`
- `GET /snapshot/audio/default-sink`

Runtime defaults for this slice:

- bind host: `127.0.0.1`
- bind port: `8080`
- response content type: `application/json; charset=utf-8`

Response rules for this slice:

- `GET /snapshot/audio/sinks` returns the `AudioState` shape with `sinks` and `selectedSinkId`
- `GET /snapshot/audio/default-sink` returns one sink object when a default sink exists
- `GET /snapshot/audio/default-sink` returns `404` with `{ "ok": false, "code": "not_found", "message": "No default audio sink available." }` when no default sink exists
- known audio snapshot endpoints return `503` with `code: "not_ready"` before the initial sink state is ready
- non-`GET` requests to known audio snapshot endpoints return `405` and `Allow: GET`
- unknown paths return `404`

The rule is case-by-case:

- continuous observation belongs on WebSocket
- rare request or response operations belong on HTTP

## Deferred Interface Areas

- grouped application views
- media state and control
- exact endpoint catalogs and wire message details
- fully mirrored HTTP and WebSocket parity
- strong authentication and pairing requirements for day one
