"use client";

import { createContext, Dispatch, ReactNode, useCallback, useContext, useEffect, useMemo, useReducer, useRef, useState } from "react";
import { WcsBridgeClient } from "@/bridge/WcsBridgeClient";
import { BridgeMessage, Modifier, ShortcutBinding, CompanionScreenPreferences } from "@/bridge/protocol";
import { getAuthToken, setAuthToken } from "@/persistence/credentials";
import { defaultPreferences, loadPreferences, savePreferences } from "@/persistence/preferences";
import { createDemoRuntimeState, isDemoRequested } from "@/demo/demoState";
import { bridgeReducer, initialRuntimeState, RuntimeAction, RuntimeState } from "./reducer";

interface CompanionScreenContextValue {
  runtime: RuntimeState;
  demoMode: boolean;
  preferences: CompanionScreenPreferences;
  updatePreferences(patch: Partial<CompanionScreenPreferences>): void;
  setBinding(name: string, binding: ShortcutBinding): void;
  pair(code: string): void;
  retry(): void;
  pressAction(slot: number): void;
  pressKey(key: string, modifiers?: Modifier[]): void;
  movePointer(dx: number, dy: number): void;
  clickPointer(button: "left" | "right" | "middle"): void;
  pointerDown(button: "left" | "right"): void;
  pointerUp(button: "left" | "right"): void;
  scrollPointer(delta: number): void;
  dispatch: Dispatch<RuntimeAction>;
}

const Context = createContext<CompanionScreenContextValue | null>(null);

export function CompanionScreenProvider({ children }: { children: ReactNode }) {
  const [runtime, dispatch] = useReducer(bridgeReducer, initialRuntimeState);
  const [preferences, setPreferences] = useState<CompanionScreenPreferences>(() => defaultPreferences());
  const [hydrated, setHydrated] = useState(false);
  const [demoMode, setDemoMode] = useState(false);
  const clientRef = useRef<WcsBridgeClient | null>(null);

  useEffect(() => {
    setPreferences(loadPreferences());
    if (isDemoRequested(window.location.search)) {
      setDemoMode(true);
      dispatch({ type: "message", message: { type: "state.snapshot", data: createDemoRuntimeState().bridgeState } });
    }
    setHydrated(true);
  }, []);
  useEffect(() => { if (hydrated) savePreferences(preferences); }, [hydrated, preferences]);

  useEffect(() => {
    if (!hydrated || demoMode) return;
    const client = new WcsBridgeClient({
      onOpen: () => dispatch({ type: "connection", connection: "connecting", session: "negotiating" }),
      onDisconnect: (reconnecting) => dispatch({ type: "reset", connection: reconnecting ? "reconnecting" : "connecting" }),
      onToken: async (token) => { await setAuthToken(token); },
      getToken: getAuthToken,
      onAuthMissing: () => dispatch({ type: "connection", connection: "error", session: "auth-missing" }),
      onMessage: (message: BridgeMessage) => {
        if (message.type === "error") {
          const code = message.code ?? "unknown";
          if (code === "client-busy") dispatch({ type: "connection", connection: "busy", session: "idle" });
          else if (code === "protocol-mismatch") dispatch({ type: "connection", connection: "error", session: "protocol-mismatch" });
          else if (code === "auth-failed") dispatch({ type: "connection", connection: "error", session: "auth-failed" });
          else if (code === "game-not-foreground") dispatch({ type: "warning", warning: "WoW must be the active window for touchpad input." });
          else if (code === "game-window-unavailable") dispatch({ type: "warning", warning: "World of Warcraft could not be found." });
          else if (code === "queue-full") dispatch({ type: "warning", warning: "The bridge is busy. Input was not resent." });
          else if (code === "auth-required") dispatch({ type: "error", error: "That pairing code was not accepted. Check the code in WoW and try again." });
          else dispatch({ type: "error", error: `Bridge error: ${code}.` });
        }
        dispatch({ type: "message", message });
      },
    });
    clientRef.current = client;
    return () => { client.disconnect(); clientRef.current = null; };
  }, [hydrated, demoMode]);

  useEffect(() => {
    if (!hydrated || demoMode) return;
    if (preferences.hostIp) clientRef.current?.connect(preferences.hostIp);
    else dispatch({ type: "reset", connection: "unconfigured" });
    return () => clientRef.current?.disconnect();
  }, [hydrated, demoMode, preferences.hostIp]);

  const updatePreferences = useCallback((patch: Partial<CompanionScreenPreferences>) => setPreferences((current) => ({ ...current, ...patch })), []);
  const setBinding = useCallback((name: string, binding: ShortcutBinding) => setPreferences((current) => ({
    ...current,
    shortcutBindings: { ...current.shortcutBindings, [name]: binding },
  })), []);

  const value = useMemo<CompanionScreenContextValue>(() => ({
    runtime,
    demoMode,
    preferences,
    updatePreferences,
    setBinding,
    pair: (code) => clientRef.current?.pair(code, { id: preferences.deviceId, name: preferences.deviceName }),
    retry: () => clientRef.current?.retry(),
    pressAction: (slot) => clientRef.current?.pressAction(slot),
    pressKey: (key, modifiers) => clientRef.current?.pressKey(key, modifiers),
    movePointer: (dx, dy) => clientRef.current?.movePointer(dx, dy),
    clickPointer: (button) => clientRef.current?.clickPointer(button),
    pointerDown: (button) => clientRef.current?.pointerDown(button),
    pointerUp: (button) => clientRef.current?.pointerUp(button),
    scrollPointer: (delta) => clientRef.current?.scrollPointer(delta),
    dispatch,
  }), [runtime, demoMode, preferences, updatePreferences, setBinding]);

  return <Context.Provider value={value}>{children}</Context.Provider>;
}

export function useCompanionScreen() {
  const value = useContext(Context);
  if (!value) throw new Error("useCompanionScreen must be used inside CompanionScreenProvider");
  return value;
}
