#!/usr/bin/env bash
set -euo pipefail

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
scenario=${1:-demo}

if [[ ! -x "$script_dir/.venv/bin/python" || \
      ! -f "$script_dir/.vendor/u8g2-python-simulator/u8g2_sim.py" ]]; then
  printf 'Run %s/setup.sh first.\n' "$script_dir" >&2
  exit 1
fi

cd "$script_dir"
export BIKECOMP_SIM_SCENARIO="$scenario"
exec "$script_dir/.venv/bin/python" \
  "$script_dir/.vendor/u8g2-python-simulator/u8g2_sim.py" \
  --file "$script_dir/draw_bikecomp.py" \
  --width 128 --height 32 --scale 6 --poll 200 \
  --u8g2-root "$script_dir/.vendor/u8g2"
