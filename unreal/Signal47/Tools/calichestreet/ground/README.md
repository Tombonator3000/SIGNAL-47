# Caliche: gate og ørkenkontakt

Original Blender-pakke for gatebiten med bensinstasjon, radiostasjon og suvenirbutikk, 1986. Retningen er Toms `Caliche-byen.png` og `Mesa-Diner.png`: slitt mørk asfalt, lys grus, brutt calichejord og sparsom vegetasjon i klynger. Ingen tredjepartsgeometri eller teksturfotografier brukes. Prosjektbiblioteket ble søkt; sjøhytte/Blender-ressursen var UNVERIFIED og ble ikke brukt som kode- eller assetkilde.

## Runtimekontrakt

Fire uavhengige deler deler ett anker og én origin:

- `SM_Caliche_Surface`: caliche, grusskuldre, grunne sedimentrenner og slitt betong foran fasadene.
- `SM_Caliche_RoadWear`: asfaltaggregat, sprekknett og grusfingre langs kantene.
- `SM_Caliche_Rocks`: lave, delvis nedgravde kalksteingrupper utenfor main-gåsonen.
- `SM_Caliche_DryPlants`: lave tørre gras-/buskklynger utenfor main-gåsonen.

Blender bruker meter og Z opp. Alle eksporter har origin `[0,0,0]`, skala 1. Anker i Three-verdenen er `[8000,0,2060]`. Formelen er `worldThree = [8000 + blenderX, blenderZ, 2060 - blenderY]`. Modellens kildeområde er Three-world X 7970–8040, Z 2020–2090. Runtime-yaw er 0.

Gjenbruk radioarray-importens faktisk verifiserte legacy-FBX-policy: `force_front_x_axis=True`, `coordinate_system_policy=MATCH_UP_FORWARD_AXES`, `import_rotation.yaw=90`. Den målte basisen er Blender `(x,y,z)` til UE `(x,-y,z)*100`. `model_contract.json` har `parts[SM-navn].bounds_local_m` og `triangles` slik den felles importøren forventer.

**Alle fire deler må bruke NoCollision.** Ingen nye gangrektangler, kjørehøyder, fotunderlag eller blokkeringer er definert av denne pakken. Ingen runtime- eller importkode endres her.

## Flatehøyder og bevarte passasjer

`terrain.py` porter den relevante delen av `roadShape.ts` og `roadTerrain.ts`, med Float32-grid og samme trekantsplitt. `FS47Town::FloorAt` bruker denne høyden. Det opprinnelige fortauet og apron ligger allerede 5–5,5 cm over fotunderlaget. Disse eksisterende avvikene beholdes; det blir ikke konstruert et nytt høydefelt.

`prepare_ground.py` leser faktiske eksporterte triangler for g873 (ørken/grade), g874 (asfalt) og g1004 (Town-pave). `source_contact.json` dokumenterer manifesthash, kildeantall og punktvise kilde-/fotunderlagshøyder. Selve triangelutdraget lagres i `contact_triangles.json`, så modellen kan regenereres uten å åpne hele spillet.

Nye jordflater ligger normalt 8 mm over målt gammel tegnet flate; tørre sedimentfelt og støv 12 mm; smale grusfingre høyst 13 mm. Betongfeltene ligger 6 mm over gammel pave, under de gamle wear-merkene som ligger 8 mm over pave. Overgangene mellom hovedjord og grus er material-/normalvariasjoner med høyst 2 mm høydesprang. Den ytre kanten er uregelmessig og følger det opprinnelige høydefeltet. Ingen forhøyd kantstein eller dyp grøft er lagt til.

Veien er sammenhengende detaljert over begge felt. Bare gulmerkingen ved X ±0,1 m (0,1 m bred) og hvite kantlinjer ved X ±3,6 m (0,12 m bred) holdes udekket: alpha er 0 innenfor X ±0,18 m og ved kantlinjen ±0,09 m, med myk overgang videre. Ingen bred, mørk midtbuffer beholdes. Grusskuldrene møter eksisterende asfalt, apron og fortau. Stein og vegetasjon utelukkes fra hele main-gåsonen og dens margin, alle bygningsfotavtrykk og ekstra soner rundt payphone, Sentinel-journal, dørtilnærminger, radiobuzzer, studiovindu og A02s opprinnelige og senere posisjoner. Renner er grunne visuelle sediment-/grusspor, ikke nye kolliderende utgravninger.

## PBR og UV

Revisjon 4 / native R3 bruker 13 originalmaterialer og 39 kart. Asfalt, jord, grus og slitt betong har albedo på 2048 × 2048; øvrig albedo, normal og ruhet er 1024 × 1024. `texture_detail.py` kombinerer uregelmessig makrovariasjon, fine mineralkorn, kantete steinfliser og forgrenede sprekkforløp. Ingen fotopiksler, store tjærepolygoner eller sinusmønstre er brukt. Jordtonen er varm sandtan/gråbrun og ligger nærmere det eksisterende ørkenunderlaget, slik at regionen ikke leses som en separat betongplate.

`materials.json` angir eksakte navn, normaleffekt, UV-skala og teksturstier. `ShoulderBlend` og `DustFeather` har RGBA med gjennomsiktige, uregelmessige kanter og brukes bare i avgrensede tynne kantfelt. Importøren skal lese `opacity_from_base_color=True`, med `alpha_mode=dithered_masked`, `vertex_alpha_opacity=False` og `specular=0.0`. Native shader bruker atlasalpha gjennom DitherTemporalAA til OpacityMask. Dette unngår glassaktig spekulær respons utenfor støv-/grusmasken. Både `CalicheSoil` og `OldAsphalt` bruker `vertex_alpha_opacity=True`, `alpha_mode=dithered_masked`. Hvert mesh har RGBA-attributtet `Color`, også de delene der alle verdier er hvite. RGB er alltid 1. Alpha er nøyaktig 0 på alle faktiske åpne polygonkanter, inkludert hull rundt opprinnelig pave/bygg. Alpha går med smoothstep til 1 over 1,8 m for jord og 0,45 m for asfalt. I UE må `DitherTemporalAA(VertexColor.A)` drive OpacityMask; et vanlig alpha-clip ville bare flytte den harde sikksakkgrensen. FBX-importen må bruke vertex_color_import_option=REPLACE. Lokale shoulder-patcher har nå teksturalpha 0 på alle fire kanter. Store flater bruker sammenhengende UV per 6 meter; kantpatcher har lokal UV 0–1.

`GrassAtlas` og `SageAtlas` har fire forskjellige tue-/busksilhuetter i hvert RGBA-atlas. Kortene bruker bare én slik rute per plante, tosidig `alpha_mode=masked`, UV-skala 1 og ingen atlasoverskridelse. Bevar mip-alphadekning ved materialets terskel, slik at de tynne stråene ikke forsvinner på spilleravstand. Yucca har geometriske sverdblader; tumbleweeds har faktisk åpen kvistgeometri. Gras og sage samles ved bygningskantene, i tillegg til de tre synlige gruppene i åpningen mellom radio og suvenir.

Lavt fasadegras bruker et eget, presist plasseringsfilter: hovedgåsonens faktiske rektangel og bygningsfotavtrykk beholdes, mens egne beskyttede bokser holder dørtilnærminger, gas-payphone/journal, radio-buzzer, studiovindu og begge A02-posisjonene frie. Stein og høyere planter får større margin. Alle runtime-delene skal fortsatt ha NoCollision.

Normalene bruker DirectX-grønnkanal. Blender-visningen inverterer grønnkanalen bare i visningsnoden. Ruhetskart er enkanals L-PNG; den eksisterende importøren bruker TC_DEFAULT/LinearColor med R-kanal. De nye materialene er fullstendig beskrevet av sidecaren og krever ingen Blender-prosedyrenoder i motoren.

## Generering og kontroll

Ikke kjør Blender eller Unreal samtidig med en annen tung jobb på maskinen. Etter at maskinen er frigitt:

```sh
python3 prepare_ground.py ~/scratch-bm/Signal47-web
blender -b --threads 2 --python build_ground.py
blender -b --threads 2 --python verify_ground.py
```

Generatoren lagrer redigerbar `Caliche_Ground.blend`, FBX/GLB per del, samlet GLB og kontrakter. Teksturene pakkes også inn i blend-filen. Generering rendrer ikke automatisk. Etter separat klarsignal kan `-- --render` gi tre CPU-inspeksjonsbilder. `verification.json` bekrefter den faktiske FBX-tilbakeimporten, UV, materialplasser, frie passasjer og alle registrerte flatehøyder.

Kildebilder og Blender-kontroll er ikke runtimebevis. Endelig aksept krever samme kameravinkler ved spillerhøyde i dag, skumring og natt, med synlig forbedret asfalt/grus foran fasadene. Import, runtimeplassering, kollisjonskontroll og sluttprøver utføres av parent.

Native R3 verifiserer i tillegg at FBX faktisk bevarer `Color` og alpha-intervall, at alle åpne kanter har alpha 0, at dupliserte posisjoner har identisk alpha, og at de faktiske veimerkingssonene er udekket. Tidligere native bilder og rapporter er beholdt i arbeidsbevisene før regenerering. Fire tydeligere, avgrensede tue-/sagegrupper ved bygningshjørner og små grasrøtter ved radiobenkens nordende er lagt til utenfor de beskyttede sonene.

## GLB-alpha og eksportbegrensning

Blender 5.2 sin glTF-eksportør støtter ikke vertexalpha brukt uten vertex-RGB i shaderen. `gltf_export.py` legger derfor midlertidig inn teksturRGB × hvit vertexRGB under GLB-eksport, slik at eksportørens støttede RGBA-vei brukes. RGB er 1 i kilden, så fargen endres ikke. Koblingene gjenopprettes etterpå; FBX, redigerbar blend og sidecar berøres ikke av GLB-refresh. glTF bruker vanlig alpha-blending som portabel erstatning for motorens temporale dithering.

`verify_glb.py` leser faktiske binære COLOR_0-verdier for hver materialprimitive. Soil/asfalt må ha alpha 0, 1 og mellomverdier; øvrige vertexfarger må ha alpha 1. `verify_ground.py` kaller denne kontrollen. `refresh_glb.py` sammenligner dessuten hvert GLB-punkts alpha med Blender-kilden og dokumenterer SHA256 for uendret FBX/blend/kontrakter i `glb_refresh.json`. `glb_verification.json` inneholder de målte intervallene og avvikene.

## R5: tydelige, lokale vegetasjonsøyer

`r5_dressing_plan()` i terrain.py definerer tolv avgrensede øyer ved radio-/souvenirveggene, i mellomrommet og ved to ubebygde gas-apron-kanter. Etter uendrede gang-/foto-/dørfiltre og hard regiongrense er tilleggene 12 store sagebusker, 35 mindre sage, 86 grastuer og 63 småsteiner. Gas-øyene står utenfor betong-apron. Alle tilleggene holder hovedgåsonen og pumpe-/payphone-/journal-/foto-/dørtilnærminger frie.

Sage bruker fem forskjøvede og roterte atlasplan innenfor deklarert fotavtrykk. Lysere støvet olivengrått løv, mer løvdekning og lys transparent padding gjør plantene lesbare også under blått kveldslys. Gras er tørt strågult; det er fortsatt åpne mellomrom mellom øyene. Oppdaterte originalatlas er pakket inn i blend-filen.

Surface/RoadWear-geometri, UV, vertexalpha, transform og materialplasser har eksakt samme SHA256 som før R5; partskontraktene er identiske. `r5_frozen_surfaces.json` og `r5_report.json` dokumenterer dette. Samlet kildetriantall er 56 323. Native bilder avgjør endelig visuell aksept.

## R6 — localized surface use, actual contact authority

Six controlled fields add coherent wheel-use dust on the east apron/service gap and matte scuff/oil traces in the gas pump lanes/turnout. No new plants were added. The three soil fields use original rendered top+12mm, above existing Soil top+8mm. The three concrete fields use original rendered top+7mm. Triangle centroid and edge-midpoint tests reject bridges across source height steps: one cell was removed, with its open edge faded. Centroid clearance is5.24–12.94mm above original ground and at least2.74mm above the actual unchanged Soil triangles; concrete6.97–8.08mm with no Soil underneath. The original maximum14mm test remains strict.

The gas apron in `gs_preview.py` is preview-only; original exported town `g1004` triangles remain the actual contact authority. The raised gas pump island and protected approaches are excluded. Collision and player floor are unchanged.

Surface now has19,568triangles and15slots. RoadWear26,838, Rocks11,055, DryPlants6,828 keep their13slots. Total64,289triangles. Both new materials have original2048 RGBA maps, texture-alpha multiplied by vertex-alpha, actual0.55m boundary fade, and specular0. New maps use a single0..1UV field rather than repeated wheel patterns.

`r6_geometry_preservation.json` checks every original face's coordinates, UV and Color RGBA on all four parts against the saved R5 blend. All are hash-identical; the other three part contracts are identical. `verification.json` checks FBX, contact and protected placement; `glb_verification.json` checks actual binary RGBA values against saved Blender points; `r6_glb_texture_alpha.json` confirms embedded2048RGBA pixel identity and nontrivial alpha. `r6_report.json` contains per-field footprints, measured contact ranges and the gas preview distinction. Prior R5 exports are preserved at `/home/tom/scratch-bm/Signal47-caliche-concept/ground-before-r6`; rejected R6 evidence is under `evidence/r6-rejected-7mm-under-soil`. Native visual approval is pending.
