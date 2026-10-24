#!/bin/bash
set -euo pipefail
UE_ROOT=${UE_ROOT:-$HOME/.local/opt/UnrealEngine-5.8.4}
PROJECT_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
WORK=${S47_SUBSTATION_WORK:-$HOME/scratch-bm/Signal47-substation}
python3 "$PROJECT_ROOT/Scripts/unreal_guard.py" require-idle
mkdir -p "$WORK"
REPORT="$(mktemp "$WORK/import.XXXXXXXX.json")"
S47_SUBSTATION_REPORT="$REPORT" "$UE_ROOT/Engine/Binaries/Linux/UnrealEditor-Cmd" "$PROJECT_ROOT/Signal47.uproject" \
 -run=pythonscript -script="$PROJECT_ROOT/Tools/substation/import_assets.py" \
 -unattended -nullrhi -nosplash -NoSound '-ini:Engine:[ConsoleVariables]:Interchange.FeatureFlags.Import.FBX=0'
python3 - "$REPORT" <<'PY'
import json,sys
r=json.load(open(sys.argv[1]));assert r['status']=='PASS'and all(c['pass']for c in r['checks']),r
print('SUBSTATION IMPORT PASS',len(r['meshes']),'meshes',sys.argv[1])
PY
