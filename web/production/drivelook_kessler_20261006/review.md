# Kessler: kjørebilder og avgrenset visuell vurdering

Seks av seks avtalte konfigurasjoner er ferdig: 234 originale PNG-er, 174 rutebilder og 60 stillstandsbilder fordelt på 30 A/B-par. **Ett P2-funn: Ultra viser harde, lyse sømmer langs fjernt terreng.** Ingen runtime-rettelse eller nye assets er gjort i denne leveransen.

Bygggrunnlag: main `eb09d86`, GitHub Pages-jobb [37433440104](https://github.com/Tombonator3000/SIGNAL-47/actions/runs/37433440104), artefakt `11397423376`. Det nedlastede, uendrede artefaktet ble servert lokalt fra `/SIGNAL-47/`. [build.json](build.json) registrerer arkivhash og alle 120 filhasher. Pakkekontrollen sammenligner hver lastet dokument-/kodefil med artefaktet, inkludert integrert OldRoad. Dette er ikke en ny live Pages- eller fysisk maskinvaretest.

| Konfigurasjon | Originale PNG | Rutebilder | A/B-par | Metadata/integritet |
| --- | ---: | ---: | ---: | --- |
| [low_844x390](derived/low_844x390/gallery.html) | 35 | 29 | 3 | 104/104 PASS |
| [high_844x390](derived/high_844x390/gallery.html) | 41 | 29 | 6 | 152/152 PASS |
| [ultra_844x390](derived/ultra_844x390/gallery.html) | 41 | 29 | 6 | 153/153 PASS |
| [low_1280x800](derived/low_1280x800/gallery.html) | 35 | 29 | 3 | 98/98 PASS |
| [high_1280x800](derived/high_1280x800/gallery.html) | 41 | 29 | 6 | 140/140 PASS |
| [ultra_1280x800](derived/ultra_1280x800/gallery.html) | 41 | 29 | 6 | 141/141 PASS |

## Én samlet funnliste

1. **K01 / P2: Ultra gir harde, lyse sømmer langs fjernt terreng og mesaer.** High har mørkere og mykere overganger ved identisk pose og klokke. Funn i både 844×390 og 1280×800, også med Ultra `picture=off`. Klarest ved faktisk mile **3.255981623**, venstre yaw **+1.2 rad**, kamera XYZ **(12676.117795, 10.136784, -265.380293)**, blikkretning og map-/world-XYZ i [review.json](review.json). Spillklokke **05:23:39.83**. [High/Ultra 844×390](derived/K01-horizon-844x390.png), [High/Ultra 1280×800](derived/K01-horizon-1280x800.png). Originale [Ultra 3.25, 844×390](ultra_844x390/kessler_017_mile3.25_left.png) og [1280×800](ultra_1280x800/kessler_017_mile3.25_left.png); ved 3.20 også [844×390](ultra_844x390/kessler_014_mile3.20_left.png) og [1280×800](ultra_1280x800/kessler_014_mile3.20_left.png). Omtrentlige pikselområder i 844×390-originalen: venstre mesa `(183,192)-(286,214)`, grus/gård `(367,184)-(430,202)`, fjernkant til høyre `(535,166)-(625,185)`. **Ansvar til Claude:** felles Ultra-post eller fjernterreng/materiale. Årsak **UNVERIFIED**; `core/vhs.ts:196` beholder Ultra AO/bloom ved picture off, så tapeeffekten alene forklarer ikke funnet. Ingen påvist modulfeil som krever egen Codex-rettelse. Etter eventuell rettelse: målrettet K01-sammenligning og én samlet integrasjonstest med Claude.

## Rute og opptaksmetode

Fersk isolert kontekst per konfigurasjon, `localStorage["s47.quality"]` som JSON før lasting. Low bruker spillerstandard off, High/Ultra VHS. Ultra var faktisk aktiv og støttet. Desktop-pointer og DPR1 i begge skjermstørrelser.

Kapittel 6 initialiseres med den eksisterende testkroken `S47.jump('chapter6')`, og opptaket venter på faktisk diner-plass og drive-stadium. Den ordinære overgangen fra forrige kapittel er ikke testet her. World-autopiloten kjører videre fra plassen på veiens faktiske rute, 20 m/s mål og 30 Hz driver; ingen `drive.place` under turen. Første opptak ved mile **2.903856515**, siste **3.504897334**. Frontbilder ved alle 24 terskler 40 til 960 m, pluss start/slutt; terskelkryssinger gir de faktiske posene, ikke interpolerte eller plasserte bilder. Etter siste bilde er det tre separate, uttrykkelig plasserte stillstandsfixturer 400/200/80 m før mile 3.2.

De tre venstreblikkene er tatt ved første kryssing av mile 3.15/3.20/3.25: **3.152782489**, **3.201346967**, **3.255981623**, spillklokker **05:23:33.08**, **05:23:36.26**, **05:23:39.83**. Blikket roteres +1.2 rad mens spillet holdes; bil, heading, klokke og kamera er kontrollert og gjenopprettet. Alle seks konfigurasjoner har samme rute-/sideposer og klokker. Det naturlige kapittelforløpet ga **05:23:16.80 til 05:23:56.12**, litt senere enn omtrent 05:22 i bestillingen. Klokka er ikke satt kunstig.

## Stillstand, differanser og begrensninger

Spillerstandardparene gjør ett faktisk scenesteg på **1/30 s**, med samme gjenopprettede kamera/bil og eksplisitte shader-tider t0 og t0+1/30. Den målte veggklokkeavstanden er større på programvarerendereren og registreres separat. Kapittel 6 kan klemme spillklokka i fixturen; spillklokkedelta er derfor ikke alltid 1/30 s. High/Ultra har i tillegg off-par uten tick og med identisk shader-tid som diagnostikk for statisk gjentakbarhet; Low bruker allerede off og dupliseres ikke.

Farm-/grus-/ferist-/skiltområdene beregnes fra projiserte modulvertexer. Rektanglene inkluderer bakgrunn og mulig skjuling av førerhuset. Utenfor skjermen eller under 8×8 piksler er **UNVERIFIED**. Målinger i [review.json](review.json) og originalmanifestene: endrede piksler ved største RGB-kanaldelta >0/>2/>4/>8, MAE, p95/p99 og maksimum. Tabellen viser >4-andel og maksimum for gård/grus; dette er **pikselvariasjon, ingen automatisk z-fighting- eller visuell PASS**.

| Konfigurasjon | Avstand | Par | Gård: >4 / maks | Grus: >4 / maks |
| --- | ---: | --- | --- | --- |
| low_844x390 | 400 m | player_default | 0.58% / 5 | 0.23% / 5 |
| low_844x390 | 200 m | player_default | 0.00% / 4 | 0.14% / 6 |
| low_844x390 | 80 m | player_default | 0.36% / 5 | 1.24% / 7 |
| high_844x390 | 400 m | player_default | 20.47% / 10 | 18.61% / 16 |
| high_844x390 | 400 m | picture_off_repeatability | 0.58% / 6 | 0.05% / 5 |
| high_844x390 | 200 m | player_default | 0.00% / 1 | 0.00% / 3 |
| high_844x390 | 200 m | picture_off_repeatability | 0.41% / 5 | 1.08% / 7 |
| high_844x390 | 80 m | player_default | 19.08% / 9 | 19.22% / 8 |
| high_844x390 | 80 m | picture_off_repeatability | 0.00% / 4 | 0.10% / 5 |
| ultra_844x390 | 400 m | player_default | 0.00% / 2 | 0.00% / 3 |
| ultra_844x390 | 400 m | picture_off_repeatability | 2.78% / 7 | 0.75% / 6 |
| ultra_844x390 | 200 m | player_default | 0.00% / 2 | 0.00% / 2 |
| ultra_844x390 | 200 m | picture_off_repeatability | 0.00% / 4 | 0.15% / 6 |
| ultra_844x390 | 80 m | player_default | 19.44% / 10 | 19.99% / 12 |
| ultra_844x390 | 80 m | picture_off_repeatability | 0.00% / 4 | 0.32% / 6 |
| low_1280x800 | 400 m | player_default | 2.90% / 6 | 0.55% / 6 |
| low_1280x800 | 200 m | player_default | 0.15% / 5 | 0.16% / 6 |
| low_1280x800 | 80 m | player_default | 1.21% / 7 | 2.29% / 7 |
| high_1280x800 | 400 m | player_default | 0.00% / 2 | 0.09% / 8 |
| high_1280x800 | 400 m | picture_off_repeatability | 6.60% / 8 | 1.00% / 7 |
| high_1280x800 | 200 m | player_default | 18.94% / 8 | 19.62% / 13 |
| high_1280x800 | 200 m | picture_off_repeatability | 0.57% / 6 | 0.39% / 7 |
| high_1280x800 | 80 m | player_default | 0.07% / 6 | 0.09% / 8 |
| high_1280x800 | 80 m | picture_off_repeatability | 0.34% / 5 | 0.29% / 6 |
| ultra_1280x800 | 400 m | player_default | 20.19% / 8 | 16.67% / 12 |
| ultra_1280x800 | 400 m | picture_off_repeatability | 3.43% / 7 | 0.52% / 6 |
| ultra_1280x800 | 200 m | player_default | 20.86% / 11 | 19.19% / 24 |
| ultra_1280x800 | 200 m | picture_off_repeatability | 0.65% / 6 | 0.64% / 7 |
| ultra_1280x800 | 80 m | player_default | 21.54% / 9 | 19.31% / 12 |
| ultra_1280x800 | 80 m | picture_off_repeatability | 2.70% / 7 | 5.94% / 9 |

Små, ikke-null differanser finnes også i enkelte off-par uten tick. Årsaken er UNVERIFIED; resultatet kalles ikke perfekt gjentakbarhet eller z-fighting. Tidsvariasjon i standardpar inkluderer sky/lys/scenesteg og posteffekter. Null endring ville heller ikke utelukke flimring under bevegelig kamera.

## Visuell dekning

- **Port, KESSLER-forside, gjerdeåpning og grusspor ved veien: MANUALLY_REVIEWED.** Sammenhengende i tilgjengelige rute-/sidebilder. Ingen separat P1-P3-feil observert. Low har forventet flatere kontrast.
- **Alle 156 frontbilder og 18 venstreblikk: CONTACT_SHEETS_REVIEWED.** Alle seks konfigurasjoners kontaktark gjennomgått, med originalbilder ved 3.20 og representative 3.15/3.25. Kontaktark er ikke full pikselinspeksjon av hvert originalbilde.
- **Stillstandspar og flimring: MEASURED_NOT_A_VISUAL_PASS.** 30 A/B-par; alle par på kontaktark, representative originale A/B og differanseplater inspisert. Numeriske endringer kan skyldes scene-/lys-/sky-/posteffekter og beviser ikke z-fighting.
- **Baksiden av KESSLER-skiltet: UNVERIFIED.** Det foreskrevne venstreblikket viser ikke en lesbar bakside. Ved 3.25 ligger porten bak utsnittet; ingen ekstra teleport-/kamerarute lagt til.
- **Gårdens fjerne veranda-, tank- og vindmøllefundament, skuldre og nordkappe: UNVERIFIED_AT_NEAR_RANGE.** Mørke, små eller delvis skjulte elementer i denne kjøreavstanden. Tidligere geometrikontroll er separat bevis, ikke nærvisuell PASS her.
- **Fysisk mobil, ekte fps, manuell kjøring, lyd og bevegelig z-fighting: UNVERIFIED.** Chromium med SwiftShader, desktop pointer, DPR1. 844x390 er et skjermutsnitt, ikke en fysisk mobiltest; stillbilder erstatter ikke video/manuell kjøring.

## Kontroll og gjenbruk

[Målrettet verktøykontroll](tool_checks.json): **7/7 PASS**, inkludert 40 m-overshoot, kjente pikselendringer/UNVERIFIED-ROI, nye kvalitetsvalg med gammel standard bevart, alle sju nettlesersnutter parsbare og byteidentisk PR #74-konsollpolicy. Konsollregresjon **11/11 PASS**. Bare URL-bekreftet eksakt `/favicon.ico` HTTP404 fra samme origin fritas; råmeldinger beholdes. De seks fullførte opptakene har ingen registrerte konsoll-/side-/HTTP-feil.

[Pakkekontrollen](packaging.json) verifiserer seks av seks, originalhasher/-mål, presets, ekte Ultra, rute-/fixturseparasjon, sidegjenoppretting, parenes tider, omberegnede pikselmålinger og lastede byggehasher. Originaler og råmanifest er uendret. De rå manifestenes generiske UNVERIFIED-flagg er beholdt; denne rapporten og pakkekontrollen er egne, etterfølgende vurderinger.

[Leveringskontrollen](delivery_checks.json) kontrollerer lokale gallerilenker, råbilde-/manifesthasher og at alle seks ruter og venstreblikk har identiske kamerastillinger og klokker. Den teller 788 av 788 metadata-/integritetskontroller; disse er ikke 788 gameplay-tester.

Den avtalte samlede integrasjonsrunden er allerede grønn på `8225bdc`: kapittel 6 **42/42**, drives **23/23**, art **6/6**, CSP, steercheck og typecheck; Pages-undermappe art **9/9** og drives **23/23**. Runtime, tester og byggeoppsett har ingen delta til fanget main `eb09d86`, dokumentert i build.json. Disse resultatene gjenbrukes, ikke kjørt på nytt eller omtalt som nye tester. Egen runtime er ikke endret. Claude eier én ny samlet runde dersom han retter K01 i runtime.

Første oppsettsforsøk feilet med ERR_CONNECTION_REFUSED fordi den lokale serveren var stoppet. Det historiske mislykkede forsøket er bevart i [diagnostics/connection_refused](diagnostics/connection_refused/index.html); serveren ble startet før alle seks vellykkede opptak. Ingen mislykket kjøring er ommerket.

Reproduksjon fra `web/` med aktivert Python-miljø med Playwright/Pillow og tilgjengelig Chromium. Kjør én konfigurasjon om gangen mot samme uendrede bygg; opptakets utmappe kan ikke eksistere på forhånd. Første kommando lager ett nytt opptak. Andre kommando verifiserer og pakker den leverte mappa med alle seks konfigurasjoner; den pakker ikke automatisk det nye enkeltopptaket:

```sh
S47_URL=http://127.0.0.1:8877/SIGNAL-47/ S47_CHROMIUM=/sti/til/chromium python tools/drivelook.py /ny/tom/evidensmappe 1280x800 --trip kessler --quality ultra
python production/drivelook_kessler_20261006/package_evidence.py production/drivelook_kessler_20261006 --require-all
```

Eid omfang: `tools/drivelook.py`, denne evidensmappa og eget UTC-avsnitt i log.md. Tool-SHA256 `b013332bda5d4eb8a48c8eb8181c1352c89c8ca2a03602e1619f64bce758e20a`. Ingen endring i spillruntime, eksisterende tester, art, Unity, read-only sources eller Voices of the Void-arkivet. Ingen merge eller publisering i denne leveransen.
