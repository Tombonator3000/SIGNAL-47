# Komplett nedlasting for Linux-testere uten Git

**Status:** Referanse. **Gjelder:** Linux-testutgave 29. oktober 2026, Farm R1 gjennom PR223. **Sist kontrollert:** 29. oktober 2026.

[Testutgaven](https://github.com/Tombonator3000/SIGNAL-47/releases/tag/playtest-linux-2026-10-29) distribuerer den samme Farm R1-runtime som er [installert og kontrollert lokalt](INSTALL_LAUNCHER_2026-10-29.md). Spillere trenger ikke Git, repo, Unreal-editor eller separate Artifacts. Den nye [startfilen](https://github.com/Tombonator3000/SIGNAL-47/releases/download/playtest-linux-2026-10-29/Spill-SIGNAL47.sh) kan lastes ned alene til en skrivbar mappe og kjøres med `bash Spill-SIGNAL47.sh`. Den henter og kontrollerer alle spillarkivene. Kilde, installasjon og offentlig nedlasting føres separat i [STATUS](../../Active/STATUS.md).

## Innhold og bevaring

Pakken inkluderer sheriff, kirke, vaskeri, fôrbutikk, kino, fire bolighus, gård og tidligere leveranser, samt kveld/natt, SC01, SC02 og meteor. Senere HUM01/ART01-endringer inngår ikke. Spillet er fortsatt under utvikling; hele SC03 til SC13 er ikke ferdig. Nøyaktig runtimekilde er c301ead9313510125d037a4cc020d667cd9c5661, runtimeidentisk med main094a67cb gjennom PR223. Git-taggens dokumentcommit endrer ikke den angitte runtimekilden.

Tre ZIP-er på til sammen omtrent 4 GB er nødvendige på grunn av GitHubs vedleggsgrense. Spillpakken og begge dataarkivene lastes og pakkes ut automatisk av startfilen; ved manuell installasjon pakkes alle tre ut til samme mappe. Play.sh kontrollerer forventet UCAS-hash også hvis en eldre datafil finnes. Sammensetting bruker lås, midlertidig fil, hashkontroll og atomisk bytte; gammel fil og deler beholdes ved kontrollfeil. Starteren markerer nedlasting ferdig først etter at alle arkivene er pakket ut, slik at avbrutt utpakking kan gjenopptas. Gamle Saved flyttes eller slettes ikke automatisk. Pakk normalt opp i en ny datert mappe og ta kopi av egen Saved før overføring.

Ingen av Toms Saved, innstillinger, logger eller shadercache inngår. Debugsymboler og debugmanifest utelates; spillbinær og øvrige runtimefiler er identiske med kandidaten. Kreditering og vilkår følger pakken. Midlertidige arkiver lages i egen /tmp-mappe på grunn av lav plass på hjemmedisken; kilder, installasjon og rollback endres ikke.

## Wayland og XWayland

`--x11` velger X11-backenden. Under Wayland skjer dette normalt gjennom XWayland, som må kjøre og sette DISPLAY. Et helt Xorg-skrivebord er ikke nødvendig. Prøv vanlig oppstart først, eventuelt `--wayland` hvis XWayland mangler. Faktisk oppstart på mottakerens compositor/GPU er ikke prøvd. Se [Waylands beskrivelse av XWayland](https://wayland.freedesktop.org/faq) og [SDLs Wayland-dokumentasjon](https://github.com/libsdl-org/SDL/blob/main/docs/README-wayland.md).

Lokal UE5.8.4-kilde i LinuxPlatformApplicationMisc.cpp prioriterer RenderOffScreen, så `-sdlvideodriver=`, miljønavn og INI/auto. Starteren setter begge SDL_VIDEO_DRIVER/SDL_VIDEODRIVER og det eksplisitte UE-argumentet; offscreen brukes ikke til spillerstart. Manglende DISPLAY ved tvunget X11 gir en forklaring før nedlasting eller oppstart. Ingen pakke installeres automatisk på spillerens system.

## Kontrollgrunnlag

[Portabel distribusjonskvittering](DOWNLOAD_LINUX_2026-10-29.json) binder arkivhasher, opplastede vedlegg og anonyme HTTP-kontroller. Alle 44 arkivoppføringer er lest/dekomprimert med CRC og filhash; de to UCAS-delene er strømmet i rekkefølge og gir nøyaktig den testede originalhashen. Filsett og kjørerettigheter er kontrollert, ingen personlig datafil inngår.

Tolv små launchertester bruker isolerte filer og falsk spillprosess: fersk join, gammel UCAS, korrupt join, lås, beholdt Saved, title/argumenter, begge SDL-navn, X11/DISPLAY og Wayland. Åtte nedlastingskontroller bruker ekte curl/unzip/sha256sum mot små lokale file-URL-fixtures, der bare base-URL er byttet: selvstendig fil uten Git, avbrutt utpakking, alle tre arkiver, reparasjon av korrupt cache, ferdigmarkør, gammel Saved, trygg cacheopprydding og ny start uten server. Dette er ikke grafisk kompatibilitetsbevis eller en ny full gjennomspilling.

Farm R1s tidligere 111/111, 1720 selvtester, pakket meteor18+18 og installert grafikk47+15/Fortsett beholdes med sine grenser. Byteidentisk runtime krever ingen ny motorserie. Kjent RT-residentadvarsel, første historiske startup-OOM og E04-klump er fortsatt åpne; menneskelig input, lytting, FPS og andre maskiner er UNVERIFIED. Ingen av andres motorprosesser er stoppet eller startet i dette distribusjonspasset.
