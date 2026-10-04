# Konsepter og lokasjonskart, 4. oktober 2026

Seks nye miljøkonsepter og fire kart til Three.js-versjonen av SIGNAL / 47. Alle bildene er lagret i repoet. Åpne [det lokale galleriet](src/assets/art/concept/index.html) for å se hele leveransen.

## Miljøkonsepter

| Fil under `src/assets/art/concept/` | Motiv | Status i Three.js |
| --- | --- | --- |
| `ch1_service_yard.jpg` | S-03 og hevet servicegang i natriumlys | Videre visuell retning for bygd område |
| `ch1_photolab.jpg` | Kompakt mørkerom med rødt sikkerhetslys | Videre visuell retning for bygd område |
| `saro_archive.jpg` | Lite arkiv med tre dokumentsamlinger | Foreslått senere interiør |
| `station01_field.jpg` | Feltbygg, transit, tre merker og kabel | Senere Three.js-område; Unity-referanse finnes |
| `sierra_court.jpg` | Motellkontor, parkering og overbygd gang | Forslag til spillbart område; bakgrunn finnes |
| `ch3_room47.jpg` | Vanlig motellrom med CRT-TV og koffert | Senere interiør; romnummer ikke fastlagt |

Filnavnet `ch3_room47.jpg` er beholdt fra ART_BRIEF.md. Det vedtar verken rom 47 eller kapittel 3. Designbibelen plasserer motellet i K4 og bruker rom 6; dette er fortsatt en åpen beslutning. Bildene har ikke innbakt romnummer.

Alle seks er 1920 × 1080 sRGB-JPEG. Generatorkildene er 1672 × 941 og er skalert til leveringsmålet. De er ikke native 1080p. Originale PNG-er er bevart ved genereringsverktøyets kildebaner. De eksporterte prosjektfilene og komplette prompts med referanse- og filhash ligger i repoet.

## Kart

Alle ligger i `src/assets/art/maps/`:

- `world-overview.jpg`: illustrert oversikt over SARO, STATION 01 og Sierra Motor Court, 2400 × 1350. Tegnede antenner er representative ikoner. Spillets 27 antenner er uendret. Generatorkilden er 1672 × 941. Skjematiske piler viser foreslåtte reiser og retur, ikke avstander eller en kjørbar veibane.
- `saro-plan.svg`: nærkart avledet fra dagens Three.js-kode. Nord er -Z. S-03-kabinettet og S-03-antennen er forskjellige punkter. B-12 er nord for kabinettet og sør for antennen. Arkivet står separat som uavklart utvidelse.
- `station01-plan.svg`: forslag til én hytte, transitflate, tre referansemerker og en kort gangsløyfe. Ingen ny by eller større anlegg.
- `motel-plan.svg`: forslag med kontor og Noras rom som de to interiørene. Andre rom og basseng er bakgrunn. Romnummeret er åpent.

Nærkartene er redigerbare SVG-er med PNG-kopier. SARO-kartet er avledet fra kode, ikke en ny kjørt kollisjonsmåling. De andre kartene viser foreslått blokkering. Det illustrerte oversiktskartet fastlegger ikke lokale fasonger; nærkartene er de lesbare planreferansene. Ingen av kartene er en geografisk oppmåling av New Mexico.

## Opphav og kontroll

Miljøbildene og oversiktskartet er laget med den innebygde ChatGPT-bildegeneratoren. Eksakt modellversjon er ikke eksponert. Ingen ekstern betalt bildegenerering er brukt. Nærkartene er laget som prosjektets egne SVG-diagrammer. Se `concept/room-concepts.json`, `concept/exterior-concepts.json`, `concept/additional-concepts.json` og `maps/map-spec.json` for prompts, kilder og kontrollgrunnlag.

Dette er konseptkunst og produksjonskart. Bildene er ikke spillbilder, spillerens fotografier eller ferdige spillområder. Lys, terreng og objekttetthet må tilpasses motoren ved senere implementering. Spillkode, konstanter, originale assets og Unity-arkivet er uendret.

PASS: sju JPEG-filer med dimensjoner, sRGB og samsvarende fil-/referansehash; tre SVG-er med XML- og PNG-kontroll; ti kartkildehasher og seks karteksporthasher. Alle bilder og kart er visuelt inspisert. Galleriet er kjørt i headless Chromium ved 1360×900 og 390×844: alle ti bilder lastet, ingen sidefeil, mislykkede forespørsler eller vannrett overflow. Se `evidence/concepts-2026-10-04/verification.json`.

Spillkode og runtimeassets er uendret. Tidligere beståtte spilltester er derfor ikke kjørt om igjen for denne konseptleveransen. Faktisk spilling på mobil og målt ytelse er fortsatt utenfor denne kontrollen.

## Nyere kildegrunnlag ved Claude-gjennomgang

Etter denne første leveransen ble `origin/ccr-30e38858-767d90` lest på `e5706fb`. Der er rom 6 og to slutter registrert som avklart, og et arkivtilbygg er kodet sør for kontrollrommet. Det tidligere forbeholdet om romnummer ovenfor er historikk. Et nytt `concept/ch4_room6.jpg` med to stoler, lampe og papirbevis er levert; den eldre filen er bevart. Galleriet viser den nye varianten. Nærkartet viser fortsatt det eldre main-grunnlaget `65ad59f`, ikke det nye arkivtilbygget. Se `CLAUDE_HANDOFF.md` for status og kilder.
