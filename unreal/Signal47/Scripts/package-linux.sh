#!/bin/bash
# Builds, cooks and packages a Linux Development build into OUT (default ~/scratch-bm/Signal47-build).
set -euo pipefail
UE_ROOT=${UE_ROOT:-$HOME/.local/opt/UnrealEngine-5.8.4}
OUT=${1:-$HOME/scratch-bm/Signal47-build}
P="$(cd "$(dirname "$0")/.." && pwd)/Signal47.uproject"
python3 "$(dirname "$P")/Scripts/unreal_guard.py" require-idle
"$UE_ROOT/Engine/Build/BatchFiles/RunUAT.sh" BuildCookRun -project="$P" -noP4 -platform=Linux -clientconfig=Development \
  -build -cook -stage -pak -iostore -archive -archivedirectory="$OUT" -utf8output -nocompileeditor
# The 3D models' credits with their links (CC BY 4.0 asks for them), beside the game (Tools/freemodels/credits.py).
cp "$(dirname "$P")/CREDITS.txt" "$OUT/Linux/CREDITS.txt"
