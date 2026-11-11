#!/bin/bash
# D46-10: the trip camera's model (made in Blender by build_tripcam.py, kept outside Git) into
# /Game/Licensed/Models/tripcam (ignored by Git). Needs the editor target built and no editor of this project running.
# Usage: Tools/tripcam/import_assets.sh [source folder]
#   default: ~/Documents/ChatGPT/Signal 47/modeller/tripcam-20261110 (tripcam.glb, tripcam.json)
set -euo pipefail
UE_ROOT=${UE_ROOT:-$HOME/.local/opt/UnrealEngine-5.8.4}
D="$(cd "$(dirname "$0")/../.." && pwd)"
SRC=${1:-$HOME/Documents/ChatGPT/Signal 47/modeller/tripcam-20261110}
[ -f "$SRC/tripcam.glb" ] || { echo "tripcam: no $SRC/tripcam.glb (run build_tripcam.py in Blender first)"; exit 1; }
rm -f "$D/Saved/tripcam-import.json"
S47_TRIPCAM_SRC="$SRC" nice -n 10 "$UE_ROOT/Engine/Binaries/Linux/UnrealEditor-Cmd" "$D/Signal47.uproject" -run=pythonscript -script="$D/Tools/tripcam/import_assets.py" -unattended -nullrhi -nosplash 2>&1 | tee "$SRC/import.log" | grep -o "S47TRIPCAM.*\|Traceback.*\|Error:.*" | awk '!seen[$0]++'
grep -q '"pass": true' "$D/Saved/tripcam-import.json" && echo "tripcam: imported, see Saved/tripcam-import.json" || { echo "tripcam: import FAILED, see $SRC/import.log"; exit 1; }
