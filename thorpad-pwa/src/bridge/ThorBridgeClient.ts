import { BridgeMessage, isSupportedKey, Modifier } from "./protocol";

interface ClientCallbacks {
  onOpen(): void;
  onMessage(message: BridgeMessage): void;
  onDisconnect(reconnecting: boolean): void;
  onToken(token: string): Promise<void>;
  getToken(): Promise<string | null>;
  onAuthMissing(): void;
}

export class ThorBridgeClient {
  private socket: WebSocket | null = null;
  private host: string | null = null;
  private retryTimer: ReturnType<typeof setTimeout> | null = null;
  private attempt = 0;
  private intentional = false;
  private halted = false;

  constructor(private callbacks: ClientCallbacks) {}

  connect(host: string) {
    this.disconnect();
    this.host = host;
    this.intentional = false;
    this.halted = false;
    this.open();
  }

  retry() {
    if (!this.host) return;
    this.disconnect();
    this.intentional = false;
    this.halted = false;
    this.attempt = 0;
    this.open();
  }

  disconnect() {
    this.intentional = true;
    if (this.retryTimer) clearTimeout(this.retryTimer);
    this.retryTimer = null;
    this.socket?.close();
    this.socket = null;
  }

  private open() {
    if (!this.host) return;
    this.callbacks.onDisconnect(this.attempt > 0);
    try {
      const socket = new WebSocket(`ws://${this.host}:18423/thor`);
      this.socket = socket;
      socket.onopen = () => {
        this.attempt = 0;
        this.callbacks.onOpen();
        this.sendRaw({ type: "hello", protocol: 1, client: "thor" });
      };
      socket.onmessage = async (event) => {
        let message: BridgeMessage;
        try { message = JSON.parse(String(event.data)); } catch { return; }
        if (message.type === "auth.required") {
          const token = await this.callbacks.getToken().catch(() => null);
          if (token) this.sendRaw({ type: "auth", token });
          else { this.callbacks.onAuthMissing(); return; }
        }
        if (message.type === "pairing.complete" && typeof message.token === "string") await this.callbacks.onToken(message.token);
        if (message.type === "error" && message.code === "client-busy") this.halted = true;
        this.callbacks.onMessage(message);
      };
      socket.onerror = () => socket.close();
      socket.onclose = () => {
        if (this.socket === socket) this.socket = null;
        if (!this.intentional && !this.halted) this.scheduleReconnect();
      };
    } catch { this.scheduleReconnect(); }
  }

  private scheduleReconnect() {
    this.attempt += 1;
    this.callbacks.onDisconnect(true);
    const delay = Math.min(10000, 500 * 2 ** Math.min(this.attempt, 5)) + Math.random() * 300;
    this.retryTimer = setTimeout(() => this.open(), delay);
  }

  private sendRaw(payload: object) {
    if (this.socket?.readyState === WebSocket.OPEN) this.socket.send(JSON.stringify(payload));
  }

  pair(code: string, device: { id: string; name: string }) {
    this.sendRaw({ type: "pair.request", code: code.trim().toUpperCase(), device });
  }
  pressAction(slot: number) { if (slot >= 1 && slot <= 24) this.sendRaw({ type: "action.press", slot }); }
  pressKey(key: string, modifiers: Modifier[] = []) { if (isSupportedKey(key)) this.sendRaw({ type: "key.press", key, modifiers: [...new Set(modifiers)].slice(0, 3) }); }
  keyDown(key: string, modifiers: Modifier[] = []) { if (isSupportedKey(key)) this.sendRaw({ type: "key.down", key, modifiers }); }
  keyUp(key: string, modifiers: Modifier[] = []) { if (isSupportedKey(key)) this.sendRaw({ type: "key.up", key, modifiers }); }
  movePointer(dx: number, dy: number) { if (dx || dy) this.sendRaw({ type: "pointer.move", dx: Math.max(-32768, Math.min(32767, Math.round(dx))), dy: Math.max(-32768, Math.min(32767, Math.round(dy))) }); }
  clickPointer(button: "left" | "right" | "middle") { this.sendRaw({ type: "pointer.click", button }); }
  pointerDown(button: "left" | "right") { this.sendRaw({ type: "pointer.down", button }); }
  pointerUp(button: "left" | "right") { this.sendRaw({ type: "pointer.up", button }); }
  scrollPointer(delta: number) { if (delta) this.sendRaw({ type: "pointer.scroll", delta: Math.max(-120, Math.min(120, Math.round(delta))) }); }
}
