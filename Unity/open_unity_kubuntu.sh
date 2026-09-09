#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")" && pwd)"
CANDIDATES=(
  "$HOME/Unity/Hub/Editor"/6000.3*/Editor/Unity
  "$HOME/Unity/Hub/Editor"/6000.*/Editor/Unity
  "/opt/Unity/Hub/Editor"/6000.3*/Editor/Unity
)
for u in "${CANDIDATES[@]}"; do
  if [[ -x "$u" ]]; then exec "$u" -projectPath "$ROOT"; fi
done
echo "Fant ikke Unity Editor automatisk. Legg mappen til i Unity Hub og åpne med Unity 6.3 LTS."
exit 1
