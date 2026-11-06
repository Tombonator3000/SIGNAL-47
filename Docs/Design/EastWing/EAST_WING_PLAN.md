> **Status: Referanse etter samling 6. november 2026.** Gjeldende samordnet design står i [samlet game bible](../GAME_BIBLE.md). Innholdet under er bevart detaljgrunnlag med kildens opprinnelige dato; eldre tidslinje, status og arbeidsfordeling overstyrer ikke hovedbibelen eller Active.

# SARO østfløy: testlab, lager og verksted

**Status:** Under arbeid. **Gjelder:** Ny modell og innredning av SARO-østfløyen og flytting av Freeform-benken inn. **Sist kontrollert:** 3. november 2026.

Tom bestilte 3. november en detaljert ny modell av det ubrukte SARO-bygget ved skiltet etter seks konseptbilder (eksteriør, planløsning, testlab, garasje, kalibrering, delelager). Det skal være en god Blender-modell i Unreal, ikke lavpoly. Freeform-benken ute flyttes inn. Toms avgjørelser: benkens funksjoner fordeles på kalibrering/reparasjon og testlab; Jeepen skal kunne kjøres inn og lastes av i garasjen; bygget er åpent i Freeform og låst i Story med samme nye utside. Konseptbildene er visuelle mål, ikke bevis på implementert innhold.

## Fasit fra verden

Østfløyen ligger i nettverdenens SARO-område: x 20,2 til 39,0 (18,8 m), z -5,9 til 5,9 (11,8 m), tak 3,6 m (`web/src/world/Grounds.ts`, `dressSaro.ts`). Negativ z er nord. Sørfasaden vender mot personalparkeringen; SARO-skriften står på sørfasaden x 22,4 til 30,4. Serviceinngangen med måler er på østgavlen. Benken står i dag ute ved x 24, z 6,2 (`S47FreeformWorld.cpp`, `BenchAt`). Personalbilene står med nesen mot sørveggen ved x 27 og 31 og flyttes vestover så garasjeporten får fri forplass. Tom ga 3. november fritt valg til å slette den gamle bygningen og lage en helt ny modell. Valgt løsning: hele den eksporterte fløyen (skall, fasadebilde, takutstyr og veggdetaljer) pensjoneres presist som i `S47Office`, og en ny native modell står på samme fotavtrykk og plassering, så verdensankre, terreng og nabobygg beholdes.

## Romprogram (vest til øst)

| Rom | Omtrentlig plass | Innhold og spillfunksjon |
| --- | --- | --- |
| GENERAL TEST LAB | x 20,2 til 26,0, hele dybden | Laboratoriebenker med vask, mikroskop, sentrifuge, vekt, prøveglass, kjøleskap, plansjer. Prøveanalyse (`AnalyzeSample`) og undersøkelse av etterspor i sak 3. Personaldør og tak over døren i sørvest. |
| PARTS STORAGE | x 26,0 til 31,6, nordre halvdel | Stålhyller med merkede kasser (FUSES / RELAYS), kabeltromler, EQUIPMENT CHECKOUT og låsbart instrumentskap. Depot for sikringer og reléer; utlån av feltloggeren når sak 1 er konkludert. |
| CALIBRATION / REPAIR | x 26,0 til 31,6, søndre halvdel | Stålbenk med instrumenter, verktøyvegg, oscilloskop, multimeter, loddebolt, skuffeskap, kalibreringsintervaller. Relé- og donortest med måler og indikator (dagens benk). |
| GARAGE / WORKSHOP | x 31,6 til 39,0, hele dybden | Rulleport i sørfasaden, gule linjer, sluk, verktøyvegg, verktøyvogn, dekk, kjemikalieskap (låst), brannslukker. Jeepen kjøres inn og parkeres; last losses her. |

Dører: personaldør sørvest inn til testlab, testlab til kalibrering, kalibrering til garasje, delelager til kalibrering og garasje. Eksisterende serviceinngang på østgavlen beholdes. Vinduer på nordfasaden som i planen, bevart lesbarhet av SARO-skiltet på sør.

## Leveranser og eierskap

| Del | Eier | Merknad |
| --- | --- | --- |
| Kartlegging, blokkmodell, gåprøve, Jeep-klaring | Claude | Før detaljmodellering, med skjermbilde til Tom |
| Bygningsskall og interiørskall i Blender (bpy-kilde `unreal/ArtSource/EastWing/`) | Claude | Ekte veggtykkelse, LOD0/1/2, UCX-kollisjon, prosjektets eksportkontrakt |
| PBR-teksturer og detaljrekvisitter (Tripo/Blender) | Codex med agent-harness | Lisens kontrollert per asset; Tripo-utdata privat utenfor Git som TripoFleet |
| Innredning, lys koblet til strøm, dag/natt | Claude | Lookdev-pakken |
| Spillintegrasjon (`S47EastWing`, `S47FreeformWorld`, migrering av gamle lagringer) | Claude | Benk, depot, prøve, last, garasjeparkering; Story låst |
| Kontroll og levering | Claude | Bygg, selvtest, Freeform-prøver, evidensreise, før/etter dag/natt High/Low, PR, pakke, installasjon |

Ved øktslutt skriver Claude overlevering i `unreal/OPPGAVER.md` slik at Codex kan fortsette.

## Kartlegging 3. november (Claude, faktisk kjøring med `-S47EastWingDump`)

Nodeundersøkelsen i `S47EastWing::Dump` viser hva fløyen består av i nettverdenen:

| Node | Innhold | Behandling |
| --- | --- | --- |
| `house:wing` (geo 15) | Veggene x 20,6 til 38,6, z -5,5 til 5,5, y -0,6 til 3,6 | Pensjoneres; ny native bygning med samme ytterflater |
| geo 16 og `look:wingLetters` | SARO-skriften på sørfasaden x 22,4 til 30,4 | Pensjoneres; erstattes av konseptets grønne skiltplate |
| `house:roof` (geo 17) | Felles tak for flere bygg, x -6,7 til 39,0 | Beholdes som taktopp; ny takkant og interiørtak under |
| `dress/site/props` (geo 454/455) og geo 18 | Sammenslått tomtedressing: takkjølere, ventiler, takkant, nedløp, måler på østgavlen, tønner | Beholdes; ny bygning har nøyaktig samme veggplan så detaljene sitter riktig |

Spillsystemer som må endres for å kunne gå og kjøre inn:

- `S47World.cpp` SARO-utesoner (`L.Out`) holder spilleren ute av `{20.2, 39.0, -5.9, 5.9}`; erstattes av veggkollisjon med døråpninger.
- Gulv: bygninger ligger på y 0, forplassen på y -0,576. `AS47Props::Floors` støtter skråplan og brukes for rampe inn i garasjen; personaldøren får trinn.
- Kjøretøyets kollisjon og parkering i garasjen kontrolleres separat i kjøresystemet.
- Personalbilene sør for fløyen (x 27 og 31) og benken ved x 24 flyttes.

Modulen `S47EastWing` er opprettet (eies av `US47Main`, initialiseres etter kontoret i `S47HUD`). Neste steg er blokkmodellen: pensjonere de tre nodene, bygge vegger med åpninger, gulv, rampe og kollisjon, og ta før/etter-bilder.

## Fotorealisme i Unreal

Etter Toms lenke 3. november (MCP for Blender-artikkel om fotorealistiske renders): arbeidsrekkefølgen referanse, blokkmodell, form, modeller, materialer og nærkritikk følges. Artikkelens prosedyrale shader-slitasje følger ikke med FBX til Unreal og må derfor bakes til teksturer (lookdev-pakken) eller lages som Unreal-decals. Krav for østfløyen: fotobaserte CC0 PBR-teksturer (puss, lettbetong, betong), avfasede kanter på benker, skap, karmer og hjørner, slitasjelag (avskallet maling, støv, renner, oljeflekker, falmede og flassende etiketter), variasjon mellom like objekter, ekte lyskilder med riktig fargetemperatur, og kontroll med før/etter-bilder i spillet. Ingen kjøp av MCP Premium; Tripo finnes allerede.
