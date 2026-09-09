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

export function createDemoBridgeState(permission: "leader" | "member" | "dungeon-finder" = "leader"): BridgeState {
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
    party: {
      groupType: permission === "dungeon-finder" ? "dungeon-finder" : "party",
      generation: `demo-${permission}`, canRemove: permission === "leader", canPromote: permission === "leader", canLeave: true,
      members: [
        { slot: 1, unit: "party1", guid: "0x0000000000001001", name: "Ironward", level: 72, class: { name: "Warrior", token: "WARRIOR" }, race: { name: "Dwarf", token: "Dwarf" }, sex: "male", health: { current: 18420, maximum: 22100 }, resource: { type: "rage", current: 64, maximum: 100 }, targeted: true, leader: permission !== "leader", connected: true, dead: false, ghost: false, afk: false, dnd: false },
        { slot: 2, unit: "party2", guid: "0x0000000000001002", name: "Moonbloom", level: 71, class: { name: "Druid", token: "DRUID" }, race: { name: "Night Elf", token: "NightElf" }, sex: "female", health: { current: 12450, maximum: 15600 }, resource: { type: "mana", current: 11280, maximum: 14800 }, targeted: false, leader: false, connected: true, dead: false, ghost: false, afk: false, dnd: false },
        { slot: 3, unit: "party3", guid: "0x0000000000001003", name: "Cogspinner", level: 72, class: { name: "Rogue", token: "ROGUE" }, race: { name: "Gnome", token: "Gnome" }, sex: "male", health: { current: 0, maximum: 14100 }, resource: { type: "energy", current: 100, maximum: 100 }, targeted: false, leader: false, connected: true, dead: true, ghost: false, afk: false, dnd: false },
        { slot: 4, unit: "party4", guid: "0x0000000000001004", name: "Emberveil", level: 70, class: { name: "Mage", token: "MAGE" }, race: { name: "Blood Elf", token: "BloodElf" }, sex: "female", health: { current: 9800, maximum: 12100 }, resource: { type: "mana", current: 7200, maximum: 13200 }, targeted: false, leader: false, connected: false, dead: false, ghost: false, afk: true, dnd: false },
      ],
    },
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
