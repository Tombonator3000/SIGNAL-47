# WorldCase22 — testplan

Denne leveransen samler den tidligere selvstendige arkivprøven med hovedsaken: P04 dokumentert rapportendring, P05 begrunnet feltmål og lagring av framdrift. Områdekartet er konsept og produksjonsramme; det dokumenterer ikke ferdige gangruter eller implementert spilletid.

## Automatiske kontroller i faktisk Unity-spiller

`WorldCase22Checks` aktiveres kun med `--signal47-worldcase22-checks`. Profilen må angis med `--signal47-save-dir`, hete `worldcase22-test-*` og inneholde `ALLOW_WORLDCASE22_TEST`. Den skal være ny, og ha `Seed/case.json` fra et tidligere fullført v1-checkpoint samt kopier av de to tilhørende originalfotoene i `FieldPhotos/`. Brukerens aktive profil skal aldri brukes.

| Område | Hva som må påvises |
| --- | --- |
| Tilgang | Arkivet er utilgjengelig før den eksisterende saken er fullført; ugyldig side eller svar endrer ikke framdrift. |
| Eldre lagring | En historisk v1-sak kan fortsettes gjennom faktisk scenelasting; arkivframdrift starter uløst, mens avsluttet lokal sak og begge foto gjenopprettes. |
| P04 kildegrunnlag | Ingen slutning uten åpnet sammenligning og begge rapportversjoner. Leserekkefølgen kan varieres, og dokumentene kan gjenåpnes. |
| P04 begrunnelse | Påstand om skyld eller bare framkallingsfeil avvises med tilbakemelding; den dokumenterte utelatelsen av C godtas. Ingen motivpåstand låses inn. |
| P05 kausalitet | P04 og begge P05-kort kreves. Et stedsnavn alene, riktig sted med feil ID, trekant uten strek eller −39 LY som årstall er utilstrekkelig. Bare samsvarende sted, STATION 01 og trekant med strek godtas. |
| Avbryt og feilvalg | Lukking lagrer ikke et ubekreftet svar som løst; lesestatus beholdes og feil er reversible. Ingen automatisk reise ved åpning av dokument. |
| Lagring | Delvis lesing, fullført P04 og fullført P05 skrives av den ordinære checkpointmekanismen og gjenopprettes gjennom reell scenegjenlasting. Serialisering alene er ikke denne testen. |
| Tilstandsvalidering | Ukjent versjon, P05 uten P04 og inkonsistent kildegrunnlag avvises; fraværende utvidelse i eldre checkpoint godtas. |
| Regresjon | Opprinnelig kapittelsak og antall foto beholdes. Alle kopierte originalfoto har uendrede bytes etter hele forløpet. |

Testen bevarer rapport og uendrede skjermbilder i profilens `Evidence/`. PASS må komme fra faktisk kjøring av den aktuelle byggidentiteten; eksistensen av testkode er ikke bestått bevis.

## Leveringskontroller

1. Bygg med prosjektets fastsatte Unity-/URP-versjon. Kontroller byggresultat og runtime-feil, kildehash og at siste fungerende standardstarter er bevart.
2. Kjør den nye reisen direkte i kandidaten med isolert profil. Kjør også berørte Resources19-kontroller og Recovery15-regresjon i separate profiler.
3. Pakk identisk kilde/payload og pakk ut i fersk mappe. Verifiser manifest, runtimefiler, lisensfiler og bildehash. Kjør WorldCase22-reisen gjennom den utpakkede starteren.
4. Inspiser originale runtime-bilder av begge P04-kilder, sammenligning, begge P05-kilder, feil tilbakemelding og løst rutegrunnlag. Tekst skal være fullt tilgjengelig ved 1280 × 800; rulleområde må ikke skjule svarvalg.
5. Kjør én Unity-prosess av gangen med minne-/tidsgrenser og stopp ved gjentatte rendererfeil. Splat21/OpenGL-kombinasjonen skal ikke brukes; splats inngår ikke i dette spillet.

## Kontroller som fortsatt krever menneskelig gjennomføring

- Ekte tastatur/mus: gå til arkivpunktet, åpne/lukk, naviger alle dokumenter, velg svar og prøv Escape, pause og gjenåpning. API-kjøring beviser ikke dette.
- Ny leser uten fasit: begrunn P04/P05 med konkrete kilder, noter hint og tidsbruk. Ingen ny fakta skal gis av fasilitator. Dette er fortsatt en åpen blindtest.
- Vanlig gange, lydmiks og separat ytelsesmåling på kandidaten; korte skjermbildeprøver er ikke FPS- eller lydgodkjenning.
- Områdekartet må senere holdes opp mot faktisk skala, siktlinjer og gangtid i implementerte områder. Målet på 5–6 timer er ikke verifisert av denne leveransen.

## Utvidede kontroller utført i WorldCase22

118 kontroller inkluderer nå også ekte Previous Shifts-retur med nøyaktig sakstilstand, checksum-korrekt men ulovlig kombinasjon av originalsak og arkivframdrift, og fjerning av ett disponibelt fotofil-eksemplar fulgt av lagring og gjenoppretting under reparasjon. Originalfilene er bevart. Endelige 1280×800- og 1600×900-bilder er kontrollert; større oppløsning avdekket en tegnefeil i indeksmerket som ble rettet før siste bygg. Nøyaktige kjøringer og resterende åpne porter finnes i [verifikasjonen](Evidence/verification.json).
