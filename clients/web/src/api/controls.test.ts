import { afterEach, describe, expect, it, vi } from "vitest";

import { activateWindow, controlMedia, controlPath, mediaControlPath, mediaSeekPath, seekMedia, windowControlPath } from "./controls";

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

  it("builds media control paths", () => {
    expect(mediaControlPath("play-pause")).toBe("/control/media/current/play-pause");
  });

  it("builds the media seek path", () => {
    expect(mediaSeekPath()).toBe("/control/media/current/seek");
  });

  it("sends window activation as a bodyless POST", async () => {
    const fetchMock = vi.fn().mockResolvedValue(new Response("{}", { status: 200, statusText: "OK" }));
    vi.stubGlobal("fetch", fetchMock);

    await activateWindow("http://127.0.0.1:8080/", "window with space");

    expect(fetchMock).toHaveBeenCalledWith("http://127.0.0.1:8080/control/windows/window%20with%20space/active", {
      method: "POST"
    });
  });

  it("sends media transport controls as bodyless POSTs", async () => {
    const fetchMock = vi.fn().mockResolvedValue(new Response("{}", { status: 200, statusText: "OK" }));
    vi.stubGlobal("fetch", fetchMock);

    await controlMedia("http://127.0.0.1:8080/", "next");

    expect(fetchMock).toHaveBeenCalledWith("http://127.0.0.1:8080/control/media/current/next", {
      method: "POST"
    });
  });

  it("sends media seek as JSON", async () => {
    const fetchMock = vi.fn().mockResolvedValue(new Response("{}", { status: 200, statusText: "OK" }));
    vi.stubGlobal("fetch", fetchMock);

    await seekMedia("http://127.0.0.1:8080/", 42000);

    expect(fetchMock).toHaveBeenCalledWith("http://127.0.0.1:8080/control/media/current/seek", {
      method: "POST",
      headers: {
        "Content-Type": "application/json"
      },
      body: JSON.stringify({ positionMs: 42000 })
    });
  });
});
