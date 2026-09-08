"use client";

import { Backpack, Settings } from "lucide-react";
import { useCompanionScreen } from "@/state/CompanionScreenContext";
import { compactValue } from "../party/partyPresentation";
import { ShortcutBar } from "./ShortcutBar";
import { ActionGrid } from "./ActionGrid";
import { PartyRoster } from "../party/PartyRoster";

export function HomeScreen({ openSettings }: { openSettings(): void }) {
  const { runtime } = useCompanionScreen();
  const { bridgeState, hasSnapshot, sessionState } = runtime;
  const world = hasSnapshot && sessionState === "ready" && bridgeState.game.state === "world";
  const player = world ? bridgeState.player : null;
  const party = world ? bridgeState.party.members : [];

  return (
    <div className="home-screen">
      <section className="status-strip panel-frame">
        <div className="character-block">
          <div className="character-name">{player?.name || gameTitle(bridgeState.game.state, runtime.connectionState)}</div>
          <div className="level-label">{player ? `LEVEL ${player.level}` : gameSubtitle(bridgeState.game.state, runtime.connectionState)}</div>
        </div>
        <div className="money-block">
          {player ? <Money copper={player.money} /> : <div className="muted-value">—</div>}
          {player && <div className="bags-value"><Backpack /><span><b>{player.bags.used}</b> / {player.bags.total} slots</span></div>}
        </div>
        <ExperienceBar player={player} />
        <div className="status-tools">
          <button className="icon-button companion-settings" onClick={openSettings} aria-label={`Companion settings, ${runtime.connectionState}, ${sessionState}`}><Settings /><span className={`connection-dot ${world ? "ready" : "waiting"}`} aria-hidden="true" /></button>
        </div>
      </section>
      <ShortcutBar enabled={world} />
      <div className={`combat-layout${party.length ? " has-party" : ""}`}>
        {party.length > 0 && <PartyRoster members={party} />}
        <div className="action-panel">
          <div className="section-heading"><span>Quick Actions</span></div>
          <ActionGrid enabled={world} slots={world ? bridgeState.actions.slots : []} />
        </div>
      </div>
    </div>
  );
}

function Money({ copper }: { copper: number }) {
  const gold = Math.floor(copper / 10000);
  const silver = Math.floor((copper % 10000) / 100);
  const coins = copper % 100;
  return <div className="money"><span>{gold.toLocaleString()} <b>G</b></span><span>{silver} <b>S</b></span><span>{coins} <b>C</b></span></div>;
}

function ExperienceBar({ player }: { player: ReturnType<typeof useCompanionScreen>["runtime"]["bridgeState"]["player"] }) {
  const xp = player?.experience;
  const capped = xp?.capped;
  const percent = capped ? 100 : xp?.required ? Math.min(100, xp.current / xp.required * 100) : 0;
  const rested = !capped && xp?.required ? Math.min(100 - percent, xp.rested / xp.required * 100) : 0;
  const progress = capped ? "A champion of Azeroth" : xp ? `${xp.current.toLocaleString()} / ${xp.required.toLocaleString()}${xp.rested > 0 ? ` · ${xp.rested.toLocaleString()} rested` : ""}` : "Waiting for player data";
  return (
    <div className="xp-block">
      <div className="xp-track">
        <div className="xp-fill" style={{ width: `${percent}%` }} />
        <div className="rested-fill" style={{ left: `${percent}%`, width: `${rested}%` }} />
        <div className="xp-overlay" role="img" aria-label={progress} title={progress}><span>{capped ? `LEVEL ${xp?.level ?? 80}` : "XP"} <b>{capped ? "MAX" : xp ? `${Math.round(percent)}%` : "—"}</b></span><strong aria-hidden="true">{capped ? "Maximum level" : xp ? `${compactValue(xp.current)} / ${compactValue(xp.required)}${xp.rested > 0 ? " · Rested" : ""}` : "Waiting for player"}</strong></div>
      </div>
    </div>
  );
}

function gameTitle(state: string, connection: string) {
  if (["disconnected", "reconnecting", "unconfigured", "error"].includes(connection)) return "WoW PC Offline";
  if (state === "loading") return "Loading";
  if (state === "character-select") return "Character Select";
  if (state === "world") return "Entering World";
  return "Login Screen";
}
function gameSubtitle(state: string, connection: string) {
  if (["disconnected", "reconnecting", "unconfigured", "error"].includes(connection)) return "Reconnect to continue";
  if (state === "loading" || state === "world") return "Entering World";
  if (state === "character-select") return "Select a Character";
  return "Waiting for Character";
}
