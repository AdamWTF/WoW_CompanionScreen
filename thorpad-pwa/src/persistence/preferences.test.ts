import { afterEach, describe, expect, it, vi } from "vitest";
import { defaultPreferences, loadPreferences, shortcutNames } from "./preferences";

afterEach(() => vi.unstubAllGlobals());

describe("preferences", () => {
  it("shows every WoW shortcut by default", () => {
    const preferences = defaultPreferences();
    expect(shortcutNames.every((name) => preferences.shortcutVisibility[name])).toBe(true);
  });

  it("adds visibility defaults to older saved preferences", () => {
    vi.stubGlobal("window", {});
    vi.stubGlobal("localStorage", { getItem: () => JSON.stringify({ shortcutBindings: { Player: { key: "X", modifiers: [] } } }) });
    const preferences = loadPreferences();
    expect(preferences.shortcutBindings.Player.key).toBe("X");
    expect(shortcutNames.every((name) => preferences.shortcutVisibility[name])).toBe(true);
  });

  it("preserves explicitly hidden shortcuts while defaulting new ones to visible", () => {
    vi.stubGlobal("window", {});
    vi.stubGlobal("localStorage", { getItem: () => JSON.stringify({ shortcutVisibility: { Settings: false } }) });
    const preferences = loadPreferences();
    expect(preferences.shortcutVisibility.Settings).toBe(false);
    expect(preferences.shortcutVisibility.Player).toBe(true);
  });
});
