#!/bin/bash
# D45-29: the alien bobblehead on the truck's dashboard. Splits Tom's download into head and body
# (prepare.py, plain Python) and imports both into /Game/Licensed/Models/alienbubblehead (ignored by Git).
# Needs the editor target built and no editor of this project running.
# Usage: Tools/alienbubblehead/import_assets.sh [source.glb] [prepared folder]
#   defaults: ~/Desktop/alienbubblehead.glb, ~/Documents/ChatGPT/Signal 47/modeller/alienbubblehead-20261110/prepared
set -euo pipefail
UE_ROOT=${UE_ROOT:-$HOME/.local/opt/UnrealEngine-5.8.4}
D="$(cd "$(dirname "$0")/../.." && pwd)"
SRC=${1:-$HOME/Desktop/alienbubblehead.glb}
OUT=${2:-$HOME/Documents/ChatGPT/Signal 47/modeller/alienbubblehead-20261110/prepared}
python3 -I "$D/Tools/alienbubblehead/prepare.py" "$SRC" "$OUT"
rm -f "$D/Saved/alienbubblehead-import.json"
S47_BOBBLE_PREPARED="$OUT" nice -n 10 "$UE_ROOT/Engine/Binaries/Linux/UnrealEditor-Cmd" "$D/Signal47.uproject" -run=pythonscript -script="$D/Tools/alienbubblehead/import_assets.py" -unattended -nullrhi -nosplash 2>&1 | tee "$OUT/import.log" | grep -o "S47BOBBLE.*\|Traceback.*\|Error:.*" | awk '!seen[$0]++'
grep -q '"pass": true' "$D/Saved/alienbubblehead-import.json" && echo "alienbubblehead: imported, see Saved/alienbubblehead-import.json" || { echo "alienbubblehead: import FAILED, see $OUT/import.log"; exit 1; }
