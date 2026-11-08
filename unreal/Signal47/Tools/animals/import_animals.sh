#!/bin/bash
# Imports the CC0 animals (Tools/animals/src, KILDER.md) into /Game/Free/Animals (ignored by Git, like the other free models).
# Needs the editor target built and no editor of this project running; run it through the project's queue, one heavy job at a time:
#   python3 ~/.claude/skills/signal47-nattskift/scripts/q.py Tools/animals/import_animals.sh [PROJECT_DIR]
# PROJECT_DIR: the Unreal project to import into (default: the one this script is in).
set -euo pipefail
UE_ROOT=${UE_ROOT:-$HOME/.local/opt/UnrealEngine-5.8.4}
HERE="$(cd "$(dirname "$0")" && pwd)"
D="$(cd "${1:-$HERE/../..}" && pwd)"
LOG=${S47_ANIMALS_LOG:-$HOME/scratch-bm/Signal47-animals-import.log}
mkdir -p "$(dirname "$LOG")"
"$UE_ROOT/Engine/Binaries/Linux/UnrealEditor-Cmd" "$D/Signal47.uproject" -run=pythonscript -script="$HERE/import_animals.py" -unattended -nullrhi -nosplash 2>&1 | tee "$LOG" | grep -o "S47ANIMALS.*\|Traceback.*\|Error:.*" | awk '!seen[$0]++'
grep -q "S47ANIMALS DONE PASS" "$LOG"
