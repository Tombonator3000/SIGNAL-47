#!/bin/bash
# Imports the Blender models from the Unity archive and makes the materials (Scripts/import-models.py).
# Needs the editor target built first (build-editor.sh).
set -euo pipefail
UE_ROOT=${UE_ROOT:-$HOME/.local/opt/UnrealEngine-5.8.4}
D="$(cd "$(dirname "$0")/.." && pwd)"
"$UE_ROOT/Engine/Binaries/Linux/UnrealEditor-Cmd" "$D/Signal47.uproject" -run=pythonscript -script="$D/Scripts/import-models.py" -unattended -nullrhi -nosplash 2>&1 | grep -E "S47:|Error:|Traceback" | grep -v "LogPython: Display: Python"
