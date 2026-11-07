# Gårdsbyggene på Alderete-gården

**Status:** Kilde bygd og kontrollert i Blender. Importskript og spillkode er skrevet, men ikke kjørt: ikke importert i Unreal, ikke kompilert, ikke sett i spillet. **Gjelder:** FARM-BUILDINGS-20261106-05, låve, hus og to skjul. **Sist kontrollert:** 7. november 2026.

Originale prosedyrale `bpy`-modeller, uten nedlastede modeller og uten uttrekk. Mesh-klossene gjenbruker `../substation/meshkit.py` på samme måte som `../townbuildings`. Binærfiler ligger privat i `~/Documents/ChatGPT/Signal 47/modeller/farmbuildings-2026-11-07/` (`fbx/`, `blend/`, `textures/`, `renders/`, `milepæler/`, `referanser/`), aldri i Git.

## Filer

| Fil | Innhold |
|---|---|
| `spec.py` | Bestilte mål og trekantbudsjett, fastsatt før bygging. |
| `fbkit.py` | Vegg med ekte hull, tak i plater, bokser, bjelker, skittstripe mot bakken. |
| `barn.py`, `house.py`, `sheds.py` | Selve byggene. `build(lod)` og `collision()`. |
| `textures.py` | Egne kart, 1024 px, sømløse. BC, N (DirectX) og R, pluss E for lysende vindu. |
| `run_blender.py` | Bygger LOD 0, 1 og 2 og UCX, eksporterer FBX, rendrer `clay`, `mat`, `back` (baksider røde) og `wire`, og leser FBX tilbake i `verify`. |
| `heavy.py` | Kjører `run_blender.py` gjennom `HeavyReservation` og `unreal_guard.run_owned`, venter ved opptatt, stopper aldri andre. |
| `check.py` | Rask kontroll uten Blender: trekanter, bounds og materialspor mot `spec.py`. |
| `placement.py` | Skriver `placement.json`: plassering, yaw og hindringer i spillets koordinater. `--cpp` skriver radene til `S47FarmBuildings.cpp`. |
| `placement.json` | Kopi av siste utskrift, grunnlaget for tabellen i `S47FarmBuildings.cpp`. |
| `import_assets.py` | Bygger materialene og importerer de fire meshene til `/Game/FarmBuildings`. Kontrollerer hver LOD mot `reimport_*.json`. |

## Kjøring

```sh
python3 check.py
~/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/bin/python3 textures.py "<utrot>/textures"
python3 heavy.py barn mat + house mat + shed_a mat + shed_b mat
python3 heavy.py barn wire hero + barn verify
python3 placement.py
```

Kjør alltid Blender gjennom `heavy.py`. En løs Blender-prosess stopper andres vernede kjøringer.

## Import til Unreal

Fra `unreal/Signal47` i kopien som skal få assetene, med bygd editor og ingen annen tung jobb. Rapporten må være en ny bane. Importen leser `fbx/`, `textures/`, `reimport_*.json` og `stats_*.json` fra leveransen; annen plassering gis med `S47_FARM_SOURCE`.

```sh
python3 Scripts/unreal_guard.py require-idle && S47_FARM_REPORT="$HOME/scratch-bm/Signal47-farmbuildings/import-$(date +%H%M%S).json" "$HOME/.local/opt/UnrealEngine-5.8.4/Engine/Binaries/Linux/UnrealEditor-Cmd" "$PWD/Signal47.uproject" -run=pythonscript -script="$PWD/Tools/farmbuildings/import_assets.py" -unattended -nullrhi -nosplash -NoSound '-ini:Engine:[ConsoleVariables]:Interchange.FeatureFlags.Import.FBX=0'
```

Resultatet er `Content/FarmBuildings/**.uasset`, egne modeller gjennom Git LFS som `Content/EastWing`. FBX og blend blir utenfor Git. FBX-flagget er nødvendig: uten det får LOD1 og LOD2 +90 grader (`townintegration/README.md`), og `check_lods` stopper importen.

## Spillet

Prøve etter import, fra pakket eller editorkjøring: `-S47MenuPointerTest -S47MenuPointerBuildings -S47Title -UserDir=/tmp/S47-menu-pointer-buildings`. Bildene `buildings-day-*.png` og `buildings-night-*.png` havner i `Saved/AlbumTest`, og loggen sier `S47: buildings PASS`.

`S47FarmBuildings` holder plassering, hindringer, tomter og gulv. Mangler meshene, viser Freeform de gamle boksene med sin gamle kollisjon. Story ser aldri byggene; det eneste Story merker, er at planter ikke vokser på de fire tomtene, langt fra veien, slik som tomta til det forlatte huset.

## Akser og import

Blender er meter og Z opp, nullpunkt midt i fotavtrykket på bakkenivå, Y er nord, fronten vender mot sør (-Y). FBX-valgene er de samme som `townbuildings`: `axis_forward=-Y`, `axis_up=Z`, `FBX_SCALE_UNITS`. Importen følger `Tools/motelv2/native.py` og `Tools/townintegration/import_assets.py`: `force_front_x_axis=True`, importyaw 90, `vertex_color_import_option=REPLACE`. Den er målt i `townbuildings/README.md` til Blender `(2,4,6)` = UE `(200,-400,600)` cm.

Road-lokal plassering fra `placement.py`: `X = ax + x cos t - y sin t`, `Z = az - x sin t - y cos t`, `Y = SurfaceY(ax,az) + z`. UE-yaw er `-t` i grader. Verden følger `S47FarmWorld::RoadToArea`.

Materialene ganger BC med vertex color. Uten den mister byggene skitten nederst og mørke innsider.

## Valg som avviker fra dagens gård

Dagens spill har bare to bokser i `g910`, `g911` og `g912`: hus 12 x 8 m ved (108, 192) og låve 16 x 11 m ved (136, 170), begge yaw 0,12 rad. Skjulene er nye og plassert etter driften: maskinskjulet ved (116,5, 170) mellom hus og låve, åpent mot tunet, og fôrskjulet ved (84,5, 206,5) med døra mot beiteporten. Huset er flyttet til (108, 190), fordi portalens sørvestre hjørne ellers står 1,2 m inn i veistripen. Låven er 6,85 m høy mot dagens 7,5 m, med ett bredt tak og ett lavt sideskip. Plasseringene er ikke prøvd i spillet.

## Mot bakken

Ingen sokkel: veggkledningen går rett ned til `fbkit.WALL_BASE` (-0,5 m). `fbkit.finish` kutter alle flater ved z 0, 0,13 og 0,75, så vegg, list og stolpe får samme skittstripe fra `grime` og `earth`. `S47FarmBuildings::FSite::Sink` senker hvert bygg 2 til 3 cm, og gulvene i `Floors` må følge `FLOOR`, `A_FLOOR` og `B_FLOOR` og ligge over høyeste terreng i fotavtrykket: selvtesten sjekker det. Terrenget varierer -0,08 til +0,14 m rundt ankeret, målt med `S47RoadTerrain::SurfaceY`.

Øst for road x 107,66 er SARO-ringen eneste bakke i SARO-området, og den ligger opptil 0,79 m under `SurfaceY`. `S47FarmWorld::MakeYardGround` legger derfor samme ørkengitter videre over tunet i Freeform.

## Kritikerrunder

Fire runder. Runde 1 på leire og runde 2 og 3 på materialer hadde to kritikere per bygg, men skjulene delte par. Runde 4 hadde én kritiker per bygg. Etter runde 4 gjensto middels og små punkter, mest slitasje. Ingen er store.

Referansene ligger privat i `referanser/`: Commons `Barn in Chacon, New Mexico` (CC BY 4.0), HABS Romero Barn og Bouquet Ranch House og NARA Coe Ranch (public domain). De er bare sammenligningsgrunnlag, ingenting er kopiert inn i modell eller tekstur.
