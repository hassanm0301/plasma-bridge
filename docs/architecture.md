# Architecture

## Overview

`plasma_bridge` is a standalone service that runs in a KDE Plasma user session and exposes local desktop state to local clients.

## Current Scope

The current implementation is intentionally narrow and audio-first:

- observe all KDE-visible output sinks
- observe all KDE-visible input sources
- identify the current default output sink
- identify the current default input source
- report per-device volume and mute state
- expose snapshot reads plus local default, mute, and sink-volume control over HTTP
- expose live updates over WebSocket

Window state, media state, and broader control actions are not part of the current public HTTP or WebSocket runtime surface.
Window inspection is currently available only through optional snapshot probe tools such as `window_probe`.

## System Flow

The runtime has four main parts:

1. An audio adapter reads sink data from the Plasma audio stack.
   It also reads user-facing input sources and filters monitor sources from the public state.
2. A shared in-memory state store keeps one canonical audio snapshot.
3. An HTTP server serves output and input snapshots from that state and forwards local sink/source default, sink/source mute, and sink volume-control requests.
4. A WebSocket server streams the same state to connected clients as it changes.

Both transports use the same underlying state so HTTP snapshots and WebSocket updates stay aligned, including after local HTTP default, mute, and volume changes flow back through the observer and state store.

The HTTP listener also serves local API documentation pages and runtime-adjusted OpenAPI and AsyncAPI documents under `/docs/`.
The checked-in API specs remain the source of truth, while the interactive Swagger UI and AsyncAPI viewer assets are downloaded at build time into the build tree and then served locally by the app.

## Platform Assumptions

The current build targets KDE Plasma on Linux and reads audio state from the Plasma session's audio stack. By default, the service binds only to localhost.

## Defaults

The current defaults are:

- HTTP on `127.0.0.1:8080`
- WebSocket on `ws://127.0.0.1:8081/ws/audio`
- local audio sink/source snapshot, default, mute, and sink volume-control HTTP behavior
- localhost binding unless the operator explicitly changes it
- hosted docs on the HTTP listener under `/docs/`

## Not Yet Included

These areas are still outside the current public scope:

- source volume-control actions
- broader output-control actions beyond default-device and mute changes
- window discovery and active-window tracking in the long-running HTTP/WebSocket service surface
- authentication and pairing
- packaging and service installation workflow
- cross-desktop support beyond KDE Plasma
