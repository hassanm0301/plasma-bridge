# API Reference

`plasma_bridge` publishes its API documentation as checked-in spec files plus hosted local docs pages.
The checked-in OpenAPI and AsyncAPI files are the source of truth. The interactive viewers are served locally by the app using browser assets downloaded into the build tree during `cmake --build`.

Repo files:

- [OpenAPI](openapi.yaml): HTTP sink/source snapshot endpoints plus sink volume-control endpoints
- [AsyncAPI](asyncapi.yaml): WebSocket protocol for live sink and source state

Hosted local docs from a running service:

- `http://127.0.0.1:8080/docs/`
- `http://127.0.0.1:8080/docs/http`
- `http://127.0.0.1:8080/docs/ws`

Runtime-served spec URLs:

- `http://127.0.0.1:8080/docs/openapi.yaml`
- `http://127.0.0.1:8080/docs/asyncapi.yaml`

The checked-in spec files use the default localhost addresses. The runtime-served spec URLs reflect the actual `--host`, `--port`, and `--ws-port` values of the running service.
