#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p Artifacts/WorldCapture
rm -f Artifacts/WorldCapture/*.png Artifacts/WorldCapture/capture-manifest.json Artifacts/world-capture.log
PLAYER="$PWD/Artifacts/Linux/Signal47.x86_64"
[[ -x "$PLAYER" ]] || { echo "WORLD_CAPTURE_UNAVAILABLE missing player: $PLAYER"; exit 2; }

run_player() {
  "$PLAYER" --signal47-world-capture -screen-fullscreen 0 -screen-width 1280 -screen-height 800 -logFile "$PWD/Artifacts/world-capture.log"
}

# Prefer a live X display when the runner service can authenticate to one. This gives us the
# installed Mesa/Intel path. Otherwise fall back to Xvfb/software rendering for visual review only.
if [[ -n "${DISPLAY:-}" ]] && command -v xdpyinfo >/dev/null 2>&1 && xdpyinfo >/dev/null 2>&1; then
  echo "WORLD_CAPTURE_DISPLAY live:$DISPLAY"
  run_player
else
  live_ok=0
  if command -v xdpyinfo >/dev/null 2>&1; then
    for sock in /tmp/.X11-unix/X*; do
      [[ -e "$sock" ]] || continue
      d=":${sock##*X}"
      for auth in "$HOME/.Xauthority" /run/user/"$(id -u)"/*xauth* /run/user/"$(id -u)"/xauth_*; do
        [[ -f "$auth" ]] || continue
        if DISPLAY="$d" XAUTHORITY="$auth" xdpyinfo >/dev/null 2>&1; then
          export DISPLAY="$d" XAUTHORITY="$auth"
          echo "WORLD_CAPTURE_DISPLAY discovered:$DISPLAY auth:$XAUTHORITY"
          live_ok=1
          break 2
        fi
      done
    done
  fi
  if [[ "$live_ok" == 1 ]]; then
    run_player
  elif command -v xvfb-run >/dev/null 2>&1; then
    echo "WORLD_CAPTURE_DISPLAY xvfb-software"
    xvfb-run -a -s "-screen 0 1280x800x24" env LIBGL_ALWAYS_SOFTWARE=1 "$PLAYER" --signal47-world-capture -screen-fullscreen 0 -screen-width 1280 -screen-height 800 -logFile "$PWD/Artifacts/world-capture.log"
  else
    echo "WORLD_CAPTURE_UNAVAILABLE no authenticated X display and xvfb-run missing"
    echo "X sockets:"; ls -la /tmp/.X11-unix 2>/dev/null || true
    echo "Runtime dir:"; find /run/user/"$(id -u)" -maxdepth 2 -type f -printf '%p\n' 2>/dev/null | head -80 || true
    exit 3
  fi
fi

for f in 01-control-room-array.png 02-service-yard-array.png 03-sierra-motor-court-blockout.png capture-manifest.json; do
  [[ -s "Artifacts/WorldCapture/$f" ]] || { echo "WORLD_CAPTURE_FAIL missing $f"; tail -120 Artifacts/world-capture.log || true; exit 4; }
done
grep -q 'WORLD_CAPTURE_PASS' Artifacts/world-capture.log || { echo "WORLD_CAPTURE_FAIL pass marker missing"; exit 5; }
echo "WORLD_CAPTURE_FILES"
ls -lh Artifacts/WorldCapture
