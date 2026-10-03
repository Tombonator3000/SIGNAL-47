# memory.md: hva prosjektet vet

Fast kunnskap og beslutninger for nettversjonen. Oppdateres når noe endres. Nyeste beslutning står øverst i hver seksjon.

## Beslutninger

- 2026-10-03: web/ ble lagt inn av Claude Code (commit `3dd5b37` på grenen `ccr-30e38858-767d90`) og føres til main gjennom [PR #24](https://github.com/Tombonator3000/SIGNAL-47/pull/24), fordi Tom ba om PR og merge. Grenen `web/threejs-prologue` som chatloggen nevner, ble aldri pushet.
- 2026-10-03: Spillet bygges videre i three.js i `web/` i samme repo (Tombonator3000/SIGNAL-47). Unity-prosjektet blir liggende som arkiv. Vi tar med design, historie og konstanter, ikke kode.
- 2026-10-03: Claude er lead og koder. ChatGPT lager 2D-grafikk etter `ART_BRIEF.md`. Tom bestemmer og tester.
- 2026-10-03: Stilisert lavpoly etter Toms referansebilder: natt, Melkeveien, oransje natriumlys, grønne CRT-er, røde blinklys på antennene, neon fra Sierra Motor Court.
- 2026-10-03: Én selvstendig HTML-fil (`npm run build:single`) er leveringsformatet for mobiltesting.
- 2026-10-03: Grafikknivå High (MeshStandardMaterial) og Low (MeshLambertMaterial-kopier, pikselforhold maks 1). Valget lagres.

## Kanon (må ikke endres uten beslutning)

- Sted og tid: SARO, Southwest Astronomical Research Observatory, New Mexico, 1986. Skiftet starter 23:41.
- Kollega: Dale har lagt igjen skiftloggen ("Don't break anything. D."). Unity-versjonen signerte "R.", webversjonen bruker Dale.
- Mottaker: RX bank 3 må slås på med spaken på racket.
- Signaltrinn (portert 1:1 fra Unity `SignalProfile`):
  - Kalibrering: 1419.900 MHz (±0.025), gain 45 til 65, BW 34 til 62, az 35 til 49.
  - Interferens: 1420.110 MHz (±0.03), gain 40 til 72, BW 4 til 22, az fritt.
  - Anomali: 1420.405 MHz (±0.008), gain 78 til 100, BW 4 til 14, az 79 til 87.
  - Kvalitet: exp(-(f² + g² + b² + a²)) som i Unity `SignalFeedback.Quality`.
- Tidshopp til 02:13. Residualen dukker opp 02:13:47.
- Pulsmønster: fire pulser, pause, sju pulser (4 / 7), gjentas hvert 5,2 sekund.
- Retningsløsning: RA 05h 17m 32s, DEC -05° 23' 14", S/N 4.71, SOURCE DISTANCE -39 LY.
- Telefonen: framtidsopptak av kontrollrommet (romlyd, skriver, dunk, gisp, keramikk som knuses). Etter at linjen dør går det nøyaktig 47 sekunder til smellet, koppen faller og knuses.
- Antennene: 27 stykk, S-01 til S-27. Etter smellet snur alle samtidig til az 026, el 32, uten styrekommando.
- Neste kapittel: S-03 i servicegården. Planlagt 042°, enkoder 026°, commands received 0.
- Hele historien med tre slutter (Silence, Answer, Listen) står i ChatGPT-samtalen "Utvikle spillområde visuelt" og i Unity-repoets `Docs/DesignBible13`.

## Referansebilder

Tom la ved disse bildene da web/ ble lagt inn 3. oktober. De ligger ikke i repoet.

- Seks SARO-bilder: motellskiltet ved arrayet, kontrollrommet, skrivebordet med feltkamera og S-03-profilen (planlagt 042, enkoder 026, commands received 0, neste spor Sierra Motor Court), servicegården ved S-03 (AZIMUTH 026, NO CONTROL COMMAND RECORDED), konsollen med SIGNAL LOCK og EVENT IN 00:00:47, og spektrumanalysatoren med 1419.900 CAL, 1420.110 RFI, 1420.405 ANOMALY og -39.0 LY.
- Klokkeslett, SNR, asimut og repetisjonstid i bildene avviker fra kanon over (for eksempel 03:14:27, SNR 12.4 dB og "repeats every 47s"). Kanon gjelder, bildene er stemning og stil.
- Et skjermbilde fra et annet spill: håndskrevet dagbok til venstre og ren transkripsjon til høyre. Dokumentvisningen i `UI.document()` har samme oppsett.

## Arkitektur

- `src/main.ts`: oppstart, step/draw-løkke, menyer, adaptiv oppløsning, testkroker på `window.S47` (hold, tick, jump, setQuality, game, room, ext, player, renderer, scene).
- `src/story/Prologue.ts`: faser i rekkefølge intro, shift, survey, skip, residual, locked, solving, printing, printed, ringing, call, countdown, event, turning, end. Alle forsinkelser bruker spilltid (`after()`), så pause og omstart virker.
- `src/world/kit.ts`: materialer, byggeklosser, statisk sammenslåing, falske natriumflomlys (`addFlood`, `floodlit`).
- `src/world/Dish.ts`: antenne med asimut- og elevasjonsledd. lod 1 for fjerne antenner.
- `src/core/quality.ts`: bytte mellom High og Low. Kode som setter materialer under spillet skal bruke `materialFor()`.
- Sjekkpunkt: `localStorage` nøkkel `s47.checkpoint`, settes til `residual` ved 02:13.

## Kjente fallgruver

- Fullskjermlag i `#ui` får `pointer-events: auto` fra `#ui > *`. Lag som dekker skjermen må overstyres eksplisitt, ellers sluker de klikk og berøring.
- Headless Chromium stopper skjermbilder mens pekeren er låst. Testene erstatter `requestPointerLock`.
- Flomlysene var for sterke og gjorde natten til dag i High. Nå har de vindusavtagning og skala 0.07.
- Bygget er reproduserbart. `npm ci` og `npm run build:single` ga 3. oktober en fil som var byte-identisk med den publiserte artefakten (sha256 `8a7dce16c5a6c806...`). Sammenlign sha256 når du vil vite om artefakten og repoet er samme versjon.
- Sluttkortet toner inn med CSS-animasjoner (tittel etter 1 s, knapper etter 6,5 s). I programvare-renderingen går de mye tregere enn vegguret. Tester må vente på synlighet, ikke på fast tid.
- I Claude Code-skyen ligger Chromium 141 ferdig i `/opt/pw-browsers`. Python Playwright 1.56.0 passer til den. Ikke kjør `playwright install`.
