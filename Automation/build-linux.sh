#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
UNITY_EDITOR="${UNITY_EDITOR:-/home/tombonator3000t/Unity/Hub/Editor/6000.3.22f1/Editor/Unity}"
mkdir -p Artifacts
"$UNITY_EDITOR" -batchmode -nographics -quit -projectPath "$PWD/Unity" -executeMethod Signal47.Editor.Automation.BuildLinux -logFile "$PWD/Artifacts/build.log"
# systemd runners do not inherit the desktop audio socket environment.
# FMOD initializes before the managed test watchdog, even with -noaudio.
task_runtime_dir="${XDG_RUNTIME_DIR:-/run/user/$(id -u)}"
if [[ -S "$task_runtime_dir/pulse/native" ]]; then
  export XDG_RUNTIME_DIR="$task_runtime_dir"
  export PULSE_SERVER="unix:$task_runtime_dir/pulse/native"
fi
SDL_VIDEODRIVER=dummy timeout --signal=TERM --kill-after=10s 240s ./Artifacts/Linux/Signal47.x86_64 -batchmode -nographics -noaudio --signal47-smoke --signal47-save-dir "$PWD/Artifacts/SmokeProfile" -logFile "$PWD/Artifacts/smoke.log"
tar -czf Artifacts/SIGNAL-47-Linux.tar.gz -C Artifacts/Linux .
