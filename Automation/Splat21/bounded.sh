#!/usr/bin/env bash
# Linux user cgroup boundary. Refuse to run if limits cannot be installed/verified.
set -euo pipefail
memory=${1:?memory bytes}; seconds=${2:?maximum seconds}; shift 2
[[ "$memory" =~ ^[1-9][0-9]+$ && "$seconds" =~ ^[1-9][0-9]*$ && $# -gt 0 ]] || exit 2
signal47_env=()
for signal47_var in DISPLAY XDG_RUNTIME_DIR XAUTHORITY; do
  if [[ -v "$signal47_var" ]]; then signal47_env+=("--setenv=$signal47_var=${!signal47_var}"); fi
done
exec systemd-run --user --wait --pipe --collect --quiet --working-directory="$PWD" \
  -p "MemoryMax=$memory" -p MemorySwapMax=0 -p "RuntimeMaxSec=$seconds" \
  -p TimeoutStopSec=5 -p KillMode=control-group -p OOMPolicy=kill \
  "${signal47_env[@]}" /bin/bash -c '
    set -euo pipefail
    group=$(sed -n "s/^0:://p" /proc/self/cgroup)
    [[ -n "$group" && "$(cat "/sys/fs/cgroup$group/memory.max")" == "$1" && "$(cat "/sys/fs/cgroup$group/memory.swap.max")" == 0 ]] || exit 125
    shift
    exec "$@"
  ' signal47-limited "$memory" "$@"
