# Kjøreturer, visuell gjennomgang 6. oktober 2026

Begge avtalte turer er dokumentert i originalbilder. Manuell vurdering: funn nedenfor. Automatisk konsollkontroll: **FAIL**, ikke visuell PASS. Ingen `src/` eller eksisterende tester er endret.

## Grunnlag og dekning

- Pages: https://tombonator3000.github.io/SIGNAL-47/. Registrert kildegrunnlag: `77d62f5e1349ff2eeeec1f44b62112858262d95f`.
- Begge turer lastet `index-C0RfO_xx.js`, SHA256 `2a2db861479c10828779282b2037b0cae2740975fe440650a965f3d4bcc71cb1`. Lokale kilder er registrert separat; byteidentitet mellom kildene og det serverte bygget er UNVERIFIED.
- 1280×800, High, Picture Off, isolerte ferske nettleserkontekster, hold og autopilot 20 m/s. Faktiske bilder fra spillet, uten bildebehandling. Kontaktarkene er tydelig merkede oversiktskopier.
- Frontbilder ved hver passerte 400 m-grense, samt front og begge sider ved registrerte kryss, gjerdeåpninger, milestolper og dinerplass. Sidebilder bruker den eksisterende `S47.game.d.view.draw()` uten tick; truckposisjon, heading og klokke er kontrollert uendret og kameraet gjenopprettet.
- `cab_yaw_rad` er simulatorens hodevinkel; de fotograferte sidevinklene er dokumentert av `view_direction`. Sluttbildet ved dineren bruker spillerkameraet etter parkering.

| Tur | Originalbilder | Sluttpunkt | Klokke ved slutt | Sidekontroller |
| --- | ---: | --- | --- | ---: |
| saro_diner | 16 | diner parkert, road-z 2003.780 | 05:09:39.400 | 6 |
| diner_oldroad | 66 | mile 8.100033, før THE EVENT | 05:28:52.804 | 22 |

[Galleri](index.html), [alle bildemetadata](manifest.json), [originalbilde- og sidekontroller](artifact_checks.json).

## Samlet funnliste

Ingen P1 funnet i denne stillbildegjennomgangen. P3 er en subjektiv lesbarhetsvurdering. Hver rad viser den faktiske bilposisjonen og kameraets retning, ikke en interpolert mileposisjon.

| Prioritet | Tur og sted | World XYZ | Blikkretning XYZ | Bilde | Defekt |
| --- | --- | --- | --- | --- | --- |
| P2 D01 | diner_oldroad, mile 0.002561 | (7999.410255, 4.318435, 2002.040373) | (0.997578, -0.067081, -0.018398) | [diner_oldroad_003_highway_oldroad_crossroads_forward.png](diner_oldroad_003_highway_oldroad_crossroads_forward.png) | Sandfargede flater med skarpe polygonkanter bryter asfalten i begge kjørefelt rett etter riksveikrysset. |
| P2 D02 | saro_diner, road-z 505.766 | (7998.200000, -2.383651, 505.766452) | (0.999999, 0.001159, -0.000001) | [saro_diner_004_survey_junction_fence_opening_left.png](saro_diner_004_survey_junction_fence_opening_left.png) | Et sammenhengende lyseblått bånd foran de mørke mesaene gir en tydelig kunstig overgang ved horisonten. |
| P3 D03 | saro_diner, road-z 1951.120 | (7998.186064, 4.287680, 1951.119547) | (-0.002067, -0.073280, 0.997309) | [saro_diner_009_diner_approach_fence_opening_forward.png](saro_diner_009_diner_approach_fence_opening_forward.png) | Dineren framstår hovedsakelig som en bygningsbakside og en trailer i siste frontbilde før innkjøringen, og neonskiltet gir svak veivisning her. |

Ingen kodeforslag inngår i funnlisten.

## Rolige strekninger

Subjektivt er strekningen etter riksveikrysset og fram til rundt mile 5,5 rolig og ensartet: asfalt, gjerder og telefonlinje dominerer frontbildene, og milestolpene er de tydeligste avbrekkene. Rundt mile 5,7 begynner mørke steiner å bli tydelige. SARO-riksveien mellom åpningen og dineren har samme preg. Dette er en vurdering av de fangede stillbildene, ikke en kontroll av radiolyden eller et behov for nye assets; Claude avgjør eventuell ranch eller andre landemerker.

## Kontroller og avgrensninger

- Verktøy: seks dokumenterte målrettede kontroller PASS, inkludert eksisterende utmappe og brukerfil bevart, avgrenset turvalg og SIGTERM som beholder delrapport med eksplisitt FAIL. [Kontrollbevis](tool_checks.json). Tidligere grønne hjelpekontroller gjenbrukes; ingen ekstra gameplay-rekke kjørt.
- De fem registrerte runtime-kildefilene er byte-like mellom forsøkene og den leverte arbeidskopien. Verktøyet fikk diagnostikk og avgrenset retry mellom forsøkene; de ulike verktøyhashene står i metadata.
- SARO-kjøringen registrerte to 404-meldinger uten kilde-URL. Opphavet er UNVERIFIED. Oldroad-retry registrerte to 404-meldinger med URL `https://tombonator3000.github.io/favicon.ico`, og ingen øvrige konsollfeil eller mislykkede HTTP-svar.
- Første prosess endte med kode 143 etter fullført SARO og delvis oldroad til mile 0,742666. Årsak er UNVERIFIED. Bare den manglende oldroad-turen ble kjørt på nytt, med samme lastede inngangsfil. [Arkivnotat](diagnostics/README.md) skiller avbrutte råmetadata fra de leverte bildene.
- Alle original-PNG-er og sidekontroller er sjekket under pakking; kontaktark erstatter ikke originalene. Ingen nye rasterassets produsert.
- Stolpene inngår i frontbildene; et 90-graders sidebilde tatt før en stolpe viser ikke nødvendigvis selve stolpen. Ingen nye klare stolpe-, terrenggap- eller lysdefekter registrert utover funnlisten. Stillbilder frikjenner ikke flimrende z-fighting.
- SwiftShader ble brukt. Maskinvare-fps, manuell kjørefølelse og lyd på høyttalere er UNVERIFIED. C-linjen og THE EVENT ligger utenfor avtalt dekning.
- Avtalt gameplay-baseline gjenbrukes fra PR #72: på `7e894dd` drives 23/23 og chapter6 42/42; på `5fc1a17` diner 15/15, reachcheck alle fire områder PASS, chapter3 66/66, art 6/6, CSP PASS, Pages art 9/9 og drives 23/23. Dette er tidligere kontroller, ikke nye tester kjørt av drivelook.
