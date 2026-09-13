#!/usr/bin/env bash
# API-controlled isolated player; does not send desktop input or unlock a screen.
set -euo pipefail
cd "$(dirname "$0")/../.."
signal47_hybrid_player=$(realpath "${1:?isolated player}")
signal47_hybrid_backend=${2:?vulkan}
signal47_hybrid_out=${3:?fresh evidence directory}
[[ "$signal47_hybrid_backend" == vulkan ]] || { echo 'This Scan25 runner currently permits only the reviewed Vulkan path.' >&2; exit 2; }
[[ ! -e "$signal47_hybrid_out" ]] || { echo 'Preserve earlier evidence; use a fresh directory.' >&2; exit 2; }
mkdir -p "$signal47_hybrid_out"
signal47_hybrid_out=$(realpath "$signal47_hybrid_out")
export DISPLAY="${DISPLAY:-:0}"
export XDG_RUNTIME_DIR="${XDG_RUNTIME_DIR:-/run/user/$(id -u)}"
if [[ -z "${XAUTHORITY:-}" ]]; then
  for signal47_hybrid_auth in "$XDG_RUNTIME_DIR"/xauth_*; do
    if [[ -r "$signal47_hybrid_auth" ]]; then export XAUTHORITY="$signal47_hybrid_auth"; break; fi
  done
fi
exec 9>"$XDG_RUNTIME_DIR/signal47-gauntlet.lock"
flock -n 9 || { echo 'Another SIGNAL47 graphical task is active.' >&2; exit 1; }
set +e
bash Automation/Splat21/bounded.sh 2147483648 120 /usr/bin/time -v -o "$signal47_hybrid_out/process-resources.txt" "$signal47_hybrid_player" -force-vulkan -screen-width 1280 -screen-height 800 -screen-fullscreen 0 --scan25-checks --scan25-out "$signal47_hybrid_out" -logFile "$signal47_hybrid_out/player.log" > "$signal47_hybrid_out/stdout.log" 2>&1
signal47_hybrid_exit=$?
set -e
printf '%s\n' "$signal47_hybrid_exit" > "$signal47_hybrid_out/exit-code.txt"
exit "$signal47_hybrid_exit"
