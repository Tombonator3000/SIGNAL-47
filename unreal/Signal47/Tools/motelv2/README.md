# Motel V2: avgrenset modellimport

Disse verktøyene bygger på prosjektets prøvde `townintegration`-import i Unreal
5.8.4. De skriver bare egne native assets under `/Game/MotelV2`. De starter ingen
motor- eller byggeprosesser og aktiverer ikke modellen i runtime på egen hånd.

1. Lag først en separat kildepakke gjennom lesende inventar og den avgrensede
   [terrengkontakt-/rekvisittrettelsen](../../../ArtSource/MotelV2/README.md).
   Kjør `prepare_trims.py EXPORT OUT --previous CONTENT/TownBuildings` med den
   uendrede webeksporten og samme `grounding-contract.json`. Den gjenskaper og
   hashkontrollerer romkuttene før den fjerner eksakt identifiserte erstattede
   biler, basseng, ICE/avis, stoler, kontaktskygger og fundamentrester. Begge
   terrenglag åpnes under de faktiske native flatene, med bevarte originale
   bufferprefikser, interpolerte kantattributter og arealbevaring per trekant.
   Uavhengige stråleprøver leser de faktisk skrevne GLB-filene. Originale
   detaljbiler, TV-er, planter og spillinteraksjonenes ID-er beholdes.
2. Kjør `convert_trims.py` i Blender med `-- OUT`. Den kontrollerer antall trekanter,
   materialrekkefølge og akser, og skriver én FBX per ikke-tomt restmesh. Direkte
   GLB-import til Unreal unngås fordi den tidligere mistet tynne trekanter.
3. Sett `S47_MOTEL_SOURCE`, `S47_MOTEL_TRIMS` og en ny `S47_MOTEL_REPORT`.
   Kjør `import_assets.py` i eksisterende, låst/sekvensiell Unreal-commandletløype.
4. Kjør `verify_assets.py` i en ny editorprosess med ny rapportsti. Den leser lagrede
   native data: tre forfattede LOD-er og én generert fjern-LOD per ny modelldel, eksakte trekanttall/materialspor,
   hver LODs faktiske MeshDescription-grenser, UV-presisjon, probe og PBR-fargerom.
5. Kjør `stage_assembly.py --source EXPORT --trims TRIMS --project PROJECT
   --import-report IMPORT_JSON --verify-report VERIFY_JSON --report STAGE_JSON`.
   Den bruker den gjennomgåtte `runtime_layout.json`, krever reparasjons-ID og
   samme kontrakthash i kilde/trim, avleder alle synlige gulv og høyder fra
   kildekontrakten og sjekker at kildefilene fortsatt matcher begge native
   rapportene. Først da skriver den `Content/MotelV2/assembly.json`
   atomisk. Tidligere metadata sikres ved omkjøring. Pakk og prøv
   kjøring, mottak/telefon, rom 6, TV-program og lagring i isolerte testprofiler.

Den sammenhengende fotruten bruker `-S47Jump=chapter4 -S47MotelV2Route` og en ny
`-UserDir=/tmp/...`. Den har fire uttrykkelige kollisjonsplasseringer før én fysisk
Walker-rute langs mottak/TV, telefon/ICE, basseng og romdører, med faktisk bruk av
rom 6-døren og ordinær isolert disk/title/Continue. TV-bevis leser den faktisk
bundne GPU-teksturen og programmets bilde-/lydklokker. Kapittel 4 er en eksplisitt
utviklerfixture; tidligere kapitler, menneskelig input og subjektiv lytting er
ikke underforstått bestått. Pooldiagnosen identifiserer mesh/siktlinjer uten å
slette det foreløpig uidentifiserte objektet fra spillerbildet.

Normaler følger materialradens eksplisitte `normal_convention`: DIRECTX brukes
uendret, OPENGL får grønnkanalen vendt i teksturasseten. ORM har AO i R, roughness i G
og metallic i B, uten sRGB. Emissive deler beholder `EmissiveFactor`, som runtime
kan styre etter eksisterende kontor-/rom-/utebelysning.

Biblioteksøk: `scenario-unreal-pipeline-automation` ved festet revisjon
`f0adb5b5ebc1fc8f7e333554d2aeb767c5558d6b` ble lest som referanse. Kilden merker sine
motoranrop som uprøvd i Unreal; derfor gjenbrukes prosjektets allerede prøvde
FBX-/materialkode og ikke bibliotekets uprøvde motorløype. Prinsippene om eksplisitt
importpolicy, isolerte rapporter og kontroll av faktisk lagret LOD er videreført.

Kildeskript og ren geometriforberedelse kan kontrolleres uten motor. Native PASS
krever faktisk kjøring; det er ikke underforstått av syntakskontroll eller eksport.

## Avgrensede native rettelser

`set_rt_lod.py` legger til ett svært fjernt, ordinært LOD3 på de 24 nye motelldelene. Alle tre forfattede LOD-er, materialer, kollisjon og opprinnelige skjermgrenser kontrolleres uendret. LOD3 bruker skjermgrense .005 og et redusert trekantmål; små flater kopierer allerede liten LOD2. Faktisk generert geometri verifiseres etter lagring. Trims/probe og globalt RT-budsjett berøres ikke.

Første forsøk med egne RT-proxyinnstillinger var utilstrekkelig: Unreal 5.8 bygger disse bare innenfor Nanite-løypen. Disse modellene bruker ikke Nanite. At innstillingen var lagret, beviste derfor ikke at den påvirket den tegnede geometrien. De mislykkede spillprøvene er bevart som diagnostikk.

`repair_water.py` korrigerer bare eksisterende `M_MotelV2_PoolWater`: ensidig gjennomskinnelig overflate med den oppgitte vannfargen som tint på atlaset. Ruhet, normaler, opacity og geometri beholdes. Begge skript bruker `S47_MOTEL_SOURCE` og `S47_MOTEL_REPORT`; LOD-skriptet bruker også `S47_MOTEL_TRIMS`. Etter endringer kjøres `verify_assets.py` i en ny Unreal-prosess og `stage_assembly.py` før pakking. Native PASS erstatter ikke visuell kontroll i det pakkede spillet.
