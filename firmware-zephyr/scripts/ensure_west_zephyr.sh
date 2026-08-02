#!/usr/bin/env bash
# CI clones only zephyr/; tracked .west/config expects a full west workspace.
# Twister follows west and fails on missing deps/modules. Hide .west when incomplete.

hide_west_if_incomplete() {
  local repo_root="$1"
  local west_dir="${repo_root}/.west"
  if [ ! -d "${west_dir}" ]; then
    return 0
  fi
  if [ -d "${repo_root}/deps/modules" ]; then
    return 0
  fi
  mv "${west_dir}" "${repo_root}/.west.twister-hidden"
  export BIKECOMP_WEST_HIDDEN=1
}

restore_west_if_hidden() {
  local repo_root="$1"
  if [ "${BIKECOMP_WEST_HIDDEN:-}" = "1" ] &&
    [ -d "${repo_root}/.west.twister-hidden" ]; then
    mv "${repo_root}/.west.twister-hidden" "${repo_root}/.west"
    unset BIKECOMP_WEST_HIDDEN
  fi
}
