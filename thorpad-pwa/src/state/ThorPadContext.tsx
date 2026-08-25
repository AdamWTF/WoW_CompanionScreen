"use client";

import { createContext, Dispatch, ReactNode, useCallback, useContext, useEffect, useMemo, useReducer, useRef, useState } from "react";
import { ThorBridgeClient } from "@/bridge/ThorBridgeClient";
import { BridgeMessage, Modifier, ShortcutBinding, ThorPadPreferences } from "@/bridge/protocol";
import { getAuthToken, setAuthToken } from "@/persistence/credentials";
import { defaultPreferences, loadPreferences, savePreferences } from "@/persistence/preferences";
import { bridgeReducer, initialRuntimeState, RuntimeAction, RuntimeState } from "./reducer";

interface ThorPadContextValue {
  runtime: RuntimeState;
  preferences: ThorPadPreferences;
  updatePreferences(patch: Partial<ThorPadPreferences>): void;
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

const Context = createContext<ThorPadContextValue | null>(null);

export function ThorPadProvider({ children }: { children: ReactNode }) {
  const [runtime, dispatch] = useReducer(bridgeReducer, initialRuntimeState);
  const [preferences, setPreferences] = useState<ThorPadPreferences>(() => defaultPreferences());
  const [hydrated, setHydrated] = useState(false);
  const clientRef = useRef<ThorBridgeClient | null>(null);

  useEffect(() => { setPreferences(loadPreferences()); setHydrated(true); }, []);
  useEffect(() => { if (hydrated) savePreferences(preferences); }, [hydrated, preferences]);

  useEffect(() => {
    const client = new ThorBridgeClient({
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
  }, []);

  useEffect(() => {
    if (!hydrated) return;
    if (preferences.hostIp) clientRef.current?.connect(preferences.hostIp);
    else dispatch({ type: "reset", connection: "unconfigured" });
    return () => clientRef.current?.disconnect();
  }, [hydrated, preferences.hostIp]);

  const updatePreferences = useCallback((patch: Partial<ThorPadPreferences>) => setPreferences((current) => ({ ...current, ...patch })), []);
  const setBinding = useCallback((name: string, binding: ShortcutBinding) => setPreferences((current) => ({
    ...current,
    shortcutBindings: { ...current.shortcutBindings, [name]: binding },
  })), []);

  const value = useMemo<ThorPadContextValue>(() => ({
    runtime,
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
  }), [runtime, preferences, updatePreferences, setBinding]);

  return <Context.Provider value={value}>{children}</Context.Provider>;
}

export function useThorPad() {
  const value = useContext(Context);
  if (!value) throw new Error("useThorPad must be used inside ThorPadProvider");
  return value;
}
