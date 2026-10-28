#!/usr/bin/env bash
# Starter SIGNAL / 47 for Linux. Første gang lastes den ferdige testutgaven ned
# fra GitHub, kontrolleres med SHA256 og pakkes ut ved siden av dette skriptet.
set -euo pipefail

tag='playtest-linux-2026-10-27'
base="https://github.com/Tombonator3000/SIGNAL-47/releases/download/$tag"
archives=(Signal47-Linux-2026-10-27.zip Signal47-Linux-2026-10-27-Data1.zip Signal47-Linux-2026-10-27-Data2.zip)

repo_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
game_dir="$repo_dir/Spill/$tag"
play="$game_dir/Signal47-Linux/Play.sh"

if [[ ! -x "$play" ]]; then
  for tool in curl unzip sha256sum; do
    command -v "$tool" >/dev/null || { printf 'Mangler %s. Installer det og prøv igjen.\n' "$tool" >&2; exit 1; }
  done
  printf 'Laster ned SIGNAL / 47 (%s), om lag 3,7 GB. Det skjer bare første gang.\n' "$tag"
  mkdir -p "$game_dir/nedlasting"
  cd "$game_dir/nedlasting"
  curl -fL --retry 3 -o SHA256SUMS.txt "$base/SHA256SUMS.txt"
  for name in "${archives[@]}"; do
    # Hopp over et arkiv som allerede er lastet ned og stemmer.
    if ! grep " $name\$" SHA256SUMS.txt | sha256sum -c --status >/dev/null 2>&1; then
      curl -fL --retry 3 -C - -o "$name" "$base/$name"
    fi
  done
  sha256sum -c SHA256SUMS.txt
  for name in "${archives[@]}"; do
    unzip -q -o "$name" -d "$game_dir"
  done
  cd "$game_dir/Signal47-Linux"
  chmod +x Play.sh Play-Low.sh Linux/Signal47.sh Linux/Signal47/Binaries/Linux/Signal47
  rm -rf "$game_dir/nedlasting"
  printf 'Ferdig. Spillet ligger i %s\n' "$game_dir/Signal47-Linux"
fi

exec "$play" "$@"
