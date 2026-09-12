#!/usr/bin/env bash
# API-driven study only; does not send desktop input or touch chapter saves.
set -euo pipefail
cd "$(dirname "$0")/.."
signal47_archive_player="${1:?Provide the exact Archive16 player or extracted launcher}"
test -x "$signal47_archive_player"
export XDG_RUNTIME_DIR="${XDG_RUNTIME_DIR:-/run/user/$(id -u)}"
export DISPLAY="${DISPLAY:-:0}"
if [[ -z "${XAUTHORITY:-}" ]]; then
  for signal47_auth in "$XDG_RUNTIME_DIR"/xauth_*; do
    if [[ -r "$signal47_auth" ]]; then export XAUTHORITY="$signal47_auth"; break; fi
  done
fi
exec 9>"$XDG_RUNTIME_DIR/signal47-gauntlet.lock"
flock -n 9 || { echo 'Another SIGNAL 47 run owns the lock.' >&2; exit 1; }
mkdir -p Artifacts/Archive16
signal47_archive_profile=$(mktemp -d "$PWD/Artifacts/Archive16/archive16-test-XXXXXX")
touch "$signal47_archive_profile/ALLOW_ARCHIVE16_TEST"
printf '%s\n' "$signal47_archive_profile" > Artifacts/Archive16/latest-profile.txt
timeout --signal=TERM --kill-after=10s 100s "$signal47_archive_player" --signal47-archive16-checks --archive16-evidence "$signal47_archive_profile" -force-glcore -screen-width 1280 -screen-height 800 -screen-fullscreen 0 -logFile "$signal47_archive_profile/player.log"
python3 - "$signal47_archive_profile" <<'PY'
import json,sys
from pathlib import Path
r=json.loads((Path(sys.argv[1])/'result.json').read_text());assert r['result']=='PASS',r
print(json.dumps({'result':r['result'],'checks':len(r['checks']),'profile':sys.argv[1],'method':r['method']},indent=2))
PY
