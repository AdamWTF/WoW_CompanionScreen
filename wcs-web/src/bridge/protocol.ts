export type GameState = "login" | "loading" | "world" | "character-select";
export type Modifier = "SHIFT" | "CTRL" | "ALT";

export interface ShortcutBinding { key: string; modifiers: Modifier[] }
export interface Experience { level: number; current: number; required: number; rested: number; capped: boolean }
export interface Bags { used: number; total: number; free?: number }
export interface PlayerState { name: string; level: number; money: number; experience: Experience; bags: Bags }
export interface Cooldown { active: boolean; durationMs: number; remainingMs: number }

export interface EmptyAction { slot: number; empty: true }
export interface PopulatedAction {
  slot: number;
  empty: false;
  kind: string;
  id: number;
  name: string;
  icon: string;
  text: string;
  count: number;
  usable: boolean;
  insufficientResource: boolean;
  inRange: boolean | null;
  current: boolean;
  equipped: boolean;
  cooldown: Cooldown;
}
export type Action = EmptyAction | PopulatedAction;

export interface BridgeState {
  game: { state: GameState };
  player: PlayerState | null;
  actions: { slots: Action[] };
}

export type ConnectionState = "unconfigured" | "connecting" | "connected" | "reconnecting" | "disconnected" | "error" | "busy";
export type SessionState = "idle" | "negotiating" | "pairing" | "authenticating" | "awaiting-snapshot" | "ready" | "auth-missing" | "auth-failed" | "protocol-mismatch";

export interface CompanionScreenPreferences {
  hostIp: string | null;
  deviceId: string;
  deviceName: string;
  authToken: string | null;
  shortcutBindings: Record<string, ShortcutBinding>;
  shortcutVisibility: Record<string, boolean>;
  pointerSensitivity: number;
  scrollSensitivity: number;
  hapticsEnabled: boolean;
  uiScale: number;
}

export interface BridgeMessage { type: string; data?: unknown; code?: string; token?: string; [key: string]: unknown }

export const emptyBridgeState = (): BridgeState => ({
  game: { state: "login" },
  player: null,
  actions: { slots: Array.from({ length: 24 }, (_, i) => ({ slot: i + 1, empty: true as const })) },
});

export const NAMED_KEYS = new Set([
  "BACKSPACE", "TAB", "ENTER", "ESCAPE", "SPACE", "PAGEUP", "PAGEDOWN", "END", "HOME",
  "LEFT", "UP", "RIGHT", "DOWN", "INSERT", "DELETE", "SHIFT", "CTRL", "ALT",
  ...Array.from({ length: 12 }, (_, i) => `F${i + 1}`),
]);

export function isSupportedKey(key: string) {
  return (key.length === 1 && key.charCodeAt(0) <= 0x7f) || NAMED_KEYS.has(key);
}
