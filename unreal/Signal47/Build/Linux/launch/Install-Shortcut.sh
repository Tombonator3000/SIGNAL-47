#!/usr/bin/env bash
# Install only a launcher for this extracted package; no sudo or game-data copy.
set -euo pipefail
app=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd -P)
[[ -f $app/Play.sh ]] || { echo 'Play.sh mangler. Pakk ut hovedarkivet først.' >&2; exit 2; }
[[ $app != *$'\n'* && $app != *$'\r'* ]] || { echo 'Flytt spillet til en sti uten linjeskift.' >&2; exit 2; }
# Desktop Entry string escaping, then Exec's double-quoted argument escaping.
value() { local s=$1; s=${s//\\/\\\\}; printf '%s' "$s"; }
argument() {
  local s=$1
  s=${s//\\/\\\\}; s=${s//\"/\\\"}; s=${s//\$/\\\$}; s=${s//\`/\\\`}; s=${s//%/%%}
  value "\"$s\""
}
applications=${XDG_DATA_HOME:-$HOME/.local/share}/applications
mkdir -p -- "$applications"
# One entry per extraction path; never replace another package's shortcut.
id=$(printf '%s' "$app" | sha256sum); id=${id:0:12}
entry="$applications/signal47-$id.desktop"
tmp=$(mktemp "$applications/.signal47.XXXXXXXX")
trap 'rm -f -- "$tmp"' EXIT
{
  printf '[Desktop Entry]\nType=Application\nName=SIGNAL / 47\nComment=New Mexico, 1986. Night shift at SARO.\n'
  printf 'Exec=/bin/bash %s\n' "$(argument "$app/Play.sh")"
  printf 'Path=%s\n' "$(value "$app")"
  if [[ -f $app/icon.png ]]; then printf 'Icon=%s\n' "$(value "$app/icon.png")"; else printf 'Icon=applications-games\n'; fi
  printf 'Terminal=false\nCategories=Game;AdventureGame;\nStartupNotify=true\n'
} > "$tmp"
chmod 755 "$tmp"
# Atomic no-replace creation. The temporary link is removed by the EXIT trap.
if ! ln -- "$tmp" "$entry" 2>/dev/null; then
  if [[ -L $entry || ! -f $entry ]] || ! cmp -s -- "$tmp" "$entry"; then
    printf 'Eksisterende snarvei er endret eller er en lenke; bevares: %s\n' "$entry" >&2
    exit 2
  fi
fi
printf 'Snarvei installert i programmenyen: %s\n' "$entry"
if command -v xdg-user-dir >/dev/null; then
  desktop=$(xdg-user-dir DESKTOP)
  if [[ -d $desktop && $desktop != "$HOME" ]]; then
    desktop_entry="$desktop/$(basename "$entry")"
    if [[ -L $desktop_entry ]]; then
      printf 'Eksisterende skrivebordslenke bevares: %s\n' "$desktop_entry"
      exit 0
    fi
    cp -nT -- "$entry" "$desktop_entry"
    if ! cmp -s -- "$entry" "$desktop_entry"; then
      printf 'Eksisterende personlig skrivebordssnarvei bevares: %s\n' "$desktop_entry"
      exit 0
    fi
    printf 'Skrivebord: %s (skrivebordet kan kreve «Tillat oppstart»).\n' "$desktop/$(basename "$entry")"
  fi
fi
printf 'Behold spillmappen her. Etter flytting: kjør denne installasjonen på nytt.\n'
