#!/bin/bash
# Builds /Game/Signal47/Look/M_StationLook (make_look.py) headless and checks its report.
# One heavy Unreal job at a time: run it through the queue (q.py). The commandlet hangs after it is done,
# so it is held to a timeout; the report decides, not the exit code. S47_LOOK_RHI=1 runs it with the real RHI, so the
# material's shaders are compiled for the game's platform into the derived-data cache (the game then does not compile them).
set -uo pipefail
UE_ROOT=${UE_ROOT:-$HOME/.local/opt/UnrealEngine-5.8.4}
PROJECT_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
WORK=${S47_LOOK_WORK:-$HOME/scratch-bm/Signal47-look}
mkdir -p "$WORK"
REPORT="$WORK/make_look.$(date +%Y%m%d-%H%M%S).json"
LOG="${REPORT%.json}.log"
S47_LOOK_REPORT="$REPORT" timeout "${S47_LOOK_TIMEOUT:-540}" "$UE_ROOT/Engine/Binaries/Linux/UnrealEditor-Cmd" "$PROJECT_ROOT/Signal47.uproject" \
 -run=pythonscript -script="$PROJECT_ROOT/Tools/stationlook/make_look.py" -unattended ${S47_LOOK_RHI:+-nosplash} ${S47_LOOK_RHI:--nullrhi} -nosplash -NoSound > "$LOG" 2>&1 < /dev/null
python3 - "$REPORT" <<'PY'
import json, sys
r = json.load(open(sys.argv[1]))
bad = [c for c in r['checks'] if not c['pass']]
print('S47 LOOK', r.get('status'), len(r['checks']), 'checks', r.get('asset'), sys.argv[1])
for c in bad:
    print('  FAIL', c['name'], c.get('detail'))
if r.get('error'):
    print(r['error'])
sys.exit(0 if r.get('status') == 'PASS' and not bad else 1)
PY
