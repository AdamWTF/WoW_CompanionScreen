import { describe, expect, it } from "vitest";
import { resolveWowIcon } from "./resolveWowIcon";

describe("resolveWowIcon", () => {
  it("normalises case and slash direction", () => {
    expect(resolveWowIcon("Interface\\Icons\\Spell_Fire_FlameBolt")).toBe("/assets/wow-icons/spell_fire_flamebolt.webp");
    expect(resolveWowIcon("Interface/Icons/INV_Misc_Bag_08")).toBe("/assets/wow-icons/inv_misc_bag_08.webp");
  });

  it("uses the fallback for an empty path", () => {
    expect(resolveWowIcon("")).toBe("/icons/action-fallback.svg");
  });
});
