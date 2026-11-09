#!/bin/bash
# runall.sh blendscript set root -- blender, then import
cd "$(dirname "$0")"
Q=~/.claude/skills/signal47-nattskift/scripts/q.py
python3 $Q python3 run_blender.py "$1" 2>&1 | tail -6 | cut -c1-220
python3 $Q python3 run_import.py "$2" "$3" 2>&1 | tail -6 | cut -c1-220
