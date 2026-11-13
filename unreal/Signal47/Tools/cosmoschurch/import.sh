#!/bin/bash
# Tom's Cosmos pack "Demonic Village" (its plain graveyard pieces only) into SIGNAL / 47. Only the chosen meshes and what they need
# (selection.json) are taken out of the zips (extract.py, an isolated python), moved to /Game/Licensed/CosmosChurch in a throwaway
# 5.8 project (migrate.py, the way Tools/cosmosshop does it) and copied to Content/Licensed/CosmosChurch, which Git ignores.
# Nothing from the zips is run.
#   import.sh            all three steps
#   import.sh extract    only the zips into the work folder (no engine)
#   import.sh editor     only the two editor runs (the heavy step: one heavy Unreal job at a time, the guard fails closed)
#   import.sh copy       only the copy into Content/Licensed/CosmosChurch and the report
#   S47_COSMOS_ZIPS   the folder with the zips (default ~/Desktop)
#   S47_COSMOS_WORK   a work folder (default ~/scratch-bm/cosmos-church); it must not hold an earlier run (clear it first)
set -euo pipefail
UE_ROOT=${UE_ROOT:-$HOME/.local/opt/UnrealEngine-5.8.4}
PROJECT_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
HERE="$PROJECT_ROOT/Tools/cosmoschurch"
ZIPS=${S47_COSMOS_ZIPS:-$HOME/Desktop}
WORK=${S47_COSMOS_WORK:-$HOME/scratch-bm/cosmos-church}
DEST="$PROJECT_ROOT/Content/Licensed/CosmosChurch"
STEP=${1:-all}
case "$STEP" in all|extract|editor|copy) ;; *) echo "S47 COSMOS CHURCH: step is all, extract, editor or copy" >&2; exit 1;; esac
# Licensed files never in the public repo: stop before anything if Git would see them.
git -C "$PROJECT_ROOT" check-ignore -q "$DEST/Village/Meshes/Cemetery/SM_Cemetery01.uasset" || { echo "S47 COSMOS CHURCH: Content/Licensed is not ignored by Git; stop" >&2; exit 3; }
# The disk reserve: the extracted packages are ~0.5 GB, then a copy into the project; 100 GB stays free.
FREE=$(df -B1 --output=avail "$PROJECT_ROOT/Content" | tail -1)
[ "$FREE" -gt $((102*1024*1024*1024)) ] || { echo "S47 COSMOS CHURCH: under 102 GB free; stop" >&2; exit 5; }

if [ "$STEP" = all ] || [ "$STEP" = extract ]; then
 [ -f "$ZIPS/DemonicVillage.zip" ] || { echo "S47 COSMOS CHURCH: no $ZIPS/DemonicVillage.zip" >&2; exit 2; }
 [ -e "$WORK/proj58" ] && { echo "S47 COSMOS CHURCH: $WORK/proj58 is there from an earlier run; clear $WORK first" >&2; exit 4; }
 mkdir -p "$WORK"
 # the pack into its own new, empty folder; isolated python (no module from the work folder is imported)
 python3 -I "$HERE/extract.py" --zip "$ZIPS/DemonicVillage.zip" --pack demonic --out "$WORK/demonic-src"
 mkdir -p "$WORK/proj58/Content"
 cp -a --reflink=auto "$WORK/demonic-src/Content/." "$WORK/proj58/Content/"
 cat > "$WORK/proj58/Church58.uproject" <<'JSON'
{ "FileVersion": 3, "EngineAssociation": "UE_5.8", "Category": "", "Description": "",
  "Plugins": [ { "Name": "PythonScriptPlugin", "Enabled": true }, { "Name": "EditorScriptingUtilities", "Enabled": true } ] }
JSON
fi

if [ "$STEP" = all ] || [ "$STEP" = editor ]; then
 [ -f "$WORK/proj58/Church58.uproject" ] || { echo "S47 COSMOS CHURCH: no $WORK/proj58; run the extract step first" >&2; exit 4; }
 python3 "$PROJECT_ROOT/Scripts/unreal_guard.py" require-idle
 for mode in import verify; do
  # (the editor can hang after the script has finished: the report, not the exit, says how it went)
  S47_COSMOS_CHURCH_MODE=$mode S47_COSMOS_CHURCH_REPORT="$WORK/$mode.json" timeout 540 nice -n 19 "$UE_ROOT/Engine/Binaries/Linux/UnrealEditor-Cmd" \
   "$WORK/proj58/Church58.uproject" -run=pythonscript -script="$HERE/migrate.py" -unattended -nullrhi -nosplash -NoSound -NoLiveCoding \
   > "$WORK/$mode.log" 2>&1 < /dev/null || true
  python3 -c "import json,sys; r=json.load(open(sys.argv[1])); print('S47 COSMOS CHURCH', sys.argv[2], r['status'], 'passed', len(r.get('passed', [])), 'failed', sorted(r.get('failed', {}))); sys.exit(r['status'] != 'PASS')" "$WORK/$mode.json" $mode
 done
fi

if [ "$STEP" = all ] || [ "$STEP" = copy ]; then
 python3 -c "import json,sys; sys.exit(json.load(open(sys.argv[1]))['status'] != 'PASS')" "$WORK/verify.json" || { echo "S47 COSMOS CHURCH: no passed verify.json in $WORK; stop" >&2; exit 7; }
 mkdir -p "$DEST"
 cp -a --reflink=auto "$WORK/proj58/Content/Licensed/CosmosChurch/." "$DEST/"
 [ -z "$(git -C "$PROJECT_ROOT" status --porcelain -- Content/Licensed)" ] || { echo "S47 COSMOS CHURCH: Git sees Content/Licensed; stop" >&2; exit 6; }
 cp "$WORK/verify.json" "$HERE/verification.json"
 echo "S47 COSMOS CHURCH: in $DEST ($(du -sh "$DEST" | cut -f1)); report Tools/cosmoschurch/verification.json"
fi
