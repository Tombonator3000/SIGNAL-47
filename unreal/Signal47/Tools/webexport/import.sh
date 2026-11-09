#!/bin/bash
# Rebuilds the web game's scene in Unreal: builds the web game, exports its scene
# (export.py, in the browser) and imports it (import_web.py). Needs the editor target built,
# the signal47 venv with Playwright, and the private SIGNAL-47-assets clone for the bought models.
# Usage: Tools/webexport/import.sh [OUT]   (default ~/scratch-bm/Signal47-web)
set -euo pipefail
UE_ROOT=${UE_ROOT:-$HOME/.local/opt/UnrealEngine-5.8.4}
D="$(cd "$(dirname "$0")/../.." && pwd)"
OUT=${1:-$HOME/scratch-bm/Signal47-web}
WEB="$(cd "$D/../../web" && pwd)"
(cd "$WEB" && npm run build:single >/dev/null)
(cd "$WEB" && S47_GPU=1 "$HOME/.venvs/signal47/bin/python" "$D/Tools/webexport/export.py" "$OUT")
S47_WEB="$OUT" "$UE_ROOT/Engine/Binaries/Linux/UnrealEditor-Cmd" "$D/Signal47.uproject" -run=pythonscript -script="$D/Tools/webexport/import_web.py" -unattended -nullrhi -nosplash 2>&1 | grep "S47WEB\|Traceback\|Error:" | sort -u
# A new import writes /Game/Web/Geo afresh without CPU copies. The runtime clones a few generated batches without old geometry
# (wires, the ranch, the farm, the office, the arrival, the street lamps' old poles) and needs their CPU data; without it each keeps
# the old geometry and warns. Each script changes only allow_cpu_access. Tools/radiowoman/prepare_cpu.py needs its private sources
# and is run by hand (its README).
for PREP in highwaypower highwayranch farmworld office saroarrival streetlamps sarofixes; do
  "$UE_ROOT/Engine/Binaries/Linux/UnrealEditor-Cmd" "$D/Signal47.uproject" -run=pythonscript -script="$D/Tools/$PREP/prepare_cpu.py" -unattended -nullrhi -nosplash 2>&1 | grep "_CPU_PASS\|Traceback\|Error:" | sort -u
done
# the notebook's kit drawings and portraits (Content/WebData/art)
python3 "$D/Tools/webexport/notebook_art.py"
