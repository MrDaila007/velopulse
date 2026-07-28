#!/usr/bin/env bash
set -euo pipefail

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
project_dir=$(cd -- "$script_dir/.." && pwd)
build_dir="$script_dir/.build"
mkdir -p "$build_dir"

"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror \
  -I "$project_dir/firmware/include" \
  -I "$project_dir/firmware/lib/domain" \
  "$script_dir/firmware_renderer.cpp" \
  "$project_dir/firmware/lib/domain/display_formatter.cpp" \
  "$project_dir/firmware/lib/domain/display_layout.cpp" \
  -o "$build_dir/firmware_renderer"
