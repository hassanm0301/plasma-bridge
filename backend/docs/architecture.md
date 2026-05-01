# Architecture

## Overview

`plasma_bridge` is a standalone service that runs in a KDE Plasma user session and exposes local desktop state to localhost clients by default.

## Current Scope

The current implementation exposes local audio, MPRIS media-session, and Plasma window state:

- observe all KDE-visible output sinks
- observe all KDE-visible input sources
- observe Plasma windows through a KWin script helper backend
- observe the current MPRIS media player selection
- identify the active window
- identify the current default output sink
- identify the current default input source
- identify the current media player, playback position, track length, and transport/seek capabilities
- report per-device volume and mute state
- expose audio, media, and window snapshot reads plus local default, mute, sink-volume, media transport, media seek, and window activation control over HTTP
- expose live audio, media, and window updates over one WebSocket state stream

## System Flow

The runtime has six main parts:

1. An audio adapter reads sink data from the Plasma audio stack.
   It also reads user-facing input sources and filters monitor sources from the public state.
2. A media adapter watches MPRIS players on the session DBus bus, selects one current session, refreshes playback position while that session is playing, and forwards transport and seek actions.
3. A window adapter installs/enables the KWin script helper backend and reads normalized helper snapshots.
4. Shared in-memory state stores keep canonical audio, media, and window snapshots.
5. An HTTP server serves snapshots from those stores and forwards local sink/source default, sink/source mute, sink volume-control, media transport, media seek, and window activation requests.
6. A WebSocket server streams the same state to connected clients from the unified `/ws` endpoint as it changes.

For local CLI inspection, `window_probe` uses the same shared KWin script backend that writes normalized snapshots through a helper service instead of binding directly to the Plasma window-management Wayland interface.

Both transports use the same underlying stores so HTTP snapshots and WebSocket updates stay aligned, including after local HTTP default, mute, volume, media transport, media seek, and window activation changes flow back through the observer and state store.
The WebSocket `fullState` payload contains the ready audio, media, and window snapshots, and later `patch` payloads replace either the `audio`, `media`, or `windowState` subtree.

The HTTP listener also serves local API documentation pages and runtime-adjusted OpenAPI and AsyncAPI documents under `/docs/`.
The checked-in API specs remain the source of truth, while the interactive Swagger UI and AsyncAPI viewer assets are downloaded at build time into the build tree and then served locally by the app.

## Platform Assumptions

The current build targets KDE Plasma on Linux, reads audio state from the Plasma session's audio stack, reads media state from MPRIS over session DBus, and reads service window state through the KWin script helper backend. By default, the service binds only to localhost, and HTTP CORS access is limited to loopback browser origins unless the operator explicitly adds `--allow-origin` entries.

## Defaults

The current defaults are:

- HTTP on `127.0.0.1:8080`
- WebSocket on `ws://127.0.0.1:8081/ws`
- local audio sink/source snapshot, default, mute, and sink volume-control HTTP behavior
- local current-media snapshot and play/pause/play-pause/next/previous/seek HTTP behavior
- local window snapshot, active-window, and window activation HTTP behavior
- localhost binding unless the operator explicitly changes it
- hosted docs on the HTTP listener under `/docs/`

## Not Yet Included

These areas are still outside the current public scope:

- source volume-control actions
- richer media transport actions beyond play/pause/play-pause/next/previous/seek
- broader output-control actions beyond default-device and mute changes
- window control actions beyond activation
- authentication and pairing
- packaging and service installation workflow
- cross-desktop support beyond KDE Plasma
