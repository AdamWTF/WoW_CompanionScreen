import { ShortcutBinding, CompanionScreenPreferences } from "@/bridge/protocol";

export const shortcutNames = ["Player", "Bags", "Spell Book", "Talents", "Achievements", "Quests", "PvP", "Dungeon Finder", "Settings"] as const;
export type ShortcutName = typeof shortcutNames[number];

export const defaultBindings: Record<ShortcutName, ShortcutBinding> = {
  Player: { key: "C", modifiers: [] },
  Bags: { key: "B", modifiers: [] },
  "Spell Book": { key: "P", modifiers: [] },
  Talents: { key: "N", modifiers: [] },
  Achievements: { key: "Y", modifiers: [] },
  Quests: { key: "L", modifiers: [] },
  PvP: { key: "H", modifiers: [] },
  "Dungeon Finder": { key: "I", modifiers: [] },
  Settings: { key: "ESCAPE", modifiers: [] },
};

export const defaultShortcutVisibility: Record<ShortcutName, boolean> = Object.fromEntries(
  shortcutNames.map((name) => [name, true]),
) as Record<ShortcutName, boolean>;

const STORAGE_KEY = "wcs.preferences.v1";
const makeId = () => typeof crypto !== "undefined" && "randomUUID" in crypto ? crypto.randomUUID() : `wcs-${Date.now()}-${Math.random().toString(16).slice(2)}`;

export function defaultPreferences(): CompanionScreenPreferences {
  return {
    hostIp: null,
    deviceId: makeId(),
    deviceName: "AYN Thor",
    authToken: null,
    shortcutBindings: defaultBindings,
    shortcutVisibility: defaultShortcutVisibility,
    pointerSensitivity: 1,
    scrollSensitivity: 1,
    hapticsEnabled: true,
    uiScale: 1,
  };
}

export function loadPreferences(): CompanionScreenPreferences {
  const defaults = defaultPreferences();
  if (typeof window === "undefined") return defaults;
  try {
    const saved = JSON.parse(localStorage.getItem(STORAGE_KEY) ?? "{}");
    return {
      ...defaults,
      ...saved,
      authToken: null,
      shortcutBindings: { ...defaultBindings, ...(saved.shortcutBindings ?? {}) },
      shortcutVisibility: { ...defaultShortcutVisibility, ...(saved.shortcutVisibility ?? {}) },
    };
  } catch { return defaults; }
}

export function savePreferences(preferences: CompanionScreenPreferences) {
  const { authToken: _, ...safe } = preferences;
  localStorage.setItem(STORAGE_KEY, JSON.stringify(safe));
}

export function validIpv4(value: string) {
  const parts = value.trim().split(".");
  return parts.length === 4 && parts.every((part) => /^\d{1,3}$/.test(part) && Number(part) <= 255);
}
