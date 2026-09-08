// @vitest-environment jsdom
import React from "react";
import { cleanup, fireEvent, render, screen } from "@testing-library/react";
import { afterEach, beforeEach, expect, it, vi } from "vitest";
import { RemoteKeyboard } from "./RemoteKeyboard";

const state = vi.hoisted(() => ({ pressKey: vi.fn(), runtime: { connectionState: "connected", sessionState: "ready", hasSnapshot: true } }));
vi.mock("@/state/CompanionScreenContext", () => ({ useCompanionScreen: () => ({ ...state, preferences: { hapticsEnabled: false } }) }));
afterEach(cleanup);
beforeEach(() => { state.pressKey.mockClear(); Object.assign(state.runtime, { connectionState: "connected", sessionState: "ready", hasSnapshot: true }); });
const click = (name: string) => fireEvent.click(screen.getByRole("button", { name }));

it("sends each basic and advanced key's existing token", () => {
  const { container } = render(<RemoteKeyboard />);
  for (const mode of ["Basic", "Advanced"]) {
    click(mode);
    for (const button of container.querySelectorAll<HTMLButtonElement>("[data-key]")) {
      const key = button.dataset.key!;
      if (["SHIFT", "CTRL", "ALT"].includes(key)) continue;
      fireEvent.click(button);
      expect(state.pressKey).toHaveBeenLastCalledWith(key, []);
    }
  }
  expect(container.querySelectorAll(".keyboard-row")).toHaveLength(6);
  expect(screen.getByRole("button", { name: "F12" })).toBeTruthy();
});

it("shows shifted labels and clears one-shot modifiers after input", () => {
  render(<RemoteKeyboard />);
  click("Shift"); click("!");
  expect(state.pressKey).toHaveBeenLastCalledWith("1", ["SHIFT"]);
  expect(screen.getByRole("button", { name: "q" })).toBeTruthy();
  click("Shift"); click("Shift"); click("q");
  expect(state.pressKey).toHaveBeenLastCalledWith("Q", []);
  click("Advanced"); click("Shift"); click("?");
  expect(state.pressKey).toHaveBeenLastCalledWith("/", ["SHIFT"]);
});

it("preserves modifiers across modes and clears them on disconnect and remount", () => {
  const view = render(<RemoteKeyboard />);
  click("Advanced"); click("Ctrl"); click("Alt"); click("Basic"); click("c");
  expect(state.pressKey).toHaveBeenLastCalledWith("C", ["CTRL", "ALT"]);
  click("Shift"); state.runtime.connectionState = "disconnected"; view.rerender(<RemoteKeyboard />);
  for (const button of view.container.querySelectorAll<HTMLButtonElement>("[data-key]")) expect(button.disabled).toBe(true);
  const count = state.pressKey.mock.calls.length; click("q"); expect(state.pressKey).toHaveBeenCalledTimes(count);
  state.runtime.connectionState = "connected"; view.rerender(<RemoteKeyboard />); click("q");
  expect(state.pressKey).toHaveBeenLastCalledWith("Q", []);
  click("Shift"); view.unmount(); render(<RemoteKeyboard />); click("q");
  expect(state.pressKey).toHaveBeenLastCalledWith("Q", []);
});
