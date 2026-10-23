# 1420 AM i Caliche

Original Blender-modell laget til SIGNAL / 47, 23. oktober 2026. Retningen er konseptarket `03-radiostudio-1420-am.png`: kremhvit puss, olivengrønn dør, studiovindu, metallbenk, analogt studio og varm treinnredning. Geometri og tolv små PBR-kart er laget spesielt for spillet. Ingen tredjepartsmodell eller tekstur er kopiert inn.

`1420_AM_station.blend` er Blender-kilden med to kameraer og prøvelys. `EDITABLE_SOURCE_enable_to_edit` inneholder de opprinnelige, navngitte komponentene. Slå av de synlige `SM_1420_*`-objektene før denne samlingen vises, slik at geometrien ikke vises dobbelt. Samlingen er skjult både for visning og rendering som standard. Preview-skilt, renderbakke, kameraer og prøvelys inngår ikke i spilleksportene.

`exports/` inneholder separate FBX-filer for skall, interiør, dør, glass og valgfri mast. `1420_AM_station.glb` inneholder hele modellen med PBR-materialer. `materials.json` beskriver materialfarger, metall, ruhet, emissjon og relative teksturstier for Unreal-importen. `textures/` inneholder original puss, valnøtt, bordplate og betong med farge, normal og ruhet. Glass har alpha 0,14 i Unreal-sidecar; Blender-forhåndsvisningen bruker fysisk glass.

Modellen omfatter konsoll med 14 kanaler og fem VU-instrumenter, spolebåndmaskin, mikrofonarm med kabel, telefon, kontorstol, platespiller, arkivhylle med plater, utstyrsrack, klokke, programlogg, manusark, kaffekopp og bordlampe. Lobbyen ved inngangsdøren er fysisk skilt fra det lukkede studioet med en tett vegg.

## Plassering

Alle mål er meter. Blender bruker Z opp. Lokal X går langs fasaden og lokal Y inn i bygget. FBX har `-Y` fremover og `Z` opp, på samme måte som prosjektets eksisterende Blender-import.

Felles spillpivot er Three.js-verden `[8025, 4.442166614532471, 2036]`. Lokal Blender-posisjon omregnes med `worldThree = (8025 + localY, floor + localZ, 2036 + localX)`. Fasaden ligger ved lokal Y = -5. Skallet dekker 12 meter langs gata og 10 meter inn på tomta. Byggets gulv er lokal Z = 0. Parapeten når 3,65 meter. TV-antennen er høyere og inngår i skallet.

Skall, interiør, glass og valgfri mast har normalisert pivot i hver FBX. Døren har eget hengselspivot og plasseres med lokal forskyvning `[-5.07, -4.91, 0.025]` fra felles pivot. Dørbladets lukkede sentrum svarer til opprinnelig gateposisjon Z = 2031,4. Det gamle studiovinduet beholder senter `[8019.975, floor+1.55, 2037.2]` og åpning 3,7 × 1,65 meter.

Behold eksisterende studiofotografi, anomaly-flater, ON AIR, takskilt og historiedrevne materialtilstander. Fysisk fasadetekst i Blender er bare forhåndsvisning, slik at den ikke dobler spillets skilt. Behold også spillets gamle 36-meters mast, barduner og flylys. Den valgfrie, separate mastmodellen er en 18-meters forfattervariant som ikke skal erstatte den automatisk.

Lobbyens foreslåtte gangflate er world X `[8020.32,8026.5]`, world Z `[2030.4,2032.8]`. Fysisk studiovegg ligger ved world Z = 2033, og bakvegg ved world X = 8027,4. Buzzer ligger ved world X = 8019,9, world Z = 2032,6 og høyde floor+1,35. Et grunt inngangstrinn og en liten rampe er modellert foran døren. Runtime-konfigurasjon, terrengtilpasning, kollisjon og interaksjoner håndteres av spillets integrasjon.

## Bygg og kontroll

Fra denne mappen:

```sh
blender -b --factory-startup --python build_station.py
blender -b --factory-startup --python verify_station.py
```

`model_contract.json` gir nøyaktige materialplasser, lokalgrenser og trekanttall. `verification.json` er en faktisk FBX-tilbakeimport som kontrollerer skala, pivot, geometri, materialer, normaler, teksturfiler og åpningene i fasaden. `axis_probe.fbx` er en separat basisprøve: boks med Blender-størrelse `[0.2,0.4,0.6]` ved `[2,4,6]`. Den skal brukes til å bekrefte Unreal-importens akser før runtimeplassering.

`exterior.png` og `interior.png` er Blender-renderinger og er ikke bevis for hvordan spillet kjører. Unreal-import, lobbydør og historiske fototilstander må prøves separat av integrasjonen.

## Triangulering etter første Unreal-import

Unreal fjernet 144 trekantflater med nøyaktig null areal fra det første interiøret. De kom fra sammenfallende bevel-hjørner på fem tynne VU-dialer (120 trekanter) og stolryggen (24 trekanter), etter at transformene var lagt inn i felles mesh. Dette er nå ryddet i Blender-kilden, FBX, GLB og generatoren. Interiøret har nøyaktig 43 268 trekanter. De andre delene har uendret antall: skall 4 952, dør 1 852, glass 12 og valgfri mast 1 264.

Alle eksportmesh er eksplisitt triangulert. `mesh_cleanup.json` dokumenterer identisk hash før og etter for hver synlig trekants koordinater, UV-er og materialplass. Bare flater med areal lik null er fjernet. Opprinnelige vertexposisjoner, materialrekkefølge, pivoter og den redigerbare kildesamlingen er bevart. Den oppdaterte tilbakeimporten består **32/32 kontroller**, med eksakte trekanttall i FBX og GLB. De eksisterende renderingene gjelder fortsatt den samme synlige overflaten.

`source_integrity.json` kontrollerer dessuten alle 1 136 redigerbare kildeobjekter mot Blender-filen før oppryddingen. Deres hash er identisk. Eksportenes corner-normaler er kopiert; Blenders lagring som custom normals gir en maksimal komponentforskjell på 0,000496. Koordinater, UV-er og materialer for de synlige flatene er nøyaktig uendret.
