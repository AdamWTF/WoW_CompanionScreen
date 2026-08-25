import { Wifi, WifiOff } from "lucide-react";
import { useThorPad } from "@/state/ThorPadContext";

export function ConnectionIndicator({ onClick }: { onClick(): void }) {
  const { runtime } = useThorPad();
  const ready = runtime.sessionState === "ready";
  const tone = ready && runtime.bridgeState.game.state === "world" ? "green" : ["connecting", "reconnecting"].includes(runtime.connectionState) || ready ? "amber" : "red";
  return (
    <button className={`connection-indicator ${tone}`} onClick={onClick} title={`${runtime.connectionState} · ${runtime.sessionState}`}>
      {tone === "red" ? <WifiOff /> : <Wifi />}<span>{ready ? runtime.bridgeState.game.state.replace("-", " ") : runtime.connectionState}</span>
    </button>
  );
}
