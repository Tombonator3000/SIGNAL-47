# Caliche: gate og ørkenkontakt

Original Blender-pakke for gatebiten med bensinstasjon, radiostasjon og suvenirbutikk, 1986. Retningen er Toms `Caliche-byen.png` og `Mesa-Diner.png`: slitt mørk asfalt, lys grus, brutt calichejord og sparsom vegetasjon i klynger. Ingen tredjepartsgeometri eller teksturfotografier brukes. Prosjektbiblioteket ble søkt; sjøhytte/Blender-ressursen var UNVERIFIED og ble ikke brukt som kode- eller assetkilde.

## Runtimekontrakt

Fire uavhengige deler deler ett anker og én origin:

- `SM_Caliche_Surface`: caliche, grusskuldre, grunne sedimentrenner og slitt betong foran fasadene.
- `SM_Caliche_RoadWear`: asfaltaggregat, tjærelapper og grusfingre langs kantene.
- `SM_Caliche_Rocks`: lave, delvis nedgravde kalksteingrupper utenfor main-gåsonen.
- `SM_Caliche_DryPlants`: lave tørre gras-/buskklynger utenfor main-gåsonen.

Blender bruker meter og Z opp. Alle eksporter har origin `[0,0,0]`, skala 1. Anker i Three-verdenen er `[8000,0,2060]`. Formelen er `worldThree = [8000 + blenderX, blenderZ, 2060 - blenderY]`. Modellens kildeområde er Three-world X 7970–8040, Z 2020–2090. Runtime-yaw er 0.

Gjenbruk radioarray-importens faktisk verifiserte legacy-FBX-policy: `force_front_x_axis=True`, `coordinate_system_policy=MATCH_UP_FORWARD_AXES`, `import_rotation.yaw=90`. Den målte basisen er Blender `(x,y,z)` til UE `(x,-y,z)*100`. `model_contract.json` har `parts[SM-navn].bounds_local_m` og `triangles` slik den felles importøren forventer.

**Alle fire deler må bruke NoCollision.** Ingen nye gangrektangler, kjørehøyder, fotunderlag eller blokkeringer er definert av denne pakken. Ingen runtime- eller importkode endres her.

## Flatehøyder og bevarte passasjer

`terrain.py` porter den relevante delen av `roadShape.ts` og `roadTerrain.ts`, med Float32-grid og samme trekantsplitt. `FS47Town::FloorAt` bruker denne høyden. Det opprinnelige fortauet og apron ligger allerede 5–5,5 cm over fotunderlaget. Disse eksisterende avvikene beholdes; det blir ikke konstruert et nytt høydefelt.

`prepare_ground.py` leser faktiske eksporterte triangler for g873 (ørken/grade), g874 (asfalt) og g1004 (Town-pave). `source_contact.json` dokumenterer manifesthash, kildeantall og punktvise kilde-/fotunderlagshøyder. Selve triangelutdraget lagres i `contact_triangles.json`, så modellen kan regenereres uten å åpne hele spillet.

Nye jordflater ligger normalt 8 mm over målt gammel tegnet flate; grus 10 mm; tørre sediment-/tjærefelt 12 mm; smale grusfingre høyst 13 mm. Betongfeltene ligger 6 mm over gammel pave, under de gamle wear-merkene som ligger 8 mm over pave. Overgangene mellom hovedjord og grus er material-/normalvariasjoner med høyst 2 mm høydesprang. Den ytre kanten er uregelmessig og følger det opprinnelige høydefeltet. Ingen forhøyd kantstein eller dyp grøft er lagt til.

Vegens sentrum med midtstripe og en buffer på minst 1,05 m på hver side er urørt. Nye asfaltfelt stopper innenfor den gamle kantlinjen. Grusskuldrene møter eksisterende asfalt, apron og fortau. Stein og vegetasjon utelukkes fra hele main-gåsonen og dens margin, alle bygningsfotavtrykk og ekstra soner rundt payphone, Sentinel-journal, dørtilnærminger, radiobuzzer, studiovindu og A02s opprinnelige og senere posisjoner. Renner er grunne visuelle sediment-/grusspor, ikke nye kolliderende utgravninger.

## PBR og UV

Åtte originale materialer har hver farge, DirectX-normal og ruhet: 24 PNG-filer på 512 × 512. Store flater bruker sammenhengende UV i meter med repetisjon per 4 m. Steiner og tørre blader har ekte UV. Ingen rendereravhengige Blender-prosedyrenoder er nødvendige i spillet. `materials.json` gir eksakte materialnavn og relative teksturstier. DryPlant skal være tosidig. Ruhet er enkanals L-PNG: bruk TC_GRAYSCALE/LinearGrayscale, eller TC_DEFAULT/LinearColor med R-kanal. Normalene har DirectX-grønnkanal; Blender-visningen inverterer grønnkanalen bare i visningsnoden.

Første ekte motorkontroll fant for regelmessige bølger og rette tjærefelt. Teksturkilden bruker derfor innpakket flerskala verdistøy og få uregelmessige sprekkforløp. Tolv avgrensede tjæreklatter har ujevn radial omkrets og dempet fargekontrast. `normal_strength` er asfalt/tjære/betong 0,10, jord/silt 0,12, grus 0,16, stein 0,22 og planter 0,14. `uv_scale` er 1. Tre kompakte kalkstein-/grasgrupper er bevisst plassert i åpningen mellom radio og suvenir, omtrent world X 8023–8029 og Z 2048–2053, utenfor protected-sonene. Endringen krever ny teksturforberedelse, geometri-/blend-eksport og FBX-kontroll; bilder av den forrige pakken dokumenterer funnet, ikke den reviderte pakkens resultat.

## Generering og kontroll

Ikke kjør Blender eller Unreal samtidig med en annen tung jobb på maskinen. Etter at maskinen er frigitt:

```sh
python3 prepare_ground.py ~/scratch-bm/Signal47-web
blender -b --threads 4 --python build_ground.py
blender -b --threads 2 --python verify_ground.py
```

Generatoren lagrer redigerbar `Caliche_Ground.blend`, FBX/GLB per del, samlet GLB og kontrakter. Teksturene pakkes også inn i blend-filen. Generering rendrer ikke automatisk. Etter separat klarsignal kan `-- --render` gi tre CPU-inspeksjonsbilder. `verification.json` bekrefter den faktiske FBX-tilbakeimporten, UV, materialplasser, frie passasjer og alle registrerte flatehøyder.

Kildebilder og Blender-kontroll er ikke runtimebevis. Endelig aksept krever samme kameravinkler ved spillerhøyde i dag, skumring og natt, med synlig forbedret asfalt/grus foran fasadene. Import, runtimeplassering, kollisjonskontroll og sluttprøver utføres av parent.
