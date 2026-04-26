# API Specs

`plasma_bridge` publishes its API documentation as checked-in spec files plus hosted local docs pages.
The checked-in OpenAPI and AsyncAPI files are the source of truth for the backend and future clients. The interactive viewers are served locally by the app using browser assets downloaded into the backend build tree during `cmake --build`.
Window snapshot and WebSocket state are sourced from the shared KWin script helper backend.
The WebSocket API uses one state endpoint, `ws://127.0.0.1:8081/ws`, for both audio and window updates.

Repo files:

- [OpenAPI](openapi.yaml): Swagger-compatible HTTP sink/source/window snapshot endpoints plus sink/source default, mute, and sink volume-control endpoints
- [AsyncAPI](asyncapi.yaml): WebSocket protocol for the unified live sink, source, and window state stream

Hosted local docs from a running service:

- `http://127.0.0.1:8080/docs/`
- `http://127.0.0.1:8080/docs/http`
- `http://127.0.0.1:8080/docs/ws`

Runtime-served spec URLs:

- `http://127.0.0.1:8080/docs/openapi.yaml`
- `http://127.0.0.1:8080/docs/asyncapi.yaml`

The checked-in spec files use the default localhost addresses. The runtime-served spec URLs reflect the actual `--host`, `--port`, and `--ws-port` values of the running service.
