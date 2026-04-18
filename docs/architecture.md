# Architecture

## Overview

`plasma_bridge` is a standalone service that runs in a KDE Plasma user session and exposes local desktop state to local clients.

## Current Scope

The current implementation is intentionally narrow and audio-first:

- observe all KDE-visible output sinks
- identify the current default output sink
- report per-sink volume and mute state
- expose snapshot reads over HTTP
- expose live updates over WebSocket

Input devices, window state, media state, and control actions are not part of the current public runtime surface.

## System Flow

The runtime has four main parts:

1. An audio adapter reads sink data from the Plasma audio stack.
2. A shared in-memory state store keeps one canonical audio snapshot.
3. An HTTP server serves read-only snapshots of that state.
4. A WebSocket server streams the same state to connected clients as it changes.

Both transports use the same underlying state so HTTP snapshots and WebSocket updates stay aligned.

## Platform Assumptions

The current build targets KDE Plasma on Linux and reads audio state from the Plasma session's audio stack. By default, the service binds only to localhost.

## Defaults

The current defaults are:

- HTTP on `127.0.0.1:8080`
- WebSocket on `127.0.0.1:8081`
- read-only API behavior
- localhost binding unless the operator explicitly changes it

## Not Yet Included

These areas are still outside the current public scope:

- output control actions such as changing sink or volume
- window discovery and active-window tracking
- authentication and pairing
- packaging and service installation workflow
- cross-desktop support beyond KDE Plasma
