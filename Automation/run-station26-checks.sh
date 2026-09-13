#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
station26_runtime_dir="${XDG_RUNTIME_DIR:-/run/user/$(id -u)}"; export XDG_RUNTIME_DIR="$station26_runtime_dir"; export DISPLAY="${DISPLAY:-:0}"
if [[ -z "${XAUTHORITY:-}" ]]; then for station26_auth in "$station26_runtime_dir"/xauth_*; do [[ -r "$station26_auth" ]] && { export XAUTHORITY="$station26_auth"; break; }; done; fi
exec 9>"$station26_runtime_dir/signal47-gauntlet.lock"; flock -n 9 || { echo 'Another SIGNAL47 check owns the run lock.' >&2; exit 1; }
if pgrep -x Signal47.x86_64 >/dev/null; then echo "A game is already running; finish that session first." >&2; exit 1; fi
mkdir -p Artifacts/Station26
station26_profile=$(mktemp -d "$PWD/Artifacts/Station26/station26-test-XXXXXX")
printf '%s\n' 'Disposable station26 profile for integrated P06-P09 checks.' > "$station26_profile/ALLOW_STATION26_TEST"
printf '%s\n' "$station26_profile" > Artifacts/Station26/latest-profile.txt
station26_player="${1:-$PWD/Artifacts/GauntletLinux/Signal47.x86_64}"; station26_seed="${2:-$PWD/Artifacts/Chapter09/Profiles/Visual10FinalActive}"
python3 - "$station26_profile" "$station26_seed" <<'PY'
import hashlib,json,shutil,sys
from pathlib import Path
profile,seed=map(Path,sys.argv[1:]); envelope=json.loads((seed/'case.json').read_text()); snapshot=json.loads(envelope['payload'])
assert json.loads(snapshot['chapter'])['complete'] and not snapshot.get('worldCase'), 'Use a completed historical v1 fixture without archive extension.'
(profile/'Seed').mkdir(); shutil.copy2(seed/'case.json',profile/'Seed/case-source.json'); shutil.copytree(seed/'FieldPhotos',profile/'FieldPhotos')
camera=json.loads(snapshot['camera']); assert len(camera['frames'])==2
for frame in camera['frames']:
    photo=profile/'FieldPhotos'/Path(frame['path']).name; assert hashlib.sha256(photo.read_bytes()).hexdigest()==frame['sha256']; frame['path']=str(photo)
snapshot['camera']=json.dumps(camera,separators=(',',':')); envelope['payload']=json.dumps(snapshot,separators=(',',':')); envelope['sha256']=hashlib.sha256(envelope['payload'].encode()).hexdigest().upper()
(profile/'Seed/case.json').write_text(json.dumps(envelope,indent=2))
paths=[seed/'case.json',*sorted((seed/'FieldPhotos').glob('*'))]; (profile/'Seed/source-hashes.json').write_text(json.dumps([{'source':str(p),'sha256':hashlib.sha256(p.read_bytes()).hexdigest()} for p in paths if p.is_file()],indent=2))
PY
set +e
bash Automation/Splat21/bounded.sh 2147483648 360 "$station26_player" --signal47-station26-checks --signal47-save-dir "$station26_profile" -force-glcore -screen-width "${3:-1280}" -screen-height "${4:-800}" -screen-fullscreen 0 -logFile "$station26_profile/player.log" > "$station26_profile/stdout.log" 2>&1
station26_exit=$?; set -e; printf '%s\n' "$station26_exit" > "$station26_profile/exit-code.txt"
[[ "$station26_exit" == 0 ]] || { cat "$station26_profile/Evidence/result.json" 2>/dev/null || true; exit "$station26_exit"; }
python3 - "$station26_profile" <<'PY'
import json,sys
from pathlib import Path
p=Path(sys.argv[1]); r=json.loads((p/'Evidence/result.json').read_text()); assert r['result']=='PASS',r
print(json.dumps({'result':r['result'],'checks':len(r['checks']),'profile':str(p),'method':r['method']},indent=2))
PY
