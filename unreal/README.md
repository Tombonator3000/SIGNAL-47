# SIGNAL / 47 i Unreal (prøve)

En prøve av prologen «Night Shift» i Unreal Engine 5.8.4, med plass til hele spillet hvis prøven holder. Nettversjonen i `web/` er fortsatt spillet som publiseres; dette er en parallell utgave for desktop, bygget på det samme grunnlaget: Blender-modellene i `Unity/Assets/Signal47/Art`, lyden og historien.

Startet 21. oktober 2026 etter Toms spørsmål om en Unreal-versjon. Første prøve: kontrollrommet og gården om natta, med gang i førsteperson og statusterminalen.

## Kom i gang

Unreal Engine 5.8.4 for Linux må ligge i `~/.local/opt/UnrealEngine-5.8.4` (eller `UE_ROOT`). Samme oppsett som THE BLACK MAP.

```sh
cd unreal/Signal47
Scripts/build-editor.sh        # kompilerer editor-målet (ikke mens editoren er åpen)
Scripts/import-models.sh       # importerer modellene fra Unity-arkivet til Content/Models og lager materialene
Scripts/package-linux.sh       # pakket build til ~/scratch-bm/Signal47-build (rundt 3 min)
Scripts/screenshots.sh UT      # seks kjøringer av builden med skjermbilder, sjekker loggen
~/scratch-bm/Signal47-build/Linux/Signal47.sh   # spill
```

WASD går, musa ser, E bruker det du ser på (terminalen, telefonen, dørene). Esc lukker terminalen.

## Det som finnes

- `Source/Signal47`: C++-modulen.
  - `AS47Saro` bygger stasjonen ved oppstart: rommet på 12 x 9 m med vinduet mot nord som i nettversjonen, pulten med tre CRT-er, racket med spak og 18 lamper, skriveren, telefonen, kruset, papirene, operasjonspulten, gården, gjerdet, de 27 antennene fra `arrayLayout.ts`, himmelen og lysene. Alt historien rører, har en id (`Use:<id>`) og settere.
  - `AS47Story` er prologen «Night Shift», portert fra `web/src/story/Prologue.ts`: de samme fasene i samme rekkefølge (intro, shift, survey, skip, residual, locked, solving, printing, printed, ringing, call, countdown, event, turning, end) på spilltid som stopper når noe leses.
  - `S47Docs`, `S47Ops` og `S47Rx` er dokumentene, operasjonsterminalen og mottakeren med spekteret, portert ord for ord og tall for tall og sjekket mot nettversjonen (`-S47SelfTest`).
  - `AS47HUD` og `SS47Widgets` er overlegget (sikte, ledetekst, klokke, meldinger, overtoning), papirene, notatboken (Tab), mottakerkonsollen, operasjonsterminalen og sluttkortet i Slate. `SS47Crt` tegner de fire CRT-ene i rommet til render targets.
  - `US47Audio` spiller lydene ved navn fra `/Game/Licensed/Sfx`, `/Game/Audio/Sfx` og `/Game/Audio/Synth`, og driver bærebølgen, signalet og motorene som løkker.
  - `AS47Walker` er førsteperson: WASD, mus, E eller klikk bruker det du ser på.
- `Scripts/import-models.py`: FBX-filene fra `Unity/Assets/Signal47/Art` (uendret der) og fargene fra Unity-materialfilene til `Content/Models` og `Content/Materials`.
- `Scripts/import-licensed.py`: de kjøpte modellene fra den private klonen i `web/src/assets/licensed/models` til `Content/Licensed` (ignorert av Git).
- `Scripts/import-audio.sh`: all lyd (frie lyder, kjøpte lyder, musikk, radio) konvertert med ffmpeg og importert til `Content/Audio` og `Content/Licensed` (begge ignorert av Git), pluss tonene fra `Scripts/make-synth.py`.
- `Content/Fonts`: JetBrains Mono (OFL) som reserve til VT323 er konvertert fra woff2.
- Alt i stasjonen er i cm med X mot nord, Y mot øst og Z opp. Nettversjonens (x, y, z) i meter er (Y, Z, -X) / 100.

## Regler

Se `AGENTS.md` i denne mappa. Kort: Unity-mappa er arkiv og røres ikke, kjøpte eller lisensierte assets legges ikke i det offentlige repoet, spilltekst på engelsk, dokumenter på norsk, og alt logges i `log.md`.

## Begrensninger

- Prologen er spillbar fra start til sluttkortet; kapittel 1 til 6 er ikke portert ennå.
- Ingen lagring ennå, ingen kameraristing ved smellet, og krusets skår tegnes ikke (det forsvinner med lyden).
- Rommet er en gråboks der de kjøpte modellene og arkivets modeller står på enkle flater.
- Modellenes retning og skala kommer fra FBX-filene; feilvendte modeller rettes i `S47Saro.cpp`.
- Museretningen opp og ned er ikke prøvd av et menneske.
- Bare testet på denne maskinen (RTX 3080, Vulkan, Ubuntu 26.04).
