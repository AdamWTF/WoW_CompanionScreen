import { afterEach, describe, expect, it, vi } from "vitest";
import { ThorBridgeClient } from "./ThorBridgeClient";

class FakeWebSocket {
  static readonly OPEN = 1;
  static instances: FakeWebSocket[] = [];

  readyState = FakeWebSocket.OPEN;
  sent: string[] = [];
  onopen: (() => void) | null = null;
  onmessage: ((event: { data: string }) => void | Promise<void>) | null = null;
  onerror: (() => void) | null = null;
  onclose: (() => void) | null = null;

  constructor(readonly url: string) { FakeWebSocket.instances.push(this); }
  send(payload: string) { this.sent.push(payload); }
  close() { this.readyState = 3; this.onclose?.(); }
  open() { this.onopen?.(); }
  async message(payload: object) { await this.onmessage?.({ data: JSON.stringify(payload) }); }
}

function makeClient(getToken: () => Promise<string | null>, onAuthMissing = vi.fn()) {
  return {
    client: new ThorBridgeClient({
      onOpen: vi.fn(), onMessage: vi.fn(), onDisconnect: vi.fn(), onToken: vi.fn(), getToken, onAuthMissing,
    }),
    onAuthMissing,
  };
}

describe("ThorBridgeClient authentication", () => {
  afterEach(() => { vi.useRealTimers(); vi.unstubAllGlobals(); FakeWebSocket.instances = []; });

  it("halts instead of reconnecting when this browser has no pairing token", async () => {
    vi.useFakeTimers();
    vi.stubGlobal("WebSocket", FakeWebSocket);
    const { client, onAuthMissing } = makeClient(async () => null);
    client.connect("192.168.1.50");
    const socket = FakeWebSocket.instances[0];
    socket.open();
    await socket.message({ type: "auth.required" });

    expect(onAuthMissing).toHaveBeenCalledOnce();
    expect(socket.readyState).toBe(3);
    await vi.advanceTimersByTimeAsync(30_000);
    expect(FakeWebSocket.instances).toHaveLength(1);
  });

  it("sends an available pairing token", async () => {
    vi.stubGlobal("WebSocket", FakeWebSocket);
    const { client } = makeClient(async () => "saved-token");
    client.connect("192.168.1.50");
    const socket = FakeWebSocket.instances[0];
    socket.open();
    await socket.message({ type: "auth.required" });

    expect(socket.sent.map((payload) => JSON.parse(payload))).toContainEqual({ type: "auth", token: "saved-token" });
  });
});
