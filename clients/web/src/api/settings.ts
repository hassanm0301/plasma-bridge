import { DEFAULT_HTTP_BASE_URL, DEFAULT_WS_URL, normalizeHttpBaseUrl, normalizeWebSocketUrl } from "./endpoints";

export interface EndpointSettings {
  httpBaseUrl: string;
  wsUrl: string;
}

const STORAGE_KEY = "plasma-remote-toolbar.endpoints";

export function initialEndpointSettings(): EndpointSettings {
  const fallback = {
    httpBaseUrl: DEFAULT_HTTP_BASE_URL,
    wsUrl: DEFAULT_WS_URL
  };

  try {
    const stored = window.localStorage.getItem(STORAGE_KEY);
    if (stored === null) {
      return fallback;
    }

    const parsed = JSON.parse(stored) as Partial<EndpointSettings>;
    return {
      httpBaseUrl: normalizeHttpBaseUrl(parsed.httpBaseUrl ?? fallback.httpBaseUrl),
      wsUrl: normalizeWebSocketUrl(parsed.wsUrl ?? fallback.wsUrl)
    };
  } catch {
    return fallback;
  }
}

export function persistEndpointSettings(settings: EndpointSettings): EndpointSettings {
  const normalized = {
    httpBaseUrl: normalizeHttpBaseUrl(settings.httpBaseUrl),
    wsUrl: normalizeWebSocketUrl(settings.wsUrl)
  };
  window.localStorage.setItem(STORAGE_KEY, JSON.stringify(normalized));
  return normalized;
}
