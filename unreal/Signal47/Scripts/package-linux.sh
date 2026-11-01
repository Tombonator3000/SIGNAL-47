#!/bin/bash
# Builds, cooks and packages a Linux Development build into OUT (default ~/scratch-bm/Signal47-build).
set -euo pipefail
UE_ROOT=${UE_ROOT:-$HOME/.local/opt/UnrealEngine-5.8.4}
OUT=${1:-$HOME/scratch-bm/Signal47-build}
P="$(cd "$(dirname "$0")/.." && pwd)/Signal47.uproject"
python3 "$(dirname "$P")/Scripts/unreal_guard.py" require-idle
mkdir -p "$(dirname "$P")/Saved/Logs"
BUILD_LOG=$(mktemp "$(dirname "$P")/Saved/Logs/package-linux.XXXXXXXX.log")
# UE 5.8's Zen oplog staging can stall after a recook. Cook loose packages for the
# normal IoStore container writer instead; this does not change the shipped renderer.
"$UE_ROOT/Engine/Build/BatchFiles/RunUAT.sh" BuildCookRun -project="$P" -noP4 -platform=Linux -clientconfig=Development \
  -build -cook -stage -pak -iostore -archive -archivedirectory="$OUT" -utf8output -nocompileeditor \
  '-UbtArgs=-MaxParallelActions=4 -NoUBA' '-AdditionalCookerOptions=-SkipZenStore' 2>&1 | tee "$BUILD_LOG"
# UAT can return success while replacing broken cooked materials with the default.
# Such a package is unusable for either visual comparison or PSO recording.
if grep -nE 'Failed to compile Material|Default Material will be used in game' "$BUILD_LOG"; then
  echo "Cooked material compilation failed; package rejected. Log: $BUILD_LOG" >&2
  exit 1
fi
# The 3D models' credits with their links (CC BY 4.0 asks for them), beside the game (Tools/freemodels/credits.py).
cp "$(dirname "$P")/CREDITS.txt" "$OUT/Linux/CREDITS.txt"

# Fail at packaging time if the release entry point or split payload was omitted.
# Keep Unreal's proven launcher unchanged; ship independent, read-only diagnostics.
LINUX_PACKAGE="$OUT/Linux"
for required in Signal47.sh Signal47/Binaries/Linux/Signal47; do
  [[ -s "$LINUX_PACKAGE/$required" ]] || { echo "Package rejected: missing $LINUX_PACKAGE/$required" >&2; exit 1; }
done
shopt -s nullglob
PAKS=("$LINUX_PACKAGE"/Signal47/Content/Paks/*.pak)
UTOCS=("$LINUX_PACKAGE"/Signal47/Content/Paks/*.utoc)
UCAS=("$LINUX_PACKAGE"/Signal47/Content/Paks/*.ucas)
((${#PAKS[@]} && ${#UTOCS[@]} && ${#UCAS[@]})) || { echo "Package rejected: incomplete PAK/UTOC/UCAS payload in $LINUX_PACKAGE" >&2; exit 1; }
chmod +x "$LINUX_PACKAGE/Signal47.sh" "$LINUX_PACKAGE/Signal47/Binaries/Linux/Signal47"
install -m 755 "$(dirname "$P")/Build/Linux/launch/Check-Linux.sh" "$LINUX_PACKAGE/Check-Linux.sh"
install -m 644 "$(dirname "$P")/Build/Linux/launch/README.md" "$LINUX_PACKAGE/LINUX-README.md"
