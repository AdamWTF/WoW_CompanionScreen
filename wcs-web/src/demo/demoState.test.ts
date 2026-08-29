import { describe, expect, it } from "vitest";
import { createDemoRuntimeState, isDemoRequested } from "./demoState";

describe("demo state", () => {
  it("provides a ready, representative bridge snapshot", () => {
    const runtime = createDemoRuntimeState();
    expect(runtime).toMatchObject({ connectionState: "connected", sessionState: "ready", hasSnapshot: true });
    expect(runtime.bridgeState.game.state).toBe("world");
    expect(runtime.bridgeState.player?.name).toBeTruthy();
    expect(runtime.bridgeState.actions.slots).toHaveLength(24);
    expect(runtime.bridgeState.actions.slots.some((action) => !action.empty)).toBe(true);
    expect(runtime.bridgeState.actions.slots.some((action) => action.empty)).toBe(true);
  });

  it("enables demo mode whenever the query flag is present", () => {
    expect(isDemoRequested("?demo")).toBe(true);
    expect(isDemoRequested("?demo=1")).toBe(true);
    expect(isDemoRequested("?other")).toBe(false);
  });
});
