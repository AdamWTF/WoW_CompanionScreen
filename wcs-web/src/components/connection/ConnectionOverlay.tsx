"use client";

import { useState } from "react";
import { useCompanionScreen } from "@/state/CompanionScreenContext";

export function ConnectionOverlay({ openSettings }: { openSettings(): void }) {
  const { runtime, preferences, pair, retry } = useCompanionScreen();
  const [code, setCode] = useState("");
  const connected = runtime.connectionState === "connected" && runtime.sessionState === "ready" && runtime.hasSnapshot;
  if (connected) return null;

  let title = "Connection required";
  let description = "Connect to the PC running WoW to use actions, shortcuts, the touchpad and keyboard.";
  let showProgress = false;
  let showRetry = false;

  if (runtime.connectionState === "unconfigured") {
    title = "Connect to continue";
    description = "Enter 127.0.0.1 when this app and WoW run on the same device, or enter the WoW PC's local IPv4 address for a separate companion device.";
  }
  if (runtime.connectionState === "connecting" || runtime.connectionState === "reconnecting") {
    title = runtime.connectionState === "reconnecting" ? "Reconnecting to your WoW PC" : "Connecting to your WoW PC";
    description = `Trying to reach WoW at ws://${preferences.hostIp}:18423/wcs. Controls will appear once WoW sends its current state.`;
    showProgress = true;
    showRetry = true;
  }
  if (["negotiating", "authenticating", "awaiting-snapshot"].includes(runtime.sessionState)) {
    title = runtime.sessionState === "awaiting-snapshot" ? "Loading your character" : "Finishing the connection";
    description = runtime.sessionState === "awaiting-snapshot" ? "Connected. Waiting for WoW to send your current character and actions." : "The bridge is connected and checking this device.";
    showProgress = true;
    showRetry = false;
  }
  if (runtime.connectionState === "disconnected" || (runtime.connectionState === "error" && !["protocol-mismatch", "auth-missing", "auth-failed"].includes(runtime.sessionState))) {
    title = "Unable to connect";
    description = "WoW Companion Screen could not reach the WoW PC. Check that WoW and the bridge are running. Use 127.0.0.1 on the same device, or confirm that both devices are on the same local network.";
    showRetry = true;
  }
  if (runtime.sessionState === "pairing") {
    title = "Pair this WoW Companion Screen";
    description = "Enter the pairing code shown inside WoW or the WoW Companion Screen F9 interface.";
  }
  if (runtime.connectionState === "busy") { title = "Another WoW Companion Screen client is already connected"; description = "The bridge accepts one device at a time. Close the other client, then retry."; }
  if (runtime.sessionState === "protocol-mismatch") { title = "Versions don't match"; description = "The companion app and the bridge installed in WoW are different versions. Update them so they match."; }
  if (runtime.sessionState === "auth-missing") { title = "Pairing token unavailable"; description = "Forget this device from the in-game WoW Companion Screen bridge settings, then pair it again."; }
  if (runtime.sessionState === "auth-failed") { title = "Authentication failed"; description = "Forget this device from the in-game WoW Companion Screen bridge settings, then pair it again."; }

  return (
    <div className="blocking-overlay" role="dialog" aria-modal="true">
      <div className="blocking-card panel-frame">
        <div className="rune-mark">ᚦ</div>
        <p className="eyebrow">WCS{preferences.hostIp ? ` · ${preferences.hostIp}` : ""}</p>
        <h1>{title}</h1>
        <p>{description}</p>
        {showProgress && <div className="connection-progress" role="status" aria-label="Connecting"><span /><span /><span /></div>}
        {runtime.sessionState === "pairing" && <>
          <input className="pairing-input" value={code} onChange={(event) => setCode(event.target.value)} placeholder="ABCD-2345" maxLength={16} autoCapitalize="characters" />
          {runtime.error && <p className="field-error">{runtime.error}</p>}
          <button className="gold-button" disabled={!code.trim()} onClick={() => pair(code)}>Pair Device</button>
        </>}
        {(runtime.connectionState === "busy" || showRetry) && <button className="gold-button" onClick={retry}>Retry connection</button>}
        {runtime.connectionState === "unconfigured" ? <button className="gold-button" onClick={openSettings}>Set up connection</button> : <button className="text-button" onClick={openSettings}>Connection settings</button>}
      </div>
    </div>
  );
}
