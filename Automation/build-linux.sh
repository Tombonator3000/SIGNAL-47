#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
UNITY_EDITOR="${UNITY_EDITOR:-/home/tombonator3000t/Unity/Hub/Editor/6000.3.22f1/Editor/Unity}"
mkdir -p Artifacts
"$UNITY_EDITOR" -batchmode -nographics -quit -projectPath "$PWD/Unity" -executeMethod Signal47.Editor.Automation.BuildLinux -logFile "$PWD/Artifacts/build.log"
SDL_VIDEODRIVER=dummy ./Artifacts/Linux/Signal47.x86_64 -batchmode -nographics -noaudio --signal47-smoke -logFile "$PWD/Artifacts/smoke.log"
tar -czf Artifacts/SIGNAL-47-Linux.tar.gz -C Artifacts/Linux .
