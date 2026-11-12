#!/bin/bash
# ANIM-TRY-20261112: Free100Animations (unpacked from Tom's zip, ~/Assets/signal47-free/_inbox/free100animations-20261111/unpacked) into
# SIGNAL / 47 at /Game/Licensed/Free100, which Git ignores. Only the folders in selection.json "copy" are taken (mannequin mesh,
# skeleton, materials, idle/walk/jog and the 105 clips; no control rigs, maps or blueprints), moved in a throwaway 5.8 project
# (migrate.py, as Tools/cosmosshop) and copied to Content/Licensed/Free100. Nothing from the pack is run.
# One heavy Unreal job at a time: run through q.py; the guard fails closed when another job runs.
#   S47_FREE100_SRC   the unpacked project (default ~/Assets/signal47-free/_inbox/free100animations-20261111/unpacked/Free100Animations)
#   S47_ANIMTRY_WORK  a work folder (default ~/scratch-bm/animtry); it must not hold an earlier run (clear it first)
set -euo pipefail
UE_ROOT=${UE_ROOT:-$HOME/.local/opt/UnrealEngine-5.8.4}
PROJECT_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
HERE="$PROJECT_ROOT/Tools/animtry"
SRC=${S47_FREE100_SRC:-$HOME/Assets/signal47-free/_inbox/free100animations-20261111/unpacked/Free100Animations}
WORK=${S47_ANIMTRY_WORK:-$HOME/scratch-bm/animtry}
DEST="$PROJECT_ROOT/Content/Licensed/Free100"
[ -f "$SRC/Free100Animations.uproject" ] || { echo "S47 ANIMTRY: no unpacked project at $SRC" >&2; exit 2; }
# Licensed files never in the public repo: stop before anything if Git would see them.
git -C "$PROJECT_ROOT" check-ignore -q "$DEST/Mannequins/Meshes/SK_Mannequin.uasset" || { echo "S47 ANIMTRY: Content/Licensed is not ignored by Git; stop" >&2; exit 3; }
python3 "$PROJECT_ROOT/Scripts/unreal_guard.py" require-idle
[ -e "$WORK/proj58" ] && { echo "S47 ANIMTRY: $WORK/proj58 is there from an earlier run; clear $WORK first" >&2; exit 4; }
[ -e "$DEST" ] && { echo "S47 ANIMTRY: $DEST is there already; move it away first (nothing is overwritten)" >&2; exit 4; }
# The disk reserve: ~0.25 GB copied twice; 100 GB stays free.
FREE=$(df -B1 --output=avail "$PROJECT_ROOT/Content" | tail -1)
[ "$FREE" -gt $((101*1024*1024*1024)) ] || { echo "S47 ANIMTRY: under 101 GB free; stop" >&2; exit 5; }
mkdir -p "$WORK/proj58/Content"
# the chosen folders only, reflinked where the disk can (paths read from selection.json by an isolated python)
python3 -I -c 'import json,sys; [print(p) for p in json.load(open(sys.argv[1]))["copy"]]' "$HERE/selection.json" | while read -r rel; do
 case "$rel" in *..*|/*) echo "S47 ANIMTRY: bad copy path $rel" >&2; exit 6;; esac
 mkdir -p "$WORK/proj58/Content/$(dirname "$rel")"
 cp -a --reflink=auto "$SRC/Content/$rel" "$WORK/proj58/Content/$rel"
done
cat > "$WORK/proj58/AnimTry58.uproject" <<'EOF'
{ "FileVersion": 3, "EngineAssociation": "UE_5.8", "Category": "", "Description": "",
  "Plugins": [ { "Name": "PythonScriptPlugin", "Enabled": true }, { "Name": "EditorScriptingUtilities", "Enabled": true } ] }
EOF
for mode in import verify; do
 # (the editor can hang after the script has finished: the report, not the exit, says how it went)
 S47_ANIMTRY_MODE=$mode S47_ANIMTRY_REPORT="$WORK/$mode.json" timeout 540 nice -n 19 "$UE_ROOT/Engine/Binaries/Linux/UnrealEditor-Cmd" \
  "$WORK/proj58/AnimTry58.uproject" -run=pythonscript -script="$HERE/migrate.py" -unattended -nullrhi -nosplash -NoSound -NoLiveCoding \
  > "$WORK/$mode.log" 2>&1 < /dev/null || true
 python3 -c "import json,sys; r=json.load(open(sys.argv[1])); print('S47 ANIMTRY', sys.argv[2], r['status'], 'clips', sorted(r.get('clips', {})), 'failed', r.get('failed', {})); sys.exit(r['status'] != 'PASS')" "$WORK/$mode.json" $mode
done
mkdir -p "$DEST"
cp -a --reflink=auto "$WORK/proj58/Content/Licensed/Free100/." "$DEST/"
[ -z "$(git -C "$PROJECT_ROOT" status --porcelain -- Content/Licensed)" ] || { echo "S47 ANIMTRY: Git sees Content/Licensed; stop" >&2; exit 7; }
cp "$WORK/verify.json" "$HERE/verification.json"
echo "S47 ANIMTRY: in $DEST ($(du -sh "$DEST" | cut -f1)); report Tools/animtry/verification.json. Next: retarget.py in Signal47 (README.md)."
