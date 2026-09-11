#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
UNITY_EDITOR="${UNITY_EDITOR:-/home/tombonator3000t/Unity/Hub/Editor/6000.3.22f1/Editor/Unity}"
# The CPU lightmapper still needs an actual graphics device for the Meta pass.
export DISPLAY="${DISPLAY:-:0}"
signal47_runtime_dir="${XDG_RUNTIME_DIR:-/run/user/$(id -u)}"
export XDG_RUNTIME_DIR="$signal47_runtime_dir"
if [[ -z "${XAUTHORITY:-}" ]]; then
  for signal47_auth_file in "$signal47_runtime_dir"/xauth_*; do
    if [[ -r "$signal47_auth_file" ]]; then export XAUTHORITY="$signal47_auth_file"; break; fi
  done
fi
if [[ -z "${PULSE_SERVER:-}" && -S "$signal47_runtime_dir/pulse/native" ]]; then
  export PULSE_SERVER="unix:$signal47_runtime_dir/pulse/native"
fi
exec 9>"$signal47_runtime_dir/signal47-gauntlet.lock"
flock -n 9 || { echo 'A native game check is active; finish it before building baked lighting.' >&2; exit 1; }
mkdir -p Artifacts
"$UNITY_EDITOR" -batchmode -force-glcore -quit -projectPath "$PWD/Unity" -executeMethod Signal47.Editor.Visual10LightBake.Build -logFile "$PWD/Artifacts/visual10-bake-build.log"
python3 Automation/stamp-gauntlet-build.py
tar -czf Artifacts/SIGNAL-47-Gauntlet-Linux.tar.gz -C Artifacts/GauntletLinux .
