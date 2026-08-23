export interface SerialLine {
  direction: 'tx' | 'rx';
  text: string;
  at: number;
}

export interface SerialPortInfo {
  usbVendorId?: number;
  usbProductId?: number;
}

/** Seeed XIAO (native USB CDC) resets when DTR toggles — do not assert DTR. */
const SKIP_DTR_VENDOR_IDS = new Set([0x2886]);

/** Pause read loop briefly after open on XIAO — USB CDC can glitch right after open. */
const POST_OPEN_SETTLE_MS = 400;

/** Wait for OS to re-enumerate CDC after a brief disconnect. */
const RECONNECT_WAIT_MS = 3000;

/** Auto-recover at most this many times per user-initiated connect/reconnect. */
const MAX_AUTO_RECOVERY = 2;

function samePortInfo(a: SerialPortInfo, b: SerialPortInfo): boolean {
  return a.usbVendorId === b.usbVendorId && a.usbProductId === b.usbProductId;
}

function sleep(ms: number): Promise<void> {
  return new Promise((resolve) => setTimeout(resolve, ms));
}

export class WebSerialTransport {
  private port: SerialPort | null = null;
  private reader: ReadableStreamDefaultReader<Uint8Array> | null = null;
  private writer: WritableStreamDefaultWriter<Uint8Array> | null = null;
  private readBuffer = '';
  private readonly lines: SerialLine[] = [];
  private lineListeners = new Set<(line: SerialLine) => void>();
  private readAbort: AbortController | null = null;
  private portInfo: SerialPortInfo | null = null;
  private baudRate = 115200;
  private openedAt = 0;
  private hadRxBytes = false;
  private recoveryAttempts = 0;
  private lostRecovery: Promise<void> | null = null;
  private onPortDisconnect = () => {
    void this.handleDeviceLost('disconnect');
  };

  get isConnected() {
    return this.writer !== null;
  }

  get selectedPortInfo(): SerialPortInfo | null {
    return this.portInfo;
  }

  get history(): SerialLine[] {
    return [...this.lines];
  }

  onLine(listener: (line: SerialLine) => void): () => void {
    this.lineListeners.add(listener);
    return () => this.lineListeners.delete(listener);
  }

  async connect(baudRate = 115200): Promise<void> {
    if (!navigator.serial) {
      throw new Error('Web Serial недоступен. Используйте Chrome или Edge на ПК.');
    }
    this.baudRate = baudRate;
    this.recoveryAttempts = 0;
    this.port = await navigator.serial.requestPort();
    await this.openPort({ announce: true });
  }

  /** Re-open a previously granted port (after USB reset / device lost). */
  async reconnect(): Promise<void> {
    if (!navigator.serial) {
      throw new Error('Web Serial недоступен.');
    }
    if (!this.portInfo) {
      throw new Error('Сначала выберите COM-порт.');
    }
    this.recoveryAttempts = 0;
    const port = await this.findGrantedPort();
    if (!port) {
      throw new Error('Разрешённый порт не найден — выберите COM-порт снова.');
    }
    this.port = port;
    await this.openPort({ announce: true });
  }

  async disconnect(): Promise<void> {
    this.recoveryAttempts = MAX_AUTO_RECOVERY;
    await this.teardown({ closePort: true });
    this.port = null;
    this.portInfo = null;
  }

  async send(command: string): Promise<void> {
    if (!this.writer) {
      throw new Error('Serial не подключён. Нажмите «Переподключить» или выберите порт снова.');
    }
    const line = command.trim();
    this.pushLine('tx', line);
    try {
      const data = new TextEncoder().encode(`${line}\r\n`);
      await this.writer.write(data);
    } catch (error) {
      void this.handleDeviceLost('write');
      throw error instanceof Error ? error : new Error(String(error));
    }
  }

  private async openPort(options: { announce: boolean }): Promise<void> {
    if (!this.port) throw new Error('Порт не выбран');

    await this.teardown({ closePort: true });

    this.portInfo = this.port.getInfo();
    this.port.addEventListener('disconnect', this.onPortDisconnect);
    this.openedAt = Date.now();
    this.hadRxBytes = false;

    await this.port.open({
      baudRate: this.baudRate,
      flowControl: 'none',
      bufferSize: 65536,
    });

    if (this.shouldAssertDtr()) {
      await this.assertPortSignals();
    } else if (options.announce) {
      this.pushLine('rx', 'DTR не трогаем (Seeed/XIAO — иначе сброс USB)');
    }

    if (!this.shouldAssertDtr()) {
      await sleep(POST_OPEN_SETTLE_MS);
    }

    this.writer = this.port.writable!.getWriter();
    this.readAbort = new AbortController();
    void this.readLoop();

    if (options.announce) {
      const info = this.formatPortInfo();
      this.pushLine('rx', info ? `--- подключено (${info}) ---` : '--- подключено ---');
    }
  }

  private shouldAssertDtr(): boolean {
    const vid = this.portInfo?.usbVendorId;
    return vid === undefined || !SKIP_DTR_VENDOR_IDS.has(vid);
  }

  private async assertPortSignals(): Promise<void> {
    if (!this.port) return;
    try {
      await this.port.setSignals({ dataTerminalReady: true, requestToSend: true });
    } catch (error) {
      this.pushLine(
        'rx',
        `WARN setSignals: ${error instanceof Error ? error.message : String(error)}`,
      );
    }
  }

  private formatPortInfo(): string | null {
    if (!this.portInfo?.usbVendorId && !this.portInfo?.usbProductId) return null;
    const vid = this.portInfo.usbVendorId?.toString(16).padStart(4, '0');
    const pid = this.portInfo.usbProductId?.toString(16).padStart(4, '0');
    return `USB ${vid}:${pid}`;
  }

  private async readLoop() {
    const readable = this.port?.readable;
    if (!readable) return;

    this.reader = readable.getReader();
    const decoder = new TextDecoder();

    try {
      while (!this.readAbort?.signal.aborted) {
        const { value, done } = await this.reader.read();
        if (done) break;
        if (!value?.length) continue;
        this.hadRxBytes = true;
        this.appendChunk(decoder.decode(value));
      }
    } catch (error) {
      if (!this.readAbort?.signal.aborted) {
        const message = error instanceof Error ? error.message : String(error);
        const transient =
          !this.hadRxBytes && Date.now() - this.openedAt < 5000 && message.includes('lost');
        if (!transient) {
          this.pushLine('rx', `ERROR read: ${message}`);
        }
        if (message.toLowerCase().includes('device has been lost')) {
          void this.handleDeviceLost(transient ? 'usb settle' : 'read');
        }
      }
    } finally {
      try {
        this.reader?.releaseLock();
      } catch {
        // port may already be gone
      }
      this.reader = null;
    }
  }

  private handleDeviceLost(reason: string): Promise<void> {
    if (!this.lostRecovery) {
      this.lostRecovery = this.recoverFromLost(reason).finally(() => {
        this.lostRecovery = null;
      });
    }
    return this.lostRecovery;
  }

  private async recoverFromLost(reason: string) {
    if (this.recoveryAttempts >= MAX_AUTO_RECOVERY) {
      await this.teardown({ closePort: true });
      this.port = null;
      this.pushLine('rx', 'USB нестабилен — нажмите «Переподключить».');
      return;
    }
    this.recoveryAttempts += 1;

    const earlyGlitch = reason === 'usb settle' || (!this.hadRxBytes && Date.now() - this.openedAt < 5000);
    if (earlyGlitch) {
      this.pushLine('rx', 'USB CDC переподключается (норма для XIAO), ждём…');
    } else {
      this.pushLine('rx', `WARN: USB отвалился (${reason}), ждём…`);
    }

    const savedInfo = this.portInfo;
    await this.teardown({ closePort: true });
    this.port = null;

    await sleep(RECONNECT_WAIT_MS);

    try {
      const port = await this.waitForGrantedPort(savedInfo, 12000);
      this.port = port;
      await this.openPort({ announce: false });
      this.pushLine('rx', '--- снова online ---');
    } catch {
      this.pushLine('rx', 'Автовосстановление не удалось — нажмите «Переподключить».');
    }
  }

  private async findGrantedPort(): Promise<SerialPort | null> {
    if (!navigator.serial || !this.portInfo) return null;
    const ports = await navigator.serial.getPorts();
    return ports.find((candidate) => samePortInfo(candidate.getInfo(), this.portInfo!)) ?? null;
  }

  private async waitForGrantedPort(
    info: SerialPortInfo | null,
    timeoutMs: number,
  ): Promise<SerialPort> {
    if (!navigator.serial || !info) {
      throw new Error('no port info');
    }

    const existing = (await navigator.serial.getPorts()).find((candidate) =>
      samePortInfo(candidate.getInfo(), info),
    );
    if (existing) return existing;

    return new Promise<SerialPort>((resolve, reject) => {
      const timer = window.setTimeout(() => {
        cleanup();
        reject(new Error('timeout'));
      }, timeoutMs);

      const onConnect = async () => {
        const port = (await navigator.serial!.getPorts()).find((candidate) =>
          samePortInfo(candidate.getInfo(), info),
        );
        if (!port) return;
        cleanup();
        resolve(port);
      };

      const cleanup = () => {
        window.clearTimeout(timer);
        navigator.serial?.removeEventListener('connect', onConnect);
      };

      navigator.serial.addEventListener('connect', onConnect);
    });
  }

  private async teardown(options: { closePort: boolean }) {
    this.readAbort?.abort();
    this.readAbort = null;

    try {
      await this.reader?.cancel();
    } catch {
      // ignore
    }
    try {
      this.reader?.releaseLock();
    } catch {
      // ignore
    }
    this.reader = null;

    try {
      await this.writer?.close();
    } catch {
      // ignore
    }
    try {
      this.writer?.releaseLock();
    } catch {
      // ignore
    }
    this.writer = null;

    if (this.port) {
      this.port.removeEventListener('disconnect', this.onPortDisconnect);
      if (options.closePort) {
        try {
          await this.port.close();
        } catch {
          // ignore
        }
      }
    }
  }

  private appendChunk(chunk: string) {
    this.readBuffer += chunk;
    this.readBuffer = this.readBuffer.replace(/\r\n/g, '\n').replace(/\r/g, '\n');
    let newline = this.readBuffer.indexOf('\n');
    while (newline >= 0) {
      const line = this.readBuffer.slice(0, newline);
      this.readBuffer = this.readBuffer.slice(newline + 1);
      if (line.length > 0) this.pushLine('rx', line);
      newline = this.readBuffer.indexOf('\n');
    }
  }

  private pushLine(direction: 'tx' | 'rx', text: string) {
    const entry: SerialLine = { direction, text, at: Date.now() };
    this.lines.push(entry);
    if (this.lines.length > 500) this.lines.shift();
    for (const listener of this.lineListeners) listener(entry);
  }
}

export const serialCommandPresets = [
  'status',
  'dump-config',
  'selftest',
  'reboot',
  'reset-odo',
  'open-pairing',
  'power-status',
  'display-state',
  'wake-display',
  'ambient-raw',
  'ambient-stop',
  'hall-status',
  'csc-status',
  'csc-pair',
  'csc-forget',
  'test-on',
  'test-off',
] as const;
