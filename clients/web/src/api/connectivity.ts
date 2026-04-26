import {
  buildOpenApiUrl,
  normalizeWebSocketUrl,
  WEBSOCKET_PROTOCOL_VERSION
} from "./endpoints";

export type CheckState = "idle" | "checking" | "reachable" | "unreachable";

export interface ConnectionCheckResult {
  state: Exclude<CheckState, "idle" | "checking">;
  detail: string;
}

interface HttpCheckOptions {
  fetcher?: typeof fetch;
  timeoutMs?: number;
}

interface WebSocketCheckOptions {
  timeoutMs?: number;
  WebSocketCtor?: typeof WebSocket;
}

interface WebSocketEnvelope {
  type?: string;
  payload?: unknown;
  error?: {
    code?: string;
    message?: string;
  } | null;
}

const DEFAULT_TIMEOUT_MS = 5000;

function messageFromUnknownError(error: unknown): string {
  return error instanceof Error ? error.message : "Unknown connection error.";
}

export async function checkHttpEndpoint(
  httpBaseUrl: string,
  options: HttpCheckOptions = {}
): Promise<ConnectionCheckResult> {
  const fetcher = options.fetcher ?? fetch;
  const timeoutMs = options.timeoutMs ?? DEFAULT_TIMEOUT_MS;
  const controller = new AbortController();
  const timeout = window.setTimeout(() => controller.abort(), timeoutMs);

  try {
    const response = await fetcher(buildOpenApiUrl(httpBaseUrl), {
      method: "GET",
      headers: {
        Accept: "application/yaml,text/yaml,text/plain,*/*"
      },
      signal: controller.signal
    });

    if (!response.ok) {
      return {
        state: "unreachable",
        detail: `HTTP ${response.status} ${response.statusText}`.trim()
      };
    }

    const body = await response.text();
    if (!body.includes("openapi:")) {
      return {
        state: "unreachable",
        detail: "The endpoint responded, but it did not look like the OpenAPI spec."
      };
    }

    return {
      state: "reachable",
      detail: "OpenAPI spec reached."
    };
  } catch (error) {
    return {
      state: "unreachable",
      detail: messageFromUnknownError(error)
    };
  } finally {
    window.clearTimeout(timeout);
  }
}

export function checkWebSocketEndpoint(
  wsEndpointUrl: string,
  options: WebSocketCheckOptions = {}
): Promise<ConnectionCheckResult> {
  const timeoutMs = options.timeoutMs ?? DEFAULT_TIMEOUT_MS;
  const WebSocketCtor = options.WebSocketCtor ?? WebSocket;
  const normalizedUrl = normalizeWebSocketUrl(wsEndpointUrl);

  return new Promise((resolve) => {
    let settled = false;
    const socket = new WebSocketCtor(normalizedUrl);

    const finish = (result: ConnectionCheckResult) => {
      if (settled) {
        return;
      }
      settled = true;
      window.clearTimeout(timeout);
      socket.close();
      resolve(result);
    };

    const timeout = window.setTimeout(() => {
      finish({
        state: "unreachable",
        detail: "WebSocket connection timed out."
      });
    }, timeoutMs);

    socket.addEventListener("open", () => {
      socket.send(
        JSON.stringify({
          type: "hello",
          payload: { protocolVersion: WEBSOCKET_PROTOCOL_VERSION },
          error: null
        })
      );
    });

    socket.addEventListener("message", (event) => {
      try {
        const envelope = JSON.parse(String(event.data)) as WebSocketEnvelope;
        if (envelope.type === "fullState" && envelope.error === null) {
          finish({
            state: "reachable",
            detail: "State stream reached."
          });
          return;
        }

        if (envelope.type === "error" && envelope.error?.code === "not_ready") {
          finish({
            state: "reachable",
            detail: envelope.error.message ?? "Backend reached, state is not ready yet."
          });
          return;
        }

        finish({
          state: "unreachable",
          detail: envelope.error?.message ?? "Unexpected WebSocket response."
        });
      } catch (error) {
        finish({
          state: "unreachable",
          detail: messageFromUnknownError(error)
        });
      }
    });

    socket.addEventListener("error", () => {
      finish({
        state: "unreachable",
        detail: "WebSocket connection failed."
      });
    });
  });
}
