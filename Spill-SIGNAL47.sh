#!/usr/bin/env bash
# Starter SIGNAL / 47 for Linux. Første gang lastes den ferdige testutgaven ned
# fra GitHub, kontrolleres med SHA256 og pakkes ut ved siden av dette skriptet.
# Kan også lastes ned alene, uten Git eller repo. Andre maskiner er ikke
# verifisert; systembiblioteker og en fungerende Vulkan-driver kreves.
# Kjør med --hjelp for valg og feilsøking.
set -euo pipefail

tag='playtest-linux-2026-10-29'
base="https://github.com/Tombonator3000/SIGNAL-47/releases/download/$tag"
archives=(Signal47-Linux-2026-10-29.zip Signal47-Linux-2026-10-29-Data1.zip Signal47-Linux-2026-10-29-Data2.zip)

repo_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
game_dir="$repo_dir/Spill/$tag"
play="$game_dir/Signal47-Linux/Play.sh"
download_complete="$game_dir/.download-complete"
log_file="$game_dir/Signal47-Linux/Linux/Signal47/Saved/Logs/Signal47.log"

usage() {
  cat <<'EOF'
Bruk: ./Spill-SIGNAL47.sh [valg] [-- ekstra argumenter til spillet]

  --lav          Low-grafikk i et vindu på 1280 x 720 (svakere maskiner)
  --uten-rt      slår av strålesporing (prøv ved svart skjerm eller blinking)
  --x11          tving X11/XWayland i stedet for Wayland
  --wayland      tving Wayland
  --fullskjerm   start i fullskjerm
  --vindu BxH    start i vindu med gitt størrelse, for eksempel --vindu 1920x1080
  --pakke MAPPE  bruk en allerede utpakket Signal47-Linux- eller Linux-mappe; ingen nedlasting
  --sjekk        sjekk bare systemet, uten å starte spillet
  --rapport      skriv en feilrapport med systeminfo og siste spillogg, og avslutt
  --hjelp        vis dette

Ved skjermfeil: behold terminalfeilen og prøv én tilgjengelig backend:
  ./Spill-SIGNAL47.sh --wayland --vindu 1280x720
  ./Spill-SIGNAL47.sh --x11 --vindu 1280x720
X11 krever en fungerende DISPLAY/XWayland-økt. Ingen backend tvinges som standard.
Virker ingenting, kjør ./Spill-SIGNAL47.sh --rapport og send filen den lager.
EOF
}

game_args=()
check_only=0
report_only=0
low=0
package_override=""
while (($#)); do
  case "$1" in
    --pakke) [[ -n ${2:-} ]] || { echo "--pakke trenger en mappe" >&2; exit 2; }; package_override="$2"; shift ;;
    --lav) low=1 ;;
    --uten-rt) game_args+=('-ini:Engine:[/Script/Engine.RendererSettings]:r.RayTracing=False,[/Script/Engine.RendererSettings]:r.Lumen.HardwareRayTracing=False') ;;
    --x11) export SDL_VIDEO_DRIVER=x11 SDL_VIDEODRIVER=x11; game_args+=(-sdlvideodriver=x11) ;;
    --wayland) export SDL_VIDEO_DRIVER=wayland SDL_VIDEODRIVER=wayland; game_args+=(-sdlvideodriver=wayland) ;;
    --fullskjerm) game_args+=(-fullscreen) ;;
    --vindu)
      [[ ${2:-} =~ ^([0-9]+)x([0-9]+)$ ]] || { printf 'Bruk --vindu BxH, for eksempel --vindu 1920x1080\n' >&2; exit 2; }
      game_args+=(-windowed "-ResX=${BASH_REMATCH[1]}" "-ResY=${BASH_REMATCH[2]}"); shift ;;
    --sjekk) check_only=1 ;;
    --rapport) report_only=1 ;;
    --hjelp|-h|--help) usage; exit 0 ;;
    --) shift; game_args+=("$@"); break ;;
    *) game_args+=("$1") ;;
  esac
  shift
done

# Resolve only explicit known package layouts. Never search the home directory or
# redownload over an existing manually extracted game and its Saved folder.
linux_root="$game_dir/Signal47-Linux/Linux"
if [[ -n $package_override ]]; then
  package_override="$(cd -- "$package_override" 2>/dev/null && pwd -P)" || { echo 'Pakkemappen finnes ikke.' >&2; exit 2; }
  if [[ -f $package_override/Signal47.sh ]]; then linux_root="$package_override"
  elif [[ -d $package_override/Linux ]]; then linux_root="$package_override/Linux"
  elif [[ -d $package_override/Signal47-Linux/Linux ]]; then linux_root="$package_override/Signal47-Linux/Linux"
  else echo 'Fant ikke Linux/Signal47.sh-oppsettet. Velg den utpakkede spillmappen, ikke Source code-arkivet.' >&2; exit 2; fi
  play="$linux_root/Signal47.sh"
  # Preserve the supplied low-quality wrapper if this distribution includes it.
  if ((low)); then
    [[ -f $linux_root/../Play-Low.sh ]] || { echo 'Denne pakken mangler Play-Low.sh. Start uten --lav og velg kvalitet i Settings.' >&2; exit 2; }
    play="$linux_root/../Play-Low.sh"
  fi
fi
log_file="$linux_root/Signal47/Saved/Logs/Signal47.log"
terminal_log="$repo_dir/Spill/siste-oppstart.txt"

# ---------------------------------------------------------------- systemsjekk
warnings=0
problems=0
note() { printf '  %s\n' "$*"; }
problem() { printf '  FEIL: %s\n' "$*"; problems=$((problems + 1)); }
warn() { printf '  ADVARSEL: %s\n' "$*"; warnings=$((warnings + 1)); }

gpu_vendors() {
  local v
  for v in /sys/class/drm/card[0-9]*/device/vendor; do
    [[ -r $v ]] || continue
    case "$(cat "$v")" in
      0x10de) echo nvidia ;; 0x1002) echo amd ;; 0x8086) echo intel ;; *) echo annen ;;
    esac
  done | sort -u
}

vulkan_icds() {
  local d
  for d in /usr/share/vulkan/icd.d /etc/vulkan/icd.d /usr/local/share/vulkan/icd.d \
           /usr/lib/x86_64-linux-gnu/GL/vulkan/icd.d; do
    [[ -d $d ]] && ls "$d"/*.json 2>/dev/null || true
  done
}

system_check() {
  printf 'Systemsjekk for SIGNAL / 47:\n'
  local arch; arch="$(uname -m)"
  [[ $arch == x86_64 ]] && note "Prosessor: x86-64" || warn "Spillet krever x86-64, denne maskinen er $arch."

  local distro=ukjent
  [[ -r /etc/os-release ]] && distro="$(. /etc/os-release; printf '%s' "${PRETTY_NAME:-${NAME:-ukjent}}")"
  note "Distribusjon: $distro, kjerne $(uname -r)"

  local mem_kb; mem_kb="$(awk '/MemTotal/ {print $2}' /proc/meminfo 2>/dev/null || echo 0)"
  ((mem_kb >= 15000000)) && note "Minne: $((mem_kb / 1024 / 1024)) GB" || warn "Bare $((mem_kb / 1024 / 1024)) GB minne. 16 GB anbefales; bruk --lav."

  local vendors; vendors="$(gpu_vendors | tr '\n' ' ' || true)"
  note "Skjermkort: ${vendors:-ikke funnet}"
  local icds; icds="$(vulkan_icds | sort -u)"
  if [[ -z $icds ]]; then
    warn "Fant ingen Vulkan-driver. Installer driveren for skjermkortet ditt:"
    note "  Arch: vulkan-radeon (AMD), vulkan-intel (Intel) eller nvidia-utils (Nvidia)"
    note "  Fedora: mesa-vulkan-drivers, eller akmod-nvidia fra RPM Fusion"
    note "  Debian/Ubuntu: mesa-vulkan-drivers, eller nvidia-driver-XXX"
    note "  openSUSE: libvulkan_radeon / libvulkan_intel, eller nvidia-gl-G06"
  else
    note "Vulkan-drivere: $(printf '%s\n' "$icds" | xargs -n1 basename | tr '\n' ' ')"
  fi

  if [[ $vendors == *nvidia* ]]; then
    local ver=""
    [[ -r /sys/module/nvidia/version ]] && ver="$(cat /sys/module/nvidia/version)"
    if [[ -n $ver ]]; then
      note "Nvidia-driver: $ver"
      ((${ver%%.*} >= 570)) || warn "Nvidia-driver $ver er gammel. UE 5.8 oppgir 570 eller nyere; driveren må også støtte ditt konkrete kort."
    else
      warn "Nvidia-kort, men den proprietære/åpne nvidia-driveren er ikke lastet (nouveau støttes ikke)."
    fi
    [[ $icds == *nvidia* ]] || warn "Nvidia-kort uten Nvidias Vulkan-driver (nvidia_icd.json). Installer nvidia-utils eller tilsvarende."
    local modeset; modeset="$(cat /sys/module/nvidia_drm/parameters/modeset 2>/dev/null || echo ?)"
    # Ofte bare lesbar for root; advar bare når den faktisk er av.
    if [[ ${XDG_SESSION_TYPE:-} == wayland && $modeset == N ]]; then
      warn "Wayland med Nvidia krever kjerneparameteren nvidia_drm.modeset=1 (nå: $modeset)."
    fi
  fi

  local session="${XDG_SESSION_TYPE:-ukjent}" desktop="${XDG_CURRENT_DESKTOP:-ukjent}"
  note "Økt: $session, skrivebord: $desktop, SDL_VIDEO_DRIVER=${SDL_VIDEO_DRIVER:-${SDL_VIDEODRIVER:-auto}}"
  if [[ $session == wayland || -n ${WAYLAND_DISPLAY:-} ]]; then
    if [[ -z ${DISPLAY:-} ]]; then
      note "DISPLAY er tom. Prøv uten --x11, eventuelt --wayland. For --x11 trengs XWayland,"
      note "på niri for eksempel pakken xwayland-satellite."
    fi
    if [[ ${desktop,,} == *niri* ]]; then
      note "niri flislegger vinduer. Gir spillet et smalt eller høyt vindu, bruk --fullskjerm, eller en"
      note "vindusregel i niri: window-rule { match title=\"(?i)signal ?/? ?47\"; open-fullscreen true; }"
    fi
  fi

  if [[ ! -x $play ]]; then
    local free_kb; free_kb="$(df -Pk "$repo_dir" | awk 'NR==2 {print $4}')"
    ((free_kb >= 10000000)) || warn "Nedlasting og utpakking trenger om lag 10 GB ledig plass, du har $((free_kb / 1024 / 1024)) GB."
  fi
  if [[ -n $package_override || -e $download_complete || -f $play ]]; then
    local internal="$linux_root/Signal47.sh"
    local binary="$linux_root/Signal47/Binaries/Linux/Signal47"
    [[ -x $play ]] || problem "Startfil mangler eller kan ikke kjøres: $play. Kontroller hovedarkivet og kjørerettighetene."
    [[ -f $internal ]] || problem "Internstarter mangler: $internal. Pakk ut hovedarkivet, ikke Source code, og alle dataarkiver i samme mappe."
    [[ -f $binary ]] || problem "Spillbinær mangler: $binary. Installasjonen er ufullstendig."
  else
    note "Ferdig spillpakke er ikke lastet ned ennå; internstarteren er Linux/Signal47.sh inne i hovedarkivet."
  fi
  printf '%d konkret(e) feil, %d advarsel(er). Dette er en forhåndssjekk, ikke en grafisk Vulkan-test.\n' "$problems" "$warnings"
}

write_report() {
  local out; out="$repo_dir/Spill/rapport-$(date +%Y%m%d-%H%M%S).txt"
  mkdir -p "$repo_dir/Spill"
  {
    printf 'SIGNAL / 47 feilrapport %s, utgave %s\n\n' "$(date -Is)" "$tag"
    system_check
    printf '\n--- miljø\n'
    env | grep -E '^(XDG_SESSION_TYPE|XDG_CURRENT_DESKTOP|WAYLAND_DISPLAY|DISPLAY|LANG|SDL_[[:alnum:]_]*|VK_[[:alnum:]_]*|__GL_[[:alnum:]_]*|__NV_[[:alnum:]_]*)=' || true
    printf '\n--- skjermkort\n'
    command -v lspci >/dev/null && lspci -nnk | grep -A3 -Ei 'vga|3d|display' || printf 'lspci mangler\n'
    printf '\n--- vulkaninfo --summary\n'
    if command -v vulkaninfo >/dev/null && command -v timeout >/dev/null; then
      local vk_status=0
      timeout 8s vulkaninfo --summary 2>&1 | awk 'NR<=80 {print}' || vk_status=$?
      printf 'Vulkan-diagnose exit=%d (124 betyr tidsavbrudd).\n' "$vk_status"
    else printf 'Valgfri vulkaninfo/timeout mangler; grafikkdriveren er ikke testet.\n'; fi
    local preflight="$linux_root/Check-Linux.sh"
    if [[ -f $preflight ]]; then
      printf '\n--- pakket Linux-preflight\n'
      local check_status=0
      bash "$preflight" || check_status=$?
      printf 'Pakket preflight exit=%d\n' "$check_status"
    fi
    printf '\n--- siste terminaloppstart (pakke, første feil og exitkode)\n'
    if [[ -r $terminal_log ]]; then
      head -n 80 "$terminal_log"
      printf '\n--- slutten av terminaloppstarten\n'
      tail -n 40 "$terminal_log"
    else printf 'Ingen terminaloppstart er registrert av denne starteren.\n'; fi
    printf '\n--- feil og advarsler i spillogg %s\n'  "$log_file"
    if [[ -r $log_file ]]; then
      grep -Ei 'error|fatal|vulkan|swapchain|device lost|sdl|wayland|x11|raytracing|crash' "$log_file" | tail -n 120 || true
      printf '\n--- siste 150 linjer av spilloggen\n'
      tail -n 150 "$log_file"
    else
      printf 'Fant ingen spillogg. Start spillet én gang først.\n'
    fi
  } >"$out" 2>&1
  printf 'Feilrapport skrevet til %s\nSend den filen til Tom.\n' "$out"
}

if ((report_only)); then write_report; exit 0; fi
system_check
if ((check_only)); then
  preflight="$linux_root/Check-Linux.sh"
  if [[ -f $preflight ]]; then bash "$preflight" || exit $?; fi
  ((problems==0)) || exit 2
  exit 0
fi
((problems==0)) || { printf 'Start avbrutt: se den konkrete pakkefeilen over. Saved er ikke endret.\n' >&2; exit 2; }
if [[ ${SDL_VIDEO_DRIVER:-} == x11 && -z ${DISPLAY:-} ]]; then
  printf 'Kan ikke bruke --x11 uten DISPLAY. Start XWayland via skrivebordet, eller prøv uten --x11 / med --wayland.\n' >&2
  exit 2
fi

# ---------------------------------------------------------------- nedlasting
if [[ -z $package_override ]] && [[ ! -x "$play" || ! -f "$download_complete" ]]; then
  for tool in curl unzip sha256sum; do
    command -v "$tool" >/dev/null || { printf 'Mangler %s. Installer det og prøv igjen.\n' "$tool" >&2; exit 1; }
  done
  printf 'Laster ned SIGNAL / 47 (%s), om lag 3,9 GB. Det skjer bare første gang.\n' "$tag"
  for old in "$repo_dir"/Spill/playtest-linux-*; do
    [[ -d "$old" && "$old" != "$game_dir" ]] && printf 'Eldre utgave i %s kan slettes. Flytt Linux/Signal47/Saved derfra hvis du vil beholde lagringene.\n' "$old"
  done
  mkdir -p "$game_dir/nedlasting"
  cd "$game_dir/nedlasting"
  curl -fL --retry 3 -o SHA256SUMS.txt "$base/SHA256SUMS.txt"
  for name in "${archives[@]}"; do
    # Hopp over et arkiv som allerede er lastet ned og stemmer.
    if ! grep " $name\$" SHA256SUMS.txt | sha256sum -c --status >/dev/null 2>&1; then
      if ! curl -fL --retry 3 -C - -o "$name" "$base/$name" ||
         ! grep " $name\$" SHA256SUMS.txt | sha256sum -c --status >/dev/null 2>&1; then
        # Bare denne nedlastingens kjente cachefil slettes, aldri spill/Saved.
        rm -f -- "$name"
        curl -fL --retry 3 -o "$name" "$base/$name"
      fi
    fi
  done
  sha256sum -c SHA256SUMS.txt
  for name in "${archives[@]}"; do
    unzip -q -o "$name" -d "$game_dir"
  done
  cd "$game_dir/Signal47-Linux"
  for required in Play.sh Play-Low.sh Linux/Signal47.sh Linux/Signal47/Binaries/Linux/Signal47; do
    [[ -s $required ]] || { printf 'Ufullstendig utpakking: %s mangler i %s. Behold arkivene og pakk ut alle tre i samme mappe.\n' "$required" "$PWD" >&2; exit 2; }
  done
  chmod +x Play.sh Play-Low.sh Linux/Signal47.sh Linux/Signal47/Binaries/Linux/Signal47
  printf '%s\n' "$tag" > "$download_complete"
  rm -rf "$game_dir/nedlasting"
  printf 'Ferdig. Spillet ligger i %s\n' "$game_dir/Signal47-Linux"
fi

if ((low)) && [[ -z $package_override ]]; then play="$game_dir/Signal47-Linux/Play-Low.sh"; fi
printf 'Spilloggen skrives til %s\n' "$log_file"
mkdir -p "$repo_dir/Spill"
printf 'Oppstart %s\nPakke: %s\nStarter: %s\n' "$(date -Is)" "$linux_root" "$play" > "$terminal_log"
# Retain early loader/SDL errors even when Unreal never creates its own log.
# Keep the game's exit status, not tee's, and do not retry another backend silently.
set +e
"$play" "${game_args[@]}" 2>&1 | tee -a "$terminal_log"
launch_status=${PIPESTATUS[0]}
set -e
printf 'Spillprosess exit=%d\n' "$launch_status" | tee -a "$terminal_log"
if ((launch_status)); then
  printf 'Oppstarten feilet. Terminalfeil er lagret i %s\nKjør samme starter med --rapport (og samme --pakke hvis brukt).\n' "$terminal_log" >&2
fi
exit "$launch_status"
