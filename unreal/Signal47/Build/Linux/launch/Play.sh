#!/usr/bin/env bash
set -euo pipefail
APP_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
game_args=(-S47Title)
descriptor=${S47_VULKAN_DESCRIPTORS:-auto}
while (($#)); do
    case "$1" in
        --descriptor-buffer) descriptor=buffer ;;
        --descriptor-heap) descriptor=heap ;;
        --uten-rt) game_args+=('-ini:Engine:[/Script/Engine.RendererSettings]:r.RayTracing=False,[/Script/Engine.RendererSettings]:r.Lumen.HardwareRayTracing=False') ;;
        --x11) export SDL_VIDEO_DRIVER=x11 SDL_VIDEODRIVER=x11; game_args+=(-sdlvideodriver=x11) ;;
        --wayland) export SDL_VIDEO_DRIVER=wayland SDL_VIDEODRIVER=wayland; game_args+=(-sdlvideodriver=wayland) ;;
        --hjelp|--help|-h) printf 'Bruk: ./Play.sh [--x11 | --wayland] [--descriptor-buffer | --descriptor-heap] [--uten-rt] [spillargumenter]\nPå Wayland krever --x11 XWayland og DISPLAY. Prøv først uten tvungen backend.\n'; exit 0 ;;
        *) game_args+=("$1") ;;
    esac
    shift
done
if [[ ${SDL_VIDEO_DRIVER:-} == x11 && -z ${DISPLAY:-} ]]; then
    printf 'DISPLAY mangler. Start XWayland via skrivebordet, eller prøv uten --x11 / med --wayland.\n' >&2
    exit 2
fi
# RTX 50 / NVIDIA 615 startup workaround; keep the tested heap path on RTX 30.
# /proc needs no utility. nvidia-smi is only a bounded fallback.
is_rtx50() {
    local info model=""
    for info in /proc/driver/nvidia/gpus/*/information; do
        [[ ! -r $info ]] || model+=$(cat "$info")
    done
    if [[ -z $model ]] && command -v nvidia-smi >/dev/null && command -v timeout >/dev/null; then
        model=$(timeout 3s nvidia-smi --query-gpu=name --format=csv,noheader 2>/dev/null) || model=""
    fi
    [[ $model =~ RTX[[:space:]]+50[0-9][0-9] ]]
}
if [[ $descriptor == auto ]] && is_rtx50; then descriptor=buffer; fi
case "$descriptor" in
    buffer) game_args+=('-ini:Engine:[ConsoleVariables]:r.Vulkan.Bindless.PreferredExtension=0') ;;
    heap) game_args+=('-ini:Engine:[ConsoleVariables]:r.Vulkan.Bindless.PreferredExtension=1') ;;
    auto) ;;
    *) echo 'S47_VULKAN_DESCRIPTORS må være auto, buffer eller heap.' >&2; exit 2 ;;
esac
printf 'Vulkan descriptors: %s; SDL: %s\n' "$descriptor" "${SDL_VIDEO_DRIVER:-auto}"
UCAS="$APP_DIR/Linux/Signal47/Content/Paks/Signal47-Linux.ucas"
expected_hash=""
if [[ -f $APP_DIR/UCAS.sha256 ]]; then
    read -r expected_hash _ < "$APP_DIR/UCAS.sha256"
    [[ $expected_hash =~ ^[0-9a-f]{64}$ ]] || { echo 'Ugyldig UCAS.sha256.' >&2; exit 2; }
fi
if [[ ! -f $UCAS ]] || [[ -n $expected_hash && $(sha256sum < "$UCAS" | cut -d' ' -f1) != $expected_hash ]]; then
    if [[ -n $expected_hash && -f $UCAS.part1 && -f $UCAS.part2 ]]; then
        mkdir "$UCAS.join-lock" 2>/dev/null || { printf 'En sammensetting kjører allerede. Ved avbrutt jobb: kontroller at ingen Play.sh kjører før du fjerner .join-lock.\n' >&2; exit 1; }
        trap 'rm -f -- "$UCAS.tmp"; rmdir -- "$UCAS.join-lock"' EXIT
        echo 'Setter sammen spilldataene (bare første gang) ...'
        cat "$UCAS.part1" "$UCAS.part2" > "$UCAS.tmp"
        if [[ $(sha256sum < "$UCAS.tmp" | cut -d' ' -f1) != $expected_hash ]]; then
            echo 'Spilldataene stemmer ikke. Last ned og pakk ut Data1 og Data2 på nytt.' >&2
            exit 1
        fi
        mv "$UCAS.tmp" "$UCAS"
        rm -f -- "$UCAS.part1" "$UCAS.part2"
        rmdir -- "$UCAS.join-lock"
        trap - EXIT
    else
        printf 'Pakk også ut Data1.zip og Data2.zip fra samme release til samme mappe som hovedarkivet.\n' >&2
        exit 1
    fi
fi
cd "$APP_DIR/Linux"
exec ./Signal47.sh "${game_args[@]}"
