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
shot radio "S47: radio caliche live, AM 1420" "-S47Radio=caliche -S47Jump=chapter1 -S47Face=room"
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
# the phones, the archive and the records (S47Phones): the archive's phone lifted in the records room
# (its line open), the array tipped to the horizon seen from the control room's window, the steps on
# the records room's roof over the work table, the office's crank telephone answered, T. Vega's field
# book and the envelope held up to the lamp, a closer look at the shift log, and a time away's thread
shot archive "S47: archive phase line" "-S47Jump=chapter2 -S47Face=at:-3.5;9.6;1.5708;0 -S47Archive=lift" 10
shot arraytip "S47: array down" "-S47Jump=chapter2 -S47Face=window -S47Archive=tip" 32
shot roof "S47: roof steps come" "-S47Jump=chapter2 -S47Face=at:-4.2;9.6;3.1416;0.5 -S47Roof" 10
shot office "S47: office telephone line" "-S47Jump=chapter1 -S47Face=at:-45.7;674.3;0;0.05 -S47Office=answer" 12
shot fieldbook "S47: field book opened" "-S47Jump=chapter1 -S47Face=room -S47FieldBook" 8
shot envelope "S47: envelope held up to the lamp" "-S47Jump=chapter1 -S47Face=room -S47FieldBook=envelope" 10
shot zoom "S47: closer look: Dale's shift log" "-S47Jump=residual -S47Face=logbook -S47Zoom" 8
shot same "S47: same thread for time away 1" "-S47Jump=chapter1 -S47Face=room -S47Same" 8
exit $fail
