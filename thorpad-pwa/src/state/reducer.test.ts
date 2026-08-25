import { describe, expect, it } from "vitest";
import { emptyBridgeState } from "@/bridge/protocol";
import { bridgeReducer, initialRuntimeState } from "./reducer";

describe("bridgeReducer", () => {
  it("ignores incrementals before an authoritative snapshot", () => {
    const result = bridgeReducer(initialRuntimeState, { type: "message", message: { type: "player.money", data: { copper: 999 } } });
    expect(result).toBe(initialRuntimeState);
  });

  it("replaces snapshots and applies slot updates", () => {
    const snapshot = { ...emptyBridgeState(), game: { state: "world" as const } };
    const ready = bridgeReducer(initialRuntimeState, { type: "message", message: { type: "state.snapshot", data: snapshot } });
    const updated = { slot: 8, empty: false as const, kind: "spell", id: 1, name: "Fireball", icon: "", text: "", count: 0, usable: true, insufficientResource: false, inRange: true, current: false, equipped: false, cooldown: { active: false, durationMs: 0, remainingMs: 0 } };
    const result = bridgeReducer(ready, { type: "message", message: { type: "action.updated", data: updated } });
    expect(result.bridgeState.actions.slots[7]).toEqual(updated);
    expect(result.bridgeState.actions.slots).toHaveLength(24);
  });

  it("clears stale bridge state while reconnecting", () => {
    const ready = bridgeReducer(initialRuntimeState, { type: "message", message: { type: "state.snapshot", data: { ...emptyBridgeState(), game: { state: "world" } } } });
    const result = bridgeReducer(ready, { type: "reset", connection: "reconnecting" });
    expect(result.hasSnapshot).toBe(false);
    expect(result.bridgeState.player).toBeNull();
  });
});
