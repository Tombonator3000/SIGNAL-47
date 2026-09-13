#!/usr/bin/env bash
# Packaged isolated experiment; never reads or changes SIGNAL47 save files.
set -euo pipefail
cd "$(dirname "$0")"
export DISPLAY="${DISPLAY:-:0}"
export XDG_RUNTIME_DIR="${XDG_RUNTIME_DIR:-/run/user/$(id -u)}"
if [[ -z "${XAUTHORITY:-}" ]]; then
  for signal47_hybrid_auth in "$XDG_RUNTIME_DIR"/xauth_*; do
    if [[ -r "$signal47_hybrid_auth" ]]; then export XAUTHORITY="$signal47_hybrid_auth"; break; fi
  done
fi
exec 9>"$XDG_RUNTIME_DIR/signal47-gauntlet.lock"
flock -n 9 || { echo 'En annen SIGNAL47-prøve kjører allerede.' >&2; exit 1; }
signal47_hybrid_args=()
signal47_hybrid_prefix=()
signal47_hybrid_seconds=900
if [[ ${1:-} == --checks || ${1:-} == --benchmark ]]; then
  [[ $# == 2 ]] || { echo 'Bruk --checks NY_RESULTATMAPPE' >&2; exit 2; }
  [[ ! -e "$2" ]] || { echo 'Resultatmappen finnes allerede.' >&2; exit 2; }
  mkdir -p "$2"
  signal47_hybrid_out=$(realpath "$2")
  signal47_hybrid_args=(--scan25-checks --scan25-out "$signal47_hybrid_out" -logFile "$signal47_hybrid_out/player.log")
  if [[ $1 == --benchmark ]]; then signal47_hybrid_args=(--scan25-benchmark --scan25-out "$signal47_hybrid_out" -logFile "$signal47_hybrid_out/player.log"); fi
  signal47_hybrid_prefix=(/usr/bin/time -v -o "$signal47_hybrid_out/process-resources.txt")
  signal47_hybrid_seconds=120
  if [[ $1 == --benchmark ]]; then signal47_hybrid_seconds=180; fi
elif [[ $# != 0 ]]; then
  echo 'Start uten argumenter, eller bruk --checks NY_RESULTATMAPPE.' >&2; exit 2
fi
exec bash bounded.sh 2147483648 "$signal47_hybrid_seconds" "${signal47_hybrid_prefix[@]}" "$PWD/Scan25.x86_64" -force-vulkan -screen-width 1280 -screen-height 800 -screen-fullscreen 0 "${signal47_hybrid_args[@]}"
