#!/usr/bin/env bash
# Isolated graphical player scenario; no keyboard/mouse input or screen unlocking.
set -euo pipefail
player=$(realpath "${1:?player path}")
backend=${2:?vulkan}
output=${3:?fresh result directory}
[[ "$backend" == vulkan ]] || { echo "Splat21 supports Vulkan only; the OpenGL probe failed. No player launched." >&2; exit 2; }
[[ ! -e "$output" ]] || { echo 'Use a fresh result directory.' >&2; exit 2; }
mkdir -p "$output"
output=$(realpath "$output")
export DISPLAY="${DISPLAY:-:0}"
export XDG_RUNTIME_DIR="${XDG_RUNTIME_DIR:-/run/user/$(id -u)}"
if [[ -z "${XAUTHORITY:-}" ]]; then
  for signal47_auth in "$XDG_RUNTIME_DIR"/xauth_*; do
    if [[ -r "$signal47_auth" ]]; then export XAUTHORITY="$signal47_auth"; break; fi
  done
fi
exec 9>"$XDG_RUNTIME_DIR/signal47-gauntlet.lock"
flock -n 9 || { echo 'Another graphical SIGNAL47 test is running.' >&2; exit 1; }
set +e
bash "$(dirname "$(realpath "$0")")/bounded.sh" 2147483648 60 "$player" "-force-$backend" -screen-width 1280 -screen-height 800 -screen-fullscreen 0 --probe-output "$output" -logFile "$output/player.log" > "$output/stdout.log" 2>&1
signal47_exit=$?
set -e
printf '%s\n' "$signal47_exit" > "$output/exit-code.txt"
exit "$signal47_exit"
