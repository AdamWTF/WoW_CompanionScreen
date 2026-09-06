// @vitest-environment jsdom

import { cleanup, fireEvent, render, screen } from "@testing-library/react";
import React from "react";
import { afterEach, describe, expect, it, vi } from "vitest";
import { PartyMember } from "@/bridge/protocol";
import { createDemoBridgeState } from "@/demo/demoState";
import { PartyRosterView } from "./PartyRoster";

afterEach(cleanup);

function demoMembers() {
  return createDemoBridgeState().party.members;
}

describe("PartyRosterView", () => {
  it("renders four complete party unit frames", () => {
    const { container } = render(<PartyRosterView members={demoMembers()} onSelect={() => undefined} />);

    expect(screen.getAllByRole("button")).toHaveLength(4);
    expect(container.querySelectorAll(".party-portrait-ring")).toHaveLength(4);
    expect(screen.getByText("Ironward")).toBeTruthy();
    expect(screen.getAllByText("LV 72")).toHaveLength(2);
    expect(screen.getAllByText("HP")).toHaveLength(4);
    expect(screen.getByText("18,420 / 22,100")).toBeTruthy();
    expect(screen.getByText("RAGE")).toBeTruthy();
  });

  it("applies class, resource, target, leader and status presentation", () => {
    const members = demoMembers();
    members[1] = { ...members[1], ghost: true, afk: true, dnd: true };
    const { container } = render(<PartyRosterView members={members} onSelect={() => undefined} />);

    const target = screen.getByRole("button", { name: /Current target Ironward/ });
    expect(target.classList.contains("targeted")).toBe(true);
    expect(target.getAttribute("aria-current")).toBe("true");
    expect(target.style.getPropertyValue("--class-color")).toBe("#c79c6e");
    expect(target.style.getPropertyValue("--resource-color")).toBe("#b52222");
    expect(screen.getByLabelText("Party leader")).toBeTruthy();

    const ghost = screen.getByRole("button", { name: /Moonbloom.*Ghost.*AFK.*DND/ });
    expect(ghost.classList.contains("ghost")).toBe(true);
    expect(container.querySelectorAll(".party-state-shade")).toHaveLength(3);
    expect(screen.getByText(/offline/i)).toBeTruthy();
    expect(screen.getByText(/dead/i)).toBeTruthy();
    expect(screen.getByText(/ghost/i)).toBeTruthy();
    expect(screen.getAllByText("AFK")).toHaveLength(2);
    expect(screen.getByText("DND")).toBeTruthy();
  });

  it("targets valid, dead and ghost members but disables missing identities", () => {
    const onSelect = vi.fn();
    const members = demoMembers();
    members[1] = { ...members[1], ghost: true };
    members[3] = { ...members[3], guid: "" };
    render(<PartyRosterView members={members} onSelect={onSelect} />);

    const connected = screen.getByRole("button", { name: /Target Moonbloom/ }) as HTMLButtonElement;
    const dead = screen.getByRole("button", { name: /Target Cogspinner/ }) as HTMLButtonElement;
    const unavailable = screen.getByRole("button", { name: /Target Emberveil/ }) as HTMLButtonElement;
    expect(connected.disabled).toBe(false);
    expect(dead.disabled).toBe(false);
    expect(unavailable.disabled).toBe(true);

    fireEvent.click(connected);
    fireEvent.click(dead);
    fireEvent.click(unavailable);
    expect(onSelect.mock.calls).toEqual([[2], [3]]);
  });

  it("uses offline, ghost and dead status precedence", () => {
    const base = demoMembers()[0];
    const states: PartyMember[] = [
      { ...base, slot: 1, connected: false, ghost: true, dead: true },
      { ...base, slot: 2, connected: true, ghost: true, dead: true },
      { ...base, slot: 3, connected: true, ghost: false, dead: true },
    ];
    render(<PartyRosterView members={states} onSelect={() => undefined} />);

    expect(screen.getByRole("button", { name: /Offline/ }).textContent).not.toContain("Ghost");
    expect(screen.getByRole("button", { name: /Ghost/ }).textContent).not.toContain("Dead");
    expect(screen.getByRole("button", { name: /Dead/ })).toBeTruthy();
  });
});
