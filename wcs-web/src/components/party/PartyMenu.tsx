"use client";

import React, { useEffect, useRef, useState } from "react";
import * as Dialog from "@radix-ui/react-dialog";
import { PartyOperation } from "@/bridge/protocol";
import { useCompanionScreen } from "@/state/CompanionScreenContext";
import { PartyPortrait } from "./PartyRoster";

export function PartyMenu({ slot, close, choose }: { slot: number | null; close(): void; choose(slot: number): void }) {
  const { runtime, demoMode, selectPartyMember, manageParty, requestState } = useCompanionScreen();
  const party = runtime.bridgeState.party;
  const member = party.members.find((entry) => entry.slot === slot);
  const [confirm, setConfirm] = useState<PartyOperation | null>(null);
  const [pending, setPending] = useState<string | null>(null);
  const [notice, setNotice] = useState("");
  const cancel = useRef<HTMLButtonElement>(null);
  const restore = useRef<HTMLElement | null>(null);
  const observed = useRef({ generation: "", slot: undefined as number | undefined });
  const submitting = useRef(false);
  const open = slot !== null;
  const signature = JSON.stringify([party.generation, party.groupType, party.canRemove, party.canPromote, party.canLeave,
    party.members.map((entry) => [entry.slot, entry.guid, entry.leader]), runtime.connectionState, runtime.sessionState]);
  const previous = useRef(signature);
  const closeRef = useRef(close); closeRef.current = close;
  const requestStateRef = useRef(requestState); requestStateRef.current = requestState;
  useEffect(() => {
    if (previous.current !== signature) { previous.current = signature; closeRef.current(); setPending(null); setConfirm(null); }
  }, [signature]);
  useEffect(() => { setConfirm(null); setNotice(""); }, [slot]);
  useEffect(() => { if (confirm) cancel.current?.focus(); }, [confirm]);
  useEffect(() => {
    if (!pending) return;
    const timeout = setTimeout(() => { setPending(null); setConfirm(null); setNotice("Change not confirmed"); requestStateRef.current(); }, 5000);
    return () => clearTimeout(timeout);
  }, [pending]);
  useEffect(() => {
    const result = runtime.partyResult;
    if (pending && result?.requestId === pending && result.status !== "dispatched") {
      setPending(null); setConfirm(null); setNotice(`Change not accepted (${result.status}).`); requestStateRef.current();
    }
  }, [pending, runtime.partyResult]);
  const label = confirm === "leave" ? "Leave party" : confirm === "remove" ? `Remove ${member?.name}` : `Make ${member?.name} leader`;
  const reason = party.groupType === "dungeon-finder" ? "Manage Dungeon Finder members in WoW" : "Party leader only";
  const begin = (operation: PartyOperation) => { observed.current = { generation: party.generation ?? "", slot: member?.slot }; setConfirm(operation); };
  useEffect(() => { if (!pending) submitting.current = false; }, [pending]);
  const submit = () => {
    if (!confirm || pending || submitting.current || !observed.current.generation) return;
    if (demoMode) { setConfirm(null); setNotice("Demo only — no party changes sent."); return; }
    const request = Array.from(crypto.getRandomValues(new Uint32Array(4)), (part) => part.toString(16).padStart(8, "0")).join("");
    submitting.current = true;
    setPending(request);
    manageParty(confirm, observed.current.generation, request, observed.current.slot);
  };
  return <Dialog.Root open={open} onOpenChange={(value) => { if (!value) close(); }}>
    <Dialog.Portal><Dialog.Overlay className="dialog-overlay" />
      <Dialog.Content className="party-menu panel-frame" onOpenAutoFocus={() => { restore.current = document.activeElement as HTMLElement; }}
        onCloseAutoFocus={(event) => { event.preventDefault(); if (restore.current?.isConnected) restore.current.focus(); }}>
        <Dialog.Title>{confirm ? label : member ? member.name : "Party"}</Dialog.Title>
        <Dialog.Description>{confirm === "promote" ? "This transfers your party leadership to this member." : confirm ? "Confirm this party change." : "Target a member or manage your party."}</Dialog.Description>
        {member && <div className="party-menu-identity"><PartyPortrait member={member} /><span className="character-name">{member.name}<small className="level-label">LEVEL {member.level}</small></span></div>}
        {confirm ? <>
          <button ref={cancel} onClick={() => setConfirm(null)} disabled={!!pending}>Cancel</button>
          <button onClick={submit} disabled={!!pending}>{pending ? "Waiting for party update…" : label}</button>
        </> : member ? <>
          <button disabled={!member.guid || !!pending} onClick={() => { selectPartyMember(member.slot); close(); }}>Target {member.name}</button>
          <button disabled={!party.canPromote || !!pending} onClick={() => begin("promote")}>Make party leader{!party.canPromote && <small>{reason}</small>}</button>
          <button disabled={!party.canRemove || !!pending} onClick={() => begin("remove")}>Remove from party{!party.canRemove && <small>{reason}</small>}</button>
        </> : <>
          {party.members.map((entry) => <button key={entry.guid} onClick={() => choose(entry.slot)}>{entry.name}</button>)}
          <button disabled={!party.canLeave || !!pending} onClick={() => begin("leave")}>Leave party</button>
        </>}
        <div role="status">{notice}</div>
        {!confirm && <Dialog.Close asChild><button>Close</button></Dialog.Close>}
      </Dialog.Content>
    </Dialog.Portal>
  </Dialog.Root>;
}
