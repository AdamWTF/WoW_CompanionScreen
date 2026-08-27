"use client";

import { PointerEvent as ReactPointerEvent, useCallback, useEffect, useRef } from "react";
import { MousePointer2 } from "lucide-react";
import { useThorPad } from "@/state/ThorPadContext";

interface Point { x: number; y: number; startX: number; startY: number; started: number }

export function Touchpad() {
  const { runtime, preferences, updatePreferences, movePointer, clickPointer, pointerDown, pointerUp, scrollPointer, dispatch } = useThorPad();
  const pointers = useRef(new Map<number, Point>());
  const moveBuffer = useRef({ x: 0, y: 0 });
  const scrollBuffer = useRef(0);
  const frame = useRef<number | null>(null);
  const holdTimer = useRef<number | null>(null);
  const dragging = useRef(false);
  const moved = useRef(false);
  const multiTouch = useRef(false);
  const tapHandled = useRef(false);
  const ready = runtime.sessionState === "ready" && runtime.hasSnapshot;

  const flush = useCallback(() => {
    frame.current = null;
    if (moveBuffer.current.x || moveBuffer.current.y) {
      movePointer(moveBuffer.current.x * preferences.pointerSensitivity, moveBuffer.current.y * preferences.pointerSensitivity);
      moveBuffer.current = { x: 0, y: 0 };
    }
    const threshold = Math.max(8, 24 / preferences.scrollSensitivity);
    if (Math.abs(scrollBuffer.current) >= threshold) {
      const steps = Math.trunc(scrollBuffer.current / threshold);
      scrollPointer(Math.max(-120, Math.min(120, steps)));
      scrollBuffer.current -= steps * threshold;
    }
  }, [movePointer, preferences.pointerSensitivity, preferences.scrollSensitivity, scrollPointer]);

  const schedule = () => { if (frame.current === null) frame.current = requestAnimationFrame(flush); };
  useEffect(() => () => { if (frame.current !== null) cancelAnimationFrame(frame.current); if (holdTimer.current !== null) clearTimeout(holdTimer.current); if (dragging.current) pointerUp("left"); }, [pointerUp]);

  const onDown = (event: ReactPointerEvent<HTMLDivElement>) => {
    if (!ready) return;
    event.currentTarget.setPointerCapture(event.pointerId);
    const point = { x: event.clientX, y: event.clientY, startX: event.clientX, startY: event.clientY, started: performance.now() };
    pointers.current.set(event.pointerId, point);
    if (pointers.current.size === 1) { moved.current = false; multiTouch.current = false; tapHandled.current = false; }
    if (pointers.current.size > 1) multiTouch.current = true;
    if (pointers.current.size === 1) holdTimer.current = window.setTimeout(() => {
      if (!moved.current && pointers.current.size === 1) { dragging.current = true; pointerDown("left"); }
    }, 450);
    else if (holdTimer.current !== null) clearTimeout(holdTimer.current);
  };

  const onMove = (event: ReactPointerEvent<HTMLDivElement>) => {
    const point = pointers.current.get(event.pointerId);
    if (!point || !ready) return;
    const dx = event.clientX - point.x;
    const dy = event.clientY - point.y;
    point.x = event.clientX; point.y = event.clientY;
    if (Math.hypot(event.clientX - point.startX, event.clientY - point.startY) > 7) moved.current = true;
    if (pointers.current.size >= 2) scrollBuffer.current += dy;
    else { moveBuffer.current.x += dx; moveBuffer.current.y += dy; }
    schedule();
  };

  const onUp = (event: ReactPointerEvent<HTMLDivElement>) => {
    const point = pointers.current.get(event.pointerId);
    const countBefore = pointers.current.size;
    pointers.current.delete(event.pointerId);
    if (holdTimer.current !== null) clearTimeout(holdTimer.current);
    if (dragging.current) { dragging.current = false; pointerUp("left"); }
    else if (point && !moved.current && !tapHandled.current && performance.now() - point.started < 450) {
      clickPointer(multiTouch.current ? "right" : "left");
      tapHandled.current = true;
    }
    if (!pointers.current.size) { moved.current = false; multiTouch.current = false; tapHandled.current = false; scrollBuffer.current = 0; }
  };

  return (
    <section className="touchpad-page">
      <header className="page-heading"><div><p className="eyebrow">REMOTE CONTROL</p><h1>Touchpad</h1></div><label className="touchpad-sensitivity"><span>Sensitivity <b>{preferences.pointerSensitivity.toFixed(1)}×</b></span><input aria-label="Pointer sensitivity" type="range" value={preferences.pointerSensitivity} min={0.4} max={2.5} step={0.1} onChange={(event) => updatePreferences({ pointerSensitivity: Number(event.target.value) })} /></label></header>
      {runtime.touchpadWarning && <button className="warning-banner" onClick={() => dispatch({ type: "warning", warning: null })}>{runtime.touchpadWarning}<b>×</b></button>}
      <div className={!ready ? "touch-surface disabled" : "touch-surface"} onPointerDown={onDown} onPointerMove={onMove} onPointerUp={onUp} onPointerCancel={onUp} onContextMenu={(event) => event.preventDefault()}>
        <div className="touch-glyph"><MousePointer2 /><strong>{ready ? "Move to control" : "Connect to the WoW PC"}</strong><span>Tap · click &nbsp; Two-finger tap · right click</span><span>Hold and move · drag &nbsp; Two fingers · scroll</span></div>
      </div>
    </section>
  );
}
