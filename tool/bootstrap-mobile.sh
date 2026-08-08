#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
tooling_dir="${repo_root}/.tooling"
downloads_dir="${tooling_dir}/downloads"
flutter_dir="${tooling_dir}/flutter"
jdk_dir="${tooling_dir}/jdk-17"

flutter_archive="${downloads_dir}/flutter_linux_3.44.7-stable.tar.xz"
flutter_url="https://storage.googleapis.com/flutter_infra_release/releases/stable/linux/flutter_linux_3.44.7-stable.tar.xz"
flutter_sha256="a0edd646c159c0e816788c0e46a4f071199c1320495898f5a679599b583a05a4"

jdk_archive="${downloads_dir}/OpenJDK17U-jdk_x64_linux_hotspot_17.0.20_8.tar.gz"
jdk_url="https://github.com/adoptium/temurin17-binaries/releases/download/jdk-17.0.20%2B8/OpenJDK17U-jdk_x64_linux_hotspot_17.0.20_8.tar.gz"
jdk_sha256="be7668bc030d578b83d6d5ef9221d6d6729bbbca8cf94a7d52e16ac68b5a5a35"

android_sdk_dir="${tooling_dir}/android-sdk"
android_tools_archive="${downloads_dir}/commandlinetools-linux-15859902_latest.zip"
android_tools_url="https://dl.google.com/android/repository/commandlinetools-linux-15859902_latest.zip"
android_tools_sha256="4e4c464f145a7512b57d088ac6c278c03c9eea610886b35a5e0804e74eedf583"
sdkmanager="${android_sdk_dir}/cmdline-tools/latest/bin/sdkmanager"

mkdir -p "${downloads_dir}"

download_and_verify() {
  local url="$1"
  local archive="$2"
  local expected_sha256="$3"

  if [[ ! -f "${archive}" ]]; then
    curl --fail --location --retry 3 --output "${archive}" "${url}"
  fi

  echo "${expected_sha256}  ${archive}" | sha256sum --check -
}

if [[ ! -x "${flutter_dir}/bin/flutter" ]]; then
  download_and_verify "${flutter_url}" "${flutter_archive}" "${flutter_sha256}"
  mkdir -p "${flutter_dir}"
  tar --extract --xz --file "${flutter_archive}" \
    --strip-components=1 --directory "${flutter_dir}"
fi

if [[ ! -x "${jdk_dir}/bin/java" ]]; then
  download_and_verify "${jdk_url}" "${jdk_archive}" "${jdk_sha256}"
  mkdir -p "${jdk_dir}"
  tar --extract --gzip --file "${jdk_archive}" \
    --strip-components=1 --directory "${jdk_dir}"
fi

if [[ ! -x "${sdkmanager}" ]]; then
  if ! command -v unzip >/dev/null 2>&1; then
    echo "The bootstrap requires unzip." >&2
    exit 1
  fi
  download_and_verify \
    "${android_tools_url}" "${android_tools_archive}" "${android_tools_sha256}"
  extract_dir="$(mktemp -d "${tooling_dir}/android-tools.XXXXXX")"
  unzip -q "${android_tools_archive}" -d "${extract_dir}"
  mkdir -p "${android_sdk_dir}/cmdline-tools"
  mv "${extract_dir}/cmdline-tools" "${android_sdk_dir}/cmdline-tools/latest"
  rmdir "${extract_dir}"
fi

if [[ ! -d "${android_sdk_dir}/platforms/android-36" || \
      ! -d "${android_sdk_dir}/build-tools/36.0.0" || \
      ! -d "${android_sdk_dir}/ndk/28.2.13676358" || \
      ! -d "${android_sdk_dir}/cmake/3.22.1" ]]; then
  export JAVA_HOME="${jdk_dir}"
  set +o pipefail
  yes | "${sdkmanager}" --sdk_root="${android_sdk_dir}" --licenses >/dev/null
  set -o pipefail
  "${sdkmanager}" --sdk_root="${android_sdk_dir}" \
    "platform-tools" "platforms;android-33" "platforms;android-35" \
    "platforms;android-36" "build-tools;36.0.0" \
    "ndk;28.2.13676358" "cmake;3.22.1"
fi

"${repo_root}/tool/flutterw" config --no-analytics
"${repo_root}/tool/flutterw" --version
