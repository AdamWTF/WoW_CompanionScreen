"use client";

import React, { useEffect, useState } from "react";
import { Crown } from "lucide-react";
import { PartyMember } from "@/bridge/protocol";
import { withBasePath } from "@/deployment/basePath";
import { useCompanionScreen } from "@/state/CompanionScreenContext";
import { classColor, compactValue, partyPortraitPath, partyStatuses, percent, resourceColor } from "./partyPresentation";

export function PartyRoster({ members }: { members: PartyMember[] }) {
  const { selectPartyMember } = useCompanionScreen();
  return <PartyRosterView members={members} onSelect={selectPartyMember} />;
}

export function PartyRosterView({ members, onSelect }: { members: PartyMember[]; onSelect(member: number): void }) {
  return (
    <section className="party-roster panel-frame" aria-label="Party members">
      {members.map((member) => {
        const statuses = partyStatuses(member);
        const style = { "--class-color": classColor(member.class.token), "--resource-color": resourceColor(member.resource.type) } as React.CSSProperties;
        const stateClasses = [
          member.targeted && "targeted",
          !member.connected && "offline",
          member.ghost && "ghost",
          member.dead && "dead",
        ].filter(Boolean).join(" ");
        const statusLabel = statuses.length > 0 ? `, ${statuses.join(", ")}` : "";
        return (
          <button
            key={member.slot}
            className={`party-member${stateClasses ? ` ${stateClasses}` : ""}`}
            style={style}
            disabled={!member.guid}
            title={member.name || "Unknown"}
            aria-current={member.targeted ? "true" : undefined}
            aria-label={`${member.targeted ? "Current target" : "Target"} ${member.name}, level ${member.level > 0 ? member.level : "unknown"} ${member.class.name}${statusLabel}`}
            onClick={() => onSelect(member.slot)}
          >
            <span className="party-identity">
              <span className="party-portrait-wrap">
                <PartyPortrait member={member} />
                {(member.dead || member.ghost || !member.connected) && <span className="party-state-shade" aria-hidden="true" />}
              </span>
              <span className="party-details">
                <strong className="character-name party-name">{member.name || "Unknown"}</strong>
                <span className="party-level-row"><small className="level-label">LEVEL {member.level > 0 ? member.level : "??"}</small>{member.leader && <Crown className="party-leader" aria-label="Party leader" />}</span>
                {statuses.length > 0 && <span className="party-statuses">{statuses.map((status) => <b key={status}>{status}</b>)}</span>}
              </span>
            </span>
            <span className="party-stats">
              <StatBar kind="health" label="HP" current={member.health.current} maximum={member.health.maximum} />
              <StatBar kind="resource" label={resourceLabel(member.resource.type)} current={member.resource.current} maximum={member.resource.maximum} />
            </span>
          </button>
        );
      })}
    </section>
  );
}

function PartyPortrait({ member }: { member: PartyMember }) {
  const requested = withBasePath(partyPortraitPath(member));
  const fallback = withBasePath("/assets/wow-icons/inv_misc_questionmark.webp");
  const [source, setSource] = useState(requested);
  useEffect(() => setSource(requested), [requested]);
  return <img className="party-portrait" src={source} onError={() => setSource(fallback)} alt="" draggable={false} />;
}

function StatBar({ kind, label, current, maximum }: { kind: "health" | "resource"; label: string; current: number; maximum: number }) {
  const value = maximum > 0 ? `${current.toLocaleString()} / ${maximum.toLocaleString()}` : "Unknown";
  return (
    <span className={`party-stat ${kind}`} role="img" aria-label={`${label}: ${value}`} title={`${label}: ${value}`}>
      <i style={{ width: `${percent(current, maximum)}%` }} />
      <span aria-hidden="true"><b>{label}</b><em>{maximum > 0 ? `${compactValue(current)} / ${compactValue(maximum)}` : "—"}</em></span>
    </span>
  );
}

function resourceLabel(type: PartyMember["resource"]["type"]) {
  if (type === "runic-power") return "RP";
  if (type === "unknown") return "POWER";
  return type.toUpperCase();
}
