import { normalizeHttpBaseUrl } from "./endpoints";

type DeviceKind = "sinks" | "sources";

interface ApiEnvelope {
  error?: {
    message?: string;
  } | null;
}

export function controlPath(kind: DeviceKind, deviceId: string, action: "mute" | "volume"): string {
  return `/control/audio/${kind}/${encodeURIComponent(deviceId)}/${action}`;
}

async function postJson(httpBaseUrl: string, path: string, body: unknown): Promise<void> {
  const response = await fetch(`${normalizeHttpBaseUrl(httpBaseUrl)}${path}`, {
    method: "POST",
    headers: {
      "Content-Type": "application/json"
    },
    body: JSON.stringify(body)
  });

  if (response.ok) {
    return;
  }

  let message = `HTTP ${response.status} ${response.statusText}`.trim();
  try {
    const envelope = (await response.json()) as ApiEnvelope;
    message = envelope.error?.message ?? message;
  } catch {
    // Keep the HTTP status fallback.
  }

  throw new Error(message);
}

export function setSinkVolume(httpBaseUrl: string, sinkId: string, value: number): Promise<void> {
  return postJson(httpBaseUrl, controlPath("sinks", sinkId, "volume"), { value });
}

export function setDeviceMuted(
  httpBaseUrl: string,
  kind: DeviceKind,
  deviceId: string,
  muted: boolean
): Promise<void> {
  return postJson(httpBaseUrl, controlPath(kind, deviceId, "mute"), { muted });
}
