// @vitest-environment jsdom
import React from "react";
import { cleanup, fireEvent, render, screen } from "@testing-library/react";
import { afterEach, expect, it, vi } from "vitest";
import { createDemoBridgeState } from "@/demo/demoState";
import { ActionGrid } from "./ActionGrid";
const pressAction = vi.hoisted(() => vi.fn());
vi.mock("@/state/CompanionScreenContext", () => ({ useCompanionScreen: () => ({ pressAction, preferences: { hapticsEnabled: false } }) }));
afterEach(() => { cleanup(); pressAction.mockClear(); });
it.each([["", "", ""], ["  ", " \t", ""], ["", " Mount ", "Mount"], [" Spell ", "Other", "Spell"]])("renders only meaningful captions: %j / %j", (name, text, expected) => {
  const slots = createDemoBridgeState().actions.slots;
  if (slots[0].empty) throw new Error("Expected populated demo action");
  slots[0] = { ...slots[0], name, text };
  render(<ActionGrid slots={slots} enabled />);
  const button = screen.getByRole("button", { name: (expected || "Unnamed action") + ", slot 1" });
  expect(button.querySelector(".action-name")?.textContent ?? "").toBe(expected);
  expect(button.querySelector(".slot-number")?.textContent).toBe("1");
  fireEvent.click(button); expect(pressAction).toHaveBeenCalledWith(1);
});
