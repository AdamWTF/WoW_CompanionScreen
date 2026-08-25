"use client";

import { useState } from "react";
import { Home, Keyboard, MousePointer2 } from "lucide-react";
import { HomeScreen } from "./home/HomeScreen";
import { Touchpad } from "./touchpad/Touchpad";
import { RemoteKeyboard } from "./keyboard/RemoteKeyboard";
import { SettingsPanel } from "./settings/SettingsPanel";
import { ConnectionOverlay } from "./connection/ConnectionOverlay";
import { useThorPad } from "@/state/ThorPadContext";

type Tab = "home" | "touchpad" | "keyboard";

export function ThorShell() {
  const { preferences, runtime } = useThorPad();
  const [tab, setTab] = useState<Tab>("home");
  const [settingsOpen, setSettingsOpen] = useState(false);
  const connectionLocked = runtime.connectionState !== "connected" || runtime.sessionState !== "ready" || !runtime.hasSnapshot;

  return (
    <main className="viewport" style={{ "--ui-scale": preferences.uiScale } as React.CSSProperties}>
      <div className="shell">
        <div className="content" inert={connectionLocked ? true : undefined} aria-hidden={connectionLocked || undefined}>
          {tab === "home" && <HomeScreen openSettings={() => setSettingsOpen(true)} />}
          {tab === "touchpad" && <Touchpad />}
          {tab === "keyboard" && <RemoteKeyboard />}
        </div>
        <nav className="bottom-nav" aria-label="Primary navigation" inert={connectionLocked ? true : undefined} aria-hidden={connectionLocked || undefined}>
          <NavButton active={tab === "home"} label="Home" onClick={() => setTab("home")} icon={<Home />} />
          <NavButton active={tab === "touchpad"} label="Touchpad" onClick={() => setTab("touchpad")} icon={<MousePointer2 />} />
          <NavButton active={tab === "keyboard"} label="Keyboard" onClick={() => setTab("keyboard")} icon={<Keyboard />} />
        </nav>
        <SettingsPanel open={settingsOpen} onOpenChange={setSettingsOpen} />
        <ConnectionOverlay openSettings={() => setSettingsOpen(true)} />
      </div>
    </main>
  );
}

function NavButton({ active, label, onClick, icon }: { active: boolean; label: string; onClick(): void; icon: React.ReactNode }) {
  return <button className={active ? "nav-button active" : "nav-button"} onClick={onClick}>{icon}<span>{label}</span></button>;
}
