#!/usr/bin/env bash
# Read-only diagnostics for the trusted, unpacked Linux game. Never starts Unreal.
set -uo pipefail
here="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
# Installed beside Linux/Signal47.sh; the source copy accepts an explicit package root.
root="${1:-$here}"
root="$(cd -- "$root" 2>/dev/null && pwd -P)" || { echo 'Package directory does not exist.' >&2; exit 2; }
errors=0
fail() { printf 'ERROR: %s\n' "$*"; errors=$((errors+1)); }
info() { printf '%s\n' "$*"; }
bounded() {
  local result
  printf '\n--- %s\n' "$*"
  if ! command -v "$1" >/dev/null 2>&1; then info 'Optional diagnostic tool unavailable.'; return; fi
  if ! command -v timeout >/dev/null 2>&1; then info 'timeout unavailable; optional command skipped.'; return; fi
  timeout 8s "$@" 2>&1 | awk 'NR<=100 {print}'
  result=${PIPESTATUS[0]}
  printf 'Diagnostic exit: %d%s\n' "$result" "$([[ $result == 124 ]] && printf ' (8 second timeout)')"
  if [[ $1 == vulkaninfo ]] && ((result)); then fail "Vulkan device probe failed with exit $result; retain its first error above."; fi
}
info 'SIGNAL / 47 Linux preflight (no game or graphics test)'
info "Package: $root"
info "Architecture: $(uname -m); kernel: $(uname -r)"
[[ $(uname -m) == x86_64 ]] || fail 'This package requires x86-64 Linux with glibc.'
if [[ -r /etc/os-release ]]; then (. /etc/os-release; printf 'Distribution: %s\n' "${PRETTY_NAME:-unknown}"); fi
binary="$root/Signal47/Binaries/Linux/Signal47"
for file in "$root/Signal47.sh" "$binary"; do
  [[ -f $file ]] || { fail "Missing $file. Extract the main game archive, not GitHub Source code, with all data parts into the same directory."; continue; }
  [[ -x $file ]] || fail "Not executable: $file. Check archive permissions and the filesystem's executable mount policy."
done
shopt -s nullglob
paks=("$root"/Signal47/Content/Paks/*.pak)
utocs=("$root"/Signal47/Content/Paks/*.utoc)
ucas=("$root"/Signal47/Content/Paks/*.ucas)
((${#paks[@]} && ${#utocs[@]} && ${#ucas[@]})) || fail 'Missing PAK/UTOC/UCAS data. Extract every release data archive alongside the main package.'
if [[ -f $binary ]]; then
  # ldd is used only for this trusted packaged game, never for downloaded unknown executables.
  if command -v ldd >/dev/null && command -v timeout >/dev/null; then
    libs="$(timeout 8s ldd "$binary" 2>&1)"; result=$?
    printf '\n--- Direct ELF dependencies (does not cover SDL/Vulkan dynamic loading)\n%s\n' "$libs"
    if ((result)); then fail "ELF loader check failed with exit $result (124 means timeout)."; fi
    if [[ $libs == *'not found'* ]]; then fail 'Required shared libraries are missing; see exact sonames above.'; fi
  else info 'ldd/timeout unavailable; ELF resolution not checked.'; fi
fi
info "Session: ${XDG_SESSION_TYPE:-unknown}; desktop: ${XDG_CURRENT_DESKTOP:-unknown}"
info "DISPLAY=${DISPLAY:-unset}; WAYLAND_DISPLAY=${WAYLAND_DISPLAY:-unset}"
backend="${SDL_VIDEO_DRIVER:-${SDL_VIDEODRIVER:-auto}}"
info "SDL backend: $backend (auto preserves the desktop default)"
if [[ $backend == x11 && -z ${DISPLAY:-} ]]; then fail 'Explicit X11 backend requires a working X11/XWayland DISPLAY.'; fi
if [[ -n ${WAYLAND_DISPLAY:-} ]]; then
  socket="$WAYLAND_DISPLAY"; [[ $socket == /* ]] || socket="${XDG_RUNTIME_DIR:-/run/user/$UID}/$socket"
  if [[ -S $socket ]]; then info "Wayland socket exists: $socket (server/device access still untested)."
  elif [[ $backend == wayland ]]; then fail "Requested Wayland socket is unavailable: $socket."
  else info "Wayland socket unavailable: $socket; desktop backend selection may need attention."; fi
elif [[ $backend == wayland ]]; then fail 'Explicit Wayland backend requires WAYLAND_DISPLAY and access to its socket.'; fi
if [[ -z ${DISPLAY:-} && -z ${WAYLAND_DISPLAY:-} ]]; then fail 'No graphical session advertised. Run from the desktop session; do not invent DISPLAY=:0.'; fi
bounded getconf GNU_LIBC_VERSION
bounded vulkaninfo --summary
bounded nvidia-smi --query-gpu=name,driver_version,memory.total --format=csv,noheader
info ''
info 'Dependencies: Bash/coreutils; x86-64 glibc, libgcc_s; Vulkan loader and a hardware driver supporting your GPU; an accessible Wayland or X11/XWayland session; an SDL-supported audio service/backend.'
info 'Package names differ by distribution. Missing optional tools are not missing game dependencies. No packages, drivers or system settings were changed.'
info "Detected blocking errors: $errors. A successful preflight does not prove a visible Vulkan game window."
((errors==0)) || exit 2
