#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
exec 9>"${XDG_RUNTIME_DIR:-/run/user/$(id -u)}/signal47-gauntlet.lock"
flock -n 9 || { echo 'Another SIGNAL 47 build/check is running.' >&2; exit 1; }
mkdir -p Artifacts/Workstation18
"${UNITY_EDITOR:-/home/tombonator3000t/Unity/Hub/Editor/6000.3.22f1/Editor/Unity}" -batchmode -nographics -quit -projectPath "$PWD/Unity" -executeMethod Signal47.Editor.Workstation18.Build -logFile "$PWD/Artifacts/Workstation18/build.log"
python3 Automation/stamp-gauntlet-build.py
