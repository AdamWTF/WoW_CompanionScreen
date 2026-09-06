import { PartyMember, PartyResourceType } from "@/bridge/protocol";

const CLASS_COLORS: Record<string, string> = {
  DEATHKNIGHT: "#c41f3b", DRUID: "#ff7d0a", HUNTER: "#abd473", MAGE: "#69ccf0", PALADIN: "#f58cba",
  PRIEST: "#ffffff", ROGUE: "#fff569", SHAMAN: "#0070de", WARLOCK: "#9482c9", WARRIOR: "#c79c6e",
};

const RESOURCE_COLORS: Record<PartyResourceType, string> = {
  mana: "#2679d8", rage: "#b52222", energy: "#d8bd21", "runic-power": "#36c9d8", unknown: "#777064",
};

const RACES: Record<string, string> = {
  bloodelf: "bloodelf", draenei: "draenei", dwarf: "dwarf", gnome: "gnome", human: "human",
  nightelf: "nightelf", orc: "orc", scourge: "undead", tauren: "tauren", troll: "troll", undead: "undead",
};

export function classColor(token: string) { return CLASS_COLORS[token.toUpperCase()] ?? "#b3a58b"; }
export function resourceColor(type: PartyResourceType) { return RESOURCE_COLORS[type]; }

export function partyPortraitPath(member: Pick<PartyMember, "race" | "sex">) {
  const race = RACES[member.race.token.toLowerCase().replace(/[^a-z]/g, "")];
  if (!race || (member.sex !== "male" && member.sex !== "female")) return "/assets/wow-icons/inv_misc_questionmark.webp";
  return `/assets/wow-icons/achievement_character_${race}_${member.sex}.webp`;
}

export function partyStatuses(member: PartyMember) {
  const statuses: string[] = [];
  if (!member.connected) statuses.push("Offline");
  else if (member.ghost) statuses.push("Ghost");
  else if (member.dead) statuses.push("Dead");
  if (member.afk) statuses.push("AFK");
  if (member.dnd) statuses.push("DND");
  return statuses;
}

export function percent(current: number, maximum: number) {
  return maximum > 0 ? Math.max(0, Math.min(100, current / maximum * 100)) : 0;
}
