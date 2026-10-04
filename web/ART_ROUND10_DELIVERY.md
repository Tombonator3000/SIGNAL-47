# Runde 10: papirene og avisbildet

Fire tekstfrie bilder er levert etter `ART_BRIEF.md` på main `2de9947`. Claude eier innkobling, tekst, historiefiler og én samlet integrasjonstest. Ingen runtimefiler, himmel, eksisterende fargebilder, runde 9-kart eller Unity-filer er endret.

| Fil under `src/assets/art/docs/` | Mål | Byte | Resultat |
|---|---|---:|---|
| `photo_1947_master.jpg` | 2400×1600 | 1 105 625 | PASS |
| `poster_halley_1986_blank.jpg` | 1024×1536 | 293 045 | PASS |
| `fanfold_1986.jpg` | 1024×1186 | 117 845 | PASS |
| `fanfold_1947.jpg` | 1024×1186 | 220 593 | PASS |

Alle er JPEG85, RGB uten alfa, med innebygd og kontrollert sRGB-profil. Masterens største kanalavvik er 0, og den er under 1,2 MB.

## Ett foto, tre utsnitt

Samme uforanderlige master brukes ved alle tre visninger. Claude skjærer ut med `drawImage`; egne genererte utsnitt skal ikke brukes.

| Utsnitt | x, y, bredde, høyde | Visuell kontroll |
|---|---|---|
| A | 600, 300, 960, 720 | Lys, kort komethale og mesatopp. Ingen personer. |
| B | 0, 220, 1840, 1380 | To ranchere. Ingen landmålere, stativbein, stang, skygge eller bevegelsesuskarphet. |
| C | 0, 0, 2400, 1600 | Alle fem. Tre landmålere helt til høyre; tredje er uskarp med lys bryststripe. |

Lys rundt (899, 412), komet rundt (1375, 837), flat mesatopp rundt y 971. Landmålernes venstre synlige avtrykk ligger rundt x 1940, med margin til både sikkerhetsstripen ved 1900 og B-kanten ved 1840. Ingen ansikter kan kjennes igjen. Fotoet har grovt avisraster; det er et oppdiktet historisk bilde, ikke en ekte aviskilde.

`production/round10_qa/photo_crop_A.png`, B og C er pikselidentiske med faktisk dekodet JPEG beskåret etter rektanglene. Hele høyden av Bs høyre 240-pikselstripe er vist 2× i `photo_B_right_edge_zoom.png`. Begge forhold er uavhengig kontrollert.

## Plakat og papir

Plakatens kjerne ligger rundt (680, 640), mesatoppen rundt y 924 og den tomme kremboksen rundt (71, 1011) til (952, 1288), med svak horisont y 1240. Tekstsonene er rolige; hovedhalen unngår lappfeltet. Prøveteksten er hentet fra gjeldende `KAPITLER.md`. Full prøve er lesbar uten overlapp. Ved 256×384 er tittel og motiv tydelige; lappens og kartets småtekster må leses i dokumentvisningen. Dette er QA-oppsett, ikke Claudes endelige tekstlayout.

Begge fanfoldark har 22 hull per side, samme radplassering, perforering rundt x 54/970 og bretter øverst/nederst. 1947-arket har ett revnet hull på venstresiden, rad 4, gulbrunt papir, falmede striper og små rustflekker. Prøvetekst fra `BINDER.page` er lesbar på begge.

Den brede, automatiske hulldiagnostikken er **UNVERIFIED** på gammelt papir fordi mørke aldringsflekker forskyver terskelmålingen. Dette er beholdt i rapporten. Uavhengig måling av mørke kjerner ved tre faste lave terskler finner 22 på alle fire sider, uten tilpasning til bestilt antall. Største radavvik mellom arkene er 0,586 px. Største x-avvik er 3,769 px ved det revne hullet. Visuell hull-/alderskontroll er PASS; en terskelmåling beviser ikke fysisk hullform.

## Opphav og reproduksjon

Rasterbildene er laget med den innebygde ChatGPT-bildegeneratoren. Eksakt modellversjon ble ikke eksponert. Ingen ekstern bildetjeneste eller bildebibliotek er brukt, og bildene hevdes ikke å være CC0. Første kandidater fikk feil motivplassering og hullantall; endelige foto, plakat og nytt papir bruker derfor de lagrede måltegningene som eneste komposisjonsreferanse. Det gamle papiret er en aldring av den endelige nye papiroriginalen.

Originale genererte PNG-er, tekniske måltegninger, alle promter, frosset sRGB-profil og eksportposter ligger i `production/round10_qa/sources/`. Fotoet ble generert i 1536×1024 og fanfold i 1165×1350. Bare måltilpasning med Lanczos, gråtonekonvertering av foto, sRGB/JPEG-eksport er gjort etterpå. Ingen motivretusj er gjort med skriptene. Plakaten ble generert i sluttmålet.

`production/round10_manifest.json` fører SHA-256 for sluttbilder, kilder, referanser, promter, verktøy og QA. De fire eksportene kan kjøres igjen fra `web/`:

```sh
python3 tools/round10_export.py photo_1947_master src/assets/art/production/round10_qa/sources/photo_1947_master_generated.png
python3 tools/round10_export.py poster_halley_1986_blank src/assets/art/production/round10_qa/sources/poster_halley_1986_blank_generated.png
python3 tools/round10_export.py fanfold_1986 src/assets/art/production/round10_qa/sources/fanfold_1986_generated.png
python3 tools/round10_export.py fanfold_1947 src/assets/art/production/round10_qa/sources/fanfold_1947_generated.png
python3 tools/round10_qa.py
```

Format, ICC, gråtone, filstørrelse, prøveoppsett og uendrede inputhash: PASS. Visuell inspeksjon og uavhengig teknisk kontroll: PASS. Verktøyets positive/negative metadata-, ICC-, gråtone-, tekst- og rektangelkontroller: PASS. Alle fire JPEG-er er reprodusert byteidentisk fra lagrede originaler. Privat nettlesergalleri laster 12/12 slutt-/kontrollbilder uten fangede konsolladvarsler eller feil; skjermbilde og innlastingsrapport ligger i `round10_qa/`. Dette er bildevisning, ikke spillintegrasjon. Det automatiske skriptet hevder ikke at motivsemantikk er maskinverifisert; `manual_review.json` fører den separate inspeksjonen.

## Til Claude

Legg de fire bildene til som senere bilder, aldri i startgruppen. Bruk plakaten under tekst i `halleyPosterTex`, med dagens reserve; fanfold under permateksten; masterens B i dineren. A og C er klare for senere kapittelarbeid. Milestolper, oppmålingsbolt og endringer i runde 9 er ikke laget.

Legg opphavet i `THIRD_PARTY_NOTICES.md` ved integrasjonen: «Runde 10: fire originale, oppdiktede dokumentbilder laget for SIGNAL / 47 med innebygd ChatGPT-bildegenerator; tekniske komposisjonsguider og mål-/sRGB-/JPEG-eksport ved Codex. Ingen ekte avis, personer, merkenavn eller eksternt bildearkiv. Promter og originale kilder i production/round10_qa/sources; eksakt modellversjon ikke eksponert; ikke hevdet CC0.» Denne fila eies fortsatt av Claude og er ikke redigert her.

Én samlet integrasjonstest med artcheck, CSP og Pages fra undermappe er avtalt med Claude. Faktisk bildeinnlasting, tekstlayout/cropping i spillet, fysisk PC-fps og ekte mobilinput er **UNVERIFIED** i denne assetleveransen. Forrige uendrede grønne spilltestrekke er gjenbrukt; ingen overlappende full testrekke er kjørt av Codex.
