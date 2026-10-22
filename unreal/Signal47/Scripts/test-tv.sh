#!/bin/bash
# Focused packaged regression for the optional TV programme and the existing 47-second event.
set -uo pipefail
OUT=${1:?usage: test-tv.sh OUTDIR BUILD}
BUILD=${2:?usage: test-tv.sh OUTDIR BUILD}
mkdir -p "$OUT"
fail=0
shot() {
  local name=$1 expected=$2 args=$3 secs=${4:-10} result=0
  timeout $((secs + 142)) "$BUILD/Linux/Signal47.sh" -S47SmokeTest -S47Seconds="$secs" $args -S47Shot="$OUT/$name.png" -windowed -ResX=1600 -ResY=900 -log > "$OUT/$name.log" 2>&1 || result=$?
  if [ "$result" -eq 0 ] && ! rg -q 'Critical error:|Assertion failed:|Signal 11 caught' "$OUT/$name.log" && rg -q 'smoke test OK' "$OUT/$name.log" && rg -q "$expected" "$OUT/$name.log" && [ -s "$OUT/$name.png" ]; then
    echo "$name OK"
  else echo "$name FAILED"; fail=1; fi
}
shot selftest 'self test OK' '-S47SelfTest -S47Face=room'
shot programme 'Unarius TV decoded frame' '-S47Jump=chapter1 -S47MotelTv=programme'
shot programme-later 'motel television test card' '-S47Jump=chapter1 -S47MotelTv=programme' 22
shot room6-tv 'Unarius TV decoded frame [1-9][0-9]+' '-S47Jump=chapter4 -S47Ch4Drive=tv'
shot room6talk 'ch4 drive: table: wrong' '-S47Jump=chapter4 -S47Ch4Drive=table'
shot moteltv 'motel television test carrier' '-S47Jump=chapter1 -S47MotelTv' 12
shot moteltvframe 'motel television save roundtrip OK' '-S47Jump=chapter1 -S47MotelTv=frame' 64
exit "$fail"
