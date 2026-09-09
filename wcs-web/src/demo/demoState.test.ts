import { describe, expect, it } from "vitest";
import { createDemoBridgeState, createDemoRuntimeState, isDemoRequested } from "./demoState";

describe("demo state", () => {
  it("represents leader, member and Dungeon Finder permissions", () => {
    expect(createDemoBridgeState().party).toMatchObject({ canRemove: true, canPromote: true, canLeave: true });
    expect(createDemoBridgeState("member").party).toMatchObject({ canRemove: false, canPromote: false, canLeave: true });
    expect(createDemoBridgeState("dungeon-finder").party).toMatchObject({ groupType: "dungeon-finder", canRemove: false, canPromote: false, canLeave: true });
  });
  it("provides a ready, representative bridge snapshot", () => {
    const runtime = createDemoRuntimeState();
    expect(runtime).toMatchObject({ connectionState: "connected", sessionState: "ready", hasSnapshot: true });
    expect(runtime.bridgeState.game.state).toBe("world");
    expect(runtime.bridgeState.player?.name).toBeTruthy();
    expect(runtime.bridgeState.actions.slots).toHaveLength(24);
    expect(runtime.bridgeState.actions.slots.some((action) => !action.empty)).toBe(true);
    expect(runtime.bridgeState.actions.slots.some((action) => action.empty)).toBe(true);
    expect(runtime.bridgeState.party.members).toHaveLength(4);
    expect(runtime.bridgeState.party.members.some((member) => member.targeted)).toBe(true);
  });

  it("enables demo mode whenever the query flag is present", () => {
    expect(isDemoRequested("?demo")).toBe(true);
    expect(isDemoRequested("?demo=1")).toBe(true);
    expect(isDemoRequested("?other")).toBe(false);
  });
});
