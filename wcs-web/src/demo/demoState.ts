import { Action, BridgeState } from "@/bridge/protocol";
import { RuntimeState } from "@/state/reducer";

const demoActions = [
  ["Lightning Bolt", "spell_nature_lightning"],
  ["Chain Lightning", "spell_nature_chainlightning"],
  ["Healing Wave", "spell_nature_healingwavegreater"],
  ["Earthbind Totem", "spell_nature_earthbind"],
  ["Flame Shock", "spell_fire_flameshock"],
  ["Rockbiter Weapon", "spell_nature_rockbiter"],
  ["Ambush", "ability_ambush"],
  ["Backstab", "ability_backstab"],
  ["Cheap Shot", "ability_cheapshot"],
  ["Kick", "ability_kick"],
  ["Healing Potion", "inv_potion_01"],
  ["Conjured Food", "inv_misc_food_15"],
] as const;

function action(slot: number, name: string, icon: string): Action {
  return {
    slot,
    empty: false,
    kind: slot > 10 ? "item" : "spell",
    id: 10_000 + slot,
    name,
    icon,
    text: "",
    count: slot === 11 ? 5 : slot === 12 ? 20 : 0,
    usable: slot !== 9,
    insufficientResource: slot === 6,
    inRange: slot === 8 ? false : true,
    current: slot === 5,
    equipped: slot === 6,
    cooldown: slot === 4
      ? { active: true, durationMs: 30_000, remainingMs: 18_000 }
      : { active: false, durationMs: 0, remainingMs: 0 },
  };
}

export function createDemoBridgeState(): BridgeState {
  const populated = demoActions.map(([name, icon], index) => action(index + 1, name, icon));
  const empty = Array.from({ length: 12 }, (_, index): Action => ({ slot: index + 13, empty: true }));
  return {
    game: { state: "world" },
    player: {
      name: "Stormcaller",
      level: 72,
      money: 128_475_39,
      experience: { level: 72, current: 834_200, required: 1_520_000, rested: 218_000, capped: false },
      bags: { used: 61, total: 88, free: 27 },
    },
    actions: { slots: [...populated, ...empty] },
  };
}

export function createDemoRuntimeState(): RuntimeState {
  return {
    connectionState: "connected",
    sessionState: "ready",
    bridgeState: createDemoBridgeState(),
    hasSnapshot: true,
    error: null,
    touchpadWarning: null,
  };
}

export function isDemoRequested(search: string) {
  return new URLSearchParams(search).has("demo");
}
