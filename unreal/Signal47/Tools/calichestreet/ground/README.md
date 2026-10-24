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
