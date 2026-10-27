# Motel V2: avgrenset modellimport

Disse verktøyene bygger på prosjektets prøvde `townintegration`-import i Unreal
5.8.4. De skriver bare egne native assets under `/Game/MotelV2`. De starter ingen
motor- eller byggeprosesser og aktiverer ikke modellen i runtime på egen hånd.

1. Kjør `prepare_trims.py EXPORT OUT --previous CONTENT/TownBuildings` med den
   uendrede webeksporten. Den gjenskaper og hashkontrollerer de tidligere
   romkuttene før den fjerner den gamle enkle bilen, de to enkle dressingbilene og
   det gamle bassenget. To terrenglag klippes nøyaktig langs bassenghullet: bare
   fem berørte trekanter deles, med interpolerte kantattributter. Arealbevaring og
   separate punkt/trekantprøver i de faktiske GLB-filene kontrollerer hullet.
   Originale detaljbiler, TV-er, ICE, planter og proxyer beholdes.
2. Kjør `convert_trims.py` i Blender med `-- OUT`. Den kontrollerer antall trekanter,
   materialrekkefølge og akser, og skriver én FBX per ikke-tomt restmesh. Direkte
   GLB-import til Unreal unngås fordi den tidligere mistet tynne trekanter.
3. Sett `S47_MOTEL_SOURCE`, `S47_MOTEL_TRIMS` og en ny `S47_MOTEL_REPORT`.
   Kjør `import_assets.py` i eksisterende, låst/sekvensiell Unreal-commandletløype.
4. Kjør `verify_assets.py` i en ny editorprosess med ny rapportsti. Den leser lagrede
   native data: tre LOD-er per ny modelldel, eksakte trekanttall/materialspor,
   hver LODs faktiske MeshDescription-grenser, UV-presisjon, probe og PBR-fargerom.
5. Kjør `stage_assembly.py --source EXPORT --trims TRIMS --project PROJECT
   --import-report IMPORT_JSON --verify-report VERIFY_JSON --report STAGE_JSON`.
   Den bruker den gjennomgåtte `runtime_layout.json`, sjekker at kildefilene fortsatt
   matcher begge native rapportene og skriver `Content/MotelV2/assembly.json`
   atomisk. Tidligere metadata sikres ved omkjøring. Pakk og prøv
   kjøring, mottak/telefon, rom 6, TV-program og lagring i isolerte testprofiler.

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
