#!/usr/bin/env bash
set -euo pipefail

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
if [[ ! -x "$script_dir/.venv/bin/python" ]]; then
  printf 'Run %s/setup.sh first.\n' "$script_dir" >&2
  exit 1
fi

cd "$script_dir"
exec xvfb-run -a "$script_dir/.venv/bin/python" -m unittest discover -s tests -v
