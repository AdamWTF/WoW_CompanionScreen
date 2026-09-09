"use client";
import React, { useEffect, useState } from "react";
import { Delete, CornerDownLeft, ArrowUp, ArrowDown, ArrowLeft, ArrowRight } from "lucide-react";
import { Modifier } from "@/bridge/protocol";
import { useCompanionScreen } from "@/state/CompanionScreenContext";

const basicRows = [
  [..."1234567890"], [..."QWERTYUIOP"], [..."ASDFGHJKL", "BACKSPACE"],
  ["SHIFT", ..."ZXCVBNM", ",", "."], ["ESCAPE", "TAB", "/", "SPACE", "ENTER"],
];
const advancedRows = [
  Array.from({ length: 6 }, (_, i) => "F" + (i + 1)),
  Array.from({ length: 6 }, (_, i) => "F" + (i + 7)),
  [String.fromCharCode(96), ..." -=[]\\;',./".trim()],
  ["INSERT", "DELETE", "HOME", "END", "PAGEUP", "PAGEDOWN"],
  ["SHIFT", "CTRL", "ALT", "ESCAPE", "UP", "BACKSPACE"],
  ["TAB", "SPACE", "ENTER", "LEFT", "DOWN", "RIGHT"],
];
const shifted = Object.fromEntries(
  [..."1234567890", String.fromCharCode(96), ..."-=[]\\;',./"].map((key, index) => [key, [...'!@#$%^&*()~_+{}|:"<>?'][index]])
);
const labels: Record<string, string> = { ESCAPE: "Esc", TAB: "Tab", SPACE: "Space", ENTER: "Enter", BACKSPACE: "Backspace", INSERT: "Insert", DELETE: "Delete", HOME: "Home", END: "End", PAGEUP: "Page Up", PAGEDOWN: "Page Down", SHIFT: "Shift", CTRL: "Ctrl", ALT: "Alt", UP: "Up", DOWN: "Down", LEFT: "Left", RIGHT: "Right" };
const icons: Record<string, React.ReactNode> = { BACKSPACE: <Delete />, ENTER: <CornerDownLeft />, UP: <ArrowUp />, DOWN: <ArrowDown />, LEFT: <ArrowLeft />, RIGHT: <ArrowRight /> };
const isModifier = (key: string): key is Modifier => key === "SHIFT" || key === "CTRL" || key === "ALT";

export function RemoteKeyboard() {
  const { runtime, pressKey, preferences } = useCompanionScreen();
  const [mode, setMode] = useState<"basic" | "advanced">("basic");
  const [modifiers, setModifiers] = useState<Modifier[]>([]);
  const enabled = runtime.connectionState === "connected" && runtime.sessionState === "ready" && runtime.hasSnapshot;
  useEffect(() => { if (!enabled) setModifiers([]); }, [enabled]);
  const press = (key: string) => {
    if (!enabled) return;
    if (isModifier(key)) {
      setModifiers((current) => current.includes(key) ? current.filter((item) => item !== key) : [...current, key]);
      return;
    }
    pressKey(key, modifiers);
    if (preferences.hapticsEnabled) navigator.vibrate?.(8);
    setModifiers([]);
  };
  const label = (key: string) => labels[key] ?? (/^F[0-9]+$/.test(key) ? key : modifiers.includes("SHIFT") ? shifted[key] ?? key : key.toLowerCase());
  const weight = (key: string) => mode === "advanced" ? 1 : key === "SPACE" ? 3 : key === "ENTER" ? 2 : key === "SHIFT" || key === "BACKSPACE" ? 1.5 : 1;
  return <section className="keyboard-page">
    <header className="keyboard-header"><h1>Keyboard</h1><div className="keyboard-modes" role="group" aria-label="Keyboard layout">
      {(["basic", "advanced"] as const).map((choice) => <button key={choice} aria-pressed={mode === choice} onClick={() => setMode(choice)}>{choice === "basic" ? "Basic" : "Advanced"}</button>)}
    </div></header>
    <div className={"keyboard-board " + mode}>
      {(mode === "basic" ? basicRows : advancedRows).map((row, index) => <div className="keyboard-row" key={mode + index}>
        {row.map((key) => <button key={key} type="button" className={"keyboard-key" + (isModifier(key) ? " modifier" : "")} style={{ flexGrow: weight(key) }}
          disabled={!enabled} aria-label={label(key)} aria-pressed={isModifier(key) ? modifiers.includes(key) : undefined} data-key={key} onClick={() => press(key)}>
          {icons[key] ? <span aria-hidden="true" className="keyboard-key-icon">{icons[key]}</span> : <span>{label(key)}</span>}
        </button>)}
      </div>)}
    </div>
    <p className="keyboard-hint" role="status">{!enabled ? "Connect to WoW to use the keyboard." : modifiers.length ? modifiers.join(" + ") + " — next key only" : "Enter opens chat • Enter again sends • Esc cancels"}</p>
  </section>;
}
