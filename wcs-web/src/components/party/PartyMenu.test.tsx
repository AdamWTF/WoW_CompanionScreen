// @vitest-environment jsdom
import React from "react";
import { act, cleanup, fireEvent, render, screen } from "@testing-library/react";
import { afterEach, expect, it, vi } from "vitest";
import { createDemoRuntimeState } from "@/demo/demoState";
import { PartyMenu } from "./PartyMenu";
import { PartyRosterView } from "./PartyRoster";

const context = vi.hoisted(() => ({ current: {} as any }));
vi.mock("@/state/CompanionScreenContext", () => ({ useCompanionScreen: () => context.current }));
afterEach(() => { cleanup(); vi.useRealTimers(); });
function setup(slot = 1) {
  context.current = { runtime: createDemoRuntimeState(), demoMode: false, manageParty: vi.fn(), selectPartyMember: vi.fn(), requestState: vi.fn() };
  const close = vi.fn();
  const view = render(<PartyMenu slot={slot} close={close} choose={vi.fn()} />);
  return { ...view, close };
}
it("confirms removal with Cancel focused and sends once using the observed generation", () => {
  setup();
  fireEvent.click(screen.getByRole("button", { name: "Remove from party" }));
  expect(document.activeElement).toBe(screen.getByRole("button", { name: "Cancel" }));
  fireEvent.click(screen.getByRole("button", { name: "Remove Ironward" }));
  expect(context.current.manageParty).toHaveBeenCalledWith("remove", "demo-leader", expect.any(String), 1);
  expect((screen.getByRole("button", { name: /Waiting/ }) as HTMLButtonElement).disabled).toBe(true);
  expect(context.current.runtime.bridgeState.party.members).toHaveLength(4);
});
it("explains promotion and exposes leave in the header menu", () => {
  const view = setup();
  fireEvent.click(screen.getByRole("button", { name: "Make party leader" }));
  expect(screen.getByText(/transfers your party leadership/)).toBeTruthy();
  fireEvent.click(screen.getByRole("button", { name: "Cancel" }));
  view.rerender(<PartyMenu slot={0} close={view.close} choose={vi.fn()} />);
  fireEvent.click(screen.getByRole("button", { name: "Leave party" }));
  fireEvent.click(screen.getByRole("button", { name: "Leave party" }));
  expect(context.current.manageParty).toHaveBeenCalledWith("leave", "demo-leader", expect.any(String), undefined);
});
it("disables management but retains target for nonleaders and LFD", () => {
  const view = setup();
  context.current.runtime.bridgeState.party.canRemove = false;
  context.current.runtime.bridgeState.party.canPromote = false;
  view.rerender(<PartyMenu slot={1} close={view.close} choose={vi.fn()} />);
  expect((screen.getByRole("button", { name: /Remove from party.*Party leader only/ }) as HTMLButtonElement).disabled).toBe(true);
  expect((screen.getByRole("button", { name: "Target Ironward" }) as HTMLButtonElement).disabled).toBe(false);
  context.current.runtime.bridgeState.party.groupType = "dungeon-finder";
  view.rerender(<PartyMenu slot={1} close={view.close} choose={vi.fn()} />);
  expect(screen.getAllByText("Manage Dungeon Finder members in WoW")).toHaveLength(2);
});
it("closes stale confirmations and on disconnect, but not health updates", () => {
  const view = setup();
  fireEvent.click(screen.getByRole("button", { name: "Remove from party" }));
  context.current.runtime.bridgeState.party.members[0].health.current--;
  view.rerender(<PartyMenu slot={1} close={view.close} choose={vi.fn()} />);
  expect(view.close).not.toHaveBeenCalled();
  context.current.runtime.bridgeState.party.generation = "new";
  view.rerender(<PartyMenu slot={1} close={view.close} choose={vi.fn()} />);
  expect(view.close).toHaveBeenCalledOnce();
  context.current.runtime.connectionState = "disconnected";
  view.rerender(<PartyMenu slot={1} close={view.close} choose={vi.fn()} />);
  expect(view.close).toHaveBeenCalledTimes(2);
  expect(context.current.manageParty).not.toHaveBeenCalled();
});
it("times out without retry and keeps demo operations local", () => {
  vi.useFakeTimers(); setup();
  fireEvent.click(screen.getByRole("button", { name: "Remove from party" }));
  fireEvent.click(screen.getByRole("button", { name: "Remove Ironward" }));
  act(() => vi.advanceTimersByTime(5000));
  expect(screen.getByRole("status").textContent).toBe("Change not confirmed");
  expect(context.current.requestState).toHaveBeenCalledOnce();
  expect(context.current.manageParty).toHaveBeenCalledOnce();
  context.current.demoMode = true;
  fireEvent.click(screen.getByRole("button", { name: "Remove from party" }));
  fireEvent.click(screen.getByRole("button", { name: "Remove Ironward" }));
  expect(context.current.manageParty).toHaveBeenCalledOnce();
});
it("targets taps, opens on hold, suppresses release and supports keyboard/context menus", () => {
  vi.useFakeTimers();
  const target = vi.fn(), menu = vi.fn();
  render(<PartyRosterView members={createDemoRuntimeState().bridgeState.party.members} onSelect={target} onMenu={menu} />);
  const button = screen.getAllByRole("button")[0];
  fireEvent.click(button); expect(target).toHaveBeenCalledOnce();
  // jsdom does not implement PointerEvent; supply its mouse-compatible fields.
  vi.stubGlobal("PointerEvent", MouseEvent);
  fireEvent.pointerDown(button, { button: 0, clientX: 10, clientY: 10 });
  act(() => vi.advanceTimersByTime(500));
  expect(menu).toHaveBeenCalledWith(1);
  fireEvent.pointerUp(button); fireEvent.click(button);
  expect(target).toHaveBeenCalledOnce();
  fireEvent.pointerDown(button, { button: 0, clientX: 10, clientY: 10 });
  fireEvent.pointerMove(button, { clientX: 21, clientY: 10 });
  act(() => vi.advanceTimersByTime(500));
  expect(menu).toHaveBeenCalledOnce();
  fireEvent.contextMenu(button); fireEvent.keyDown(button, { key: "F10", shiftKey: true });
  expect(menu).toHaveBeenCalledTimes(3);
  vi.unstubAllGlobals();
});
