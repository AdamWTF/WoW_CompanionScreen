import { BridgeMessage, BridgeState, ConnectionState, emptyBridgeState, SessionState } from "@/bridge/protocol";

export interface RuntimeState {
  connectionState: ConnectionState;
  sessionState: SessionState;
  bridgeState: BridgeState;
  hasSnapshot: boolean;
  error: string | null;
  touchpadWarning: string | null;
}

export const initialRuntimeState: RuntimeState = {
  connectionState: "unconfigured",
  sessionState: "idle",
  bridgeState: emptyBridgeState(),
  hasSnapshot: false,
  error: null,
  touchpadWarning: null,
};

export type RuntimeAction =
  | { type: "connection"; connection: ConnectionState; session?: SessionState }
  | { type: "message"; message: BridgeMessage }
  | { type: "reset"; connection: ConnectionState }
  | { type: "error"; error: string | null }
  | { type: "warning"; warning: string | null };

export function bridgeReducer(state: RuntimeState, action: RuntimeAction): RuntimeState {
  if (action.type === "warning") return { ...state, touchpadWarning: action.warning };
  if (action.type === "error") return { ...state, error: action.error };
  if (action.type === "reset") return { ...initialRuntimeState, connectionState: action.connection };
  if (action.type === "connection") return {
    ...state,
    connectionState: action.connection,
    sessionState: action.session ?? state.sessionState,
    ...(action.connection === "connected" ? {} : { hasSnapshot: false, bridgeState: emptyBridgeState() }),
  };

  const message = action.message;
  switch (message.type) {
    case "pairing.required": return { ...state, sessionState: "pairing" };
    case "auth.required": return { ...state, sessionState: "authenticating" };
    case "auth.ok": return { ...state, sessionState: "awaiting-snapshot" };
    case "state.snapshot":
      return {
        ...state,
        bridgeState: message.data as BridgeState,
        hasSnapshot: true,
        connectionState: "connected",
        sessionState: "ready",
        error: null,
      };
    case "player.state":
      if (!state.hasSnapshot || !state.bridgeState.player) return state;
      return { ...state, bridgeState: { ...state.bridgeState, player: { ...state.bridgeState.player, ...(message.data as Partial<BridgeState["player"]>) } } };
    case "player.money":
      if (!state.hasSnapshot || !state.bridgeState.player) return state;
      return { ...state, bridgeState: { ...state.bridgeState, player: { ...state.bridgeState.player, money: (message.data as { copper: number }).copper } } };
    case "player.experience":
    case "player.bags": {
      if (!state.hasSnapshot || !state.bridgeState.player) return state;
      const field = message.type === "player.experience" ? "experience" : "bags";
      return { ...state, bridgeState: { ...state.bridgeState, player: { ...state.bridgeState.player, [field]: message.data } } };
    }
    case "actions.state":
      if (!state.hasSnapshot) return state;
      return { ...state, bridgeState: { ...state.bridgeState, actions: message.data as BridgeState["actions"] } };
    case "action.updated": {
      if (!state.hasSnapshot) return state;
      const updated = message.data as BridgeState["actions"]["slots"][number];
      return { ...state, bridgeState: { ...state.bridgeState, actions: { slots: state.bridgeState.actions.slots.map((slot) => slot.slot === updated.slot ? updated : slot) } } };
    }
    default: return state;
  }
}
