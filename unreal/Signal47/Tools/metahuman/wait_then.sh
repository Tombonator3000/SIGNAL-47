#!/bin/bash
# Wait politely for the machine: Codex's current turn has ended (task_complete is the last turn
# marker in its main session) and no heavy Unreal job has run for three consecutive minutes.
# Then run the given script. Tom's own game counts as a heavy job, so his play is never disturbed.
set -uo pipefail
CODEX="$HOME/.codex/sessions/2026/10/26/rollout-2026-10-26T13-38-04-01a17837-cbb2-7a03-9f07-374f984d752a.jsonl"
GUARD="$HOME/SIGNAL-47/unreal/Signal47/Scripts/unreal_guard.py"
idle=0
while true; do
  last=$(grep -a -o '"type":"task_\(started\|complete\)"' "$CODEX" | tail -1)
  if [[ "$last" == *task_complete* ]] && python3 "$GUARD" require-idle >/dev/null 2>&1; then
    idle=$((idle+1))
  else
    idle=0
  fi
  [ $idle -ge 3 ] && break
  sleep 60
done
echo "machine free at $(date +%H:%M:%S)"
exec "$@"
