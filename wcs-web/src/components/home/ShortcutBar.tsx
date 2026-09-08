import { Backpack, BookOpen, CircleUserRound, Crown, Flag, Medal, ScrollText, Settings, UsersRound } from "lucide-react";
import { shortcutNames } from "@/persistence/preferences";
import { useCompanionScreen } from "@/state/CompanionScreenContext";

const icons = [CircleUserRound, Backpack, BookOpen, Crown, Medal, ScrollText, Flag, UsersRound, Settings];

export function ShortcutBar({ enabled }: { enabled: boolean }) {
  const { preferences, pressKey } = useCompanionScreen();
  const shortcuts = shortcutNames
    .map((name, index) => ({ name, Icon: icons[index] }))
    .filter(({ name }) => preferences.shortcutVisibility[name] !== false);
  return (
    <section className="shortcut-bar" aria-label="World of Warcraft panels">
      {shortcuts.map(({ name, Icon }) => {
        const binding = preferences.shortcutBindings[name];
        return (
          <button key={name} disabled={!enabled} className="shortcut-button" aria-label={`WoW ${name}`} onClick={() => pressKey(binding.key, binding.modifiers)}>
            <Icon />
            <span>{({ "Spell Book": "Spellbook", Achievements: "Achieve.", "Dungeon Finder": "Dungeon" } as Record<string, string>)[name] ?? name}</span>
          </button>
        );
      })}
    </section>
  );
}
