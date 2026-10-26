#!/bin/bash
# Compiles the editor target. UE_ROOT defaults to the local install. Do not run while the editor is open.
set -euo pipefail
UE_ROOT=${UE_ROOT:-$HOME/.local/opt/UnrealEngine-5.8.4}
P="$(cd "$(dirname "$0")/.." && pwd)/Signal47.uproject"
python3 "$(dirname "$P")/Scripts/unreal_guard.py" require-idle
"$UE_ROOT/Engine/Build/BatchFiles/Linux/Build.sh" Signal47Editor Linux Development -project="$P" -waitmutex -MaxParallelActions=4 "$@"
