import { describe, expect, it } from "vitest";
import { normalizeBasePath, withBasePath } from "./basePath";

describe("deployment base path", () => {
  it("keeps root-hosted asset paths unchanged", () => {
    expect(normalizeBasePath("/")).toBe("");
    expect(withBasePath("/icons/wcs.svg", "")).toBe("/icons/wcs.svg");
  });

  it("normalizes and prefixes project-hosted asset paths", () => {
    expect(normalizeBasePath("WoW_CompanionScreen/")).toBe("/WoW_CompanionScreen");
    expect(withBasePath("/icons/wcs.svg", "/WoW_CompanionScreen/")).toBe("/WoW_CompanionScreen/icons/wcs.svg");
  });
});
