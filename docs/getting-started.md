# Getting Started

Build and run commands target the backend service, the React web client, and the Flutter mobile client.

## Backend

From the monorepo root:

```bash
cmake -S backend -B backend/build -G Ninja
cmake --build backend/build --target plasma_bridge
```

Run the service from a KDE Plasma user session:

```bash
./backend/build/src/app/plasma_bridge
```

Default backend endpoints:

- HTTP: `http://127.0.0.1:8080`
- WebSocket: `ws://127.0.0.1:8081/ws`
- Hosted docs: `http://127.0.0.1:8080/docs/`

The backend binds to localhost by default. To reach it from another machine on the same network, start it with a non-loopback `--host`. If the other machine will use the browser dashboard, also pass `--allow-origin http://<web-host>:5173` or a matching web origin.

For full backend setup, verification, and test instructions, see [backend/docs/getting-started.md](../backend/docs/getting-started.md).

## Web Client

From the monorepo root:

```bash
cd clients/web
npm install
npm run dev
```

Open the Vite dev server URL. The dashboard auto-connects with the default backend endpoints:

- HTTP: `http://127.0.0.1:8080`
- WebSocket: `ws://127.0.0.1:8081/ws`

The Vite dev server binds on all interfaces, so you can open it from another machine on the same network. The dashboard still defaults to localhost backend endpoints, so remote users must update Settings to point at the backend machine. The main screen shows windows first, then the current media session with icon transport controls and draggable progress, then audio sinks and sources.

See [clients/web](../clients/web/) for web-specific docs.

## Mobile App

From the monorepo root:

```bash
cd clients/app
flutter pub get
flutter test
flutter run -d android
```

The mobile client uses the same backend endpoints as the web client, but first launch requires manual endpoint setup instead of assuming localhost. On a physical Android device, point the app at the KDE machine's LAN address.

The Android UI is tuned for landscape tablet usage. It keeps connection state in the top-right app bar, gives windows and current media full-width sections, and packs the rest of the controls more tightly than the web dashboard.

See [clients/app](../clients/app/) for app-specific docs.

## Specs

The checked-in API contracts live in [specs](../specs/). The backend also serves runtime-adjusted copies from `/docs/openapi.yaml` and `/docs/asyncapi.yaml` while running. The current runtime includes audio, current-media, and window state on both transports, plus HTTP media seek control.
