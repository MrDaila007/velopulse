#!/usr/bin/env bash
set -euo pipefail

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
scenario=${1:-demo}
display_height=${2:-64}

if [[ "$display_height" != 32 && "$display_height" != 64 ]]; then
  printf 'Display height must be 32 or 64.\n' >&2
  exit 2
fi

if [[ ! -x "$script_dir/.venv/bin/python" || \
      ! -f "$script_dir/.vendor/u8g2-python-simulator/u8g2_sim.py" ]]; then
  printf 'Run %s/setup.sh first.\n' "$script_dir" >&2
  exit 1
fi

cd "$script_dir"
export BIKECOMP_SIM_SCENARIO="$scenario"
export BIKECOMP_DISPLAY_HEIGHT="$display_height"
exec "$script_dir/.venv/bin/python" \
  "$script_dir/.vendor/u8g2-python-simulator/u8g2_sim.py" \
  --file "$script_dir/draw_bikecomp.py" \
  --width 128 --height "$display_height" --scale 6 --poll 200 \
  --u8g2-root "$script_dir/.vendor/u8g2"
