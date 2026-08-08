import { spawn } from 'node:child_process';

const url = process.env.BIKECOMP_URL ?? 'http://localhost:5173/scan';

/** Chrome/Chromium on Linux often hides Web Bluetooth until flags are enabled. */
const chromeBleArgs = [
  '--new-window',
  '--enable-features=WebBluetooth,WebBluetoothAPIs',
  '--enable-blink-features=WebBluetooth',
  url,
];

const launchers = [
  { cmd: 'google-chrome', args: chromeBleArgs },
  { cmd: 'google-chrome-stable', args: chromeBleArgs },
  { cmd: 'chromium', args: chromeBleArgs },
  { cmd: 'chromium-browser', args: chromeBleArgs },
  { cmd: 'microsoft-edge', args: chromeBleArgs },
  // Fallback without flags (user may enable chrome://flags manually)
  { cmd: 'google-chrome', args: ['--new-window', url] },
  { cmd: 'xdg-open', args: [url] },
];

function tryLaunch(cmd, args = []) {
  return new Promise((resolve) => {
    const child = spawn(cmd, args, { detached: true, stdio: 'ignore' });
    child.on('error', () => resolve(false));
    child.on('spawn', () => {
      child.unref();
      resolve(true);
    });
  });
}

for (const { cmd, args = [url] } of launchers) {
  if (await tryLaunch(cmd, args)) {
    const withBleFlags = args.includes('--enable-blink-features=WebBluetooth');
    console.log(`Opened ${url} via ${cmd}${withBleFlags ? ' (Web Bluetooth flags)' : ''}`);
    process.exit(0);
  }
}

console.error(`Could not open a browser. Open manually: ${url}`);
console.error('Linux: enable chrome://flags/#enable-experimental-web-platform-features');
process.exit(1);
