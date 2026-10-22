# SIGNAL / 47 i Unreal (prøve)

En prøve av prologen «Night Shift» i Unreal Engine 5.8.4, med plass til hele spillet hvis prøven holder. Nettversjonen i `web/` er fortsatt spillet som publiseres; dette er en parallell utgave for desktop, bygget på det samme grunnlaget: Blender-modellene i `Unity/Assets/Signal47/Art`, lyden og historien.

Startet 21. oktober 2026 etter Toms spørsmål om en Unreal-versjon. Første prøve: kontrollrommet og gården om natta, med gang i førsteperson og statusterminalen.

## Kom i gang

Unreal Engine 5.8.4 for Linux må ligge i `~/.local/opt/UnrealEngine-5.8.4` (eller `UE_ROOT`). Samme oppsett som THE BLACK MAP. Den private klonen av assets må ligge i `web/src/assets/licensed` (som for nettversjonen), og venv-en `~/.venvs/signal47` med Playwright må finnes.

```sh
cd unreal/Signal47
Scripts/build-editor.sh        # kompilerer editor-målet (ikke mens editoren er åpen)
Tools/webexport/import.sh      # bygger nettversjonen, eksporterer scenen fra nettleseren og importerer den (rundt 25 min)
Scripts/import-audio.sh        # all lyd og musikk til Content/Audio og Content/Licensed
Scripts/package-linux.sh       # pakket build til ~/scratch-bm/Signal47-build
Scripts/screenshots.sh UT      # 14 kjøringer av builden med skjermbilder, sjekker loggen
~/scratch-bm/Signal47-build/Linux/Signal47.sh   # spill (-S47Picture=off uten VHS-bildet)
```

Spillet starter på tittelen (Continue, New night, Load case, Settings); med et `-S47`-flagg går det rett i spill som før. WASD går (Shift løper), musa ser, E eller klikk bruker det du ser på. Tab eller N er notatboken (M, I og H åpner kartet, kitet og kontrollene i den), Esc eller P er pausen med innstillinger, lagring og lasting. Lagringene og innstillingene ligger i `Saved/SaveGames`.

## Slik kommer nettversjonen inn

Verden bygges ikke for hånd. `Tools/webexport/export.py` starter nettversjonen i Chromium, gjør klar alle områdene og henter hele three.js-scenen ut med `export.js`: hver node med matrise og synlighet, all geometri som glb, hvert materiale med farger, kart og UV-matriser, alle teksturer, lysene, de falske flomlysene (`kit.ts floodlit`), glødepunktene, dørene med åpen og lukket stilling, antennene, alt som kan brukes, gangsonene og kolliderne per område, og himmelen per område som et ekvirektangulært bilde (8192 x 4096). `Tools/webexport/import_web.py` gjør det om til Unreal-assets under `/Game/Web` (ignorert av Git) og kopierer `manifest.json` til `Content/WebData`.

Ved oppstart leser `AS47Web` manifestet og bygger scenen node for node på de samme stedene. Bildet regnes som i nettleseren: Lumen er av, lysene har de samme enhetene (candela), halvkule-lyset og flomlysene ligger i materialene med three.js sine formler, og `M_WebTone` erstatter Unreals tonekurve med three.js sin tåke (FogExp2), ACES Filmic med eksponering 1,05, sRGB og VHS-båndet fra `core/vhs.ts`.

Spillogikken bruker nettversjonens koordinater (meter, y opp); bare tegningen regnes om. three (x, y, z) er Unreal (100x, 100z, 100y).

## Det som finnes

- `Source/Signal47`: C++-modulen.
  - `AS47Web` er nettversjonens verden, bygget fra eksporten, med setterne historien trenger (spaken, lampene, CRT-ene, skriveren, telefonrøret, kruset, kaffen, klokka, lysrørene, antennene, dørene).
  - `S47Three` regner mellom three.js og Unreal (matriser, Euler-vinkler, kamera, farger).
  - `AS47Story` er prologen «Night Shift», portert fra `web/src/story/Prologue.ts`: de samme fasene i samme rekkefølge på spilltid som stopper når noe leses, og de samme plasseringene.
  - `S47Docs`, `S47Ops` og `S47Rx` er dokumentene, operasjonsterminalen og mottakeren med spekteret, portert ord for ord og tall for tall og sjekket mot nettversjonen (`-S47SelfTest`).
  - `AS47HUD` og `SS47Widgets` er overlegget, papirene, notatboken, mottakerkonsollen, operasjonsterminalen og sluttkortet i Slate. `SS47Crt` tegner de fire CRT-ene til render targets.
  - `US47Audio` er `core/Audio.ts`: opptakene ved navn og alle lydene nettversjonen lager med Web Audio (bærebølgen, signalet, telefonlinja, radioene, tordenen, natta ute), portert graf for graf på en liten Web Audio i C++ (`S47WebAudio`, `S47SignalVoice`, `S47Ambience`), sjekket mot Chromiums egen gjengivelse.
  - `AS47Walker` er spilleren fra `player/Player.ts`: gangsonene og kolliderne, øyehøyde, gange, løp og blikk som i nettversjonen.
- `Scripts/import-audio.sh`: all lyd (frie lyder, kjøpte lyder, musikk, radio, stemmebankene) med ffmpeg til `Content/Audio` og `Content/Licensed` (begge ignorert av Git), pluss tonene fra `Scripts/make-synth.py`. Først skriver `Scripts/audio-raw.py` opptakene lydmotoren dekoder selv, med oppslagstabellene, til `Content/Audio/Raw` og `Content/Licensed/Raw` (bare python3 og ffmpeg, ingen editor).
- `Content/Fonts`: VT323 (OFL) og Special Elite (Apache 2.0) som i nettversjonen, med lisensene, og JetBrains Mono (OFL) som reserve.

## Regler

Se `AGENTS.md` i denne mappa. Kort: Unity-mappa er arkiv og røres ikke, kjøpte eller lisensierte assets legges ikke i det offentlige repoet, spilltekst på engelsk, dokumenter på norsk, og alt logges i `log.md`.

## Begrensninger

- Prologen er spillbar fra start til sluttkortet og kan lagres fra 02:13; kapittel 1 til 6 og kjøringen er ikke portert ennå. Verdenene deres (stasjon 01, veien, dineren, rom 6, Roswell) er med i eksporten.
- Ni shader-materialer fra nettversjonen (vann, enkelte himmeleffekter) er skjult. Himmelen tegnes levende (`S47Sky`: kuppelen, stjernene, stjernehullet, stjerneskudd, kometen og den malte horisonten); bildet fra eksporten brukes bare når livsmaterialene mangler.
- Museretningen opp og ned er ikke prøvd av et menneske.
- Bare testet på denne maskinen (RTX 3080, Vulkan, Ubuntu 26.04).
