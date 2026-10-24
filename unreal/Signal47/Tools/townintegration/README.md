# Sammensatte Caliche-kutt

Kun lett kildeforberedelse og import i en allerede startet editorprosess. Ingen av verktøyene starter Blender eller Unreal. Original `manifest.json` og GLB under eksportens `geo/` er skrivebeskyttet målegrunnlag. Tidligere kutt leses bare som kontroll av kjente trekanttall; nye kutt bygges alltid fra originalen.

```sh
python3 Tools/townintegration/prepare_trims.py \
  /home/tom/scratch-bm/Signal47-web \
  /home/tom/scratch-bm/Signal47-production-models/town-trim \
  --previous-street /home/tom/scratch-bm/Signal47-caliche-concept/trim/street-patches.json
```

Fem nye bygg bruker `Tools/townbuildings/design.py` som koordinatautoritet. Sheriff/Laundry/Feed/Cinema/Church komponeres med gas/radio/suvenir fra eksisterende kilder. Bare direkte statiske meshbarn under `world.town.st` og `world.town.dressing.group` er kandidater. Benk, patruljebil/dør, personer, A02, interaksjonsproxies, husdører og dynamiske bildepanelet er egne objekter og holdes utenfor. Kirkens tavle ved x18.9,z2087.3 og Sheriffens flaggstang ved x−14.6,z2099.2 ligger utenfor kuttene. Smale lave stoopbokser tar bare de navngitte gamle trinnene, ikke benken. Ingen gang-/bilkollidere endres.

Resultatet er `trim_geo_008.glb` og `town-patches.json`. JSON-kopien her i kildekatalogen dokumenterer siste kontrollerte originaleksport. Ved nytt eksportgrunnlag regenereres både GLB og metadata; importer må aldri lese utdatert metadata sammen med en ny GLB.

## Native importer

Hovedagenten setter følgende miljø før eksisterende guard-/headless-editorjobb kjører `import_trims.py`:

```
S47_TOWN_TRIM=/home/tom/scratch-bm/Signal47-production-models/town-trim
S47_TOWN_TRIM_REPORT=/ny/rapportbane/import-town-trims.json
```

Bare det eide `/Game/TownBuildings/Trim` slettes/bygges på nytt. Skallmodellene under `/Game/TownBuildings` røres ikke. Importen normaliserer faktiske mesh til `Trim/StaticMeshes/gId`, setter FP32-UV, kompleks synlighetskollisjon og midlertidig DefaultMaterial. Runtime gjenbinder originalmaterialene. Alle faktiske trekanttall og slotantall må stemme før metadata kopieres til `Content/TownBuildings/town-patches.json`. Kjør samme fil i en fersk editorprosess med `S47_TOWN_TRIM_VERIFY_ONLY=1` og en ny rapportbane for å kontrollere lagrede meshes og staged metadata uten reimport. Dette er verktøy for senere autorisert kjøring, ikke bevis på at nativeimport har kjørt.

## Schema v2 for runtime

Toppen inneholder `manifestSha256`, `buildingPlanSha256`, `sourceGeometrySha256`, `fileSha256`, `groups`, `meshes`, `boxes` og `totalsRemovedBy`. Hver `meshes.gId` har:

- `node`, `geo`, `group`, `parent`: eksakt originalidentitet og hierarki.
- `tris`: originalt trekanttall fra uendret eksport.
- `acceptedPreviousTris`: originale og kjente gas/radio/aksepterte CalicheStreet-kutt, kontrollert mot originalen. Tallet beskriver meshressursen; et allerede skjult objekt kan fremdeles ha sin opprinnelige mesh.
- `resultTris`, `hidden`, `asset`: ny uniongeometri. Helt tomme geometrier skjules, uten et tomt nativeasset.
- `originalMaterialIndices`: per beholdt native slot, hvilket ORIGINALT slotnummer som må gjenbrukes. Ikke bind nytt slot N til gammelt slot N blindt dersom et primitive ble tomt. `sourceMaterialIds` og `sourceMaterialCount` dokumenterer den opprinnelige listen.
- `removedBy`: eksklusive antall for gas/radio/souvenir/Sheriff/Laundry/Feed/Cinema/Church. Summen er `removed`, og `tris−removed=resultTris`.
- `retainedIndexSha256` og `originalIndexSha256`: nøyaktig triangle-/indeksrekkefølge.

Runtime må først kontrollere ALLE fem komplette modeller, forventet kildenode/hierarki, tidligere trekanttall, alle native trimmeshes og slotkart. Deretter byttes unionen samlet. Gjør ingen global synlighetsendring på Town, dressing eller dynamiske foreldre. Ved avvik beholdes forrige komplette geometri. Root eier den atomiske nativeintegrasjonen og dens anvendelse av bevart synlighet/kollisjon.

Opaque Sheriff-/Laundry-interiørbilder foran de nye vinduene må behandles eksplisitt av nativeintegrasjonen, samtidig som panel-state og proxies beholdes. Disse dynamiske objektene er med hensikt ikke med i statiske meshkutt.

## Faktisk kildekontroll

For originalmanifest `69fcf95d...a0925c9`: 21 rader, 14 ikke-tomme nativepatcher og 7 helt skjulte. 50808 originaltrekanter blir 39994. Kuttet er gas528, radio230, suvenir534, Sheriff1578, Laundry1622, Feed2789, Cinema2303 og Church1230. Faktisk utfil er lest tilbake: alle opprinnelige position/normal/UV/color-bufferbytes er byteidentiske; bare refererte indekslister endres, i bevart rekkefølge. Alle aktuelle kildegeometrier har ett materialslot, så nåværende ikke-tomme slotkart er `[0]`.

Dette er kildebevis. Nativeimport, rendering, åpninger, folk, benk, interaksjoner og pakket fallback testes separat av hovedagenten.

## Motell og rom 6, separate presise kutt

`prepare_room_trims.py EXPORT OUT --recipe Tools/townbuildings/runtime_replacement.json` leser de 11 blandede geometrioppskriftene. Bruk egen `OUT=/home/tom/scratch-bm/Signal47-production-models/room-trim`. Skriptet skriver aldri i `town-trim`. Resultatet er `room-patches.json` med samme v2-rader og tre GLB: `trim_geo_003.glb`, `trim_geo_004.glb`, `trim_geo_009.glb`. Beskyttelsesprismer inngår i hver rads `recipe`. Desimalgrensene får kun 1e−5m numerisk margin for opprinnelige float32-vertexkoordinater. Et trekantvern krever at alle tre hjørner ligger i samme beskyttelsesprisme.

`import_room_trims.py` bruker **kun** `/Game/TownBuildings/RoomTrim` og miljøet `S47_ROOM_TRIM`, `S47_ROOM_TRIM_REPORT`, eventuelt `S47_ROOM_TRIM_VERIFY_ONLY=1`. Dermed røres ingen eksisterende Town/Trim-assets. Metadata stages som `Content/TownBuildings/room-patches.json`. Samme faktiske slot-, indeks-, trekant- og FP32-UV-kontroller brukes.

11 rader er 10 nativepatcher og én helt skjult g468. Registreringskortets opprinnelige trykk i g488 beholdes presist som 2 av 18 trekanter. AC-gitteret i rom 6 g1125 mister 108 trekanter, mens de 48 opprinnelige askebegertrekantene utenfor AC-prismet beholdes. Motellfeltene beholder både original lot/bil/basseng/utelys og beskyttet isautomat/potter; ingen brede foreldre skjules av trimmeren. Den separate listen med 47 hele draw-komponenter eies av nativeassemblyen.

Alle 11 partialrader har originalt trekanttall som eneste `acceptedPreviousTris`, fordi de ligger utenfor Kelvoras TV-kutt og den separate bed-subtreet. `wholeLeafPreviousStateAudit` dokumenterer derimot nativeassemblyens kjente Kelvora-varianter: g482 196 eller 132, g486 64 eller 20, g489 280 eller 224. G492 er allerede skjult av Kelvora, men beholder sin originale 2-triangle-mesh. Disse må ikke erstattes med antatt null mesh-trekantantall.

## Native LOD-retning: påvist M1-feil

Legacy-FBX-importen må starte editoren med den faktiske globale innstillingen `-ini:Engine:[ConsoleVariables]:Interchange.FeatureFlags.Import.FBX=0`. Den tidligere brukte `-InterchangeImport=False` erstatter ikke denne innstillingen. Behold den verifiserte importkontrakten med `force_front_x_axis=True`, `MATCH_UP_FORWARD_AXES` og import-yaw 90°. Blender `(x,y,z)` meter skal ende som UE `(100x,−100y,100z)` centimeter i **hvert** LOD.

Den faktiske, skrivebeskyttede M1-F2-kontrollen (`/home/tom/scratch-bm/Signal47-production-models/town-native-lod-M1-F2.json`) undersøkte 28 lagrede meshes og 84 `StaticMeshDescription`-er: alle 196 LOD0-kontroller besto, mens samtlige 56 LOD1/2 hadde +90° Z-rotasjon fra forventet. Alle 84 trekanttall og unit-buildscale besto. De roterte asymmetriske punktprøvene samsvarte i alle 56 tilfeller, med største boundsavvik 0.000418 cm etter diagnostisk rotasjon. Dette forklarer hvorfor korrekt LOD0-probe og trekanttall kunne passere samtidig som kirkeinteriør/tak viste feil retning. Kildegeometrien ble ikke endret for å kompensere.

Kjør `verify_lod_geometry.py` i den allerede guardede editorjobben med `S47_TOWN_LOD_REPORT` satt til en ny JSON-bane etter reimport. Skriptet leser faktiske lagrede vertexposisjoner, bounds, trekanttall og åtte asymmetriske prøver per LOD mot de originale FBX-kontrollpunktene. Toleransen er 0.025 cm. Hele punktmengden kontrolleres begge veier; ingen asset lagres eller importeres av kontrollen. `StaticMeshEditorSubsystem` bruker `new_object`-fallback i commandlet. Først en ny PASS-rapport og faktiske nativebilder kan bekrefte rettingen; rapporten kontrollerer lagret kildegeometri, ikke direkte GPU-renderbuffere eller materialets utseende.
