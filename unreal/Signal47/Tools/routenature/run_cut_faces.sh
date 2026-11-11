#!/bin/bash
# PT-ROCKS-20261109: cut_faces.py (read-only) in the editor, one heavy Unreal job at a time:
#   S47_PROJECT_ROOT=<project with the private kit> python3 ~/.claude/skills/signal47-nattskift/scripts/q.py bash Tools/routenature/run_cut_faces.sh
# The report and the OBJ go to S47_RN_WORK (default ~/scratch-bm/Signal47-rocks-20261109), never into the project.
set -euo pipefail
UE_ROOT=${UE_ROOT:-$HOME/.local/opt/UnrealEngine-5.8.4}
HERE="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT=${S47_PROJECT_ROOT:-"$(cd "$HERE/../.." && pwd)"}
export S47_RN_WORK=${S47_RN_WORK:-$HOME/scratch-bm/Signal47-rocks-20261109}
mkdir -p "$S47_RN_WORK"
LOG=$S47_RN_WORK/cut_faces.log; : > "$LOG"
python3 "$PROJECT_ROOT/Scripts/unreal_guard.py" require-idle
"$UE_ROOT/Engine/Binaries/Linux/UnrealEditor-Cmd" "$PROJECT_ROOT/Signal47.uproject" -run=pythonscript \
  -script="$HERE/cut_faces.py" -unattended -nullrhi -nosplash -NoSound >> "$LOG" 2>&1 || true
grep -E 'S47 ROCK CUT|Traceback|Error:' "$LOG" | tail -30
if grep -q 'S47 ROCK CUTS ->' "$LOG" && ! grep -q 'Traceback' "$LOG"; then echo "ROCK CUTS PASS ($S47_RN_WORK/cut_faces.json)"; else echo "ROCK CUTS FAIL ($LOG)" >&2; exit 1; fi
