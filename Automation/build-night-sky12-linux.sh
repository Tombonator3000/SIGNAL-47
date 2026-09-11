#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
UNITY_EDITOR="${UNITY_EDITOR:-/home/tombonator3000t/Unity/Hub/Editor/6000.3.22f1/Editor/Unity}"
signal47_runtime_dir="${XDG_RUNTIME_DIR:-/run/user/$(id -u)}"
export XDG_RUNTIME_DIR="$signal47_runtime_dir"
export DISPLAY="${DISPLAY:-:0}"
if [[ -z "${XAUTHORITY:-}" ]]; then
  for signal47_auth_file in "$signal47_runtime_dir"/xauth_*; do
    if [[ -r "$signal47_auth_file" ]]; then export XAUTHORITY="$signal47_auth_file"; break; fi
  done
fi
if [[ -z "${PULSE_SERVER:-}" && -S "$signal47_runtime_dir/pulse/native" ]]; then
  export PULSE_SERVER="unix:$signal47_runtime_dir/pulse/native"
fi
exec 9>"$signal47_runtime_dir/signal47-gauntlet.lock"
flock -n 9 || { echo 'Finish the active SIGNAL 47 check before building.' >&2; exit 1; }
mkdir -p Artifacts/Sky12
"$UNITY_EDITOR" -batchmode -force-glcore -quit -projectPath "$PWD/Unity" \
  -executeMethod Signal47.Editor.NightSky12.Build -logFile "$PWD/Artifacts/Sky12/build.log"
python3 Automation/stamp-gauntlet-build.py
