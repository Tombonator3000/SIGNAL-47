#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."

# Opt-in only: this runner never builds Unity and uses a disposable profile.
environment27_runtime_dir="${XDG_RUNTIME_DIR:-/run/user/$(id -u)}"
export XDG_RUNTIME_DIR="$environment27_runtime_dir"
export DISPLAY="${DISPLAY:-:0}"
if [[ -z "${XAUTHORITY:-}" ]]; then
  for environment27_auth in "$environment27_runtime_dir"/xauth_*; do
    [[ -r "$environment27_auth" ]] && { export XAUTHORITY="$environment27_auth"; break; }
  done
fi
if [[ -S "$environment27_runtime_dir/pulse/native" ]]; then export PULSE_SERVER="unix:$environment27_runtime_dir/pulse/native"; fi

exec 9>"$environment27_runtime_dir/signal47-gauntlet.lock"
flock -n 9 || { echo 'Another SIGNAL 47 check owns the desktop lock.' >&2; exit 3; }
if pgrep -x Signal47.x86_64 >/dev/null; then
  echo 'A Signal47 player is already running; refusing to attach to it.' >&2
  exit 3
fi

environment27_player="$PWD/Artifacts/GauntletLinux/Signal47.x86_64"
environment27_seed="$PWD/Artifacts/Station26"
environment27_invocation="$0 $*"
environment27_width="${SIGNAL47_ENVIRONMENT27_WIDTH:-1280}"
environment27_height="${SIGNAL47_ENVIRONMENT27_HEIGHT:-800}"
environment27_preview=0
environment27_api=glcore
environment27_positionals=()
while [[ "$#" -gt 0 ]]; do
  case "$1" in
    --preview|--signal47-environment27-preview) environment27_preview=1; shift ;;
    --signal47-environment27-checks) shift ;;
    --width) [[ "$#" -ge 2 ]] || { echo '--width requires pixels' >&2; exit 2; }; environment27_width="$2"; shift 2 ;;
    --height) [[ "$#" -ge 2 ]] || { echo '--height requires pixels' >&2; exit 2; }; environment27_height="$2"; shift 2 ;;
    --graphics-api) [[ "$#" -ge 2 ]] || { echo '--graphics-api requires auto, glcore or vulkan' >&2; exit 2; }; environment27_api="$2"; shift 2 ;;
    *) environment27_positionals+=("$1"); shift ;;
  esac
done
if [[ "${#environment27_positionals[@]}" -gt 0 ]]; then environment27_player="${environment27_positionals[0]}"; fi
if [[ "${#environment27_positionals[@]}" -gt 1 ]]; then environment27_seed="${environment27_positionals[1]}"; fi
[[ "$environment27_width" =~ ^[1-9][0-9]+$ && "$environment27_height" =~ ^[1-9][0-9]+$ ]] || { echo 'Width and height must be positive pixel counts.' >&2; exit 2; }
environment27_graphics=()
case "$environment27_api" in auto) ;; glcore) environment27_graphics=(-force-glcore) ;; vulkan) environment27_graphics=(-force-vulkan) ;; *) echo 'Graphics API must be auto, glcore or vulkan.' >&2; exit 2 ;; esac
[[ -x "$environment27_player" ]] || { echo "Player not executable: $environment27_player" >&2; exit 2; }
environment27_player="$(realpath "$environment27_player")"
[[ -e "$environment27_seed" ]] || { echo "Completed WorldCase seed path not found: $environment27_seed" >&2; exit 2; }
environment27_player_sha256="$(sha256sum "$environment27_player" | awk '{print $1}')"
environment27_build_stamp="$(dirname "$environment27_player")/build-id.txt"
environment27_build_id='unknown'
[[ -f "$environment27_build_stamp" ]] && environment27_build_id="$(tr -d '\n' < "$environment27_build_stamp")"

mkdir -p Artifacts/Environment27
environment27_profile=$(mktemp -d "$PWD/Artifacts/Environment27/environment27-test-XXXXXX")
printf '%s\n' 'Disposable completed-WorldCase visual/performance probe profile.' > "$environment27_profile/ALLOW_ENVIRONMENT27_TEST"
printf '%s\n' "$environment27_profile" > Artifacts/Environment27/latest-profile.txt
environment27_mode=route
environment27_seconds=180
environment27_flag=--signal47-environment27-checks
if [[ "$environment27_preview" == 1 ]]; then
  environment27_mode=preview; environment27_seconds=60; environment27_flag=--signal47-environment27-preview
fi
printf '%s\n' "mode=$environment27_mode" "invocation=$environment27_invocation" "player=$environment27_player" "player_sha256=$environment27_player_sha256" "build_id=$environment27_build_id" "seed=$environment27_seed" "resource_cap_bytes=2147483648" "runtime_cap_seconds=$environment27_seconds" "resolution=${environment27_width}x${environment27_height}" "graphics_api_request=$environment27_api" "native_input_claim=false" > "$environment27_profile/invocation.txt"

python3 - "$environment27_profile" "$environment27_seed" <<'PY'
import hashlib, json, shutil, sys
from pathlib import Path

profile = Path(sys.argv[1])
requested = Path(sys.argv[2])
candidates = [requested] if requested.is_file() else sorted(requested.rglob('case.json'), key=lambda p: p.stat().st_mtime, reverse=True)
selected = None
for candidate in candidates:
    try:
        envelope = json.loads(candidate.read_text())
        snapshot = json.loads(envelope['payload'])
        chapter = json.loads(snapshot['chapter'])
        world = json.loads(snapshot['worldCase'])
        if chapter.get('complete') is True and world.get('p05Complete') is True and world.get('destination') and world.get('surveyId'):
            selected = candidate
            break
    except (OSError, KeyError, TypeError, ValueError, json.JSONDecodeError):
        pass
if selected is None:
    raise SystemExit('No completed WorldCase seed found; pass a Station26 profile containing chapter.complete and worldCase.p05Complete.')

envelope = json.loads(selected.read_text())
snapshot = json.loads(envelope['payload'])
camera = json.loads(snapshot['camera'])
source_photos = selected.parent / 'FieldPhotos'
if not source_photos.is_dir():
    raise SystemExit(f'Seed has no FieldPhotos directory: {source_photos}')
if not snapshot.get('station') or len(camera.get('frames', [])) < 4:
    raise SystemExit('Completed WorldCase seed must also include the Station26 state and four camera frames.')
(profile / 'Seed').mkdir()
(profile / 'FieldPhotos').mkdir()
for source in sorted(source_photos.iterdir()):
    if source.is_file(): shutil.copy2(source, profile / 'FieldPhotos' / source.name)
for frame in camera.get('frames', []):
    source = Path(frame['path'])
    if not source.exists(): source = source_photos / source.name
    if not source.is_file(): raise SystemExit(f'Camera frame is missing: {source}')
    digest = hashlib.sha256(source.read_bytes()).hexdigest()
    if frame.get('sha256') and frame['sha256'].lower() != digest: raise SystemExit(f'Camera frame hash mismatch: {source}')
    target_photo = profile / 'FieldPhotos' / source.name
    shutil.copy2(source, target_photo)
    assert hashlib.sha256(target_photo.read_bytes()).hexdigest() == digest
    frame['path'] = str(target_photo)
snapshot['camera'] = json.dumps(camera, separators=(',', ':'))
station = json.loads(snapshot.get('station') or '{}')
if station:
    # Opening the completed station is setup in this disposable profile only.
    station['inStation'] = True
    station['lampCovered'] = False
    snapshot['station'] = json.dumps(station, separators=(',', ':'))
envelope['payload'] = json.dumps(snapshot, separators=(',', ':'))
envelope['sha256'] = hashlib.sha256(envelope['payload'].encode()).hexdigest().upper()
(profile / 'Seed' / 'case.json').write_text(json.dumps(envelope, indent=2))
paths = [selected] + sorted(source_photos.iterdir())
(profile / 'Seed' / 'source-hashes.json').write_text(json.dumps([
    {'source': str(path), 'sha256': hashlib.sha256(path.read_bytes()).hexdigest()}
    for path in paths if path.is_file()
], indent=2))
(profile / 'Seed' / 'selection.json').write_text(json.dumps({
    'selected': str(selected), 'requirement': 'chapter.complete=true and worldCase.p05Complete=true',
    'sourceCaseSha256': hashlib.sha256(selected.read_bytes()).hexdigest()
}, indent=2))
PY

set +e
bash Automation/Splat21/bounded.sh 2147483648 "$environment27_seconds" "$environment27_player" "$environment27_flag" --signal47-save-dir "$environment27_profile" "${environment27_graphics[@]}" -screen-width "$environment27_width" -screen-height "$environment27_height" -screen-fullscreen 0 -logFile "$environment27_profile/player.log" > "$environment27_profile/stdout.log" 2>&1
environment27_exit=$?
set -e
printf '%s\n' "$environment27_exit" > "$environment27_profile/exit-code.txt"
if pgrep -x Signal47.x86_64 >/dev/null; then
  echo 'Signal47 player remained after bounded probe; refusing to claim a clean result.' >&2
  exit 3
fi
[[ "$environment27_exit" == 0 ]] || { cat "$environment27_profile/Evidence/result.json" 2>/dev/null || true; exit "$environment27_exit"; }

python3 - "$environment27_profile" "$environment27_preview" <<'PY'
import json, sys
from pathlib import Path
p = Path(sys.argv[1])
preview = sys.argv[2] == '1'
result = json.loads((p / 'Evidence' / 'result.json').read_text())
assert result['result'] == 'PASS', result
assert result['screenshots'] == 6, result
if preview:
    assert result['preview'] and result['frames'] == 0, result
else:
    assert result['frames'] > 0 and result['measuredSeconds'] >= 119, result
    assert (p / 'Evidence' / 'frames.csv').is_file(), result
print(json.dumps({'result': result['result'], 'mode': 'preview' if preview else 'route', 'frames': result['frames'], 'screenshots': result['screenshots'], 'profile': str(p)}, indent=2))
PY
