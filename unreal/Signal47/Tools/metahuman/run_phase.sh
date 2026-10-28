#!/bin/bash
# One MHStaging commandlet run with rendering (no window), after the shared Unreal guard passes.
# Usage: run_phase.sh <logname> step [step...]
set -euo pipefail
B="$HOME/scratch-bm/Signal47-metahuman-20261026"
UE="$HOME/.local/opt/UnrealEngine-5.8.4"
NAME="$1"; shift
python3 "$HOME/SIGNAL-47/unreal/Signal47/Scripts/unreal_guard.py" require-idle
exec "$UE/Engine/Binaries/Linux/UnrealEditor-Cmd" "$B/MHStaging/MHStaging.uproject" \
  -run=pythonscript -script="$B/tools/phase.py $*" \
  ${S47MH_RENDER--AllowCommandletRendering -RenderOffscreen} -unattended -nosplash -nosound -stdout -FullStdOutLogOutput \
  -NoMetaHumanAccountPortalLoginFallback \
  -abslog="$B/logs/$NAME.log"
