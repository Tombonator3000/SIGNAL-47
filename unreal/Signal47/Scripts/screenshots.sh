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
  local log="$OUT/$1.log" secs=${4:-8} result=0
  rm -f "$OUT/$1.png"
  timeout $(( ${secs%.*} + 142 )) "$BUILD/Linux/Signal47.sh" -S47SmokeTest -S47Seconds=$secs ${3:-} -S47Shot="$OUT/$1.png" -windowed -ResX=1600 -ResY=900 -log > "$log" 2>&1 || result=$?
  if [ "$result" -eq 0 ] && ! grep -Eq "Critical error:|Assertion failed:|Signal 11 caught" "$log" && grep -q "smoke test OK" "$log" && grep -q "$2" "$log" && [ -s "$OUT/$1.png" ]; then echo "$1 OK"; else echo "$1 FAILED"; fail=1; fi
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
# the menus (S47Theme, the approved mockups): a night in case 1 autosaved first, so the title has
# something to continue and the case menus a tape; the title, every settings tab, the pause menu, its
# Controls (the settings at that tab, -S47Press feeds the menu its keys), saving and loading
shot menucase "chapter ch2 begins" "-S47Case=1 -S47Jump=chapter2 -S47Face=room" 12
shot title "title shown" "-S47Title"
for tab in game picture sound voices controls accessibility; do
  shot settings-$tab "S47: settings opened at $tab" "-S47Title -S47Open=settings:$tab"
done
shot pause "pause opened" "-S47Face=room -S47Jump=residual -S47Open=pause"
shot pausecase "pause opened" "-S47Case=1 -S47Jump=chapter2 -S47Face=room -S47Open=pause"
shot pausecontrols "S47: settings opened at controls" "-S47Case=1 -S47Jump=chapter2 -S47Face=room -S47Open=pause -S47Press=Down,Down,Down,Down,Enter"
shot load "S47: case menu load, [1-9]" "-S47Title -S47Open=load"
shot save "S47: case menu save" "-S47Case=1 -S47Jump=chapter2 -S47Face=room -S47Open=save"
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
shot call "S47: call .* at [0-9]* m" "-S47Jump=chapter1 -S47Face=outside -S47Call" 10
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
# (and the time away's own: taken west of the yard, then the nearest time away walks, its figure the one standing)
shot recordingAway "S47: recording walks time away 1" "-S47Jump=chapter1 -S47Face=at:-20;30;3.14;0 -S47Abduct=4 -S47RecWalk=near" 34
# chapter three: from the room after chapter two, and in the truck all the way out to STATION 01
# (the autopilot, about 132 s of driving) to look at the hut from the gate
shot chapter3 "chapter ch3 begins" "-S47Jump=chapter3 -S47Face=room"
shot station "ch3 drive: at the hut" "-S47Jump=chapter3 -S47Drive=saro:station01 -S47Auto=13 -S47Ch3Drive=hut" 160
# the time away and the trucks (S47Away): the warning on the ground west of the yard (cut to 4 s), the
# light, the single frames (the hand at 13 s), waking in the frost with one of them over you (20 s);
# the truck on B's four at the turn-off on the way out in chapter two; taken in the truck (its warning
# cut to 5 s): woken at the wheel 300 m on facing north, one of them on the bonnet; no stars at 026
# looked at; the lost nights on Night Shift's cork; FLASH's frames; the third one in front of the eye
shot abduct "S47: the time away: back at" "-S47Jump=chapter1 -S47Face=at:-20;30;3.14;0 -S47Abduct=4" 24
shot abductframes "S47: the time away: taken" "-S47Jump=chapter1 -S47Face=at:-20;30;3.14;0 -S47Abduct=4" 14
shot abductover "S47: the time away: one of them over you" "-S47Jump=chapter1 -S47Face=at:-20;30;3.14;0 -S47Abduct=4" 32
shot truckb "S47: the truck on B: four done" "-S47Jump=chapter2 -S47Drive=saro:station01 -S47Throttle=0.35" 30
shot trucktaken "S47: taken in the truck: woken at the wheel" "-S47Jump=chapter2 -S47Drive=saro:station01 -S47Throttle=0.35 -S47TruckTaken=5 -S47AwayAt=16" 48
shot starhole "S47: no stars at 026: noticed" "-S47Jump=chapter2 -S47Face=at:-20;30;3.14;0 -S47StarHole" 10
shot lostnights "S47: the lost nights: the binder and the green pins on the board, 1 book" "-S47Jump=chapter2 -S47Face=room -S47LostNights -S47AwayAt=1.5" 8
shot flash "S47: FLASH frames shown, 10" "-S47Jump=chapter1 -S47Face=room -S47Flash -S47AwayAt=6.6" 14
shot third "S47: the third one stands" "-S47Jump=chapter1 -S47Face=at:-20;30;3.14;0 -S47Third" 8
# chapter four: into room 6 from the motel's front, at her table (and the talk with the evidence on it)
shot chapter4 "chapter ch4 begins" "-S47Jump=chapter4 -S47Ch4Drive=room" 10
shot room6talk "ch4 drive: table: wrong" "-S47Jump=chapter4 -S47Ch4Drive=table" 10
# chapter five: five o'clock, by the truck on its pad at SARO; and in Mesa Diner on a stool, looking at
# the counter (the truck left on the lot, World.goDiner)
shot chapter5 "chapter ch5 begins" "-S47Jump=chapter5 -S47Face=at:10.6;8.6;1.5708;-0.1"
shot diner "ch5 drive: at the counter" "-S47Jump=chapter5 -S47Ch5Drive=counter" 10
# the town, road and motel: the developer-menu actions use the normal modules and exported places.
shot townphone "S47: town test phone, ringing" "-S47Jump=chapter1 -S47Town=phone" 9
shot townstop "S47: town road stop OK" "-S47Jump=chapter1 -S47Town=stop" 9
shot townreturn "S47: town test answer2, line" "-S47Jump=chapter1 -S47Town=answer2" 9
shot townlamps "S47: town test lamps, lamps" "-S47Jump=chapter1 -S47Town=lamps" 9
shot towncar "S47: town test car" "-S47Jump=chapter1 -S47Town=car" 9
shot towncutout "S47: town test cutout" "-S47Jump=chapter1 -S47Town=cutout" 9
shot roadside "S47: roadside test silence" "-S47Jump=chapter1 -S47Roadside=silence" 9
shot roadcar "S47: roadside test car" "-S47Jump=chapter1 -S47Roadside=car" 9
shot motelroof "S47: roadside test roof" "-S47Jump=chapter1 -S47Roadside=roof" 9
shot pasture "S47: pasture test still" "-S47Jump=chapter1 -S47Pasture=still" 9
shot pasturemissing "S47: pasture test short" "-S47Jump=chapter1 -S47Pasture=missing" 9
shot ninth "S47: ninth cow test down" "-S47Jump=chapter1 -S47Ninth" 9
shot moteltv "S47: motel television test carrier" "-S47Jump=chapter1 -S47MotelTv" 12
shot moteltvframe "S47: motel television save roundtrip OK" "-S47Jump=chapter1 -S47MotelTv=frame" 64
shot morning "S47: morning test held" "-S47Jump=chapter1 -S47Morning" 9
shot cutout "S47: cutout test foot placed" "-S47Jump=chapter1 -S47Face=outside -S47Cutout" 9
shot cutpost "S47: cutout test post placed" "-S47Jump=chapter1 -S47Face=outside -S47Cutout=post" 9
shot caliche "S47: Caliche conversation attendant, people 1" "-S47Jump=chapter1 -S47Caliche=attendant" 9
# the world's own life (S47WorldLife): a beacon's blink over the array (the log has its level), the
# night sky at 02:16 with the star hole over 026 coming in, a shooting star where the camera looks,
# the dawn at 05:20 behind the mesas, the flora along the road far from SARO on the drive, and the
# mesas round STATION 01 without the fog (dark against the sky, no rim)
shot beacon "S47: life clock .* beacon 0\.[5-9]" "-S47Jump=chapter1 -S47Face=outside -S47LifeLog" 8
shot starhole "S47: life clock 02:1.* hole 0\.[1-9]" "-S47Jump=chapter1 -S47Face=at:10;-12;-0.4538;0.5585 -S47LifeLog" 14
shot meteor "S47: shooting star now" "-S47Jump=chapter1 -S47Face=at:10;-12;0.6;0.7 -S47Meteor=3 -S47MeteorAim" 4.6
shot dawn "S47: life clock 05:2.* dawn 0\.[3-6]" "-S47Jump=chapter1 -S47Face=at:10;-12;-1.379;0.08 -S47Clock=05:20 -S47LifeLog" 12
shot roadflora "S47: life clock .* road:[1-9]" "-S47Drive=saro:station01 -S47Throttle=0.35 -S47LifeLog" 110
shot mesas "S47: the world's own life" "-S47Jump=chapter3 -S47Drive=saro:station01 -S47Auto=13 -S47Ch3Drive=hut" 30
# Chapter six: from the diner's lot, then the developer menu's exact line pose with autopilot.
shot chapter6 "chapter ch6 begins" "-S47Jump=chapter6" 8
shot chapter6event "S47: ch6 THE EVENT begins, 47 seconds" "-S47Jump=chapter6:line -S47Auto=20" 24
# Full event and pull-out first; the ending title uses real time once the 3D world has stopped.
shot chapter6title "S47: ch6 ending title" "-S47Jump=chapter6:line -S47Auto=20" 99
exit $fail
