#!/usr/bin/env bash
# Shared local-environment loader and path discovery for Zephyr commands.

if [[ "${BIKECOMP_ENV_SH_LOADED:-0}" == "1" ]]; then
  return 0 2>/dev/null || exit 0
fi
BIKECOMP_ENV_SH_LOADED=1

BIKECOMP_ZEPHYR_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIKECOMP_REPO_ROOT="$(cd "${BIKECOMP_ZEPHYR_ROOT}/.." && pwd)"
BIKECOMP_ENV_FILE="${BIKECOMP_ENV_FILE:-${BIKECOMP_ZEPHYR_ROOT}/.env}"

bikecomp_prepend_path() {
  local directory="$1"
  [[ -d "${directory}" ]] || return 0
  case ":${PATH}:" in
    *":${directory}:"*) ;;
    *) export PATH="${directory}:${PATH}" ;;
  esac
}

bikecomp_source_local_env() {
  [[ -f "${BIKECOMP_ENV_FILE}" ]] || return 0

  local names=(
    ZEPHYR_BASE ZEPHYR_SDK_INSTALL_DIR WEST_TOP WEST_BIN ZEPHYR_PYTHON
    U8G2_ROOT USER_CACHE_DIR CCACHE_DIR CCACHE_TEMPDIR BOARD BUILD_DIR OVERLAY
  )
  local name
  declare -A supplied=()
  for name in "${names[@]}"; do
    if [[ -v "${name}" ]]; then
      supplied["${name}"]="${!name}"
    fi
  done

  # shellcheck disable=SC1090
  source "${BIKECOMP_ENV_FILE}"

  # Explicit command/environment values take precedence over the local file.
  for name in "${names[@]}"; do
    if [[ -v "supplied[${name}]" ]]; then
      printf -v "${name}" '%s' "${supplied[${name}]}"
      export "${name}"
    fi
  done
}

bikecomp_find_marker() {
  local suffix="$1"
  local roots="${BIKECOMP_SEARCH_ROOTS:-${HOME}:/opt:/data}"
  local root marker
  local old_ifs="${IFS}"
  IFS=':'
  for root in ${roots}; do
    [[ -d "${root}" ]] || continue
    marker="$(find "${root}" -maxdepth 7 -type f -path "*/${suffix}" -print -quit 2>/dev/null || true)"
    if [[ -n "${marker}" ]]; then
      IFS="${old_ifs}"
      printf '%s\n' "${marker}"
      return 0
    fi
  done
  IFS="${old_ifs}"
  return 1
}

bikecomp_discover_zephyr_base() {
  if [[ -n "${ZEPHYR_BASE:-}" && -f "${ZEPHYR_BASE}/zephyr-env.sh" ]]; then
    return 0
  fi
  unset ZEPHYR_BASE

  local candidate marker
  for candidate in \
    "${BIKECOMP_REPO_ROOT}/deps/zephyr" \
    "${BIKECOMP_REPO_ROOT}/zephyr" \
    "${HOME}/zephyrproject-v4.4/zephyr" \
    "${HOME}/zephyrproject/zephyr" \
    /opt/zephyrproject/zephyr \
    /data/zephyrproject-v4.4/zephyr; do
    if [[ -f "${candidate}/zephyr-env.sh" ]]; then
      export ZEPHYR_BASE="${candidate}"
      return 0
    fi
  done

  [[ "${BIKECOMP_AUTO_DISCOVER:-1}" == "1" ]] || return 1
  marker="$(bikecomp_find_marker 'zephyr/zephyr-env.sh' || true)"
  if [[ -n "${marker}" ]]; then
    export ZEPHYR_BASE="${marker%/zephyr-env.sh}"
    return 0
  fi
  return 1
}

bikecomp_discover_west() {
  if [[ -z "${WEST_TOP:-}" || ! -d "${WEST_TOP}/.west" ]]; then
    unset WEST_TOP
    local zephyr_parent=""
    if [[ -n "${ZEPHYR_BASE:-}" ]]; then
      zephyr_parent="$(dirname "${ZEPHYR_BASE}")"
    fi
    local candidate
    for candidate in \
      "${zephyr_parent}" \
      "$(dirname "${zephyr_parent:-/}")" \
      "${BIKECOMP_REPO_ROOT}"; do
      if [[ -d "${candidate}/.west" ]]; then
        export WEST_TOP="${candidate}"
        break
      fi
    done
  fi

  if [[ -n "${WEST_BIN:-}" && -x "${WEST_BIN}" ]]; then
    bikecomp_prepend_path "$(dirname "${WEST_BIN}")"
  else
    unset WEST_BIN
    local candidate
    for candidate in \
      "${WEST_TOP:-}/.venv/bin/west" \
      "${BIKECOMP_REPO_ROOT}/.venv/bin/west"; do
      if [[ -x "${candidate}" ]]; then
        export WEST_BIN="${candidate}"
        bikecomp_prepend_path "$(dirname "${candidate}")"
        break
      fi
    done
    if [[ -z "${WEST_BIN:-}" ]] && command -v west >/dev/null 2>&1; then
      export WEST_BIN="$(command -v west)"
    fi
  fi

  if [[ -n "${ZEPHYR_PYTHON:-}" && -x "${ZEPHYR_PYTHON}" ]]; then
    :
  elif [[ -n "${WEST_BIN:-}" && -x "$(dirname "${WEST_BIN}")/python" ]]; then
    export ZEPHYR_PYTHON="$(dirname "${WEST_BIN}")/python"
  else
    export ZEPHYR_PYTHON="$(command -v python3 || true)"
  fi
}

bikecomp_discover_sdk() {
  if [[ -n "${ZEPHYR_SDK_INSTALL_DIR:-}" &&
        -f "${ZEPHYR_SDK_INSTALL_DIR}/cmake/Zephyr-sdkConfig.cmake" ]]; then
    return 0
  fi
  unset ZEPHYR_SDK_INSTALL_DIR

  local candidate marker
  for candidate in \
    "${BIKECOMP_REPO_ROOT}/zephyr-sdk-1.0.0" \
    "${HOME}/zephyr-sdk-1.0.0" \
    "${HOME}/zephyr-sdk-0.17.0" \
    "${HOME}/zephyr-sdk-0.16.8" \
    /opt/zephyr-sdk-1.0.0 \
    /data/zephyr-sdk-1.0.0; do
    if [[ -f "${candidate}/cmake/Zephyr-sdkConfig.cmake" ]]; then
      export ZEPHYR_SDK_INSTALL_DIR="${candidate}"
      return 0
    fi
  done

  [[ "${BIKECOMP_AUTO_DISCOVER:-1}" == "1" ]] || return 1
  marker="$(bikecomp_find_marker 'cmake/Zephyr-sdkConfig.cmake' || true)"
  if [[ -n "${marker}" ]]; then
    export ZEPHYR_SDK_INSTALL_DIR="${marker%/cmake/Zephyr-sdkConfig.cmake}"
    return 0
  fi
  return 1
}

bikecomp_discover_u8g2() {
  if [[ -n "${U8G2_ROOT:-}" && -f "${U8G2_ROOT}/csrc/u8g2.h" ]]; then
    return 0
  fi
  unset U8G2_ROOT

  local candidate marker
  for candidate in \
    "${BIKECOMP_REPO_ROOT}/deps/u8g2" \
    "${WEST_TOP:-}/deps/u8g2" \
    "${WEST_TOP:-}/modules/u8g2" \
    "${BIKECOMP_REPO_ROOT}/simulator/.vendor/u8g2"; do
    if [[ -f "${candidate}/csrc/u8g2.h" ]]; then
      export U8G2_ROOT="${candidate}"
      return 0
    fi
  done

  [[ "${BIKECOMP_AUTO_DISCOVER:-1}" == "1" ]] || return 1
  marker="$(bikecomp_find_marker 'u8g2/csrc/u8g2.h' || true)"
  if [[ -n "${marker}" ]]; then
    export U8G2_ROOT="${marker%/csrc/u8g2.h}"
    return 0
  fi
  return 1
}

bikecomp_load_env() {
  bikecomp_source_local_env
  bikecomp_prepend_path "${HOME}/.local/bin"
  bikecomp_discover_zephyr_base || true
  bikecomp_discover_west
  bikecomp_discover_sdk || true
  bikecomp_discover_u8g2 || true

  export USER_CACHE_DIR="${USER_CACHE_DIR:-${BIKECOMP_ZEPHYR_ROOT}/.cache/zephyr}"
  export CCACHE_DIR="${CCACHE_DIR:-${BIKECOMP_ZEPHYR_ROOT}/.cache/ccache}"
  export CCACHE_TEMPDIR="${CCACHE_TEMPDIR:-${CCACHE_DIR}/tmp}"
}

bikecomp_require_build_env() {
  local missing=()
  [[ -n "${ZEPHYR_BASE:-}" ]] || missing+=(ZEPHYR_BASE)
  [[ -n "${ZEPHYR_SDK_INSTALL_DIR:-}" ]] || missing+=(ZEPHYR_SDK_INSTALL_DIR)
  [[ -n "${WEST_TOP:-}" ]] || missing+=(WEST_TOP)
  [[ -n "${WEST_BIN:-}" ]] || missing+=(WEST_BIN)
  [[ -n "${U8G2_ROOT:-}" ]] || missing+=(U8G2_ROOT)
  if (( ${#missing[@]} > 0 )); then
    echo "Missing Zephyr environment: ${missing[*]}" >&2
    echo "Run ${BIKECOMP_ZEPHYR_ROOT}/scripts/configure-env.sh or bootstrap.sh." >&2
    return 1
  fi
  if ! "${ZEPHYR_PYTHON}" -c \
    'import sys; raise SystemExit(sys.version_info < (3, 12))'; then
    echo "Zephyr 4.4 requires Python 3.12 or newer; found ${ZEPHYR_PYTHON}." >&2
    echo "Run ${BIKECOMP_ZEPHYR_ROOT}/scripts/bootstrap.sh to create a local venv." >&2
    return 1
  fi
}

bikecomp_print_env() {
  printf 'ZEPHYR_BASE=%s\n' "${ZEPHYR_BASE:-<not found>}"
  printf 'ZEPHYR_SDK_INSTALL_DIR=%s\n' "${ZEPHYR_SDK_INSTALL_DIR:-<not found>}"
  printf 'WEST_TOP=%s\n' "${WEST_TOP:-<not found>}"
  printf 'WEST_BIN=%s\n' "${WEST_BIN:-<not found>}"
  printf 'ZEPHYR_PYTHON=%s\n' "${ZEPHYR_PYTHON:-<not found>}"
  printf 'U8G2_ROOT=%s\n' "${U8G2_ROOT:-<not found>}"
  printf 'USER_CACHE_DIR=%s\n' "${USER_CACHE_DIR:-<not found>}"
  printf 'CCACHE_DIR=%s\n' "${CCACHE_DIR:-<not found>}"
  printf 'CCACHE_TEMPDIR=%s\n' "${CCACHE_TEMPDIR:-<not found>}"
}
