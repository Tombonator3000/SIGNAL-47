#!/bin/bash
# Runs the packaged build through the prologue's beats and views and checks the log. The game
# saves its own screenshots (-S47Shot). Needs a desktop session.
# Usage: Scripts/screenshots.sh OUTDIR [BUILD]
set -uo pipefail
OUT=${1:?usage: screenshots.sh OUTDIR [BUILD]}
BUILD=${2:-$HOME/scratch-bm/Signal47-build}
mkdir -p "$OUT"
fail=0
shot() { # name expected-log-pattern [extra args] [seconds]
  local log="$OUT/$1.log"
  timeout 150 "$BUILD/Linux/Signal47.sh" -S47SmokeTest -S47Seconds=${4:-8} ${3:-} -S47Shot="$OUT/$1.png" -windowed -ResX=1600 -ResY=900 -log > "$log" 2>&1
  if grep -q "smoke test OK" "$log" && grep -q "$2" "$log" && [ -s "$OUT/$1.png" ]; then echo "$1 OK"; else echo "$1 FAILED"; fail=1; fi
}
shot selftest "self test OK" "-S47SelfTest -S47Face=room"
shot room "web world .* 27 dishes" "-S47Face=room"
shot log "document log" "-S47Face=logbook -S47Use"
shot rack "walker placed at rack" "-S47Face=rack -S47Use"
shot crt "walker placed at crtCenter" "-S47Face=crtCenter -S47Jump=shift"
shot console "console opened" "-S47Face=crtCenter -S47Jump=residual -S47Open=console"
shot notebook "notebook opened" "-S47Face=room -S47Jump=residual -S47Open=notebook"
shot ops "ops terminal opened" "-S47Face=opsTerminal -S47Jump=residual -S47Open=ops"
shot locked "jumped to locked" "-S47Face=crtCenter -S47Jump=locked"
shot printed "jumped to printed" "-S47Face=printer -S47Jump=printed"
shot countdown "jumped to countdown" "-S47Face=crtCenter -S47Jump=countdown"
shot event "jumped to event" "-S47Face=window -S47Jump=event"
shot outside "walker placed at outside" "-S47Face=outside"
shot dish "walker placed at dish" "-S47Face=dish"
shot chapter2 "chapter ch2 begins" "-S47Jump=chapter2 -S47Face=room"
shot title "title shown" "-S47Title"
shot pause "pause opened" "-S47Face=room -S47Jump=residual -S47Open=pause"
# the drive: out of the yard with the throttle held and the autopilot steering, 20 s on, from the cab
shot drive "S47: trip 20 s" "-S47Drive=saro:station01 -S47Throttle=0.35" 24
# chapter one: from the room after the prologue, and at the S-03 motor bus in the service yard
shot chapter1 "chapter ch1 begins" "-S47Jump=chapter1 -S47Face=room"
shot yard "chapter ch1 begins" "-S47Jump=chapter1 -S47Face=at:11.2;-11.0;-1.5708;-0.2"
shot camera "frame01 exposed" "-S47Jump=chapter1 -S47Ch1Drive=photo"
exit $fail
