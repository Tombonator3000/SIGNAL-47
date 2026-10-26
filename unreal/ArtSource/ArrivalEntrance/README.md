# SARO-inngang og besøksparkering

Original Blender-geometri og PBR-/skiltgrafikk, 26. oktober 2026. Ingen eksterne modeller eller teksturkilder. `SARO_ArrivalEntrance.blend` har innpakkede teksturer og tre navngitte LOD-er per del. Kjør `make_textures.py` med Python/Pillow og deretter `build_entrance.py` med Blender. `preview.py` lager eksplisitte Blender-forhåndsvisninger med egne kontekstflater for eksisterende vegg, dør og rampe; disse kontekstflatene eksporteres ikke til spillet.

| Del | Funksjon | Trekanter LOD0/1/2 |
|---|---|---|
| SM_Arrival_Entrance | Baldakin/tak, foldedetaljer, beslag, skilt, terskel, skraper og glatte hylser rundt eksisterende rekkverk | 8964 / 5252 / 2020 |
| SM_Arrival_DoorDetails | Sparkplate, pakning, rundtrekk, lås, hengsler og dørlukker på eksisterende dørblad | 2412 / 1436 / 620 |
| SM_Arrival_VisitorSign | 46 × 60 cm emaljeskilt, 1,45 m senterhøyde, U-profil og bolter | 2606 / 1518 / 318 |

Inngangen og dørdetaljene bruker Three-roten `(-6.27,0,5.22)`. Dørdetaljene må følge det eksisterende animerte hengslet. Hver besøksstolpe står med origo på bakken. Three-offset `(x,y,z)` er Blender `(x,-z,y)` og Unreal-centimeter `(x,z,y)*100`. Den eksisterende rampens fysiske gulv og frie bredde endres ikke.

Importløypa er `Signal47/Tools/arrivalentrance/import.sh`, med prosjektets målte `fieldintegration/import_recipe.py`. Den skriver kun native assets under `/Game/Signal47/Art/ArrivalEntrance` og bruker tre forfatterlagde LOD-er. `verify_source.py` gjenåpner Blender-filen; `verify_assets.py` gjenåpner native assets i en fersk editorprosess. Rapporter ligger i arbeidsmappen `Signal47-arrival-office-20261026`. En vellykket Blender-render er ikke en attest for spillets gangrute, materialer eller funksjon; dette må prøves i den pakkede kandidaten.
