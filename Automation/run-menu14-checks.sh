#!/usr/bin/env bash
# API-driven runtime checks and unchanged Unity captures; never sends desktop input.
set -euo pipefail
cd "$(dirname "$0")/.."
signal47_runtime_dir="${XDG_RUNTIME_DIR:-/run/user/$(id -u)}"
export XDG_RUNTIME_DIR="$signal47_runtime_dir"
export DISPLAY="${DISPLAY:-:0}"
if [[ -z "${XAUTHORITY:-}" ]]; then
  for signal47_auth_file in "$signal47_runtime_dir"/xauth_*; do
    if [[ -r "$signal47_auth_file" ]]; then export XAUTHORITY="$signal47_auth_file"; break; fi
  done
fi
if [[ -S "$signal47_runtime_dir/pulse/native" ]]; then export PULSE_SERVER="unix:$signal47_runtime_dir/pulse/native"; fi
exec 9>"$signal47_runtime_dir/signal47-gauntlet.lock"
flock -n 9 || { echo 'Another SIGNAL 47 check owns the run lock.' >&2; exit 1; }
mkdir -p Artifacts/Menu14
signal47_menu_profile=$(mktemp -d "$PWD/Artifacts/Menu14/menu14-test-XXXXXX")
printf 'Isolated disposable menu/save contract fixtures.\n' > "$signal47_menu_profile/ALLOW_MENU14_TEST"
printf '%s\n' "$signal47_menu_profile" > Artifacts/Menu14/latest-profile.txt
signal47_menu_player="${1:-$PWD/Artifacts/GauntletLinux/Signal47.x86_64}"
signal47_menu_seed="${2:-$PWD/Artifacts/Chapter09/Profiles/Visual10FinalActive}"
python3 - "$signal47_menu_profile" "$signal47_menu_seed" <<'PY'
import hashlib,json,shutil,sys
from pathlib import Path
profile,seed=map(Path,sys.argv[1:]);envelope=json.loads((seed/'case.json').read_text());snapshot=json.loads(envelope['payload'])
assert json.loads(snapshot['chapter'])['complete'], 'Use a completed historical journey fixture.'
(profile/'Seed').mkdir();shutil.copy2(seed/'case.json',profile/'Seed/case-source.json')
shutil.copytree(seed/'FieldPhotos',profile/'FieldPhotos')
camera=json.loads(snapshot['camera'])
for frame in camera['frames']:
    photo=profile/'FieldPhotos'/Path(frame['path']).name
    assert hashlib.sha256(photo.read_bytes()).hexdigest()==frame['sha256']
    frame['path']=str(photo)
snapshot['camera']=json.dumps(camera,separators=(',',':'))
envelope['payload']=json.dumps(snapshot,separators=(',',':'))
envelope['sha256']=hashlib.sha256(envelope['payload'].encode()).hexdigest().upper()
(profile/'Seed/case.json').write_text(json.dumps(envelope,indent=2))
snapshot['savedUtc']='2026-09-11T01:02:03.0000000Z'
previous=dict(envelope,payload=json.dumps(snapshot,separators=(',',':')))
previous['sha256']=hashlib.sha256(previous['payload'].encode()).hexdigest().upper()
(profile/'Seed/previous-case.json').write_text(json.dumps(previous,indent=2))
(profile/'Seed/fixture.txt').write_text('Historical v1 fixture: only photo paths relocated to this isolated profile and envelope digest recomputed. Previous-case fixture additionally changes savedUtc to distinguish the reviewed checkpoint. Original JPEG bytes unchanged. Not a native-input journey.\n')
paths=[seed/'case.json',*sorted((seed/'FieldPhotos').glob('*'))]
(profile/'Seed/source-hashes.json').write_text(json.dumps([{'source':str(p),'sha256':hashlib.sha256(p.read_bytes()).hexdigest()} for p in paths if p.is_file()],indent=2))
PY
signal47_menu_suite="${3:---signal47-menu14-checks}"
case "$signal47_menu_suite" in --signal47-menu14-checks|--signal47-recovery15-checks) ;; *) echo 'Unknown check suite' >&2; exit 1 ;; esac
timeout --signal=TERM --kill-after=10s 180s "$signal47_menu_player" "$signal47_menu_suite" --signal47-save-dir "$signal47_menu_profile" -force-glcore -screen-width 1280 -screen-height 800 -screen-fullscreen 0 -logFile "$signal47_menu_profile/player.log"
python3 - "$signal47_menu_profile" <<'PY'
import json,sys
from pathlib import Path
profile=Path(sys.argv[1]);report=json.loads((profile/'Evidence/result.json').read_text())
assert report['result']=='PASS', report
print(json.dumps({'result':report['result'],'checks':len(report['checks']),'profile':str(profile),'method':report['method']},indent=2))
PY
