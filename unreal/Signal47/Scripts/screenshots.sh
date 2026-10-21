#!/bin/bash
# Runs the packaged build at a few spots in the station and checks the log. The game saves its
# own screenshots (-S47Shot). Needs a desktop session.
# Usage: Scripts/screenshots.sh OUTDIR [BUILD]
set -uo pipefail
OUT=${1:?usage: screenshots.sh OUTDIR [BUILD]}
BUILD=${2:-$HOME/scratch-bm/Signal47-build}
mkdir -p "$OUT"
fail=0
shot() { # name spot expected-log-pattern [extra args]
  local log="$OUT/$1.log"
  timeout 120 "$BUILD/Linux/Signal47.sh" -S47SmokeTest -S47Seconds=8 -S47Face="$2" ${4:-} -S47Shot="$OUT/$1.png" -windowed -ResX=1600 -ResY=900 -log > "$log" 2>&1
  if grep -q "smoke test OK" "$log" && grep -q "$3" "$log" && [ -s "$OUT/$1.png" ]; then echo "$1 OK"; else echo "$1 FAILED"; fail=1; fi
}
shot room room "station built"
shot window window "walker placed at window"
shot crt crt "walker placed at crt"
shot terminal crt "terminal opened" "-S47Use"
shot outside outside "walker placed at outside"
shot dish dish "walker placed at dish"
exit $fail
