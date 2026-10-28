# SC02: den bleke reaktive aktøren

Redigerbar Blender-kilde for aktøren i SC02 møte 1: en mager, to meter høy, blek kropp uten klær, med lang hodeskalle, mørke øyehuler, synlige ribben og lange fingre. Alt er ny original geometri, bygget fra tall i `build_scene.py`. Ingen eksterne modeller, skanninger eller referansebilder er brukt.

Kontrakten for SC02 bestemmer navn, origo, mål og trekantbudsjett. `model_contract.json` binder FBX-hasher, materialspor, trekanttall per del og LOD, origo og metriske grenser.

## Deler

Seks statiske mesher med origo i det proksimale leddet. Armen og beinet er venstre side (+Y i Blender); høyre side er samme mesh med skala Y = -1 i koden.

| Asset | Origo (Blender, m) | LOD0 | LOD1 |
|---|---|---|---|
| SM_SC02_PaleTorso | hoftesenter (0, 0, 1,02) | 3 600 | 1 080 |
| SM_SC02_PaleHead | nakke (0, 0, 1,70) | 2 300 | 690 |
| SM_SC02_PaleUpperArm | skulder (0, 0,21, 1,60) | 520 | 156 |
| SM_SC02_PaleLowerArm | albue (0, 0,21, 1,24) | 1 720 | 516 |
| SM_SC02_PaleThigh | hofte (0, 0,10, 1,00) | 560 | 168 |
| SM_SC02_PaleShin | kne (0, 0,10, 0,51) | 1 080 | 324 |

Hele kroppen (torso, hode og to av hver lemdel) har 13 660 trekanter i LOD0 mot budsjettet på 14 000, og 4 098 i LOD1 (30 prosent).

Leddene møtes med overlapp: nakken sitter i hodemeshen som en sylinder om dreieaksen og går ned i skulderskråningen, slik at hodet kan vende uten åpning. Skulder, albue, hofte og kne har avrundede ender som glir inn i nabodelen. Ingen del har skjelett eller vekting; koden roterer delene om leddene.

## Bygging

`build_scene.py` er deterministisk og virker både i Blender 4.5 LTS og i bpy 5.1:

```sh
blender -b -P unreal/ArtSource/HorrorSC02/build_scene.py
# eller med bpy som Python-modul
python unreal/ArtSource/HorrorSC02/build_scene.py
```

Fremgangsmåte per del: lukkede former (løftede tverrsnitt for kropp og lemmer, ellipsoider og kapsler for knokler, sener, ribben og fingre) smeltes sammen med voxel-remesh, glattes, øyehuler og kinn presses inn, og meshen reduseres med Decimate til fast trekantmål. LOD1 er samme mesh redusert til 30 prosent. Vertexfarge (multiplikator) gjør øyehulene nesten svarte og fingertuppene litt blåblekere. UV er en enkel boksprojeksjon for tangenter og lysmapping; materialet bruker ingen teksturer.

Skriptet skriver `Export/SM_SC02_*.fbx` og `Export/SM_SC02_*_LOD1.fbx`, `SC02_PaleActor.blend` (stående positur satt sammen, LOD1 skjult i egen samling), `materials.json` og `model_contract.json`.

Geometrien og trekanttallene er like ved hver kjøring. FBX-filene inneholder et tidsstempel fra eksportøren, så hashene i kontrakten binder den eksporten som ligger i mappa. Bygges kilden på nytt, skrives kontrakten med nye hasher.

## Materiale

`M_SC02_PaleSkin` er avledet fra `M_SC01_Skin` (0,59/0,62/0,54, ruhet 0,74, specular 0,26): lysere og mindre grønn (0,68/0,70/0,65), ruhet 0,56 for en svakt fuktig glans og specular 0,34. Ett materialspor per mesh. I Unreal blir basisfargen fargeparameteren ganget med vertexfargen.

## Import i Unreal

Integrasjonseieren kjører `import_assets.py` i UnrealEditor-Cmd når ingen annen Unreal-/spilljobb er aktiv. Bare `/Game/Signal47/HorrorSC02` skrives. Skriptet bruker prosjektets FBX/PBR-oppskrift (`Tools/fieldintegration/import_recipe.py`) med samme akser som SC01 (x, -y, z i centimeter) og gjør dette per del:

- importerer LOD0, setter `M_SC02_PaleSkin` og kontrollerer materialspor og hash
- lager to vanlige native LOD-er med skjermgrense 1,0 og 0,04, og erstatter LOD1 med den forfattede `_LOD1.fbx` (midlertidig asset under `HorrorSC02/LODSource`, slettet etterpå)
- slår av Nanite og separat RT-proxy, beholder maskinvare-RT, og gir meshen ingen kollisjon (koden bruker egen kapsel)
- måler trekanttall for begge LOD-er, skjermgrenser, grenser og senter mot kontrakten, og at hele kroppen holder seg under 14 000 trekanter

```sh
python3 unreal/Signal47/Scripts/unreal_guard.py require-idle
S47_SC02_IMPORT_REPORT=$HOME/scratch-bm/Signal47-sc02/native-import.json \
$HOME/.local/opt/UnrealEngine-5.8.4/Engine/Binaries/Linux/UnrealEditor-Cmd \
  <worktree>/unreal/Signal47/Signal47.uproject \
  -run=pythonscript \
  -script=<worktree>/unreal/ArtSource/HorrorSC02/import_assets.py \
  -unattended -nullrhi -nosplash -NoSound \
  '-ini:Engine:[ConsoleVariables]:Interchange.FeatureFlags.Import.FBX=0'
```

## Hva som er verifisert

Verifisert i skyøkta 27. oktober 2026 (ingen Unreal, ingen GPU):

- `build_scene.py` kjørt med bpy 5.1.2 som Python-modul. Trekanttall per del og LOD som i tabellen, hele kroppen 13 660 i LOD0. Origo ligger i leddene: toppen av hodet når z 2,00, fotsålen z 0,00, og underarmen slutter 0,44 m under albuen.
- Forhåndsvisninger rendret med Cycles på CPU fra den lagrede `.blend` (Eevee og Workbench krever OpenGL/EGL, som skyen mangler): forfra, fra siden, tre fjerdedeler, bakfra, ansikt, hånd, hodet vendt 60 grader med én arm løftet, nærbilde av nakke og skulder i den posituren, og LOD1 forfra. Leddene var lukket i normal positur og i den vendte/løftede posituren.

Ikke verifisert her, gjenstår på Kubuntu:

- Blender 4.5 LTS: kjøre `build_scene.py` der. Skriptet unngår API som bare finnes i én av versjonene, men det er bare kjørt i 5.1. `.blend` er lagret av 5.1.2; åpner den seg ikke i 4.5, bygges den på nytt med 4.5.
- Native import med `import_assets.py` i UE 5.8.4, særlig at `set_lod_from_static_mesh` beholder skjermgrensen 0,04 for LOD1 og at vertexfargen virker i materialet. Importrapporten skal være PASS.
- Pakket test, RT-minne mot grensen på 80 MiB (se SC01-avsnittet om AlwaysResident RT-geometri; LOD1 for hele kroppen er cirka 4 100 trekanter) og faktiske piksler i spillet med spillets lys.

Denne kilderevisjonen alene hevder ingen native import, pakket aksept eller pikselkontroll.

## Retting 28. oktober 2026 (Claude på Kubuntu)

Etter [revisjonen av historien](../../../Docs/Design/Story/HISTORIE_SC02_SC13_REVISJON.md) så modellen ut som en utstillingsdukke med et klassisk «grey»-hode. Rettet i `build_scene.py`, bygget i Blender 5.2.2 på Kubuntu:

- Foroverbøyd rygg: overkroppen skyves fram etter høyden (`hunch`), 12 cm ved nakken og 7,7 cm ved skuldrene. Koden fester hodet og armene tilsvarende lenger fram, og hodet løfter seg mot lenet for å se framover.
- Knoklete bekken i stedet for glatt dukkehofte: hoftekam på sidene, lårbeinshoder, lyskelinjer, kjønnsbeinskant, smalere bekken og innsunket mage.
- Nytt hode: smalere og lavere skalle, ekte kjeve og hake, tung pannekant, nese, munnlinje og små, dype øyehuler i stedet for store mørke øyne.
- Lengre fingre, kortere føtter med mellomfotsben og tær med knoker.
- Hud: samme grunnfarge som SC01-hånden (`M_SC01_Skin`), med flekker og svake blå årer i vertexfargen.
- Bevegelsen er bevisst rykkvis, ledd for ledd (koden fra skyøkten), i stedet for skjelett og vekting.

Trekanttallene er uendret: 13 660 i LOD0 og 4 098 i LOD1. Forhåndsvisningene i `Previews` er rendret fra den nye kilden.

## Kjente svakheter

- Nakken er lang og tynn. Det er bevisst, men den ser stiv ut sett rett forfra.
- Albue og kne viser en svak ring der delene møtes. Det er ingen åpning, men det synes på nært hold i flatt lys.
- LOD1 gjør ribbeina til ujevne flekker. Det er akseptert fordi LOD1 bare brukes under skjermgrense 0,04.
