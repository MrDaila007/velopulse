#!/usr/bin/env bash
set -euo pipefail

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
vendor_dir="$script_dir/.vendor"
sim_ref=b82887d23515ddc7ce9af48ef8a2d3447d46aee6
u8g2_ref=ab9e48b2228351e9476682a70b7f3ee4909cd585

clone_at_ref() {
  local url=$1
  local destination=$2
  local revision=$3
  if [[ ! -d "$destination/.git" ]]; then
    git clone --filter=blob:none "$url" "$destination"
  fi
  git -C "$destination" fetch --depth 1 origin "$revision"
  git -C "$destination" checkout --detach "$revision"
}

mkdir -p "$vendor_dir"
clone_at_ref https://github.com/colinoflynn/u8g2-python-simulator.git \
  "$vendor_dir/u8g2-python-simulator" "$sim_ref"
clone_at_ref https://github.com/olikraus/u8g2.git "$vendor_dir/u8g2" "$u8g2_ref"

python3 -m venv "$script_dir/.venv"
"$script_dir/.venv/bin/python" -m pip install --disable-pip-version-check \
  -r "$script_dir/requirements.txt"

printf 'Simulator ready. Run: %s/run_gui.sh\n' "$script_dir"
