#!/bin/bash
# Own Caliche assets only. Preserve all original/generated content and running jobs.
set -euo pipefail
UE_ROOT=${UE_ROOT:-$HOME/.local/opt/UnrealEngine-5.8.4}
PROJECT_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
WORK=${S47_STREET_WORK:-$HOME/scratch-bm/Signal47-caliche-street}
EXPORT=${S47_WEB:-$HOME/scratch-bm/Signal47-web}
python3 "$PROJECT_ROOT/Scripts/unreal_guard.py" require-idle
mkdir -p "$WORK"
REPORT=$(mktemp "$WORK/import.XXXXXXXX.json")
python3 "$PROJECT_ROOT/Tools/calichestreet/trim_street.py" "$EXPORT" "$WORK/trim" > "$WORK/trim-report.json"
S47_STREET_TRIM="$WORK/trim" S47_STREET_REPORT="$REPORT" \
 "$UE_ROOT/Engine/Binaries/Linux/UnrealEditor-Cmd" "$PROJECT_ROOT/Signal47.uproject" \
 -run=pythonscript -script="$PROJECT_ROOT/Tools/calichestreet/import_assets.py" \
 -unattended -nullrhi -nosplash -NoSound \
 '-ini:Engine:[ConsoleVariables]:Interchange.FeatureFlags.Import.FBX=0' \
 > "$WORK/import-unreal.log" 2>&1
python3 - "$REPORT" <<'PY'
import json,sys
r=json.load(open(sys.argv[1]))
assert r['status']=='PASS' and all(x['pass'] for x in r['checks']),r
print('CALICHE STREET IMPORT PASS',len(r['checks']),'checks;',len(r['meshes']),'meshes;',sys.argv[1])
PY
