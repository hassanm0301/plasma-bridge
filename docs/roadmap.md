# Backend Roadmap

## Product Goal

Build a standalone Qt 6 backend service for KDE Plasma that runs in the user session, reads selected desktop state, and exposes that state to remote clients through a narrow API.

The backend should become useful before control features exist, so the project starts with a short feasibility phase that proves the needed Plasma state is readable and observable from a separate process.

## Target Platform

- Primary target: KDE Plasma 6 on Wayland
- Deferred compatibility target: X11 support after the Wayland-first path is proven
- Desktop focus: KDE Plasma only for V1

## V1 Scope

The first real product version should stay narrow:

- read default output audio state
- read default input audio state
- read the current list of usable windows
- read the active window
- stream live state changes to connected clients
- expose a small HTTP surface for infrequent operations and operational checks

The first usable milestone after feasibility is read-only. Control features come later.

## Explicit Non-Goals

These are out of scope for the initial implementation:

- remote desktop streaming
- pointer or keyboard injection
- filesystem browsing
- notification management
- grouped application modeling
- generic non-Plasma Linux desktop support
- media integration in V1
- strong authentication as a day-one blocker

## Delivery Strategy

### Milestone 0: Feasibility Spike

Run small probes to confirm that the target Plasma session exposes the audio and window state needed for the backend. The goal of this milestone is to validate the integration path before the real service is built.

### Milestone 1: Service Skeleton

Build the process, transport, and state shell with fake adapters first.

Success means:

- the Qt service starts inside the user session
- clients can connect through WebSocket
- HTTP exposes operational endpoints
- a fake snapshot can be returned without desktop integrations

### Milestone 2: Read-Only Desktop State

Replace fake adapters with the integrations selected during feasibility.

Success means:

- live audio state is published
- live usable-window state is published
- active window changes are propagated
- the canonical backend state remains stable through ordinary desktop changes

### Milestone 3: First Safe Controls

Add a limited set of infrequent actions only after the read model is stable.

Expected first candidates:

- set output volume
- set input mute or output mute if supported cleanly
- activate a usable window
- close a usable window if reliability is acceptable

### Milestone 4: Hardening

Make the transport and state behavior predictable under reconnects, errors, and longer-running sessions.

Success means:

- protocol versioning rules are documented
- reconnect and resync behavior is deterministic
- slow or disconnected clients do not destabilize the service
- authentication and pairing strategy is defined for post-V1 hardening

### Milestone 5: Packaging And Developer Experience

Package the backend as a user-session service and document a repeatable developer workflow.

Success means:

- install and run steps are documented
- systemd user-service packaging is defined
- a simple test client or probe workflow exists for local verification

## Acceptance Gates

Do not move forward from feasibility unless the project can cleanly read audio and usable-window state on the target Plasma session.

Do not add controls before the read-only state model is stable and transport behavior is predictable.

Do not expand scope into grouped apps, media, or non-Plasma desktops until the V1 read model is complete.
