# Interfaces

## Purpose

This document defines the contributor-facing backend contract summary: what state exists, how the transports are divided, and which interface areas are deferred.

## Canonical State Model

The backend owns one canonical state model for V1:

- `audio.defaultOutput`
- `audio.defaultInput`
- `windows`
- `activeWindowId`

### Audio

Each default audio device state should provide enough information for a toolbar to display current status without exposing a full mixer model.

### Windows

The `windows` collection represents individual usable windows, not grouped apps.

Filtering rule for V1:

- include normal user-meaningful app or task windows
- exclude panels, popups, menus, notifications, tooltips, splash screens, internal desktop surfaces, and similar non-toolbar targets by default

`activeWindowId` should point at the currently active usable window when one exists.

## Transport Split

The backend exposes both WebSocket and HTTP, but they serve different jobs.

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
