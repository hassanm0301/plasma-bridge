# Backend Roadmap

## Product Goal

Build a standalone Qt 6 backend service for KDE Plasma that runs in the user session, reads selected desktop state, and later exposes that state to remote clients through a narrow API.

The backend should become useful before control features exist, so the project starts with a short feasibility phase that proves the needed Plasma state is readable and observable from a separate process.

## Target Platform

- Primary target: KDE Plasma 6 on Wayland
- Deferred compatibility target: X11 support after the Wayland-first path is proven
- Desktop focus: KDE Plasma only for V1

## V1 Scope

The first real product version should stay narrow:

- read all KDE-visible output sinks
- read the current selected or default output sink
- read per-sink volume and mute state
- observe live sink changes without polling
- support PipeWire sessions with ALSA-backed sinks
- expose a small HTTP snapshot surface for audio sinks
- expose WebSocket live updates for audio sinks and the current default sink
- defer broader desktop state until after the audio transport surface is proven

The first usable milestone after feasibility is read-only and audio-first. The first public transport slice includes HTTP snapshots and WebSocket live monitoring; controls and windows come later.

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

Run small probes to confirm that the target Plasma session exposes the sink data needed for the backend. The goal of this milestone is to validate the integration path before public API work begins.

### Milestone 1: Audio-First Skeleton

Build a minimal backend skeleton and a runnable audio probe around the validated integration path.

Success means:

- the project configures and builds with Qt 6 and `KF6PulseAudioQt`
- an `audio_probe` can enumerate all KDE-visible output sinks
- the probe can identify the selected default sink
- the probe can print per-sink volume and mute state
- the probe can observe live sink changes without polling

### Milestone 2: Public Transport And Broader Read Model

Add the first transport surface and expand beyond the audio-first probe once sink observation is stable.

Success means:

- an HTTP runtime can serve `GET /snapshot/audio/sinks`
- an HTTP runtime can serve `GET /snapshot/audio/default-sink`
- repeated HTTP snapshot reads reflect current output-sink state
- a WebSocket runtime can publish initial audio sink state after client hello
- a WebSocket runtime can publish live audio updates after sink and default-sink changes
- live usable-window state is published
- active window changes are propagated
- the canonical backend state remains stable through ordinary desktop changes

### Milestone 3: First Safe Controls

Add a limited set of infrequent actions only after the read model is stable.

Expected first candidates:

- set output volume
- set output mute if supported cleanly
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

Do not move forward from the audio-first feasibility slice unless the project can cleanly read all KDE-visible output sinks, the default sink, and live sink updates on the target Plasma session.

Do not add controls before the HTTP and WebSocket audio read surfaces are stable and the read-only state model is predictable.

Do not expand scope into grouped apps, media, or non-Plasma desktops until the V1 read model is complete.
