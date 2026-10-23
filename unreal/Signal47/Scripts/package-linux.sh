#!/bin/bash
# Builds, cooks and packages a Linux Development build into OUT (default ~/scratch-bm/Signal47-build).
set -euo pipefail
UE_ROOT=${UE_ROOT:-$HOME/.local/opt/UnrealEngine-5.8.4}
OUT=${1:-$HOME/scratch-bm/Signal47-build}
P="$(cd "$(dirname "$0")/.." && pwd)/Signal47.uproject"
python3 "$(dirname "$P")/Scripts/unreal_guard.py" require-idle
mkdir -p "$(dirname "$P")/Saved/Logs"
BUILD_LOG=$(mktemp "$(dirname "$P")/Saved/Logs/package-linux.XXXXXXXX.log")
# UE 5.8's Zen oplog staging can stall after a recook. Cook loose packages for the
# normal IoStore container writer instead; this does not change the shipped renderer.
"$UE_ROOT/Engine/Build/BatchFiles/RunUAT.sh" BuildCookRun -project="$P" -noP4 -platform=Linux -clientconfig=Development \
  -build -cook -stage -pak -iostore -archive -archivedirectory="$OUT" -utf8output -nocompileeditor \
  '-UbtArgs=-MaxParallelActions=4 -NoUBA' '-AdditionalCookerOptions=-SkipZenStore' 2>&1 | tee "$BUILD_LOG"
# UAT can return success while replacing broken cooked materials with the default.
# Such a package is unusable for either visual comparison or PSO recording.
if rg -n 'Failed to compile Material|Default Material will be used in game' "$BUILD_LOG"; then
  echo "Cooked material compilation failed; package rejected. Log: $BUILD_LOG" >&2
  exit 1
fi
# The 3D models' credits with their links (CC BY 4.0 asks for them), beside the game (Tools/freemodels/credits.py).
cp "$(dirname "$P")/CREDITS.txt" "$OUT/Linux/CREDITS.txt"
