import { describe, expect, it } from "vitest";

import { controlPath } from "./controls";

describe("control helpers", () => {
  it("URL-encodes device ids in control paths", () => {
    expect(controlPath("sinks", "sink with space", "volume")).toBe("/control/audio/sinks/sink%20with%20space/volume");
  });
});
