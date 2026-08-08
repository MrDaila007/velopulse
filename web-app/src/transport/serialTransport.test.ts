import { describe, expect, it, vi, afterEach } from 'vitest';

class SerialLineParser {
  private readBuffer = '';

  feed(chunk: string): string[] {
    const lines: string[] = [];
    this.readBuffer += chunk;
    this.readBuffer = this.readBuffer.replace(/\r\n/g, '\n').replace(/\r/g, '\n');
    let newline = this.readBuffer.indexOf('\n');
    while (newline >= 0) {
      const line = this.readBuffer.slice(0, newline);
      this.readBuffer = this.readBuffer.slice(newline + 1);
      if (line.length > 0) lines.push(line);
      newline = this.readBuffer.indexOf('\n');
    }
    return lines;
  }
}

describe('serial line parsing', () => {
  it('splits CRLF firmware lines', () => {
    const parser = new SerialLineParser();
    expect(parser.feed('OK status\r\n')).toEqual(['OK status']);
    expect(parser.feed('line1\r\nline2\n')).toEqual(['line1', 'line2']);
  });
});

describe('WebSerialTransport connect', () => {
  afterEach(() => {
    vi.unstubAllGlobals();
  });

  function mockPort(vendorId: number, productId: number) {
    const setSignals = vi.fn().mockResolvedValue(undefined);
    const writer = {
      write: vi.fn().mockResolvedValue(undefined),
      close: vi.fn().mockResolvedValue(undefined),
      releaseLock: vi.fn(),
    };
    const reader = {
      read: vi.fn().mockImplementation(() => new Promise(() => {})),
      cancel: vi.fn().mockResolvedValue(undefined),
      releaseLock: vi.fn(),
    };
    const port = {
      open: vi.fn().mockResolvedValue(undefined),
      close: vi.fn().mockResolvedValue(undefined),
      getInfo: vi.fn().mockReturnValue({ usbVendorId: vendorId, usbProductId: productId }),
      setSignals,
      addEventListener: vi.fn(),
      removeEventListener: vi.fn(),
      writable: { getWriter: () => writer },
      readable: { getReader: () => reader },
    };
    return { port, setSignals };
  }

  it('skips DTR on Seeed XIAO (2886:8044)', async () => {
    const { port, setSignals } = mockPort(0x2886, 0x8044);
    vi.stubGlobal('navigator', {
      serial: { requestPort: vi.fn().mockResolvedValue(port) },
    });

    const { WebSerialTransport } = await import('./serialTransport');
    const transport = new WebSerialTransport();
    await transport.connect();
    expect(setSignals).not.toHaveBeenCalled();
    await transport.disconnect();
  });

  it('asserts DTR on generic CDC devices', async () => {
    const { port, setSignals } = mockPort(0x2341, 0x0043);
    vi.stubGlobal('navigator', {
      serial: { requestPort: vi.fn().mockResolvedValue(port) },
    });

    const { WebSerialTransport } = await import('./serialTransport');
    const transport = new WebSerialTransport();
    await transport.connect();
    expect(setSignals).toHaveBeenCalledWith({ dataTerminalReady: true, requestToSend: true });
    await transport.disconnect();
  });
});
