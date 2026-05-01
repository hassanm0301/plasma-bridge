import { describe, expect, it } from "vitest";

import {
  WEBSOCKET_PROTOCOL_VERSION,
  buildOpenApiUrl,
  normalizeHttpBaseUrl,
  normalizeWebSocketUrl
} from "./endpoints";

describe("endpoint helpers", () => {
  it("normalizes HTTP base URLs before building the OpenAPI URL", () => {
    expect(buildOpenApiUrl(" http://127.0.0.1:8080/ ")).toBe(
      "http://127.0.0.1:8080/docs/openapi.yaml"
    );
  });

  it("keeps an HTTP path prefix when one is supplied", () => {
    expect(buildOpenApiUrl("http://localhost:9000/plasma/")).toBe(
      "http://localhost:9000/plasma/docs/openapi.yaml"
    );
  });

  it("rejects non-HTTP base URLs", () => {
    expect(() => normalizeHttpBaseUrl("ws://127.0.0.1:8081/ws")).toThrow(
      "HTTP endpoint must start with http:// or https://."
    );
  });

  it("accepts websocket endpoints", () => {
    expect(normalizeWebSocketUrl(" ws://127.0.0.1:8081/ws ")).toBe(
      "ws://127.0.0.1:8081/ws"
    );
  });

  it("rejects non-websocket endpoints", () => {
    expect(() => normalizeWebSocketUrl("http://127.0.0.1:8080")).toThrow(
      "WebSocket endpoint must start with ws:// or wss://."
    );
  });

  it("uses websocket protocol version 3 for media-aware state streams", () => {
    expect(WEBSOCKET_PROTOCOL_VERSION).toBe(3);
  });
});
