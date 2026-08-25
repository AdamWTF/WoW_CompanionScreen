"use client";

import { Settings } from "lucide-react";
import { useThorPad } from "@/state/ThorPadContext";
import { ConnectionIndicator } from "../connection/ConnectionIndicator";
import { ShortcutBar } from "./ShortcutBar";
import { ActionGrid } from "./ActionGrid";

export function HomeScreen({ openSettings }: { openSettings(): void }) {
  const { runtime } = useThorPad();
  const { bridgeState, hasSnapshot, sessionState } = runtime;
  const world = hasSnapshot && sessionState === "ready" && bridgeState.game.state === "world";
  const player = world ? bridgeState.player : null;

  return (
    <div className="home-screen">
      <section className="status-strip panel-frame">
        <div className="character-block">
          <p className="eyebrow">CHARACTER</p>
          <div className="character-name">{player?.name || gameTitle(bridgeState.game.state, runtime.connectionState)}</div>
          <div className="level-label">{player ? `LEVEL ${player.level}` : gameSubtitle(bridgeState.game.state, runtime.connectionState)}</div>
        </div>
        <div className="money-block">
          <p className="eyebrow">MONEY</p>
          {player ? <Money copper={player.money} /> : <div className="muted-value">—</div>}
        </div>
        <ExperienceBar player={player} />
        <div className="status-tools">
          <ConnectionIndicator onClick={openSettings} />
          <button className="icon-button" onClick={openSettings} aria-label="ThorPad settings"><Settings /></button>
        </div>
      </section>
      <ShortcutBar enabled={world} player={player} />
      <div className="section-heading"><span>Quick Actions</span><small>THOR SLOTS 1–24</small></div>
      <ActionGrid enabled={world} slots={world ? bridgeState.actions.slots : []} />
    </div>
  );
}

function Money({ copper }: { copper: number }) {
  const gold = Math.floor(copper / 10000);
  const silver = Math.floor((copper % 10000) / 100);
  const coins = copper % 100;
  return <div className="money"><span>{gold.toLocaleString()} <b>G</b></span><span>{silver} <b>S</b></span><span>{coins} <b>C</b></span></div>;
}

function ExperienceBar({ player }: { player: ReturnType<typeof useThorPad>["runtime"]["bridgeState"]["player"] }) {
  const xp = player?.experience;
  const capped = xp?.capped;
  const percent = capped ? 100 : xp?.required ? Math.min(100, xp.current / xp.required * 100) : 0;
  const rested = !capped && xp?.required ? Math.min(100 - percent, xp.rested / xp.required * 100) : 0;
  return (
    <div className="xp-block">
      <div className="xp-label"><span>{capped ? `LEVEL ${xp?.level ?? 80}` : "EXPERIENCE"}</span><strong>{capped ? "MAX LEVEL" : xp ? `${Math.round(percent)}%` : "—"}</strong></div>
      <div className="xp-track">
        <div className="xp-fill" style={{ width: `${percent}%` }} />
        <div className="rested-fill" style={{ left: `${percent}%`, width: `${rested}%` }} />
      </div>
      <div className="xp-numbers">{capped ? "A champion of Azeroth" : xp ? `${xp.current.toLocaleString()} / ${xp.required.toLocaleString()}${xp.rested > 0 ? ` · ${xp.rested.toLocaleString()} rested` : ""}` : "Waiting for player data"}</div>
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
