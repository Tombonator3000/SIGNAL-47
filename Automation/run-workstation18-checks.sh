#!/usr/bin/env bash
# Actual player, isolated profile and API calls; no native desktop input.
set -euo pipefail
cd "$(dirname "$0")/.."
export XDG_RUNTIME_DIR="${XDG_RUNTIME_DIR:-/run/user/$(id -u)}"
export DISPLAY="${DISPLAY:-:0}"
if [[ -z "${XAUTHORITY:-}" ]]; then
  for signal47_auth in "$XDG_RUNTIME_DIR"/xauth_*; do
    if [[ -r "$signal47_auth" ]]; then export XAUTHORITY="$signal47_auth"; break; fi
  done
fi
if [[ -S "$XDG_RUNTIME_DIR/pulse/native" ]]; then export PULSE_SERVER="unix:$XDG_RUNTIME_DIR/pulse/native"; fi
exec 9>"$XDG_RUNTIME_DIR/signal47-gauntlet.lock"
flock -n 9 || { echo 'Another SIGNAL 47 build/check is running.' >&2; exit 1; }
mkdir -p Artifacts/Workstation18
signal47_profile=$(mktemp -d "$PWD/Artifacts/Workstation18/check-XXXXXX")
printf 'Disposable API test profile.\n' > "$signal47_profile/ALLOW_WORKSTATION18_TEST"
printf '%s\n' "$signal47_profile" > Artifacts/Workstation18/latest-profile.txt
signal47_player="${1:-$PWD/Artifacts/GauntletLinux/Signal47.x86_64}"
timeout --signal=TERM --kill-after=10s 180s "$signal47_player" --signal47-workstation18-checks --signal47-save-dir "$signal47_profile" -force-glcore -screen-width 1280 -screen-height 800 -screen-fullscreen 0 -logFile "$signal47_profile/player.log" > "$signal47_profile/stdout.log" 2>&1
python3 - "$signal47_profile" <<'PY'
import json,sys
from pathlib import Path
p=Path(sys.argv[1]);r=json.loads((p/'Evidence/result.json').read_text())
assert r['result']=='PASS',r
print(json.dumps({'result':r['result'],'checks':len(r['checks']),'profile':str(p),'method':r['method']}))
PY
