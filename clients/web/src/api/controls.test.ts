import { afterEach, describe, expect, it, vi } from "vitest";

import { activateWindow, controlPath, windowControlPath } from "./controls";

afterEach(() => {
  vi.unstubAllGlobals();
});

describe("control helpers", () => {
  it("URL-encodes device ids in control paths", () => {
    expect(controlPath("sinks", "sink with space", "volume")).toBe("/control/audio/sinks/sink%20with%20space/volume");
  });

  it("URL-encodes window ids in window control paths", () => {
    expect(windowControlPath("window with space", "active")).toBe("/control/windows/window%20with%20space/active");
  });

  it("sends window activation as a bodyless POST", async () => {
    const fetchMock = vi.fn().mockResolvedValue(new Response("{}", { status: 200, statusText: "OK" }));
    vi.stubGlobal("fetch", fetchMock);

    await activateWindow("http://127.0.0.1:8080/", "window with space");

    expect(fetchMock).toHaveBeenCalledWith("http://127.0.0.1:8080/control/windows/window%20with%20space/active", {
      method: "POST"
    });
  });
});
