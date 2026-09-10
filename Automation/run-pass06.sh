#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
phase="${REVIEW_PHASE:-candidate}"
[[ "$phase" == baseline || "$phase" == candidate ]] || exit 2
mkdir -p Artifacts/Pass06
exec 9>"/run/user/$(id -u)/signal47-gauntlet.lock"
flock -n 9 || { echo 'SIGNAL47_BUSY: another verified test owns this desktop'; exit 3; }
# Only a runner checkout is used. Never open or overwrite the user's canonical Unity project.
if pgrep -x Signal47.x86_64 >/dev/null; then echo 'SIGNAL47_BUSY: another player is running; no input injected'; exit 3; fi
runtime="/run/user/$(id -u)"
export XDG_RUNTIME_DIR="$runtime"
[[ ! -S "$runtime/pulse/native" ]] || export PULSE_SERVER="unix:$runtime/pulse/native"
live=0
for sock in /tmp/.X11-unix/X*; do
  [[ -S "$sock" ]] || continue
  d=":${sock##*X}"
  for auth in "$HOME/.Xauthority" "$runtime"/*xauth*; do
    [[ -f "$auth" ]] || continue
    if DISPLAY="$d" XAUTHORITY="$auth" xdpyinfo >/dev/null 2>&1; then export DISPLAY="$d" XAUTHORITY="$auth"; live=1; break 2; fi
  done
done
[[ "$live" == 1 ]] || { echo 'PASS06_UNVERIFIED: no authenticated graphical desktop'; exit 4; }
{
  date -u; uname -a; uptime
  echo 'CPU scaling governor:'; cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor 2>/dev/null || true
  echo 'Power profile:'; command -v powerprofilesctl >/dev/null && powerprofilesctl get || true
  echo 'Power supply:'; grep -H . /sys/class/power_supply/*/online /sys/class/power_supply/*/status 2>/dev/null || true
  echo 'Temperatures before:'; grep -H . /sys/class/thermal/thermal_zone*/temp 2>/dev/null || true
  echo 'Display modes:'; xrandr --current 2>/dev/null || true
  echo 'Unity editor process count:'; pgrep -xc Unity || true
} > Artifacts/Pass06/conditions-before.txt
bash Automation/build-linux.sh
grep -q SIGNAL47_SMOKE_PASS Artifacts/smoke.log
! grep -Eq '(SMOKE_FAIL|SMOKE_TIMEOUT)' Artifacts/smoke.log
[[ "$(grep -c WORLD_PASS_SMOKE_PASS Artifacts/smoke.log)" -ge 3 ]]
[[ "$(grep -c CONTROL_PASS_SMOKE_PASS Artifacts/smoke.log)" -ge 4 ]]
cp Artifacts/smoke.log Artifacts/Pass06/smoke.log
bash Automation/build-gauntlet-linux.sh
cp Artifacts/GauntletLinux/build-manifest.json Artifacts/Pass06/
player="$PWD/Artifacts/GauntletLinux/Signal47.x86_64"
player_pid=''
cleanup(){ if [[ -n "$player_pid" ]]; then kill "$player_pid" 2>/dev/null || true; wait "$player_pid" 2>/dev/null || true; player_pid=''; fi; }
trap cleanup EXIT
launch(){
  # Fresh observer output prevents a previous run from satisfying any check.
  rm -f Artifacts/Gauntlet/state.json Artifacts/Gauntlet/command.txt Artifacts/Gauntlet/performance.json Artifacts/Gauntlet/journey-result.json
  timeout --signal=TERM --kill-after=5s 300s "$player" --signal47-gauntlet "$@" -screen-width 1280 -screen-height 800 -screen-fullscreen 0 -logFile "$PWD/Artifacts/Pass06/$log" &
  player_pid=$!
  for _ in $(seq 1 120); do [[ -s Artifacts/Gauntlet/state.json ]] && return; kill -0 "$player_pid" 2>/dev/null || return 1; sleep .25; done
  echo 'No fresh observer state'; return 1
}
mkdir -p Artifacts/Gauntlet
log=measure-player.log; launch
python3 Automation/gauntlet-user-journey.py --measure
cleanup
mkdir -p Artifacts/Pass06/Performance
cp Artifacts/Gauntlet/{frames.csv,frame-work.csv,performance.json,journey-result.json} Artifacts/Pass06/Performance/
# Screenshots and sampled sound audit run separately; their overhead is not removed from a benchmark.
if [[ "$phase" == candidate ]]; then
  log=journey-player.log; launch --signal47-audio-audit
  python3 Automation/gauntlet-user-journey.py
  cleanup
  mkdir -p Artifacts/Pass06/Journey
  cp Artifacts/Gauntlet/journey-*.png Artifacts/Gauntlet/journey-result.json Artifacts/Pass06/Journey/
  [[ ! -f Artifacts/Gauntlet/audio-audit.json ]] || cp Artifacts/Gauntlet/audio-audit.json Artifacts/Pass06/
fi
rm -f Artifacts/WorldCapture/*.png Artifacts/WorldCapture/capture-manifest.json
timeout --signal=TERM --kill-after=5s 90s "$player" --signal47-world-capture -screen-width 1280 -screen-height 800 -screen-fullscreen 0 -logFile "$PWD/Artifacts/Pass06/capture-player.log"
grep -q 'WORLD_CAPTURE_PASS 6' Artifacts/Pass06/capture-player.log
mkdir -p Artifacts/Pass06/Snapshots
cp Artifacts/WorldCapture/*.png Artifacts/WorldCapture/capture-manifest.json Artifacts/Pass06/Snapshots/
{ date -u; uptime; grep -H . /sys/class/thermal/thermal_zone*/temp 2>/dev/null || true; } > Artifacts/Pass06/conditions-after.txt
python3 Automation/pass06-evidence.py Artifacts/Pass06
