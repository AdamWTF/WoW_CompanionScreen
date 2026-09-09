// @vitest-environment jsdom
import React from "react";
import { act, cleanup, render, screen } from "@testing-library/react";
import { afterEach, expect, it, vi } from "vitest";
import { CompanionScreenShell } from "./CompanionScreenShell";

vi.mock("@/state/CompanionScreenContext", () => ({ useCompanionScreen: () => ({
  demoMode: true, preferences: { uiScale: 1.15 },
  runtime: { connectionState: "connected", sessionState: "ready", hasSnapshot: true },
}) }));
vi.mock("./home/HomeScreen", () => ({ HomeScreen: () => <div>Home content</div> }));
vi.mock("./touchpad/Touchpad", () => ({ Touchpad: () => null }));
vi.mock("./keyboard/RemoteKeyboard", () => ({ RemoteKeyboard: () => null }));
vi.mock("./settings/SettingsPanel", () => ({ SettingsPanel: () => null }));
vi.mock("./connection/ConnectionOverlay", () => ({ ConnectionOverlay: () => null }));
afterEach(() => { cleanup(); vi.unstubAllGlobals(); });

it("reserves the actual visible viewport height and updates when browser chrome changes", () => {
  const viewport = Object.assign(new EventTarget(), { height: 540 });
  vi.stubGlobal("visualViewport", viewport);
  const { unmount } = render(<CompanionScreenShell />);
  const main = screen.getByRole("main");
  expect(main.style.getPropertyValue("--visible-height")).toBe("540px");
  expect(main.style.getPropertyValue("--ui-scale")).toBe("1.15");
  expect(screen.getByRole("navigation", { name: "Primary navigation" })).toBeTruthy();
  act(() => { viewport.height = 432; viewport.dispatchEvent(new Event("resize")); });
  expect(main.style.getPropertyValue("--visible-height")).toBe("432px");
  unmount();
  act(() => { viewport.height = 400; viewport.dispatchEvent(new Event("resize")); });
  expect(main.style.getPropertyValue("--visible-height")).toBe("432px");
});

it("falls back to the window height when VisualViewport is unavailable", () => {
  vi.stubGlobal("visualViewport", undefined);
  render(<CompanionScreenShell />);
  expect(screen.getByRole("main").style.getPropertyValue("--visible-height")).toBe(`${innerHeight}px`);
});
