#!/usr/bin/env bash
# Exercise the packaged initializer and actual Continue API in an isolated profile.
set -euo pipefail
cd "$(dirname "$0")/.."
environment27_package="$(realpath "${1:?Pass the extracted Environment27 package directory}")"
[[ -x "$environment27_package/Start-STATION01.sh" && -x "$environment27_package/Start-SIGNAL47.sh" ]]
export XDG_RUNTIME_DIR="${XDG_RUNTIME_DIR:-/run/user/$(id -u)}"
export DISPLAY="${DISPLAY:-:0}"
if [[ -z "${XAUTHORITY:-}" ]]; then
  for environment27_auth in "$XDG_RUNTIME_DIR"/xauth_*; do
    [[ -r "$environment27_auth" ]] && { export XAUTHORITY="$environment27_auth"; break; }
  done
fi
exec 9>"$XDG_RUNTIME_DIR/signal47-gauntlet.lock"
flock -n 9 || { echo 'Another SIGNAL47 check owns the run lock.' >&2; exit 3; }
if pgrep -x Signal47.x86_64 >/dev/null; then echo 'A SIGNAL47 player is already running.' >&2; exit 3; fi
environment27_data=$(mktemp -d "$PWD/Artifacts/Environment27/demo-data-XXXXXX")
environment27_profile="$environment27_data/Signal47/Environment27Demo/environment27-test-packaged"
XDG_DATA_HOME="$environment27_data" python3 "$environment27_package/initialize-station01.py" --profile "$environment27_profile"
printf '%s\n' 'Disposable packaged demo Continue test.' > "$environment27_profile/ALLOW_ENVIRONMENT27_TEST"
printf '%s\n' "$environment27_profile" > Artifacts/Environment27/latest-demo-profile.txt
printf '%s\n' "package=$environment27_package" 'graphics_api_request=auto' 'resolution=1600x900' 'method=Packaged initializer and normal launcher; Continue API; no native menu input' > "$environment27_profile/invocation.txt"
environment27_before=$(sha256sum "$environment27_profile/case.json")
XDG_DATA_HOME="$environment27_data" python3 "$environment27_package/initialize-station01.py" --profile "$environment27_profile"
[[ "$environment27_before" == "$(sha256sum "$environment27_profile/case.json")" ]]
set +e
bash Automation/Splat21/bounded.sh 2147483648 60 "$environment27_package/Start-SIGNAL47.sh" --signal47-environment27-demo-checks --signal47-save-dir "$environment27_profile" -screen-width 1600 -screen-height 900 -screen-fullscreen 0 -logFile "$environment27_profile/player.log" > "$environment27_profile/stdout.log" 2>&1
environment27_exit=$?
set -e
printf '%s\n' "$environment27_exit" > "$environment27_profile/exit-code.txt"
[[ "$environment27_exit" == 0 ]]
python3 - "$environment27_profile" <<'PY'
import json,sys
from pathlib import Path
p=Path(sys.argv[1]);d=json.loads((p/'Evidence/result.json').read_text())
assert d['result']=='PASS' and d['demoSeed'] and d['screenshots']==6,d
print(json.dumps({'result':d['result'],'checks':len(d['checks']),'profile':str(p),'renderer':d['renderer'],'demoSeed':True,'nativeInputClaim':False},indent=2))
PY
