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

The dev server binds to `127.0.0.1` by default. The backend allows browser requests from loopback web origins such as `http://127.0.0.1:5173`, `http://127.0.0.1:5174`, and `http://localhost:5173`.

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
- `src/api/`: endpoint normalization, settings persistence, WebSocket state reduction, and HTTP controls
- `src/components/`: reusable settings, window, audio, and form components
- `src/theme/`: Breeze-inspired theme tokens and persistence
