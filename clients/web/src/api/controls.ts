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

export function windowControlPath(windowId: string, action: "active"): string {
  return `/control/windows/${encodeURIComponent(windowId)}/${action}`;
}

async function postJson(httpBaseUrl: string, path: string, body: unknown): Promise<void> {
  const response = await fetch(`${normalizeHttpBaseUrl(httpBaseUrl)}${path}`, {
    method: "POST",
    headers: {
      "Content-Type": "application/json"
    },
    body: JSON.stringify(body)
  });

  await assertControlResponseOk(response);
}

async function postEmpty(httpBaseUrl: string, path: string): Promise<void> {
  const response = await fetch(`${normalizeHttpBaseUrl(httpBaseUrl)}${path}`, {
    method: "POST"
  });

  await assertControlResponseOk(response);
}

async function assertControlResponseOk(response: Response): Promise<void> {
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

export function activateWindow(httpBaseUrl: string, windowId: string): Promise<void> {
  return postEmpty(httpBaseUrl, windowControlPath(windowId, "active"));
}
