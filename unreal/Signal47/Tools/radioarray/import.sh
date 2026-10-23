#!/bin/bash
# Original assets only. An active installed game/editor/build blocks this operation.
set -euo pipefail
UE_ROOT=${UE_ROOT:-$HOME/.local/opt/UnrealEngine-5.8.4}
PROJECT_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
WORK=${S47_RADIOARRAY_WORK:-$HOME/scratch-bm/Signal47-radioarray}
EXPORT=${S47_WEB:-$HOME/scratch-bm/Signal47-web}
python3 "$PROJECT_ROOT/Scripts/unreal_guard.py" require-idle
mkdir -p "$WORK"
REPORT="$(mktemp "$WORK/unreal-import.XXXXXXXX.json")"
python3 "$PROJECT_ROOT/Tools/radioarray/trim_station.py" "$EXPORT" "$WORK/station-trim" > "$WORK/trim-report.json"
S47_RADIOARRAY_TRIM="$WORK/station-trim" S47_RADIOARRAY_REPORT="$REPORT" \
 "$UE_ROOT/Engine/Binaries/Linux/UnrealEditor-Cmd" "$PROJECT_ROOT/Signal47.uproject" \
 -run=pythonscript -script="$PROJECT_ROOT/Tools/radioarray/import_assets.py" \
 -unattended -nullrhi -nosplash -NoSound \
 '-ini:Engine:[ConsoleVariables]:Interchange.FeatureFlags.Import.FBX=0' \
 > "$WORK/import.log" 2>&1
python3 - "$REPORT" <<'PY'
import json, sys
report = json.load(open(sys.argv[1]))
assert report['status'] == 'PASS', report
assert all(item['pass'] for item in report['checks']), report
print('RADIOARRAY IMPORT PASS', report['axis_probe_center_cm'], len(report['meshes']), 'meshes')
print('Report:', sys.argv[1])
PY
