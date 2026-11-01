# Linux: oppstart, avhengigheter og feilsøking

**Status:** Gjeldende veiledning, ekstern kompatibilitet under arbeid. **Gjelder:** Linux x86-64/Vulkan; offentlig Farm R1 fra 29. oktober og videre distribusjonsarbeid. **Sist kontrollert:** 31. oktober 2026.

Målet er vanlig Linux, med eksplisitte ABI-/grafikkrav og avhengigheter, ikke Ubuntu som eneste spillerplattform. Et utviklingsmiljø på Ubuntu er ikke et krav om Ubuntu hos spilleren. Arch/niri/Nvidia er ennå ikke verifisert. [STATUS](../STATUS.md) skiller offentlig utgave fra nyere lokal installasjon; [TODO](../TODO.md) fører åpne feil og videre pakking.

## Riktig startfil

Last ned [Spill-SIGNAL47.sh](https://github.com/Tombonator3000/SIGNAL-47/releases/download/playtest-linux-2026-10-29/Spill-SIGNAL47.sh) fra [testutgaven](https://github.com/Tombonator3000/SIGNAL-47/releases/tag/playtest-linux-2026-10-29) til en skrivbar mappe. Kjør fra denne mappen:

```sh
bash Spill-SIGNAL47.sh --sjekk
bash Spill-SIGNAL47.sh
```

Starteren laster ned og kontrollerer alle tre ZIP-arkivene. GitHubs automatiske «Source code»-arkiv er kildekode, ikke det ferdige spillet. Ved manuell nedlasting må hovedarkivet, Data1 og Data2 pakkes ut til samme mappe. Start deretter `bash Play.sh` inne i `Signal47-Linux`.

`Signal47.sh` er den interne motorstarteren i `Signal47-Linux/Linux/`, ikke navnet på releasevedlegget man skal lete etter. Dette er kontrollert direkte i den publiserte hoved-ZIP-ens katalog og ukomprimerte filinnhold 31. oktober, uten å hente hele spillpakken: internstarteren har SHA256 `d274178785a20058ba9d8f365096d5406c367eebe81d9458814eda5e8152d04a`, lik `BUILD.json` og `FILES.sha256`. Offentlig `Spill-SIGNAL47.sh` er også hentet og sammenlignet med repokilden, SHA256 `a20dc64ec720a0da35bb7089fd696ec7647038099dc4f6a2a973660cdd759724`. Dette beviser tilstedeværelse, ikke grafisk oppstart på en annen PC.

## Ny starter med eksisterende pakke og terminalfeil

Repokildens starter støtter nå `--pakke` for en allerede utpakket `Signal47-Linux`- eller `Linux`-mappe. Dette laster ikke ned eller pakker ut på nytt og endrer ikke Saved. Den offentlige releasefilen oppdateres ikke automatisk når repokilden endres.

```sh
bash Spill-SIGNAL47.sh --pakke "/sti/til/Signal47-Linux" --vindu 1280x720
bash Spill-SIGNAL47.sh --pakke "/sti/til/Signal47-Linux" --rapport
```

Terminalutdata og prosessens exitkode lagres i `Spill/siste-oppstart.txt` ved siden av starteren og tas med i rapporten. Pakkeplasseringen står i loggen, slik at en feil fra en annen installasjon kan oppdages. Del rapporten og navnet på det nedlastede arkivet; ikke bare skjermkortmodellen. Manglende internstarter avvises med konkret sti, uten å laste ned over en delvis installasjon. Standard skjermbackend beholdes.

Avgrenset shellprøve med testpakker dekker mellomrom i sti, begge pakkeoppsett, videresending av vindusargumenter, manglende internstarter og bevart feiltekst/exitkode37. Dette beviser ikke grafisk start på Arch/niri/RTX5090; mottakerens faktiske feilmelding mangler fortsatt.

## Krav og avhengigheter

| Del | Krav eller innhold | Kontrollgrense |
| --- | --- | --- |
| Plattform | 64-bit x86 Linux med glibc, kjørbart filsystem og skrivbar installasjons-/lagringsmappe. Ingen Unreal-editor, Git eller Proton kreves. | ARM og musl-only er ikke levert. |
| Nedlasting | Bash, `curl`, `unzip`, `sha256sum` og vanlige coreutils/awk/grep. Internstarteren bruker også `xargs` og `readlink`. Omtrent 4 GB nedlasting og minst 12 GB ledig til utpakking/sammensetting. | Disse er vertsverktøy for ZIP-distribusjonen. |
| Grunnbiblioteker | glibc-familien (`libc.so.6`, `libm.so.6`, `libpthread.so.0`, `libdl.so.2`, `librt.so.1`, `ld-linux-x86-64.so.2`) og `libgcc_s.so.1`. | Statisk ELF-lesing av lokal PR272-binær 31. oktober viser disse direkte NEEDED-avhengighetene. Farm R1-binærens fulle ABI-/symbolkrav er ikke kontrollert på nytt. |
| ABI-grunnlag | Epic oppgir glibc 2.28+ for motoren og anbefaler 2.35+ for raskere oppstart. | Dette er motorens referanse, ikke en fullført minimumskravtest av vår release. Kontroller faktisk binær og alle medfølgende DSO-er ved neste pakke. |
| Grafikk | Fungerende Vulkan-loader, maskinvare-ICD og driver som støtter det pakkede shader-/funksjonsnivået. Epic oppgir Nvidia 570+ for UE5.8; bruk en driver som også støtter det aktuelle kortet. | RTX 5090-navnet alene beviser ikke at driver/ICD/SDL/presentasjon fungerer. Starterens gamle «550+»-råd er for svakt som UE5.8-krav. |
| Skjermøkt | Enten fungerende native Wayland eller X11/XWayland. På niri krever X11-sporet XWayland-integrasjon og gyldig `DISPLAY`. | En miljøvariabel alene beviser ikke kontakt med serveren. Ikke sett `DISPLAY=:0` vilkårlig. |
| Lyd/input | Fungerende lydtjeneste/backend og øktens vindus-/inputbiblioteker. | SDL og andre deler kan laste biblioteker dynamisk; bare ELF/ldd er ikke en full runtime-test. Ikke krev alle alternative lyd-/vindusbackender samtidig. |
| Medfølgende filer | Spillbinær, PAK/UTOC/UCAS, modeller, teksturer og lyd. Offentlig `FILES.sha256` fører også Vulkan-loader/-lag, MsQuic og ONNX-biblioteker. | Driverens maskinvare-ICD følger ikke automatisk med disse filene. Installer ikke Vulkan-utviklingsverktøy som krav for vanlig spilling. |
| Feilsøking | `vulkaninfo` fra Vulkan-verktøy, `nvidia-smi` for Nvidia og eventuelt `ldd` for en betrodd lokal spillbinær. | Diagnostikkverktøyene er valgfrie, ikke nye spillavhengigheter. |

Pakkenavn varierer mellom distribusjoner. [Arch dokumenterer loader og GPU-driver separat](https://wiki.archlinux.org/title/Vulkan). På Arch er blant annet `glibc`, `gcc-libs`, `vulkan-icd-loader` og `nvidia-utils` relevante navn; AMD/Intel bruker andre ICD-er. Ingen automatisk `apt install`, systemendring eller driverinstallasjon skal utføres av spillstarteren. Nøyaktig støttet GPU-/RAM-/ytelsesminimum er fortsatt ikke målt på tvers av maskiner.

## Arch, niri og Nvidia: avgrens grafikkfeilen

Tom oppgir RTX 5090, Arch Linux, niri og DMS/Quickshell hos mottakeren. Driver-, niri- og DMS-versjon, nøyaktig feilmelding og logg mangler. DMS er ikke påvist som årsak.

Prøv først vanlig oppstart fra terminal. Hvis den feiler, behold terminalfeilen og prøv ett skjermspor om gangen:

```sh
# Native Wayland, avgrenset vindu:
bash Spill-SIGNAL47.sh --wayland --vindu 1280x720

# Alternativ etter at XWayland faktisk er tilgjengelig og DISPLAY er satt:
bash Spill-SIGNAL47.sh --x11 --vindu 1280x720
```

Dette er diagnostiske alternativer, ikke bekreftet løsning på mottakermaskinen. `--uten-rt` kan deretter avgrense ray tracing; det er ikke et generelt krav for et RTX 5090. `--lav` er en separat ytelsesprøve. Ikke bytt til `-nullrhi`, dedicated server eller headless for å få en «bestått» spillstart uten grafikk.

Ifølge [niris integrasjonsdokumentasjon](https://niri-wm.github.io/niri/Integrating-niri.html) støtter nyere niri automatisk `xwayland-satellite` når riktig versjon er tilgjengelig. Kontroller oppsettet mot installert niri-versjon fremfor å legge til en ekstra manuell XWayland-instans. Niri har også [egen Nvidia-dokumentasjon](https://niri-wm.github.io/niri/Nvidia.html), men ikke bruk globale driver-/compositorjusteringer uten at den konkrete feilen samsvarer.

```sh
vulkaninfo --summary
nvidia-smi
bash Spill-SIGNAL47.sh --rapport
```

Rapporten legges under `Spill/rapport-<tid>.txt`. Ta vare på terminalutdata i tillegg, særlig hvis spillet stopper før det lager `Signal47.log`. Dersom `--rapport` avbrytes eller mangler avslutningsmeldingen, behold den delvis skrevne filen og terminalfeilen. Send først opplysninger om feilen, driver/økt og relevant logg; ikke hele hjemmemappen eller alle miljøvariabler.

Dagens `--sjekk` viser arkitektur, distribusjon, RAM, ICD-filnavn, Nvidia-modul og økt. En funnet ICD-JSON og meldingen «Ingen problemer funnet» beviser ikke en vellykket Vulkan-enhet, korrekt biblioteklasting eller et synlig spillvindu. Videre arbeid i PLAY-31-38 skal gi tydelige feil for manglende filer/biblioteker, feil ABI, utilgjengelig skjermbackend og faktisk Vulkan-feil, uten å endre maskinen. Oppstartsdiagnostikk må ha timeout og beholde første feil og exitkode.

## Flatpak som mulig tillegg

Flatpak kan gi et fast, distribuert brukermiljø gjennom en delt runtime og pakkede ekstra biblioteker. Det er et godt kandidatformat for å redusere forskjeller mellom distribusjoner. Det er **ikke implementert eller publisert** for SIGNAL /47.

En Flatpak bruker fortsatt vertens kjerne, GPU og skjermøkt. Nvidia krever riktig grafikkutvidelse for vertens driver; sandboxen løser ikke automatisk compositor-/driverfeil. Et konkret forsøk må velge støttet Freedesktop-runtime, inkludere manglende biblioteker, avklare Vulkan/Wayland/X11/lyd/gamepad-tilgang og holde Saved/konfigurasjon i et skrivbart appområde. Ikke gi tilgang til hele hjemmemappen for å få det til å starte. Prøv grafisk oppstart, lyd, input og lagring på Arch/niri/Nvidia og et annet Linux-oppsett før dette tilbys som verifisert alternativ. Ikke krev en ny motorport eller erstatt dagens pakke med en uprøvd Flatpak.

Kilder: [Epics Linux-krav og fast sysroot](https://dev.epicgames.com/documentation/en-us/unreal-engine/linux-development-requirements-for-unreal-engine), [Flatpak-runtimer og sandbox](https://docs.flatpak.org/en/latest/basic-concepts.html), [Flatpaks grafikkutvidelser](https://docs.flatpak.org/en/latest/extension.html). Prosjektbiblioteket ble søkt først; ingen relevant verifisert Unreal/Flatpak-løsning ble funnet. Ingen motor-/spilltest eller systeminstallasjon ble kjørt i denne kartleggingen.
