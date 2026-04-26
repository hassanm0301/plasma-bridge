import { beforeEach, describe, expect, it } from "vitest";

import { DEFAULT_HTTP_BASE_URL, DEFAULT_WS_URL } from "./endpoints";
import { initialEndpointSettings, persistEndpointSettings } from "./settings";

function memoryStorage(): Storage {
  const values = new Map<string, string>();
  return {
    get length() {
      return values.size;
    },
    clear: () => values.clear(),
    getItem: (key: string) => values.get(key) ?? null,
    key: (index: number) => [...values.keys()][index] ?? null,
    removeItem: (key: string) => values.delete(key),
    setItem: (key: string, value: string) => {
      values.set(key, value);
    }
  };
}

describe("endpoint settings", () => {
  beforeEach(() => {
    Object.defineProperty(window, "localStorage", {
      configurable: true,
      value: memoryStorage()
    });
  });

  it("uses default endpoints when no settings are stored", () => {
    expect(initialEndpointSettings()).toEqual({
      httpBaseUrl: DEFAULT_HTTP_BASE_URL,
      wsUrl: DEFAULT_WS_URL
    });
  });

  it("persists normalized endpoints", () => {
    const saved = persistEndpointSettings({
      httpBaseUrl: "http://localhost:8080/",
      wsUrl: "ws://localhost:8081/ws"
    });

    expect(saved.httpBaseUrl).toBe("http://localhost:8080");
    expect(initialEndpointSettings()).toEqual(saved);
  });
});
