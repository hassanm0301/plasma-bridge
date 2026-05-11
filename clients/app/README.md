# Mobile App

Flutter client for Plasma Bridge, with Android as the first supported target and a compact Breeze-inspired tablet layout.

## Features

- First-launch endpoint setup for remote KDE hosts
- Live window, media, sink, and source state from the shared WebSocket stream
- Window activation, media transport/seek, sink volume, and sink/source mute control through the existing HTTP API
- Persisted light/dark theme plus saved endpoint settings
- Breeze-like light and dark themes tuned for a denser desktop-utility feel
- Landscape-first dashboard for tablets, with full-width windows and current-media sections plus compact audio controls

## Getting Started

From the monorepo root:

```bash
cd clients/app
flutter pub get
flutter test
flutter run -d android
```

Start `plasma_bridge` first on the KDE Plasma machine. On a physical Android device, use the backend machine's LAN address instead of `127.0.0.1` during first-run setup.

The Android UI is optimized for landscape tablet use. Portrait remains supported, but the primary layout is a denser two-stage control surface with windows and current media prioritized above audio controls.

The backend defaults remain:

- HTTP: `http://127.0.0.1:8080`
- WebSocket: `ws://127.0.0.1:8081/ws`

## Architecture

The app is organized by layer and feature:

- `lib/app/`: app shell and theme
- `lib/core/`: transport DTOs, shared domain models, endpoint helpers, and reusable widgets
- `lib/features/settings/`: persisted endpoint/theme settings plus first-run setup
- `lib/features/dashboard/`: connection state, repositories, and composed dashboard
- `lib/features/windows/`, `lib/features/media/`, `lib/features/audio/`: feature UI

The dashboard keeps connection status in the top-right app bar instead of using a dedicated panel. Windows and current media are given full-width sections so more taskbar entries and playback controls fit comfortably on a tablet in landscape mode.

The Flutter client implements the checked-in contracts under [`../../specs`](../../specs/) directly and does not require backend API changes.

## Notes

- The app is parity-first with the web dashboard; widgets, notifications, QR pairing, and discovery are intentionally out of scope.
- If your local Java version is newer than the generated Android Gradle setup supports, point Flutter at a compatible JDK before building Android artifacts.
