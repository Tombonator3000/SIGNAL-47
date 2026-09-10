#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
UNITY_EDITOR="${UNITY_EDITOR:-/home/tombonator3000t/Unity/Hub/Editor/6000.3.22f1/Editor/Unity}"
mkdir -p Artifacts
"$UNITY_EDITOR" -batchmode -nographics -quit -projectPath "$PWD/Unity" -executeMethod Signal47.Editor.Automation.BuildGauntletLinux -logFile "$PWD/Artifacts/gauntlet-build.log"
python3 Automation/stamp-gauntlet-build.py
tar -czf Artifacts/SIGNAL-47-Gauntlet-Linux.tar.gz -C Artifacts/GauntletLinux .
# Real-input verification needs a visible, focused player in the desktop session.
# See Docs/GAUNTLET_04.md before running Automation/gauntlet-user-journey.py.
