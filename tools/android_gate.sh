#!/usr/bin/env bash
# Automatic portion of Android hardware gate (Э5 DoD).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
MOBILE="$ROOT/mobile-app"
FLUTTERW="$ROOT/tool/flutterw"

echo "== Android automatic gate =="
if [[ -x "$ROOT/tool/bootstrap-mobile.sh" ]]; then
  "$ROOT/tool/bootstrap-mobile.sh"
fi

cd "$MOBILE"
"$FLUTTERW" pub get
"$ROOT/.tooling/flutter/bin/dart" format --set-exit-if-changed lib test
"$FLUTTERW" analyze
"$FLUTTERW" test
"$FLUTTERW" build apk --release

echo
echo "PASS: format, analyze, tests, release APK"
echo "Manual hardware gate (phone + XIAO):"
echo "  1. Discovery <= 5s (measure 3 runs)"
echo "  2. 10/10 connect success"
echo "  3. Bond survives XIAO reboot"
echo "  4. Write-then-verify: wheel, timeout, brightness 10/60/100, auto-start, page order"
echo "  5. MVP commands with CommandResult"
echo "  6. Reconnect after link loss"
echo "  7. Permission flows on Android <=11 and >=12"
