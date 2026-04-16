# Decision 0001: Foundation Choices

## Status

Accepted

## Context

The project needed a baseline set of product and implementation choices before backend work could begin. Without these defaults, Milestone 0 and the later service design would be underspecified.

## Decisions

- The checked-in documentation in `docs/` is the source of truth for contributor-facing project policy and architecture.
- The initial product target is KDE Plasma 6 on Wayland.
- X11 support is deferred until after the Wayland-first path is proven.
- The primary model for "opened apps" is individual usable windows.
- The first usable milestone after feasibility is read-only.
- V1 audio scope includes default output and default input state.
- The backend runtime is a standalone Qt 6 user-session service.
- Plasma-specific libraries are acceptable when they reduce integration risk.
- WebSocket is the live-state transport.
- HTTP is the infrequent request and operational transport.
- Media integration is post-V1 optional work.
- Stronger authentication is deferred until after the core backend works on the local network.

## Consequences

- The project can focus first on proving desktop-state access rather than designing a large control surface.
- Interface work can stay narrow because grouped apps, media, and cross-desktop support are out of scope for V1.
- Window integration may still require project-specific experimentation, but the decision to target Plasma directly reduces ambiguity about acceptable dependencies.

## Follow-Up Decisions Expected Later

- exact audio backend choice
- exact window integration path
- HTTP endpoint shape
- WebSocket message schema details
- authentication and pairing strategy
