"use client";

import * as Dialog from "@radix-ui/react-dialog";
import { FormEvent, KeyboardEvent, useState } from "react";
import { Eye, EyeOff, RotateCcw, X } from "lucide-react";
import { Modifier, ShortcutBinding } from "@/bridge/protocol";
import { defaultBindings, shortcutNames, validIpv4 } from "@/persistence/preferences";
import { useCompanionScreen } from "@/state/CompanionScreenContext";

export function SettingsPanel({ open, onOpenChange }: { open: boolean; onOpenChange(open: boolean): void }) {
  const { runtime, preferences, updatePreferences, setBinding, retry } = useCompanionScreen();
  const [host, setHost] = useState(preferences.hostIp ?? "");
  const [hostError, setHostError] = useState("");
  const [capturing, setCapturing] = useState<string | null>(null);

  const saveHost = (event: FormEvent) => {
    event.preventDefault();
    const value = host.trim();
    if (!validIpv4(value)) { setHostError("Enter a valid IPv4 address, such as 192.168.1.50."); return; }
    setHostError(""); updatePreferences({ hostIp: value });
  };

  const capture = (event: KeyboardEvent<HTMLButtonElement>, name: string) => {
    event.preventDefault();
    if (["Shift", "Control", "Alt", "Meta"].includes(event.key)) return;
    const key = browserKey(event.key);
    if (!key) return;
    const modifiers: Modifier[] = [];
    if (event.shiftKey) modifiers.push("SHIFT");
    if (event.ctrlKey) modifiers.push("CTRL");
    if (event.altKey) modifiers.push("ALT");
    setBinding(name, { key, modifiers }); setCapturing(null);
  };

  return (
    <Dialog.Root open={open} onOpenChange={onOpenChange}>
      <Dialog.Portal>
        <Dialog.Overlay className="dialog-overlay" />
        <Dialog.Content className="settings-panel panel-frame" onOpenAutoFocus={(event) => event.preventDefault()}>
          <div className="settings-header"><div><p className="eyebrow">WCS</p><Dialog.Title>Settings</Dialog.Title></div><Dialog.Close className="icon-button" aria-label="Close settings"><X /></Dialog.Close></div>
          <Dialog.Description className="settings-intro">Choose where WoW is running and adjust the companion controls.</Dialog.Description>
          <div className="settings-scroll">
            <SettingsSection title="Connection">
              <form className="connection-form" onSubmit={saveHost}>
                <label>WoW PC IPv4 address<input value={host} onChange={(event) => setHost(event.target.value)} inputMode="decimal" placeholder="127.0.0.1" /></label>
                <button className="gold-button" type="submit">Save & Connect</button>
              </form>
              {hostError && <p className="field-error">{hostError}</p>}
              <p className="security-note">Use 127.0.0.1 when WoW runs on this device. Use the WoW PC's local address only for a separate device on the same trusted network.</p>
              <div className="connection-details"><span>Status <b>{runtime.connectionState}</b></span><span>Session <b>{runtime.sessionState.replace("-", " ")}</b></span><span>Game <b>{runtime.hasSnapshot ? runtime.bridgeState.game.state : "unavailable"}</b></span><span>Authentication <b>{runtime.sessionState === "pairing" ? "pairing required" : runtime.sessionState === "ready" ? "authenticated" : runtime.sessionState}</b></span></div>
              <button className="secondary-button" onClick={retry}>Reconnect</button>
              <p className="security-note">LAN only · ws://{preferences.hostIp ?? "WoW-PC"}:18423/wcs · Never expose this port publicly.</p>
            </SettingsSection>
            <SettingsSection title="WoW shortcuts">
              <div className="binding-list">
                {shortcutNames.map((name) => {
                  const visible = preferences.shortcutVisibility[name] !== false;
                  return <div className="binding-row" key={name}><div><strong>{name}</strong><span>{formatBinding(preferences.shortcutBindings[name])}</span></div><button className={capturing === name ? "capture-button capturing" : "capture-button"} onClick={() => setCapturing(name)} onKeyDown={(event) => capturing === name && capture(event, name)}>{capturing === name ? "Press combination…" : "Change Binding"}</button><button className="reset-button" title="Reset default binding" aria-label={`Reset ${name} binding`} onClick={() => { setBinding(name, defaultBindings[name]); setCapturing(null); }}><RotateCcw /></button><button className="visibility-button" title={visible ? "Hide from Home" : "Show on Home"} aria-label={`${visible ? "Hide" : "Show"} ${name} on Home`} aria-pressed={!visible} onClick={() => updatePreferences({ shortcutVisibility: { ...preferences.shortcutVisibility, [name]: !visible } })}>{visible ? <Eye /> : <EyeOff />}</button></div>;
                })}
              </div>
            </SettingsSection>
            <SettingsSection title="Touchpad">
              <RangeSetting label="Pointer sensitivity" value={preferences.pointerSensitivity} min={0.4} max={2.5} step={0.1} onChange={(pointerSensitivity) => updatePreferences({ pointerSensitivity })} />
              <RangeSetting label="Scroll sensitivity" value={preferences.scrollSensitivity} min={0.4} max={2.5} step={0.1} onChange={(scrollSensitivity) => updatePreferences({ scrollSensitivity })} />
            </SettingsSection>
            <SettingsSection title="Interface">
              <label className="toggle-row"><span><strong>Haptics</strong><small>Short vibration after input where supported</small></span><input type="checkbox" checked={preferences.hapticsEnabled} onChange={(event) => updatePreferences({ hapticsEnabled: event.target.checked })} /></label>
              <RangeSetting label="UI scale" value={preferences.uiScale} min={0.85} max={1.15} step={0.05} onChange={(uiScale) => updatePreferences({ uiScale })} />
            </SettingsSection>
          </div>
        </Dialog.Content>
      </Dialog.Portal>
    </Dialog.Root>
  );
}

function SettingsSection({ title, children }: { title: string; children: React.ReactNode }) { return <section className="settings-section"><h2>{title}</h2>{children}</section>; }
function RangeSetting({ label, value, min, max, step, onChange }: { label: string; value: number; min: number; max: number; step: number; onChange(value: number): void }) { return <label className="range-row"><span>{label}<b>{value.toFixed(step < .1 ? 2 : 1)}×</b></span><input type="range" value={value} min={min} max={max} step={step} onChange={(event) => onChange(Number(event.target.value))} /></label>; }
function formatBinding(binding: ShortcutBinding) { return [...binding.modifiers, binding.key].join(" + "); }
function browserKey(key: string) {
  const named: Record<string, string> = { Backspace: "BACKSPACE", Tab: "TAB", Enter: "ENTER", Escape: "ESCAPE", " ": "SPACE", PageUp: "PAGEUP", PageDown: "PAGEDOWN", End: "END", Home: "HOME", ArrowLeft: "LEFT", ArrowUp: "UP", ArrowRight: "RIGHT", ArrowDown: "DOWN", Insert: "INSERT", Delete: "DELETE" };
  if (named[key]) return named[key];
  if (/^F([1-9]|1[0-2])$/.test(key)) return key;
  if (key.length === 1 && key.charCodeAt(0) <= 127) return key.toUpperCase();
  return null;
}
