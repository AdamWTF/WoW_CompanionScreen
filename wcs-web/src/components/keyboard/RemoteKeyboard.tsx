"use client";

import { useState } from "react";
import { Delete, CornerDownLeft, Space } from "lucide-react";
import { Modifier } from "@/bridge/protocol";
import { useCompanionScreen } from "@/state/CompanionScreenContext";

const rows = ["1234567890", "QWERTYUIOP", "ASDFGHJKL", "ZXCVBNM", ",./;'-=[]\\"];
const named = ["ESCAPE", "TAB", "ENTER", "BACKSPACE", "INSERT", "DELETE", "HOME", "END", "PAGEUP", "PAGEDOWN", "UP", "LEFT", "DOWN", "RIGHT"];

export function RemoteKeyboard() {
  const { runtime, pressKey, preferences } = useCompanionScreen();
  const [mode, setMode] = useState<"basic" | "extended">("basic");
  const [modifiers, setModifiers] = useState<Modifier[]>([]);
  const enabled = runtime.sessionState === "ready" && runtime.hasSnapshot;

  const toggle = (modifier: Modifier) => setModifiers((current) => current.includes(modifier) ? current.filter((item) => item !== modifier) : [...current, modifier]);
  const press = (key: string) => {
    if (!enabled) return;
    pressKey(key, modifiers);
    if (preferences.hapticsEnabled) navigator.vibrate?.(8);
    setModifiers([]);
  };

  return (
    <section className="keyboard-page">
      <header className="page-heading"><div><p className="eyebrow">DIRECT INPUT</p><h1>Remote Keyboard</h1></div><div className="mode-switch"><button className={mode === "basic" ? "active" : ""} onClick={() => setMode("basic")}>Basic</button><button className={mode === "extended" ? "active" : ""} onClick={() => setMode("extended")}>Extended</button></div></header>
      <div className={!enabled ? "keyboard-board disabled" : "keyboard-board"}>
        {mode === "basic" ? <>
          {rows.map((row, index) => <div className="key-row" key={row}>{[...row].map((key) => <Key key={`${index}-${key}`} label={key} onClick={() => press(key)} />)}</div>)}
          <div className="key-row utility-row"><Key label="Esc" onClick={() => press("ESCAPE")} /><ModifierKey name="SHIFT" active={modifiers.includes("SHIFT")} onClick={() => toggle("SHIFT")} /><Key wide label="Space" icon={<Space />} onClick={() => press("SPACE")} /><Key label="Back" icon={<Delete />} onClick={() => press("BACKSPACE")} /><Key label="Enter" icon={<CornerDownLeft />} onClick={() => press("ENTER")} /></div>
        </> : <>
          <div className="function-row">{Array.from({ length: 12 }, (_, i) => <Key key={i} label={`F${i + 1}`} onClick={() => press(`F${i + 1}`)} />)}</div>
          <div className="extended-grid">{named.map((key) => <Key key={key} label={friendly(key)} onClick={() => press(key)} />)}</div>
          <div className="key-row utility-row"><ModifierKey name="SHIFT" active={modifiers.includes("SHIFT")} onClick={() => toggle("SHIFT")} /><ModifierKey name="CTRL" active={modifiers.includes("CTRL")} onClick={() => toggle("CTRL")} /><ModifierKey name="ALT" active={modifiers.includes("ALT")} onClick={() => toggle("ALT")} /><Key wide label="Space" onClick={() => press("SPACE")} /></div>
        </>}
      </div>
      <p className="keyboard-hint">{enabled ? modifiers.length ? `${modifiers.join(" + ")} latched — select a key` : "Tap Enter to open chat, type, then tap Enter again to send." : "Keyboard input is available after the bridge snapshot arrives."}</p>
    </section>
  );
}

function Key({ label, onClick, wide, icon }: { label: string; onClick(): void; wide?: boolean; icon?: React.ReactNode }) {
  return <button className={wide ? "key wide" : "key"} onClick={onClick}>{icon}{label}</button>;
}
function ModifierKey({ name, active, onClick }: { name: Modifier; active: boolean; onClick(): void }) {
  return <button className={active ? "key modifier active" : "key modifier"} aria-pressed={active} onClick={onClick}>{name}</button>;
}
function friendly(key: string) { return key.replace("PAGEUP", "Page Up").replace("PAGEDOWN", "Page Down").replace("BACKSPACE", "Backspace").replace("ESCAPE", "Esc"); }
