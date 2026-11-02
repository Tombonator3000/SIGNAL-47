# Linux-pakkens start og diagnostikk

`package-linux.sh` legger `Play.sh`, `Play-Low.sh` og `Install-Shortcut.sh` over `Linux/`. Start med `bash Play.sh`. Kjør `bash Install-Shortcut.sh` for snarvei i programmenyen og, når skrivebordsmappen finnes, på skrivebordet. Behold spillmappen på samme sted; snarveien peker dit. Ingen sudo brukes. `Check-Linux.sh` og denne teksten ligger ved motorstarteren i `Linux/`.

Den publiserte nedlastingsstarteren heter `Spill-SIGNAL47.sh`; `Signal47.sh` ligger inni den utpakkede hovedpakken. GitHubs automatiske **Source code**-arkiv inneholder ikke et ferdig spill. Alle hoved-/dataarkiver må pakkes ut i samme mappe.

Krav: x86-64 Linux med glibc, `libgcc_s.so.1`, Vulkan-loader og en maskinvaredriver som støtter GPU-en, tilgjengelig Wayland eller X11/XWayland, samt fungerende SDL-lydbackend. Motoren inkluderer noen biblioteker, men ikke vertens GPU-driver. Bash/coreutils og xargs/readlink kreves for motorstarteren; curl, unzip og sha256sum brukes av nedlastingsstarteren. `vulkaninfo`, `nvidia-smi` og `ldd` er valgfrie diagnoseverktøy, ikke krav om å installere alle alternative backender. Ingen Ubuntu-, root- eller editoravhengighet for spilleren.

Kjør fra pakken, uten å starte motoren:

```sh
bash Check-Linux.sh > linux-preflight.txt 2>&1
printf 'exit=%s\n' "$?"
```

Utviklerkopien kan kontrollere en eksisterende pakke med `bash Check-Linux.sh /absolutt/pakke/Linux`. Exit 2 betyr en konkret manglende fil, ELF-avhengighet eller nødvendig skjermøkt. Valgfrie kommandoer har åtte sekunders grense og rapporterer sin egen exitkode; manglende verktøy eller Vulkan-feil skal ikke tolkes som grafisk aksept.

Standard skjermbackend beholdes. Ved en konkret SDL-/presentasjonsfeil kan én backend prøves eksplisitt, dersom den finnes i økten:

```sh
SDL_VIDEO_DRIVER=wayland SDL_VIDEODRIVER=wayland bash Signal47.sh -sdlvideodriver=wayland -windowed -ResX=1280 -ResY=720
SDL_VIDEO_DRIVER=x11 SDL_VIDEODRIVER=x11 bash Signal47.sh -sdlvideodriver=x11 -windowed -ResX=1280 -ResY=720
```

På niri krever X11-sporet faktisk XWayland-integrasjon og gyldig DISPLAY. Ikke start en ekstra X-server eller endre driveren uten feildata. Lav kvalitet eller deaktivert ray tracing er separate, eksplisitte avgrensninger, ikke automatisk standard for en bestemt distribusjon eller GPU.

Ved feil: behold første terminalfeil og exitkode, `linux-preflight.txt`, arkivnavn/utpakkingsmappe og den relevante spilloggen under `Signal47/Saved/Logs` (eventuelt valgt UserDir). En preflight er ikke en GPU-/spilletest. RTX 5090 / NVIDIA 615.78.08 / CachyOS / niri fikk fungerende tittelbilde med `r.Vulkan.Bindless.PreferredExtension=0`; X11 alene og deaktivert RT løste ikke feilen. Dette bekrefter oppstart, ikke hele spilløkten. `Play.sh` og nedlastingsstarteren velger derfor descriptor_buffer automatisk når RTX 50-serien identifiseres i NVIDIA-driverens GPU-navn. Andre GPU-er beholder motorens standard. På hybridmaskiner eller ved regresjon kan `--descriptor-buffer` / `--descriptor-heap` velges uttrykkelig, eller miljøvariabelen `S47_VULKAN_DESCRIPTORS=buffer|heap|auto`. Direkte kjøring av `Linux/Signal47.sh` omgår disse valgene; bruk `Play.sh`.

Driver-/ABI-referanse: [Epics Linux-krav for UE 5.8](https://dev.epicgames.com/documentation/en-us/unreal-engine/linux-development-requirements-for-unreal-engine) oppgir glibc 2.28+ og Nvidia 570+. Driveren må i tillegg støtte det konkrete kortet; dette er ikke en gjennomført minimumstest av vår pakke.
