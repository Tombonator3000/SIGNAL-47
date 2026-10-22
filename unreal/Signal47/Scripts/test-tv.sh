#!/bin/bash
# The TVs' local broadcast (S47TvProgramme, from Scripts/prepare-tv.py) and the office TV's
# 47-second event, in the running game. Usage: Scripts/test-tv.sh OUTDIR [BUILD]
#   BUILD  a packaged build (BUILD/Linux/Signal47.sh); without it the editor binary plays this
#          project with -game (after Scripts/build-editor.sh), with the generated content in Content.
# Each run waits its turn (no other UnrealEditor or packaged Signal47 running: they share the
# graphics card). A run whose log stands still for 90 s has hung while starting (the earlier
# version of this script waited out its whole time limit on one such start, and called it a
# failure of the recording): it is stopped and tried once more. Logs and shots go to OUTDIR.
set -uo pipefail
OUT=${1:?usage: test-tv.sh OUTDIR [BUILD]}
BUILD=${2:-}
HERE=$(cd "$(dirname "$0")/.." && pwd)
UE_ROOT=${UE_ROOT:-$HOME/.local/opt/UnrealEngine-5.8.4}
if [ -n "$BUILD" ]; then RUN=("$BUILD/Linux/Signal47.sh"); else RUN=("$UE_ROOT/Engine/Binaries/Linux/UnrealEditor" "$HERE/Signal47.uproject" -game); fi
mkdir -p "$OUT"
fail=0

busy() { pgrep -x UnrealEditor > /dev/null || pgrep -x Signal47 > /dev/null || pgrep -f 'Binaries/Linux/Signal47 ' > /dev/null; }

# once NAME SECS ARGS: 0 when the game ended by itself, 2 when it hung or ran over its time
once() {
  local name=$1 secs=$2 args=$3 log="$OUT/$1.log" pid start last size now
  while busy; do sleep 2; done
  rm -f "$OUT/$name.png"
  setsid "${RUN[@]}" -S47SmokeTest -S47Seconds="$secs" $args -S47Shot="$OUT/$name.png" -windowed -ResX=1600 -ResY=900 -log > "$log" 2>&1 &
  pid=$!; start=$SECONDS; last=$SECONDS; size=0
  while kill -0 "$pid" 2> /dev/null; do
    sleep 2
    now=$(stat -c %s "$log" 2> /dev/null || echo 0)
    if [ "$now" != "$size" ]; then size=$now; last=$SECONDS; fi
    if [ $((SECONDS - last)) -gt 90 ] || [ $((SECONDS - start)) -gt $((secs + 240)) ]; then
      kill -TERM -- "-$pid" 2> /dev/null; sleep 5; kill -KILL -- "-$pid" 2> /dev/null; wait "$pid" 2> /dev/null
      echo "$name: stopped after $((SECONDS - start)) s, the log still for $((SECONDS - last)) s" >> "$log"
      return 2
    fi
  done
  wait "$pid"
}

# shot NAME SECS ARGS PATTERN...: every pattern (grep -E) in the log (one with a leading ! must not
# be there), the smoke test's end, a shot
shot() {
  local name=$1 secs=$2 args=$3 r p ok=1; shift 3
  once "$name" "$secs" "$args"; r=$?
  if [ "$r" -eq 2 ]; then echo "$name hung; once more"; once "$name" "$secs" "$args"; r=$?; fi
  [ "$r" -eq 0 ] && [ -s "$OUT/$name.png" ] && grep -q 'smoke test OK' "$OUT/$name.log" || ok=0
  grep -Eq 'Critical error:|Assertion failed:|Signal 11 caught' "$OUT/$name.log" && ok=0
  for p in "$@"; do
    if [ "${p:0:1}" = '!' ]; then ! grep -Eq "${p:1}" "$OUT/$name.log" || { ok=0; echo "$name: '${p:1}'"; }
    else grep -Eq "$p" "$OUT/$name.log" || { ok=0; echo "$name: no '$p'"; }; fi
  done
  if [ "$ok" -eq 1 ]; then echo "$name OK"; else echo "$name FAILED"; fail=1; fi
  grep -hE 'S47: (Unarius TV|motel television (test |save|card, |carrier, |frame, )|ch4 drive: (room|tv)|self test|FAIL )' "$OUT/$name.log" | sed 's/^.*S47: /  /' | cut -c1-200
}

# (the TV's own checks; the whole self test's result is shown, a failure elsewhere is not this test's)
shot selftest 8 '-S47SelfTest -S47Face=room' 'self test (OK|FAILED) \([0-9]+ checks\)' '!FAIL (tvProgramme|motelTv|room6)'
# the office set close up, its broadcast twice: the pictures move, the sound plays with them
shot programme 8 '-S47Jump=chapter1 -S47MotelTv=programme-near' 'Unarius TV sound playing' 'motel television card, broadcast picture [0-9.]+ s \(frame [0-9]+\), sound [0-9.]+ s, level 0\.3'
shot programme-later 14 '-S47Jump=chapter1 -S47MotelTv=programme-near' 'motel television card, broadcast picture 1[0-9]\.[0-9]+ s \(frame 1[0-9][0-9]\), sound'
# room 6: the broadcast on the set, a metre in front of it, twice; then N. Vega's scene at her table
shot room6-tv 13 '-S47Jump=chapter4 -S47Ch4Drive=tvnear' 'ch4 drive: room: .*tv 1' 'ch4 drive: tv: picture 14[0-9]\.[0-9]+ s \(frame [0-9]+\), sound 14[0-9]' 'ch4 drive: tv: picture (149|15[0-9])\.[0-9]+ s \(frame 1[78][0-9][0-9]\), sound'
shot room6talk 10 '-S47Jump=chapter4 -S47Ch4Drive=table' 'ch4 drive: table: wrong'
# the event takes the picture: the carrier with the broadcast silent; then the lot from above,
# recorded and saved, and the broadcast back with its sound where its picture is
shot moteltv 12 '-S47Jump=chapter1 -S47MotelTv' 'motel television test carrier' 'motel television carrier, broadcast .*sound off'
shot moteltvframe 64 '-S47Jump=chapter1 -S47MotelTv=frame' 'motel television save roundtrip OK' 'motel television card, broadcast picture .*sound [0-9.]+ s'
exit "$fail"
