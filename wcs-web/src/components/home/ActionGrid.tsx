"use client";

import { useEffect, useState } from "react";
import { Action, emptyBridgeState, PopulatedAction } from "@/bridge/protocol";
import { resolveWowIcon } from "@/icons/resolveWowIcon";
import { useCompanionScreen } from "@/state/CompanionScreenContext";

export function ActionGrid({ slots, enabled }: { slots: Action[]; enabled: boolean }) {
  const actions = slots.length === 24 ? slots : emptyBridgeState().actions.slots;
  return <section className={!enabled ? "action-grid disabled" : "action-grid"}>{actions.map((action) => <ActionSlot key={action.slot} action={action} enabled={enabled} />)}</section>;
}

function ActionSlot({ action, enabled }: { action: Action; enabled: boolean }) {
  const { pressAction, preferences } = useCompanionScreen();
  const [fallback, setFallback] = useState(false);
  const [remaining, setRemaining] = useState(action.empty ? 0 : action.cooldown.remainingMs);
  useEffect(() => {
    if (action.empty || !action.cooldown.active) { setRemaining(0); return; }
    const started = performance.now();
    setRemaining(action.cooldown.remainingMs);
    const timer = window.setInterval(() => setRemaining(Math.max(0, action.cooldown.remainingMs - (performance.now() - started))), 100);
    return () => clearInterval(timer);
  }, [action]);

  if (action.empty) return <div className="action-slot empty" aria-label={`Empty action slot ${action.slot}`}><span>{action.slot}</span></div>;
  const cooldownPercent = action.cooldown.durationMs > 0 ? remaining / action.cooldown.durationMs * 100 : 0;
  const classes = ["action-slot", !action.usable && "unusable", action.insufficientResource && "no-resource", action.inRange === false && "out-of-range", action.current && "current", action.equipped && "equipped"].filter(Boolean).join(" ");
  return (
    <button className={classes} disabled={!enabled} onClick={() => { pressAction(action.slot); if (preferences.hapticsEnabled) navigator.vibrate?.(12); }} aria-label={`${action.name}, slot ${action.slot}`}>
      <img src={fallback ? "/icons/action-fallback.svg" : resolveWowIcon(action.icon)} onError={() => setFallback(true)} alt="" draggable={false} />
      <span className="slot-number">{action.slot}</span>
      {action.count > 0 && <b className="count-badge">{action.count}</b>}
      {action.equipped && <span className="equipped-mark">◆</span>}
      {remaining > 0 && <CooldownOverlay action={action} percent={cooldownPercent} remaining={remaining} />}
      <span className="action-name">{action.name || action.text || `Action ${action.slot}`}</span>
    </button>
  );
}

function CooldownOverlay({ action, percent, remaining }: { action: PopulatedAction; percent: number; remaining: number }) {
  return <span className="cooldown" style={{ background: `conic-gradient(rgba(2,2,2,.82) ${percent}%, transparent ${percent}% 100%)` }}><b>{remaining >= 1000 ? Math.ceil(remaining / 1000) : ""}</b><i>{action.cooldown.active ? "" : ""}</i></span>;
}
