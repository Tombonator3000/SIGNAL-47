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
# the night's layer (S47Night): the night's events as the developer menu starts them (steps behind the
# player, the control room's phone once), a sound out in the dark with the flashlight on, the figure at
# the edge of the view behind the array, and a free frame for the album (the face on it, the notebook
# open on the print coming up)
shot nightsteps "S47: night event written: .*footsteps behind me" "-S47Jump=chapter1 -S47Face=outside -S47Night=steps" 10
shot nightphone "S47: night event written: .*phone in the control room rang once" "-S47Jump=chapter1 -S47Face=outside -S47Night=phone" 10
shot call "S47: call alien" "-S47Jump=chapter1 -S47Face=outside -S47Call" 10
shot glimpse "S47: glimpse at S-11" "-S47Jump=chapter1 -S47Face=at:10;-12;-0.45;0.03 -S47Glimpse=S-11" 8
shot album "S47: free frame free1" "-S47Jump=chapter1 -S47Face=at:10.4;-11.6;0.2;0.31 -S47Album" 8
shot albumface "S47: album opened" "-S47Jump=chapter1 -S47Face=at:-20;30;3.14;0 -S47Light -S47Album=face+open" 9
# SARO's own happenings (S47SaroNight): SARO going dark from the yard (the relay's forty seconds passed, the
# eyes turned to the figure on a roof once it stands), the array put back on the schedule from the console,
# the dishes that follow through a warning of the time away (played here until the abduction is ported),
# the three lights on B past a truck driven south, reel 047 and Earl's reel at the deck, and the recording
# walking a time away kept 60 m off (the flashlight on its band)
shot saroDark "S47: saro dark figure" "-S47Jump=chapter1 -S47Face=at:10;-11;1.82;0.12 -S47Dark=look" 16
shot arrayBack "S47: array hold" "-S47Jump=chapter1 -S47Face=dish -S47Array" 24
shot dishesFollow "S47: dishes follow follow" "-S47Jump=chapter1 -S47Face=at:9.5;-18.5;0.1;0.25 -S47Follow" 20
shot lineLights "S47: line lights pace" "-S47Jump=chapter1 -S47Face=at:-19;30;2.0;0.05 -S47LineLights" 10
shot reel047 "S47: reel 047 hole" "-S47Jump=chapter1 -S47Face=decoder -S47Reel047" 26
shot reelTape "S47: reel tape heard to the end" "-S47Jump=chapter1 -S47Face=decoder -S47ReelTape" 16
shot recording "S47: recording walk" "-S47Jump=chapter1 -S47Face=at:-24;120;2.64;0 -S47RecWalk -S47Light" 12
exit $fail
