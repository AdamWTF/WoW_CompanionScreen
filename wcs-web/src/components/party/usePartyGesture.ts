import { useEffect, useRef } from "react";
import type { KeyboardEvent, MouseEvent, PointerEvent } from "react";

export function usePartyGesture(onSelect: (slot: number) => void, onMenu?: (slot: number) => void) {
  const hold = useRef<ReturnType<typeof setTimeout> | null>(null);
  const origin = useRef({ x: 0, y: 0 });
  const suppress = useRef(false);
  const cancel = () => { if (hold.current !== null) clearTimeout(hold.current); hold.current = null; };
  useEffect(() => {
    window.addEventListener("scroll", cancel, true);
    window.addEventListener("blur", cancel);
    return () => { cancel(); window.removeEventListener("scroll", cancel, true); window.removeEventListener("blur", cancel); };
  }, []);
  return (slot: number) => ({
    onPointerDown: (event: PointerEvent<HTMLButtonElement>) => {
      cancel(); suppress.current = false;
      if (!onMenu || event.button !== 0 || event.isPrimary === false) return;
      origin.current = { x: event.clientX, y: event.clientY };
      hold.current = setTimeout(() => { hold.current = null; suppress.current = true; onMenu(slot); }, 500);
    },
    onPointerMove: (event: PointerEvent<HTMLButtonElement>) => {
      if (Math.hypot(event.clientX - origin.current.x, event.clientY - origin.current.y) > 10) { cancel(); suppress.current = true; }
    },
    onPointerUp: cancel,
    onPointerCancel: () => { cancel(); suppress.current = true; },
    onPointerLeave: cancel,
    onClick: () => { if (suppress.current) { suppress.current = false; return; } onSelect(slot); },
    onContextMenu: (event: MouseEvent<HTMLButtonElement>) => { if (onMenu) { event.preventDefault(); cancel(); suppress.current = true; onMenu(slot); } },
    onKeyDown: (event: KeyboardEvent<HTMLButtonElement>) => {
      suppress.current = false;
      if (onMenu && event.shiftKey && event.key === "F10") { event.preventDefault(); cancel(); onMenu(slot); }
    },
  });
}
