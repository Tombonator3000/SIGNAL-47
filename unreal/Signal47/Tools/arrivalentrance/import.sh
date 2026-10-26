#!/bin/bash
# Run only when the root agent grants the sequential heavy-job slot.
set -euo pipefail
UE_ROOT=${UE_ROOT:-$HOME/.local/opt/UnrealEngine-5.8.4}
PROJECT_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
WORK=${S47_ARRIVAL_WORK:-$HOME/scratch-bm/Signal47-arrival-office-20261026}
mkdir -p "$WORK"
export S47_ARRIVAL_WORK="$WORK"
for action in import_assets verify_assets; do
  python3 "$PROJECT_ROOT/Scripts/unreal_guard.py" require-idle
  "$UE_ROOT/Engine/Binaries/Linux/UnrealEditor-Cmd" "$PROJECT_ROOT/Signal47.uproject" \
    -run=pythonscript -script="$PROJECT_ROOT/Tools/arrivalentrance/$action.py" \
    -unattended -nullrhi -nosplash -NoSound \
    '-ini:Engine:[ConsoleVariables]:Interchange.FeatureFlags.Import.FBX=0' \
    > "$WORK/entrance-$action.log" 2>&1
done
