# SARO arkivrekvisitter

Originalt, avgrenset modellsett for det eksisterende spillbare arkivrommet i SIGNAL / 47. Ingen adgang til Dales kontor, nye fortellingstriggere eller endring av telefon/typewriter/radio inngår. Alt visuelt materiale og all geometri er eget arbeid. Geometriprimitivene og font-rasteriseringen gjenbrukes lesende fra prosjektets egne `substation/meshkit.py` og `substation/make_textures.py`. DejaVu brukes som systemfont, uten kopiert fontfil. Ingen tredjepartsmodeller eller eksterne teksturer inngår.

`design_contract.json` beskriver skrevet generator og integrasjonsplan. `model_contract.json` skrives først ved faktisk Blender-eksport med målte meshgrenser, materialrekkefølge og trekanttall. Ingen av dem alene beviser native import, spillbarhet eller visuelt resultat.

## Avgrenset modellsett

| Mesh under /Game/ArchiveProps | Three-anker i meter | Binding |
| --- | --- | --- |
| SM_ArchiveProps_ShelvingStorage | -5.78, 0, 8.65 | Statisk, annex.interior |
| SM_ArchiveProps_FilingCabinets | -4.46, 0, 10.17 | Statisk, annex.interior |
| SM_ArchiveProps_CardIndex | -1.25, 0, 7.95 | Statisk, annex.interior |
| SM_ArchiveProps_Bookcase | -2.35, 0, 10.30 | Statisk, annex.interior |
| SM_ArchiveProps_WorkTable | -3.65, 0, 8.60 | Statisk, eksisterende arbeidshøyde 0.77 m |
| SM_ArchiveProps_FieldBox | original gruppe -5.62, 1.135, 8.6 | Lokal 0 under annex.objs.fieldBoxModel |
| SM_ArchiveProps_FireDrawer | original gruppe -5.994, 1.27, 8.6 | Lokal 0 under annex.arc.drawer |
| SM_ArchiveProps_Reel047Box | original reel-lokasjon -0.16, -0.084, 0 | Lokal 0 under annex.arc.reel, eksisterende rotasjon beholdes |
| SM_ArchiveProps_Switchboard | -1.05, 1.62, 10.22 | Statisk under annex.arc.group |
| SM_ArchiveProps_CallRecorder | -1.12, 1, 10.22 | Statisk under annex.arc.group |
| SM_ArchiveProps_ServiceBinder | -2.32, 0.9, 10.24 | Statisk, Three yaw 0.25 rad |

Fire hyller med bokser, fem fysiske hylleplan, avstivet boltet ramme, brettede platekanter, originale arkivetiketter, tolv arkivskuffer, et eget vedlikeholdskartotek med åpnet skuff og synlige kort, bundne servicekopier, ekte bordhøyde og separat fallende STATION 01-boks gir komplette nær- og mellomavstandsmotiver. Et fritt område på midtre hylle gir plass til STATION 01-boksen og den eksisterende hemmelige brannskuffen. Apparatene har metallbezel, festehull, skruer, fysisk veggtrunking og papirrull. Pilotlampene og utskriftsstrimmelen tegnes fortsatt av den fungerende native logikken.

Alle delenes vertexdata ligger i lokalt pivotrom. `site_world_three_m` er plasseringspunktet, ikke et ekstra mesh-offset. `local=true` betyr at runtime skal bruke `at=[0,0,0]` under angitt håndtak. Ikke legg `site_world_three_m` til på nytt. Sourcepreview stiller delene i deres opprinnelige verdensposisjoner. Blender bruker meter og Z opp: `(bx,by,bz)=(three.x,-three.z,three.y)`. Den etablerte FBX-importen med MATCH_UP_FORWARD_AXES og import-yaw90 gir UE `(100bx,-100by,100bz)` cm. Ingen ekstra aksekonvertering skal bakes inn i meshene.

## Eksport, bare i rootens vaktvindu

1. Kjør `make_textures.py` med prosjektets Python, Pillow og NumPy.
2. Kjør Blender bakgrunnsmodus på `build_archive.py`, maksimalt to tråder og under prosjektets owned guard.
3. Importer `exports/SM_*.fbx` til `/Game/ArchiveProps` og LOD1/2 som eksplisitte LOD-er. Hver av alle tre LOD-er har også binær GLB.
4. Importer og mål `axis_probe.fbx` før plassering. Sentrum skal være UE 200,-400,600 cm og dimensjon 20,40,60 cm.

`SARO_Archive.blend` har pakkede bilder, relative teksturbaner, navngitte komponentgrupper og en skjult redigerbar originalsamling. `SARO_Archive.glb` er samlet backup. Fem deterministiske PBR-materialfamilier har 1024 px grunnfarge og 512 px DirectX-normal/ruhet; originalt skiltatlas er 2048 px. Blender-materialet korrigerer normalenes grønnkanal, mens native importer skal beholde DirectX uten ny inversjon. Ingen emisjonsmaterialer inngår: native lampetilstand er eneste autoritet.

## Selektiv utskifting, aldri hele annex.interior

Alle retire-regler avgrenses til `annex.interior`, selv når et nodenavn også finnes i andre scener. Skjul gammel visuell primitiv med HiddenInGame og fjern dens skygge, ikke slett node, proxy, hull, rektangel, parentgruppe eller historisk tilstand.

- Hyller: g361/g362 kan byttes samlet. G378 og g379 er gammel arkivboksdekor og etiketter. De faktiske instansene `lic:archive_box` og `lic:archive_box_lid` under annex.interior byttes med de egne nye kartongene. Ikke skjul samme navn i andre rom.
- Arkivskap: g335 til g338 er fallbackskap, g401 den lisensierte samlemodellen. Alle ligger bare i dette skapsettet.
- Kartotek: g329 til g332, under gammel gruppe node505. Behold `annex.objs.lineage`.
- Bokhylle/bindere: g363 til g367. Servicefolder er separat g327/g328.
- Bord: g369 er bordplaten. G370 er sammenslått stål fra flere møbler og korridordeler. Trim bare trekanter innen Three x[-4.51,-2.79], y[0,0.73], z[8.17,9.03]; aldri pensjoner hele g370. Ikke fjern lampe, papir/blotter g371-g374, stol eller arkivtelefon. Eksisterende typewriter/telefon står på platehøyde 0.77 og beholdes.
- STATION 01-boks: `replaces=[annex.objs.fieldBoxModel]` og lokal erstatning under samme gruppe. Animasjonen bruker gruppen, ikke ny separat verdensmodell.
- FireDrawer: pensjoner bare g383 til g386. Ikke bruk rekursiv `replaces=annex.arc.drawer`, som også skjuler reel/proxybarn. Ny lokal mesh følger original X-bevegelse 0.3 m.
- Reel047Box: lokal erstatning av gammel reel-visuell gruppe med `replaces=[annex.arc.reel]`. Behold selve gruppen og dens eksisterende vinkel/visibility. Den nye modellen inneholder samme identifikator og dato.
- Bryterbord: pensjoner g389/g390/g393, men behold g391/g392 og håndtakene `annex.arc.lamps.trk1/trk2`. De nye bezelene har åpne midter, og de første to linsene finnes bare i native geometri.
- Samtaleopptaker: pensjoner g395 til g398. Behold g400, `annex.arc.strip`, alle treffproxies og native generering av tekstur, lengde og krumning. Papirets innløp er Three(-1.218,1.07,10.22). Nytt skall ligger bak dette planet.
- G376 er blandet romdekor. Det skal ikke fjernes samlet. Eventuell restdekor må klippes i konkrete møbelvolumer først etter visuell inspeksjon. G377 er romsmuss og beholdes. Spillerelevant fallpapir g387/g388 beholdes under `annex.arc.papers`.

Gamle sceneindekser og geo-ID-er er kontrollert mot gjeldende CORE-manifest og `Signal47-web/geo` 25. oktober 2026. Integrasjon må kontrollere at ID, parent og plassering fortsatt stemmer, ikke anta at framtidige eksporter har samme indeks.

## Avgrenset native kontroll

Først ett stillbilde i arkivrommet og ett nærbilde av KSU/opptaker. Eksisterende scenario `-S47Jump=chapter2 -S47Face=at:-3.5;9.6;1.5708;0 -S47Archive=lift` skal fortsatt nå loggmarkøren `S47: archive phase line` (Scripts/screenshots.sh). Det dekker en eksisterende historietilstand, ikke fysisk Xbox-input.

Kontroller deretter den faktiske hendelsen med fallende boks, begge pilotlampene, papirstrimmel, uttrukket skuff, tagning av 047 og lagring/lasting. Kontroller at det vanlige dokumentbordet fortsatt kan åpnes, og at mat kan settes på samme plate uten overlapp. Test Dales kontordør som fortsatt låst. Ingen ny full PSO-/110-scenariokø trengs bare for modellsettet; root eier passende kort regresjon og rapporterer faktisk utførte kontroller.

Bibliotekrutinen ble fulgt med Blender/Unreal/interiør og ett bredere Blender-søk. Den eneste relevante profilen var en historisk MCP-kandidat uten verifisert modell-/eksportløype; ingen løsning fra denne ble gjenbrukt. Det etablerte prosjektets egne geometri-/PBR-primitiver er brukt lesende.
