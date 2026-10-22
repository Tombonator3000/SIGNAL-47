#!/bin/bash
# Imports the free models (~/Assets/signal47-free) into /Game/Free (ignored by Git) and writes
# Content/Free/free_models.json. Needs the editor target built and no editor of this project running.
# Usage: Tools/freemodels/import_free.sh [WORK]   (default ~/scratch-bm/Signal47-free: unpacked zips)
#        S47_FREE_ONLY=pickup_c10_1966,yucca Tools/freemodels/import_free.sh   (only these)
set -euo pipefail
UE_ROOT=${UE_ROOT:-$HOME/.local/opt/UnrealEngine-5.8.4}
D="$(cd "$(dirname "$0")/../.." && pwd)"
WORK=${1:-$HOME/scratch-bm/Signal47-free}
mkdir -p "$WORK"
python3 -I "$D/Tools/freemodels/gltf_prep.py" "$WORK" --report
S47_FREE_WORK="$WORK" "$UE_ROOT/Engine/Binaries/Linux/UnrealEditor-Cmd" "$D/Signal47.uproject" -run=pythonscript -script="$D/Tools/freemodels/import_free.py" -unattended -nullrhi -nosplash 2>&1 | tee "$WORK/import.log" | grep -o "S47FREE.*\|Traceback.*\|Error:.*" | awk '!seen[$0]++'
# the credits (ending and CREDITS.txt) from the same CREDITS.txt files
python3 -I "$D/Tools/freemodels/credits.py"
