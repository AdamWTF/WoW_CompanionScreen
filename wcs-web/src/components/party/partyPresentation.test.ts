import { describe, expect, it } from "vitest";
import { PartyMember } from "@/bridge/protocol";
import { classColor, partyPortraitPath, partyStatuses, percent, resourceColor } from "./partyPresentation";

const member: PartyMember = {
  slot: 1, unit: "party1", guid: "0x0000000000000001", name: "Tester", level: 80,
  class: { name: "Death Knight", token: "DEATHKNIGHT" }, race: { name: "Undead", token: "Scourge" }, sex: "female",
  health: { current: 50, maximum: 100 }, resource: { type: "runic-power", current: 60, maximum: 100 },
  targeted: false, leader: false, connected: true, dead: false, ghost: false, afk: false, dnd: false,
};

describe("party presentation", () => {
  it("maps Wrath class, resource, race and sex values", () => {
    expect(classColor(member.class.token)).toBe("#c41f3b");
    expect(resourceColor(member.resource.type)).toBe("#36c9d8");
    expect(partyPortraitPath(member)).toContain("achievement_character_undead_female.webp");
  });

  it("falls back when no matching portrait is available", () => {
    expect(partyPortraitPath({ race: { name: "", token: "UNKNOWN" }, sex: "unknown" })).toContain("inv_misc_questionmark.webp");
  });

  it("prioritizes offline over life state and keeps activity flags", () => {
    expect(partyStatuses({ ...member, connected: false, dead: true, afk: true })).toEqual(["Offline", "AFK"]);
  });

  it("clamps percentages and handles unknown maxima", () => {
    expect(percent(125, 100)).toBe(100);
    expect(percent(10, 0)).toBe(0);
  });
});
