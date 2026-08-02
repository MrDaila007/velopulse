#!/usr/bin/env bash
# Build a signed iOS IPA for App Store / TestFlight distribution.
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
mobile_dir="${repo_root}/mobile-app"
flutterw="${repo_root}/tool/flutterw"
export_options="${mobile_dir}/ios/ExportOptions.plist"

if [[ "$(uname -s)" != "Darwin" ]]; then
  echo "iOS builds require macOS with Xcode." >&2
  exit 1
fi

if ! command -v xcodebuild >/dev/null 2>&1; then
  echo "Xcode command-line tools are required." >&2
  exit 1
fi

if [[ ! -f "${export_options}" ]]; then
  echo "Missing ${export_options}" >&2
  exit 1
fi

cd "${mobile_dir}"
"${flutterw}" pub get
"${flutterw}" build ipa --release --export-options-plist=ios/ExportOptions.plist

ipa_path="$(find build/ios/ipa -maxdepth 1 -name '*.ipa' -print -quit)"
if [[ -z "${ipa_path}" ]]; then
  echo "IPA was not produced. Check signing settings in Xcode." >&2
  exit 1
fi

echo
echo "IPA ready: ${mobile_dir}/${ipa_path}"
echo "Upload with Transporter, Xcode Organizer, or the ios-testflight GitHub workflow."
