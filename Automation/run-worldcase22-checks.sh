#!/usr/bin/env bash
# Opt-in main-game checks using a copied historical case; no desktop input.
set -euo pipefail
cd "$(dirname "$0")/.."
export XDG_RUNTIME_DIR="${XDG_RUNTIME_DIR:-/run/user/$(id -u)}"
export DISPLAY="${DISPLAY:-:0}"
if [[ -z "${XAUTHORITY:-}" ]]; then
  for signal47_auth in "$XDG_RUNTIME_DIR"/xauth_*; do
    if [[ -r "$signal47_auth" ]]; then export XAUTHORITY="$signal47_auth"; break; fi
  done
fi
exec 9>"$XDG_RUNTIME_DIR/signal47-gauntlet.lock"
flock -n 9 || { echo 'Another SIGNAL47 build/check is running.' >&2; exit 1; }
mkdir -p Artifacts/WorldCase22
signal47_profile=$(mktemp -d "$PWD/Artifacts/WorldCase22/worldcase22-test-XXXXXX")
printf 'Disposable copied case for archive checks.\n' > "$signal47_profile/ALLOW_WORLDCASE22_TEST"
printf '%s\n' "$signal47_profile" > Artifacts/WorldCase22/latest-profile.txt
signal47_player="${1:-$PWD/Artifacts/GauntletLinux/Signal47.x86_64}"
signal47_seed="${2:-$PWD/Artifacts/Chapter09/Profiles/Visual10FinalActive}"
python3 - "$signal47_profile" "$signal47_seed" <<'PY'
from pathlib import Path
import hashlib,json,shutil,sys
profile,seed=map(Path,sys.argv[1:]);envelope=json.loads((seed/'case.json').read_text());snapshot=json.loads(envelope['payload'])
assert json.loads(snapshot['chapter'])['complete'] and not snapshot.get('worldCase'), 'Use a completed historical v1 fixture without archive extension.'
(profile/'Seed').mkdir();shutil.copy2(seed/'case.json',profile/'Seed/case-source.json');shutil.copytree(seed/'FieldPhotos',profile/'FieldPhotos')
camera=json.loads(snapshot['camera']);assert len(camera['frames'])==2
for frame in camera['frames']:
 photo=profile/'FieldPhotos'/Path(frame['path']).name
 assert hashlib.sha256(photo.read_bytes()).hexdigest()==frame['sha256'];frame['path']=str(photo)
snapshot['camera']=json.dumps(camera,separators=(',',':'));envelope['payload']=json.dumps(snapshot,separators=(',',':'));envelope['sha256']=hashlib.sha256(envelope['payload'].encode()).hexdigest().upper()
(profile/'Seed/case.json').write_text(json.dumps(envelope,indent=2))
paths=[seed/'case.json',*sorted((seed/'FieldPhotos').glob('*'))]
(profile/'Seed/source-hashes.json').write_text(json.dumps([{'source':str(p),'sha256':hashlib.sha256(p.read_bytes()).hexdigest()} for p in paths if p.is_file()],indent=2))
PY
set +e
bash Automation/Splat21/bounded.sh 2147483648 240 "$signal47_player" --signal47-worldcase22-checks --signal47-save-dir "$signal47_profile" -force-glcore -screen-width "${3:-1280}" -screen-height "${4:-800}" -screen-fullscreen 0 -logFile "$signal47_profile/player.log" > "$signal47_profile/stdout.log" 2>&1
signal47_exit=$?
set -e
printf '%s\n' "$signal47_exit" > "$signal47_profile/exit-code.txt"
[[ "$signal47_exit" == 0 ]] || { cat "$signal47_profile/Evidence/result.json" 2>/dev/null || true; exit "$signal47_exit"; }
python3 - "$signal47_profile" <<'PY'
import json,sys
from pathlib import Path
p=Path(sys.argv[1]);r=json.loads((p/'Evidence/result.json').read_text());assert r['result']=='PASS',r
print(json.dumps({'result':r['result'],'checks':len(r['checks']),'profile':str(p),'method':r['method']}))
PY
