# Web Development

The web client is a Vite, React, and TypeScript app.

## Requirements

- Node.js
- npm

## Install

```bash
cd clients/web
npm install
```

## Run

```bash
npm run dev
```

The dev server binds to `0.0.0.0` by default so it can be opened from another machine on the same network. The backend still allows browser requests only from loopback web origins unless you start `plasma_bridge` with matching `--allow-origin` entries for the remote dashboard origin.

## Build

```bash
npm run build
```

## Test

```bash
npm test
```

## Source Map

- `src/app/`: app shell and dashboard orchestration
- `src/api/`: endpoint normalization, settings persistence, WebSocket state reduction, and HTTP controls for audio, current media transport/seek, and window activation
- `src/components/`: reusable settings, window, media, audio, and form components, including the media progress/timeline UI
- `src/theme/`: Breeze-inspired theme tokens and persistence
