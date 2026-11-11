#!/bin/bash
# import_truck.sh [import-root] -- the service Jeep's body and four wheels into /Game/Lived/Truck by the measured Lived route.
# D46-20261110 (D46-7, JEEP-FORM-20261108). The root holds lived-truck-contract.json, lived-truck-materials.json,
# truck/Export/SM_TripoJeep_Wheel{Body,FL,FR,RL,RR}.fbx and textures/T_Lived_*.png; by default the 10 November build
# (private, outside Git: the model is Tom's licensed Tripo source). Runs the editor's python importer (import_lived.py:
# Nanite off, three LODs, no collision, full-precision UVs) behind the machine's reservation, then reads its report.
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="${1:-$HOME/Documents/ChatGPT/Signal 47/modeller/jeep-20261110/import-root}"
for f in lived-truck-contract.json lived-truck-materials.json truck/Export/SM_TripoJeep_WheelBody.fbx truck/Export/SM_TripoJeep_WheelRR.fbx textures/T_Lived_Enamel_BC.png; do
 [ -f "$ROOT/$f" ] || { echo "import_truck: missing $ROOT/$f" >&2; exit 2; }
done
# run_import.py expects src/ beside it; the work copy lives in the project's ignored Saved folder
WORK="$HOME/SIGNAL-47/unreal/Signal47/Saved/lived-import-work"
mkdir -p "$WORK"; ln -sfn "$HERE" "$WORK/src"; cp "$HERE/run_import.py" "$WORK/run_import.py"
Q="$HOME/.claude/skills/signal47-nattskift/scripts/q.py"
( cd "$WORK" && python3 "$Q" python3 run_import.py truck "$ROOT" ) | tail -12
python3 - "$HOME/SIGNAL-47/unreal/Signal47/Saved/lived-truck-import.json" <<'EOF'
import json,sys
d=json.load(open(sys.argv[1]));bad=[c for c in d['checks'] if not c['pass']]
print('lived-truck-import',d['status'],len(d['checks']),'checks',len(bad),'failed',[m['asset'] for m in d.get('meshes',[])])
for c in bad:print(' FAIL',c['name'],c['detail'])
sys.exit(1 if bad or d['status']!='PASS' else 0)
EOF
