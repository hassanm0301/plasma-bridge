# Web Usage

Start the backend first, then run the web client.

Default endpoints:

- HTTP: `http://127.0.0.1:8080`
- WebSocket: `ws://127.0.0.1:8081/ws`

The Connect button checks both endpoints independently. The HTTP check reads `/docs/openapi.yaml`. The WebSocket check opens the state stream, sends the protocol `hello` message, and accepts either a full state response or a backend `not_ready` response as successful contact.
