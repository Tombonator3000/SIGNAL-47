#!/bin/bash
# Session A (no Epic login needed): probe, studio, preset audition captures.
# Opens MHStaging without a window, drives it over Python remote execution, then closes it.
set -uo pipefail
B="$HOME/scratch-bm/Signal47-metahuman-20261026"
UE="$HOME/.local/opt/UnrealEngine-5.8.4"
R="python3 $B/tools/mh_remote.py"
LOG="$B/logs/session_a.log"
python3 "$HOME/SIGNAL-47/unreal/Signal47/Scripts/unreal_guard.py" require-idle || exit 3
"$UE/Engine/Binaries/Linux/UnrealEditor" "$B/MHStaging/MHStaging.uproject" -RenderOffscreen -unattended -nosplash -nosound \
  -NoMetaHumanAccountPortalLoginFallback -abslog="$LOG" &
EDITOR=$!
trap 'kill $EDITOR 2>/dev/null' EXIT
# First start compiles the MetaHuman shaders; allow up to 40 minutes before giving up.
for i in $(seq 1 240); do
  $R -c 'import unreal' >/dev/null 2>&1 && break
  kill -0 $EDITOR 2>/dev/null || { echo "editor exited early"; exit 4; }
  sleep 10
done
run() { echo ">> $1"; $R -c "import s47mh; s47mh.$1"; }
$R -c "import importlib, s47mh; importlib.reload(s47mh)"
run "probe()"
run "studio()"
for p in Grace Celeste Jelena Vivian Aera Etta; do
  run "show('$p')"
  sleep 25          # streaming, grooms and shader compilation settle while the editor ticks
  run "shoot()"
done
run "clear()"
$R -c 'import unreal; unreal.SystemLibrary.quit_editor()'
wait $EDITOR
echo "session A done"
