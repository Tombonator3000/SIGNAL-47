#!/bin/bash
# Builds /Game/Signal47/FX (spark material and NS_S47_ArcSparks) from fx_contract.json, then verifies it.
# One heavy Unreal job at a time: the guard fails closed when another job runs.
set -euo pipefail
UE_ROOT=${UE_ROOT:-$HOME/.local/opt/UnrealEngine-5.8.4}
PROJECT_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
WORK=${S47_FX_WORK:-$HOME/scratch-bm/Signal47-fx}
SCRIPT=${1:-import_fx.py}
python3 "$PROJECT_ROOT/Scripts/unreal_guard.py" require-idle
mkdir -p "$WORK"
REPORT="$(mktemp "$WORK/${SCRIPT%.py}.XXXXXXXX.json")"
S47_FX_REPORT="$REPORT" "$UE_ROOT/Engine/Binaries/Linux/UnrealEditor-Cmd" "$PROJECT_ROOT/Signal47.uproject" \
 -run=pythonscript -script="$PROJECT_ROOT/Tools/fx/$SCRIPT" -unattended -nullrhi -nosplash -NoSound
python3 - "$REPORT" <<'PY'
import json,sys
r=json.load(open(sys.argv[1]))
bad=[c for c in r['checks'] if not c['pass']]
assert r.get('status')=='PASS' and not bad,(r.get('error'),bad)
print('S47 FX',r.get('status'),len(r['checks']),'checks',sys.argv[1])
PY
