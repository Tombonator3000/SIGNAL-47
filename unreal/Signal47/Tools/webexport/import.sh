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
# the notebook's kit drawings and portraits (Content/WebData/art)
python3 "$D/Tools/webexport/notebook_art.py"
