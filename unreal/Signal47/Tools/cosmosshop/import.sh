#!/bin/bash
# GAS-STATION-LIFT-01: Tom's Cosmos packs (Post Apocalyptic Coffee Shop, UE 5.3; Abandoned Hospital, UE 4.22) into SIGNAL / 47 for the
# gas station's shop (S47StationDressing, Config/Props/gas_cosmos.json). Only the chosen meshes and what they need (selection.json) are
# taken out of the zips (extract.py, an isolated python), moved to /Game/Licensed/CosmosShop in a throwaway 5.8 project (migrate.py, the
# way Tools/militarybarriers does it) and copied to Content/Licensed/CosmosShop, which Git ignores. Nothing from the zips is run.
# One heavy Unreal job at a time: the guard fails closed when another job runs.
#   S47_COSMOS_ZIPS   the folder with the zips (default ~/Desktop)
#   S47_COSMOS_WORK   a work folder (default ~/scratch-bm/cosmos-shop); it must not hold an earlier run (clear it first)
set -euo pipefail
UE_ROOT=${UE_ROOT:-$HOME/.local/opt/UnrealEngine-5.8.4}
PROJECT_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
HERE="$PROJECT_ROOT/Tools/cosmosshop"
ZIPS=${S47_COSMOS_ZIPS:-$HOME/Desktop}
WORK=${S47_COSMOS_WORK:-$HOME/scratch-bm/cosmos-shop}
DEST="$PROJECT_ROOT/Content/Licensed/CosmosShop"
for z in PostApocalypticCoffeeShop AbandonedHospital; do [ -f "$ZIPS/$z.zip" ] || { echo "S47 COSMOS SHOP: no $ZIPS/$z.zip" >&2; exit 2; }; done
# Licensed files never in the public repo: stop before anything if Git would see them.
git -C "$PROJECT_ROOT" check-ignore -q "$DEST/CoffeeShop/Meshes/SM_Fridge.uasset" || { echo "S47 COSMOS SHOP: Content/Licensed is not ignored by Git; stop" >&2; exit 3; }
# (first, before anything is made: a run refused here can simply be tried again)
python3 "$PROJECT_ROOT/Scripts/unreal_guard.py" require-idle
[ -e "$WORK/proj58" ] && { echo "S47 COSMOS SHOP: $WORK/proj58 is there from an earlier run; clear $WORK first" >&2; exit 4; }
# The disk reserve: the extracted packages are ~0.5 GB, then a copy into the project; 100 GB stays free.
FREE=$(df -B1 --output=avail "$PROJECT_ROOT/Content" | tail -1)
[ "$FREE" -gt $((102*1024*1024*1024)) ] || { echo "S47 COSMOS SHOP: under 102 GB free; stop" >&2; exit 5; }
mkdir -p "$WORK"
# each pack into its own new, empty folder; isolated python (no module from the work folder is imported)
python3 -I "$HERE/extract.py" --zip "$ZIPS/PostApocalypticCoffeeShop.zip" --pack coffee --out "$WORK/coffee-src"
python3 -I "$HERE/extract.py" --zip "$ZIPS/AbandonedHospital.zip" --pack hospital --out "$WORK/hospital-src"
mkdir -p "$WORK/proj58/Content"
cp -a --reflink=auto "$WORK/coffee-src/Content/." "$WORK/hospital-src/Content/." "$WORK/proj58/Content/"
cat > "$WORK/proj58/Shop58.uproject" <<'EOF'
{ "FileVersion": 3, "EngineAssociation": "UE_5.8", "Category": "", "Description": "",
  "Plugins": [ { "Name": "PythonScriptPlugin", "Enabled": true }, { "Name": "EditorScriptingUtilities", "Enabled": true } ] }
EOF
for mode in import verify; do
 # (the editor can hang after the script has finished: the report, not the exit, says how it went)
 S47_COSMOS_SHOP_MODE=$mode S47_COSMOS_SHOP_REPORT="$WORK/$mode.json" timeout 540 nice -n 19 "$UE_ROOT/Engine/Binaries/Linux/UnrealEditor-Cmd" \
  "$WORK/proj58/Shop58.uproject" -run=pythonscript -script="$HERE/migrate.py" -unattended -nullrhi -nosplash -NoSound -NoLiveCoding \
  > "$WORK/$mode.log" 2>&1 < /dev/null || true
 python3 -c "import json,sys; r=json.load(open(sys.argv[1])); print('S47 COSMOS SHOP', sys.argv[2], r['status'], 'passed', len(r.get('passed', [])), 'failed', sorted(r.get('failed', {}))); sys.exit(r['status'] != 'PASS')" "$WORK/$mode.json" $mode
done
mkdir -p "$DEST"
cp -a --reflink=auto "$WORK/proj58/Content/Licensed/CosmosShop/." "$DEST/"
[ -z "$(git -C "$PROJECT_ROOT" status --porcelain -- Content/Licensed)" ] || { echo "S47 COSMOS SHOP: Git sees Content/Licensed; stop" >&2; exit 6; }
cp "$WORK/verify.json" "$HERE/verification.json"
echo "S47 COSMOS SHOP: in $DEST ($(du -sh "$DEST" | cut -f1)); report Tools/cosmosshop/verification.json"
