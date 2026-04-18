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

No checked-in HTTP or WebSocket surface exists in the current audio-first milestone. The transport split below is still the intended direction once API work starts.

### WebSocket

WebSocket is the primary live-state interface.

Use WebSocket for:

- initial client handshake
- full-state snapshot after connection
- live state updates for audio and windows
- any other feature that depends on continuous observation

WebSocket should be favored whenever the client would otherwise need to poll.

### HTTP

HTTP is the infrequent request or operational interface.

Use HTTP for:

- health or readiness checks
- version or capability reporting
- rare actions that do not require a persistent subscription
- one-off snapshot reads if that proves useful during implementation

The rule is case-by-case:

- continuous observation belongs on WebSocket
- rare request or response operations belong on HTTP

## Deferred Interface Areas

- grouped application views
- media state and control
- exact endpoint catalogs and wire message details
- fully mirrored HTTP and WebSocket parity
- strong authentication and pairing requirements for day one
