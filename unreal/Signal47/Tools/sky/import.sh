#!/bin/bash
# Own sky assets only (SKY.md): the cloud material and the Caliche sky/stars/horizon materials.
# One heavy Unreal job at a time; preserves generated content and other checkouts.
set -euo pipefail
UE_ROOT=${UE_ROOT:-$HOME/.local/opt/UnrealEngine-5.8.4}
PROJECT_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
LOG=${S47_SKY_LOG:-$PROJECT_ROOT/Saved/Logs/sky-import.log}
mkdir -p "$(dirname "$LOG")"; : > "$LOG"
python3 "$PROJECT_ROOT/Scripts/unreal_guard.py" require-idle
for SCRIPT in Tools/sky/import_clouds.py Tools/calichestreet/import_atmosphere.py; do
  "$UE_ROOT/Engine/Binaries/Linux/UnrealEditor-Cmd" "$PROJECT_ROOT/Signal47.uproject" -run=pythonscript \
    -script="$PROJECT_ROOT/$SCRIPT" -unattended -nullrhi -nosplash -NoSound >> "$LOG" 2>&1 || true
done
# unreal.log lines stay below the commandlet's Display level; the cloud script warns its marker.
if grep -q 'S47 CLOUDS SAVED' "$LOG" && [ "$(grep -c 'Python script executed successfully' "$LOG")" -ge 4 ] && ! grep -q 'Traceback' "$LOG"; then
  echo "SKY IMPORT PASS ($LOG)"
else
  echo "SKY IMPORT FAIL ($LOG)" >&2; exit 1
fi
