#!/bin/bash
# Import only the cow family, then reopen the saved assets for independent checks.
set -euo pipefail
UE_ROOT=${UE_ROOT:-$HOME/.local/opt/UnrealEngine-5.8.4}
PROJECT_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
WORK=${S47_COW_WORK:-$HOME/scratch-bm/Signal47-cows-20261025}
mkdir -p "$WORK"
export S47_COW_WORK="$WORK"
for action in import_assets verify_assets; do
  python3 "$PROJECT_ROOT/Scripts/unreal_guard.py" require-idle
  "$UE_ROOT/Engine/Binaries/Linux/UnrealEditor-Cmd" "$PROJECT_ROOT/Signal47.uproject" \
    -run=pythonscript -script="$PROJECT_ROOT/Tools/cows/$action.py" \
    -unattended -nullrhi -nosplash -NoSound \
    '-ini:Engine:[ConsoleVariables]:Interchange.FeatureFlags.Import.FBX=0' \
    > "$WORK/$action.log" 2>&1
  python3 - "$WORK" "$action" <<'PY'
import json, os, sys
from pathlib import Path
work, action = Path(sys.argv[1]), sys.argv[2]
path = Path(os.environ.get('S47_COW_REPORT', str(work/'native-import.json'))) if action == 'import_assets' else Path(os.environ.get('S47_COW_VERIFY_REPORT', str(work/'native-verify.json')))
report = json.loads(path.read_text())
assert report['status'] == 'PASS' and all(row['pass'] for row in report['checks']), report
print(action, 'PASS', len(report['checks']), 'checks', len(report['meshes']), 'meshes', path)
PY
done
