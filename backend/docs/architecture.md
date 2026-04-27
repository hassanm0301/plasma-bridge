# Architecture

## Overview

`plasma_bridge` is a standalone service that runs in a KDE Plasma user session and exposes local desktop state to local clients.

## Current Scope

The current implementation exposes local audio and Plasma window state:

- observe all KDE-visible output sinks
- observe all KDE-visible input sources
- observe Plasma windows through a KWin script helper backend
- identify the active window
- identify the current default output sink
- identify the current default input source
- report per-device volume and mute state
- expose audio and window snapshot reads plus local default, mute, sink-volume, and window activation control over HTTP
- expose live audio and window updates over one WebSocket state stream

Media state and broader control actions are not part of the current public HTTP or WebSocket runtime surface.

## System Flow

The runtime has five main parts:

1. An audio adapter reads sink data from the Plasma audio stack.
   It also reads user-facing input sources and filters monitor sources from the public state.
2. A window adapter installs/enables the KWin script helper backend and reads normalized helper snapshots.
3. Shared in-memory state stores keep canonical audio and window snapshots.
4. An HTTP server serves snapshots from those stores and forwards local sink/source default, sink/source mute, sink volume-control, and window activation requests.
5. A WebSocket server streams the same state to connected clients from the unified `/ws` endpoint as it changes.

For local CLI inspection, `window_probe` uses the same shared KWin script backend that writes normalized snapshots through a helper service instead of binding directly to the Plasma window-management Wayland interface.

Both transports use the same underlying stores so HTTP snapshots and WebSocket updates stay aligned, including after local HTTP default, mute, volume, and window activation changes flow back through the observer and state store.
The WebSocket `fullState` payload contains the ready audio and window snapshots, and later `patch` payloads replace either the `audio` or `windowState` subtree.

The HTTP listener also serves local API documentation pages and runtime-adjusted OpenAPI and AsyncAPI documents under `/docs/`.
The checked-in API specs remain the source of truth, while the interactive Swagger UI and AsyncAPI viewer assets are downloaded at build time into the build tree and then served locally by the app.

## Platform Assumptions

The current build targets KDE Plasma on Linux, reads audio state from the Plasma session's audio stack, and reads service window state through the KWin script helper backend. By default, the service binds only to localhost.

## Defaults

The current defaults are:

- HTTP on `127.0.0.1:8080`
- WebSocket on `ws://127.0.0.1:8081/ws`
- local audio sink/source snapshot, default, mute, and sink volume-control HTTP behavior
- local window snapshot, active-window, and window activation HTTP behavior
- localhost binding unless the operator explicitly changes it
- hosted docs on the HTTP listener under `/docs/`

## Not Yet Included

These areas are still outside the current public scope:

- source volume-control actions
- broader output-control actions beyond default-device and mute changes
- window control actions beyond activation
- authentication and pairing
- packaging and service installation workflow
- cross-desktop support beyond KDE Plasma
