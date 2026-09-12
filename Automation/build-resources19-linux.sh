#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
signal47_editor="${UNITY_EDITOR:-/home/tombonator3000t/Unity/Hub/Editor/6000.3.22f1/Editor/Unity}"
exec 9>"${XDG_RUNTIME_DIR:-/run/user/$(id -u)}/signal47-gauntlet.lock"
flock -n 9 || { echo 'Another SIGNAL 47 check owns the run lock.' >&2; exit 1; }
mkdir -p Artifacts/Resources19
"$signal47_editor" -batchmode -nographics -quit -projectPath "$PWD/Unity" -executeMethod Signal47.Editor.Resources19.Build -logFile "$PWD/Artifacts/Resources19/build.log"
python3 Automation/stamp-gauntlet-build.py
