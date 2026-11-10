#!/bin/bash
# D45-20261110: Tom's Cosmos "VFX Smoke" pack (UE 5.0) into SIGNAL / 47 at its own package paths (/Game/Rocket/...), as the Fab packs
# are (Northwood, JustBStudios): a reflink copy of its Content/Rocket, then an editor check (import_smoke.py) that the chimney smoke
# loads, compiles in 5.8 and needs nothing outside /Game/Rocket and the engine. Licensed: Content/Rocket is in unreal/.gitignore.
# One heavy Unreal job at a time: the guard fails closed when another job runs.
set -euo pipefail
UE_ROOT=${UE_ROOT:-$HOME/.local/opt/UnrealEngine-5.8.4}
PROJECT_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
SRC=${S47_COSMOS_SMOKE:-"$HOME/Desktop/SIGNAL 47/Content from Cosmos/Smoke"}
WORK=${S47_COSMOS_WORK:-$HOME/scratch-bm/Signal47-cosmossmoke}
DEST="$PROJECT_ROOT/Content/Rocket"
[ -f "$SRC/Content/Rocket/NS1_Smoke/NS_NS1_Smoke_Chimney.uasset" ] || { echo "S47 COSMOS SMOKE: no pack at $SRC" >&2; exit 2; }
# Licensed files never in the public repo: stop before copying if Git would see them.
git -C "$PROJECT_ROOT" check-ignore -q "$DEST/NS1_Smoke/NS_NS1_Smoke_Chimney.uasset" || { echo "S47 COSMOS SMOKE: Content/Rocket is not ignored by Git; stop" >&2; exit 3; }
python3 "$PROJECT_ROOT/Scripts/unreal_guard.py" require-idle
if [ -e "$DEST" ]; then
 echo "S47 COSMOS SMOKE: Content/Rocket is there already; checking it only"
else
 # A quick look at the disk reserve (100 GB stays free); the pack is a few hundred MB at most.
 NEED=$(du -sb "$SRC/Content/Rocket" | cut -f1); FREE=$(df -B1 --output=avail "$PROJECT_ROOT/Content" | tail -1)
 [ "$FREE" -gt $((NEED + 100*1024*1024*1024)) ] || { echo "S47 COSMOS SMOKE: copying would leave under 100 GB free; stop" >&2; exit 4; }
 mkdir -p "$DEST"
 cp -a --reflink=auto "$SRC/Content/Rocket/." "$DEST/"
fi
mkdir -p "$WORK"
REPORT="$(mktemp "$WORK/import_smoke.XXXXXXXX.json")"
S47_COSMOS_REPORT="$REPORT" "$UE_ROOT/Engine/Binaries/Linux/UnrealEditor-Cmd" "$PROJECT_ROOT/Signal47.uproject" \
 -run=pythonscript -script="$PROJECT_ROOT/Tools/cosmossmoke/import_smoke.py" -unattended -nullrhi -nosplash -NoSound
python3 - "$REPORT" <<'PY'
import json,sys
r=json.load(open(sys.argv[1]))
bad=[c for c in r['checks'] if not c['pass']]
assert r.get('status')=='PASS' and not bad,(r.get('error'),bad)
print('S47 COSMOS SMOKE',r.get('status'),len(r['checks']),'checks',sys.argv[1])
print('  facts for S47Hap::CosmosSmokeScale:',json.dumps(r.get('facts',{})))
PY
