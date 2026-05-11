# Clients

This folder contains client applications that consume the `plasma_bridge` backend APIs.

- [web](web/): React web dashboard
- [app](app/): Flutter mobile app, Android-first, with a compact Breeze-inspired tablet layout

The project version is managed once at the repository root in [../VERSION](../VERSION), and release notes live in [../CHANGELOG.md](../CHANGELOG.md).

The implemented clients render live windows, current media playback, and audio device state from the `plasma_bridge` backend, then use the shared HTTP control endpoints for interaction.
