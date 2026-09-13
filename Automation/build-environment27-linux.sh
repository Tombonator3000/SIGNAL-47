#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
signal47_editor="${UNITY_EDITOR:-/home/tombonator3000t/Unity/Hub/Editor/6000.3.22f1/Editor/Unity}"
exec 9>"${XDG_RUNTIME_DIR:-/run/user/$(id -u)}/signal47-gauntlet.lock"
flock -n 9 || { echo 'Another SIGNAL47 check is running.' >&2; exit 1; }
if pgrep -x Signal47.x86_64 >/dev/null; then echo 'Close the running game before building/testing.' >&2; exit 1; fi
mkdir -p Artifacts/Environment27
bash Automation/Splat21/bounded.sh 4294967296 600 "$signal47_editor" -batchmode -nographics -quit -job-worker-count 2 -projectPath "$PWD/Unity" -executeMethod Signal47.Editor.Station26Build.Build -logFile "$PWD/Artifacts/Environment27/build.log"
python3 Automation/stamp-gauntlet-build.py
