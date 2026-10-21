#!/bin/bash
# Builds, cooks and packages a Linux Development build into OUT (default ~/scratch-bm/Signal47-build).
set -euo pipefail
UE_ROOT=${UE_ROOT:-$HOME/.local/opt/UnrealEngine-5.8.4}
OUT=${1:-$HOME/scratch-bm/Signal47-build}
P="$(cd "$(dirname "$0")/.." && pwd)/Signal47.uproject"
"$UE_ROOT/Engine/Build/BatchFiles/RunUAT.sh" BuildCookRun -project="$P" -noP4 -platform=Linux -clientconfig=Development \
  -build -cook -stage -pak -iostore -archive -archivedirectory="$OUT" -utf8output -nocompileeditor
