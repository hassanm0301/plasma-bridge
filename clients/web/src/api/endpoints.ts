export const DEFAULT_HTTP_BASE_URL = "http://127.0.0.1:8080";
export const DEFAULT_WS_URL = "ws://127.0.0.1:8081/ws";
export const WEBSOCKET_PROTOCOL_VERSION = 3;

export function normalizeHttpBaseUrl(value: string): string {
  const trimmed = value.trim();
  if (!trimmed) {
    throw new Error("HTTP endpoint is required.");
  }

  const url = new URL(trimmed);
  if (url.protocol !== "http:" && url.protocol !== "https:") {
    throw new Error("HTTP endpoint must start with http:// or https://.");
  }

  url.hash = "";
  url.search = "";
  url.pathname = url.pathname.replace(/\/+$/, "");

  const normalized = url.toString();
  return normalized.endsWith("/") ? normalized.slice(0, -1) : normalized;
}

export function buildOpenApiUrl(httpBaseUrl: string): string {
  return `${normalizeHttpBaseUrl(httpBaseUrl)}/docs/openapi.yaml`;
}

export function normalizeWebSocketUrl(value: string): string {
  const trimmed = value.trim();
  if (!trimmed) {
    throw new Error("WebSocket endpoint is required.");
  }

  const url = new URL(trimmed);
  if (url.protocol !== "ws:" && url.protocol !== "wss:") {
    throw new Error("WebSocket endpoint must start with ws:// or wss://.");
  }

  url.hash = "";
  return url.toString();
}
