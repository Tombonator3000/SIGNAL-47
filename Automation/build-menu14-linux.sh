#!/usr/bin/env bash
# Compile/package inputs only; retain the authored scene, sky and baked lighting.
set -euo pipefail
cd "$(dirname "$0")/.."
UNITY_EDITOR="${UNITY_EDITOR:-/home/tombonator3000t/Unity/Hub/Editor/6000.3.22f1/Editor/Unity}"
signal47_runtime_dir="${XDG_RUNTIME_DIR:-/run/user/$(id -u)}"
exec 9>"$signal47_runtime_dir/signal47-gauntlet.lock"
flock -n 9 || { echo 'Another SIGNAL 47 check owns the run lock.' >&2; exit 1; }
mkdir -p Artifacts/Menu14
"$UNITY_EDITOR" -batchmode -nographics -quit -projectPath "$PWD/Unity" -executeMethod Signal47.Editor.Automation.BuildCurrentGauntletLinux -logFile "$PWD/Artifacts/Menu14/build.log"
python3 Automation/stamp-gauntlet-build.py
