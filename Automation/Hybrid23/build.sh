#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
signal47_hybrid_project=$(realpath "${1:?isolated project}")
signal47_hybrid_log=${2:?new build log path}
[[ ! -e "$signal47_hybrid_log" ]] || { echo 'Preserve earlier build logs; use a fresh path.' >&2; exit 2; }
mkdir -p "$(dirname "$signal47_hybrid_log")"
exec 9>"${XDG_RUNTIME_DIR:-/run/user/$(id -u)}/signal47-gauntlet.lock"
flock -n 9 || { echo 'Another SIGNAL47 graphical task is active.' >&2; exit 1; }
signal47_hybrid_editor=${UNITY_EDITOR:-/home/tombonator3000t/Unity/Hub/Editor/6000.3.22f1/Editor/Unity}
bash Automation/Splat21/bounded.sh 4294967296 600 "$signal47_hybrid_editor" -job-worker-count 2 -batchmode -nographics -quit -projectPath "$signal47_hybrid_project" -executeMethod Hybrid23Build.Build -logFile "$(realpath -m "$signal47_hybrid_log")"
python3 Automation/Hybrid23/identity.py "$signal47_hybrid_project" "${signal47_hybrid_log%.log}-identity.json"
