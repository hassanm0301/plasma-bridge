import { useEffect, useState } from "react";

import { normalizeWebSocketUrl, WEBSOCKET_PROTOCOL_VERSION } from "./endpoints";
import type { BackendMessage, BackendState } from "./models";
import { applyBackendMessage, emptyBackendState } from "./state";

export type StreamStatus = "connecting" | "connected" | "not_ready" | "disconnected" | "error";

export interface StateStreamResult {
  state: BackendState;
  status: StreamStatus;
  detail: string;
}

export function useStateStream(wsUrl: string, reconnectToken: number): StateStreamResult {
  const [streamState, setStreamState] = useState<BackendState>(emptyBackendState);
  const [status, setStatus] = useState<StreamStatus>("connecting");
  const [detail, setDetail] = useState("Opening WebSocket stream...");

  useEffect(() => {
    let closedByEffect = false;
    let hasState = false;
    setStreamState(emptyBackendState);
    setStatus("connecting");
    setDetail("Opening WebSocket stream...");

    let socket: WebSocket;
    try {
      socket = new WebSocket(normalizeWebSocketUrl(wsUrl));
    } catch (error) {
      setStatus("error");
      setDetail(error instanceof Error ? error.message : "Invalid WebSocket endpoint.");
      return;
    }

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
        const message = JSON.parse(String(event.data)) as BackendMessage;
        if (message.type === "error") {
          setStatus(message.error.code === "not_ready" ? "not_ready" : "error");
          setDetail(message.error.message);
          return;
        }

        setStreamState((currentState) => applyBackendMessage(currentState, message));
        hasState = true;
        setStatus("connected");
        setDetail("Live state stream connected.");
      } catch (error) {
        setStatus("error");
        setDetail(error instanceof Error ? error.message : "Could not read WebSocket message.");
      }
    });

    socket.addEventListener("error", () => {
      setStatus("error");
      setDetail("WebSocket connection failed.");
    });

    socket.addEventListener("close", () => {
      if (!closedByEffect) {
        setStatus(hasState ? "disconnected" : "error");
        setDetail(hasState ? "WebSocket disconnected." : "WebSocket closed before state was received.");
      }
    });

    return () => {
      closedByEffect = true;
      socket.close();
    };
  }, [wsUrl, reconnectToken]);

  return {
    state: streamState,
    status,
    detail
  };
}
