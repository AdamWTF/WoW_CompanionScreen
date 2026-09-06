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

  it("uses a later full snapshot to repair stale incremental state", () => {
    const first = { ...emptyBridgeState(), game: { state: "world" as const } };
    const ready = bridgeReducer(initialRuntimeState, { type: "message", message: { type: "state.snapshot", data: first } });
    const stale = { slot: 3, empty: false as const, kind: "spell", id: 7, name: "Stale", icon: "", text: "", count: 0, usable: true, insufficientResource: false, inRange: true, current: false, equipped: false, cooldown: { active: false, durationMs: 0, remainingMs: 0 } };
    const incremented = bridgeReducer(ready, { type: "message", message: { type: "action.updated", data: stale } });
    const repaired = bridgeReducer(incremented, { type: "message", message: { type: "state.snapshot", data: first } });
    expect(repaired.bridgeState.actions.slots[2]).toEqual({ slot: 3, empty: true });
    expect(repaired.bridgeState.actions.slots).toHaveLength(24);
  });

  it("defaults old snapshots to an empty party and applies party replacement events", () => {
    const legacy = emptyBridgeState() as unknown as Record<string, unknown>;
    delete legacy.party;
    const ready = bridgeReducer(initialRuntimeState, { type: "message", message: { type: "state.snapshot", data: legacy } });
    expect(ready.bridgeState.party.members).toEqual([]);
    const party = { members: [{ slot: 1, guid: "0x0000000000000001", name: "Member" }] } as never;
    const updated = bridgeReducer(ready, { type: "message", message: { type: "party.state", data: party } });
    expect(updated.bridgeState.party).toEqual(party);
  });
});
