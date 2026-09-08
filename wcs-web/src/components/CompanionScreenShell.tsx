"use client";

import React, { useEffect, useRef, useState } from "react";
import { Home, Keyboard, MousePointer2 } from "lucide-react";
import { HomeScreen } from "./home/HomeScreen";
import { Touchpad } from "./touchpad/Touchpad";
import { RemoteKeyboard } from "./keyboard/RemoteKeyboard";
import { SettingsPanel } from "./settings/SettingsPanel";
import { ConnectionOverlay } from "./connection/ConnectionOverlay";
import { useCompanionScreen } from "@/state/CompanionScreenContext";

type Tab = "home" | "touchpad" | "keyboard";

export function CompanionScreenShell() {
  const { demoMode, preferences, runtime } = useCompanionScreen();
  const [tab, setTab] = useState<Tab>("home");
  const [settingsOpen, setSettingsOpen] = useState(false);
  const viewportRef = useRef<HTMLElement>(null);
  const [diagnostics, setDiagnostics] = useState("");
  useEffect(() => {
    const update = () => {
      const height = window.visualViewport?.height ?? window.innerHeight;
      viewportRef.current?.style.setProperty("--visible-height", `${height}px`);
      if (process.env.NODE_ENV === "development") {
        const host = viewportRef.current;
        const rect = host?.getBoundingClientRect();
        setDiagnostics(`${innerWidth}×${innerHeight} CSS · DPR ${devicePixelRatio.toFixed(2)} · visible ${Math.round(height)} · scale ${preferences.uiScale} · ${rect && rect.width / rect.height < .85 ? "phone" : "Thor"}`);
      }
    };
    update();
    window.addEventListener("resize", update);
    window.visualViewport?.addEventListener("resize", update);
    return () => {
      window.removeEventListener("resize", update);
      window.visualViewport?.removeEventListener("resize", update);
    };
  }, [preferences.uiScale]);
  const connectionLocked = runtime.connectionState !== "connected" || runtime.sessionState !== "ready" || !runtime.hasSnapshot;

  return (
    <main ref={viewportRef} className="viewport" style={{ "--ui-scale": preferences.uiScale } as React.CSSProperties}>
      <div className="shell">
        {demoMode && <div className="demo-badge">Demo data</div>}
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
        <SettingsPanel open={settingsOpen} onOpenChange={setSettingsOpen} diagnostics={diagnostics} />
        <ConnectionOverlay openSettings={() => setSettingsOpen(true)} />
      </div>
    </main>
  );
}

function NavButton({ active, label, onClick, icon }: { active: boolean; label: string; onClick(): void; icon: React.ReactNode }) {
  return <button className={active ? "nav-button active" : "nav-button"} onClick={onClick}>{icon}<span>{label}</span></button>;
}
