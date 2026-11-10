#!/bin/bash
# D48-20261110: the balloon find's model (made in Blender by build_balloon.py, kept outside Git) into
# /Game/Licensed/Models/balloon (ignored by Git). Needs the editor target built and no editor of this project running.
# Usage: Tools/balloon/import_assets.sh [source folder]
#   default: ~/Documents/ChatGPT/Signal 47/modeller/balloon-20261110 (envelope.glb, parachute.glb, cord.glb, radiosonde.glb, balloon.json)
set -euo pipefail
UE_ROOT=${UE_ROOT:-$HOME/.local/opt/UnrealEngine-5.8.4}
D="$(cd "$(dirname "$0")/../.." && pwd)"
SRC=${1:-$HOME/Documents/ChatGPT/Signal 47/modeller/balloon-20261110}
for P in envelope parachute cord radiosonde; do [ -f "$SRC/$P.glb" ] || { echo "balloon: no $SRC/$P.glb (run build_balloon.py in Blender first)"; exit 1; }; done
rm -f "$D/Saved/balloon-import.json"
S47_BALLOON_SRC="$SRC" nice -n 10 "$UE_ROOT/Engine/Binaries/Linux/UnrealEditor-Cmd" "$D/Signal47.uproject" -run=pythonscript -script="$D/Tools/balloon/import_assets.py" -unattended -nullrhi -nosplash 2>&1 | tee "$SRC/import.log" | grep -o "S47BALLOON.*\|Traceback.*\|Error:.*" | awk '!seen[$0]++'
grep -q '"pass": true' "$D/Saved/balloon-import.json" && echo "balloon: imported, see Saved/balloon-import.json" || { echo "balloon: import FAILED, see $SRC/import.log"; exit 1; }
