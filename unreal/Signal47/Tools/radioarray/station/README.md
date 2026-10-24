# 1420 AM i Caliche

Original Blender-modell laget til SIGNAL / 47, 23. oktober 2026. Retningen er konseptarket `03-radiostudio-1420-am.png`: kremhvit puss, olivengrønn dør, studiovindu, metallbenk, analogt studio og varm treinnredning. Geometri og egne PBR-kart i flere skalaer er laget spesielt for spillet. Ingen tredjepartsmodell eller tekstur er kopiert inn.

`1420_AM_station.blend` er Blender-kilden med to kameraer og prøvelys. `EDITABLE_SOURCE_enable_to_edit` inneholder de opprinnelige, navngitte komponentene. Slå av de synlige `SM_1420_*`-objektene før denne samlingen vises, slik at geometrien ikke vises dobbelt. Samlingen er skjult både for visning og rendering som standard. Preview-skilt, renderbakke, kameraer og prøvelys inngår ikke i spilleksportene.

`exports/` inneholder separate FBX-filer for skall, interiør, dør, glass og valgfri mast. `1420_AM_station.glb` inneholder hele modellen med PBR-materialer. `materials.json` beskriver materialfarger, metall, ruhet, emissjon og relative teksturstier for Unreal-importen. `textures/` inneholder original puss, valnøtt, bordplate og betong med farge, normal og ruhet. Glass har alpha 0,14 i Unreal-sidecar; Blender-forhåndsvisningen bruker fysisk glass.

Modellen omfatter konsoll med 14 kanaler og fem VU-instrumenter, spolebåndmaskin, mikrofonarm med kabel, telefon, kontorstol, platespiller, arkivhylle med plater, utstyrsrack, klokke, programlogg, manusark, kaffekopp og bordlampe. Lobbyen ved inngangsdøren er fysisk skilt fra det lukkede studioet med en tett vegg.

## Plassering

Alle mål er meter. Blender bruker Z opp. Lokal X går langs fasaden og lokal Y inn i bygget. FBX har `-Y` fremover og `Z` opp, på samme måte som prosjektets eksisterende Blender-import.

Felles spillpivot er Three.js-verden `[8025, 4.442166614532471, 2036]`. Lokal Blender-posisjon omregnes med `worldThree = (8025 + localY, floor + localZ, 2036 + localX)`. Fasaden ligger ved lokal Y = -5. Skallet dekker 12 meter langs gata og 10 meter inn på tomta. Byggets gulv er lokal Z = 0. Parapeten når 3,65 meter. TV-antennen er høyere og inngår i skallet.

Skall, interiør, glass og valgfri mast har normalisert pivot i hver FBX. Døren har eget hengselspivot og plasseres med lokal forskyvning `[-5.07, -4.91, 0.025]` fra felles pivot. Dørbladets lukkede sentrum svarer til opprinnelig gateposisjon Z = 2031,4. Det gamle studiovinduet beholder senter `[8019.975, floor+1.55, 2037.2]` og åpning 3,7 × 1,65 meter.

Behold eksisterende studiofotografi, anomaly-flater, ON AIR og historiedrevne materialtilstander. R2 har stor malt 1420 AM direkte i fasadens opake PBR-kart. Det lille tidligere takskiltet skal skjules av native-integrasjonen, slik at identiteten ikke dobles. Behold også spillets gamle 36-meters mast, barduner og flylys. Den valgfrie, separate mastmodellen er en 18-meters forfattervariant som ikke skal erstatte den automatisk.

Lobbyens foreslåtte gangflate er world X `[8020.32,8026.5]`, world Z `[2030.4,2032.8]`. Fysisk studiovegg ligger ved world Z = 2033, og bakvegg ved world X = 8027,4. Buzzer ligger ved world X = 8019,9, world Z = 2032,6 og høyde floor+1,35. Et grunt inngangstrinn og en liten rampe er modellert foran døren. Runtime-konfigurasjon, terrengtilpasning, kollisjon og interaksjoner håndteres av spillets integrasjon.

## Bygg og kontroll

Fra denne mappen:

```sh
python3 run_guarded.py build
python3 run_guarded.py verify
```

`model_contract.json` gir nøyaktige materialplasser, lokalgrenser og trekanttall. `verification.json` er en faktisk FBX-tilbakeimport som kontrollerer skala, pivot, geometri, materialer, normaler, teksturfiler og åpningene i fasaden. `axis_probe.fbx` er en separat basisprøve: boks med Blender-størrelse `[0.2,0.4,0.6]` ved `[2,4,6]`. Den skal brukes til å bekrefte Unreal-importens akser før runtimeplassering.

`exterior.png` og `interior.png` er Blender-renderinger og er ikke bevis for hvordan spillet kjører. Unreal-import, lobbydør og historiske fototilstander må prøves separat av integrasjonen.

## Triangulering etter første Unreal-import

Unreal fjernet 144 trekantflater med nøyaktig null areal fra det første interiøret. De kom fra sammenfallende bevel-hjørner på fem tynne VU-dialer (120 trekanter) og stolryggen (24 trekanter), etter at transformene var lagt inn i felles mesh. Dette er nå ryddet i Blender-kilden, FBX, GLB og generatoren. R1 hadde deretter 43 268 interiørtrekanter og 4 952 skalltrekanter. R2 utvider synlig geometri til tallene nedenfor; samme opprydding brukes.

Alle eksportmesh er eksplisitt triangulert. `mesh_cleanup.json` dokumenterer identisk hash før og etter for hver synlig trekants koordinater, UV-er og materialplass. Bare flater med areal lik null er fjernet. Opprinnelige vertexposisjoner, materialrekkefølge, pivoter og den redigerbare kildesamlingen er bevart. Den oppdaterte tilbakeimporten består **33/33 kontroller**, med eksakte trekanttall i FBX og GLB.

`source_integrity.json` er historisk R1-bevis for alle 1 136 redigerbare kildeobjekter før oppryddingen. R2 har nye navngitte komponenter og materialeendringer, og omfattes av den nye eksportkontrollen. Eksportenes corner-normaler er kopiert; Blenders lagring som custom normals gir en maksimal komponentforskjell på 0,000496. Koordinater, UV-er og materialer for de synlige flatene er nøyaktig uendret.

## R2, 24. oktober 2026

Kilderevisjon `R2-concept-match-2026-10-24` følger konseptets store malte fasadebokstaver, mørkt oliventreverk og brukte analoge arbeidsrom. Fasaden har et unikt 2048 × 1024 kart med plassbestemte pussavskallinger, hårfine sprekker, dryppspor og smuss ved bakken. Separate 1024 × 1024 kart gir flerskala puss, uregelmessig valnøtt, slitt bordplate og oliventreverk. Kartene bygger på deterministiske verdi-/kornfelt uten trigonometriske bånd. Egne DirectX- og OpenGL-normaler følger med. Alle native UV- og normalstyrker står i `materials.json`.

Dypere vindusomramming, synlige pusskanter og små reparasjoner gir fasaden mer relieff. En grunn arbeidsplate nær vinduet bærer uregelmessige cue-ark, plater, båndesker og hodetelefoner. Telefonen er flyttet til denne arbeidsflaten. Konsoll, hovedmikrofon og de historiedrevne koblingenes plassering er bevart. Ingen figur er lagt til.

| Eksportdel | Trekanter | Materialplasser |
| --- | ---: | ---: |
| Shell | 9 972 | 12 |
| Interior | 50 520 | 15 |
| Door | 1 852 | 5 |
| Glass | 12 | 1 |
| Mast | 1 264 | 3 |

Alle fem delers eksakte lokalgrenser og pivoter er identiske med R1. Byggets fotavtrykk, vindus- og døråpning og gangsoner er uendret. `revision2-contract-diff.json` dokumenterer forskjellen. `r2_material_refinement.json` dokumenterer identisk mesh-, UV-, materialplass- og transformhash ved siste teksturpakking. FBX/GLB har eksplisitt triangulering og null eksakt nullarealflater.

`r2-exterior.png`, `r2-interior.png` og `r2-street-window.png` viser første R2-kildepreview. Bokstavslitasjen er etter disse bildene gjort finere og mer sparsom; atlaset og eksportpakken er autoritative. Blender-glasset bruker fysisk transmisjon og reflekterer kraftigere enn native-glassets alpha 0,14. Kildepreviewen viser derfor ikke den endelige lesbarheten gjennom vinduet. Native lys, glass og fototilstander må vurderes i den pakkede runtimekandidaten.

Bygg, tilbakeimport og preview kjøres sekvensielt gjennom prosjektets guard med maksimalt to Blender-tråder. `verification/` inneholder faktiske kommandologger og guardbevis. Prosjektbiblioteket er søkt; ingen ekstern modell, tekstur eller kode fra treffene er gjenbrukt.
