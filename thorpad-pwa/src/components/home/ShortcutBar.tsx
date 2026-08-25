import { Backpack, BookOpen, CircleUserRound, Crown, Flag, Medal, ScrollText, Settings, UsersRound } from "lucide-react";
import { PlayerState } from "@/bridge/protocol";
import { shortcutNames } from "@/persistence/preferences";
import { useThorPad } from "@/state/ThorPadContext";

const icons = [CircleUserRound, Backpack, BookOpen, Crown, Medal, ScrollText, Flag, UsersRound, Settings];

export function ShortcutBar({ enabled, player }: { enabled: boolean; player: PlayerState | null }) {
  const { preferences, pressKey } = useThorPad();
  return (
    <section className="shortcut-bar" aria-label="World of Warcraft panels">
      {shortcutNames.map((name, index) => {
        const Icon = icons[index];
        const binding = preferences.shortcutBindings[name];
        return (
          <button key={name} disabled={!enabled} className="shortcut-button" onClick={() => pressKey(binding.key, binding.modifiers)}>
            <Icon />
            <span>{name}</span>
            <small>{[...binding.modifiers, binding.key].join(" + ")}</small>
            {name === "Bags" && player && <b className="bag-badge">{player.bags.used} / {player.bags.total}</b>}
          </button>
        );
      })}
    </section>
  );
}
