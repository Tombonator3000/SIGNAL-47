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

- `Source/Signal47`: C++-modulen. `AS47Saro` bygger stasjonen ved oppstart (rommet på 12 x 9 m med vinduet mot nord som i nettversjonen, gården, antennene, himmelen og lysene). `AS47Walker` er førsteperson. `AS47HUD` og `SS47Widgets` er overlegget og terminalen i Slate.
- `Scripts/import-models.py`: leser FBX-filene fra `Unity/Assets/Signal47/Art` (uten å endre noe der) og fargene fra Unity-materialfilene, og lager `Content/Models` og `Content/Materials`.
- `Content/Fonts`: JetBrains Mono (OFL) som reserve til VT323 er konvertert fra woff2.
- Alt på kartet av stasjonen er i cm med X mot nord, Y mot øst og Z opp. Nettversjonens (x, y, z) er (Y, Z, -X).

## Regler

Se `AGENTS.md` i denne mappa. Kort: Unity-mappa er arkiv og røres ikke, kjøpte eller lisensierte assets legges ikke i det offentlige repoet, spilltekst på engelsk, dokumenter på norsk, og alt logges i `log.md`.

## Begrensninger

- Prøven er en gråboks med de originale modellene, ikke hele prologen. Ingen historie, lyd, lagring eller feltkamera ennå.
- Modellenes retning og skala kommer fra FBX-filene; feilvendte modeller rettes i `S47Saro.cpp`.
- Museretningen opp og ned er ikke prøvd av et menneske.
- Bare testet på denne maskinen (RTX 3080, Vulkan, Ubuntu 26.04).
