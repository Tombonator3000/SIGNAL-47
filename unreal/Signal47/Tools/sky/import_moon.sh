#!/bin/bash
# PT-MOON-20261109: the moon's texture and material only (import_moon.py). One heavy Unreal job at a time:
#   python3 ~/.claude/skills/signal47-nattskift/scripts/q.py bash Tools/sky/import_moon.sh
# Touches nothing else; the bake (moon/bake_moon.py) is plain Python and already done, its PNG is in moon/.
set -euo pipefail
UE_ROOT=${UE_ROOT:-$HOME/.local/opt/UnrealEngine-5.8.4}
HERE="$(cd "$(dirname "$0")" && pwd)"
# the project to import into (S47_PROJECT_ROOT, e.g. the main checkout); the script and its PNG are read from here
PROJECT_ROOT=${S47_PROJECT_ROOT:-"$(cd "$HERE/../.." && pwd)"}
LOG=${S47_MOON_LOG:-$PROJECT_ROOT/Saved/Logs/moon-import.log}
mkdir -p "$(dirname "$LOG")"; : > "$LOG"
python3 "$PROJECT_ROOT/Scripts/unreal_guard.py" require-idle
"$UE_ROOT/Engine/Binaries/Linux/UnrealEditor-Cmd" "$PROJECT_ROOT/Signal47.uproject" -run=pythonscript \
  -script="$HERE/import_moon.py" -unattended -nullrhi -nosplash -NoSound >> "$LOG" 2>&1 || true
if grep -q 'S47 MOON SAVED' "$LOG" && ! grep -q 'Traceback' "$LOG"; then
  echo "MOON IMPORT PASS ($LOG)"
else
  echo "MOON IMPORT FAIL ($LOG)" >&2; exit 1
fi
