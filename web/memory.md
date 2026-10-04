# memory.md: hva prosjektet vet

Fast kunnskap og beslutninger for nettversjonen. Oppdateres når noe endres. Nyeste beslutning står øverst i hver seksjon.

## Beslutninger

- 2026-10-04: Tom ba ChatGPT bygge videre på SIGNAL / 47 med grafikk og teksturer til three.js. Dette passet kobler 17 nye kildeassets til eksisterende web-spill (16 lastes i runtime; slukket motellskilt er bare kildevariant). Ingen Unity-filer, spillkonstanter eller kapittelinnhold er endret. `src/core/art.ts` laster alle runtime-bilder før geometri og High/Low-materialcache bygges. Materialenes repeat-varianter deler THREE.Source. Sju PNG-bilder har WebP-kopier fra `tools/prepare_art.py`; brieforiginalene er bevart. Himmel leveres 4096x2048, men generatorkilden var 1774x887, og spillet bruker 2048x1024. Eksakt bildegeneratormodell er ikke eksponert. Opphav og prompts ligger i JSON-filene under `src/assets/art/`.
- 2026-10-04: Testkroken `S47.hold` må defineres med `Object.defineProperty`. `Object.assign` kopierte bare getterens verdi og stoppet ikke den vanlige spilløkka. Rettet før dette passets tester. Tidligere testresultater er historikk og er ikke bevis for pauset spilltid.

- 2026-10-03: Kapittel 1 "The Second Exposure" er portert fra Unity (`Chapter09`, `Visual10`, `ChapterInvestigation.cs`, `FieldCamera.cs`): kamera ved østdøra, S-03-loggen, FRAME 01 fra S-03-plattformen, framkalling i fotolaben, merking på kopien, referanse og hypotese, B-12 med passiv eller aktiv metode, FRAME 02, sammenligning og lokal rapport. Tekstene i spillet er Unitys, ordrett der det gikk. Tilpasninger: arrayet ligger nord (-Z) og servicegården øst for kontrollrommet, fotolaben ligger øst for gangveien, motorskapets logg heter SARO ARRAY (Unity: SIERRA ARRAY) og er stemplet med klokkeslettet for smellet i prologen (Unity: 23:44), og merket med tre striper heter R-07 fordi S-07 er en av de 27 antennene her. Flettet gjennom [PR #27](https://github.com/Tombonator3000/SIGNAL-47/pull/27) og ute på Pages fra 4. oktober kl. 00:18 UTC.
- 2026-10-03: Prolog og kapittel 1 sammen tilsvarer K1 i designbibelen. Arbeidsordren med Ward og overgangen til arkivet (K2) mangler. Se `FORSLAG.md`.
- 2026-10-03: Ytelse: antennene tegnes som instanser (`DishArray`) med egen synlighetstest per antenne, alle lampeglød i ett tegnekall (`GlowPoints`), lysdioder i racket som instanser, og bokser med ulik inn- og utside lages med `faced()` (to tegnekall i stedet for seks). Målt i skyen: tittel 451 til 198 draw calls, start ved pulten 388 til 157, vinduet 262 til 74. Ute og i laben 34 til 202.
- 2026-10-03: Lagring: `s47.checkpoint` er `residual` eller `chapter1`. Saken i kapittel 1 ligger i `s47.case`, med begge fotografiene som JPEG-tekst.
- 2026-10-03: Tom ba om at spillet skal kunne spilles fra GitHub Pages. Det publiseres på https://tombonator3000.github.io/SIGNAL-47/ med `.github/workflows/pages.yml` hver gang main får endringer i `web/`. Det vanlige bygget (`npm run build`, `dist/`) publiseres, fordi kode, lyd og fonter da mellomlagres hver for seg. `build:single` brukes fortsatt til artefakter og deling som én fil.
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
- Slutter: designbibelen (`Docs/DesignBible13/design-bible.md`) har to, A "Bryt referansen" og B "Fullfør én registrering", og sier at det ikke finnes en tredje. Tre slutter (Silence, Answer, Listen) står bare i ChatGPT-samtalen "Utvikle spillområde visuelt". Her stod det tidligere at designbibelen også hadde tre. Det var feil. Tom har ikke valgt ennå.
- Navn i designbibelen: spilleren heter Reyes, vaktansvarlig er Dr. Evelyn Ward, vitnet er Nora Vega (rom 6 på Sierra Motor Court), broren hennes Tomás Vega. ART_BRIEF.md og todo.md sier rom 47. Ikke avgjort.

## Referansebilder

Tom la ved disse bildene da web/ ble lagt inn 3. oktober. De ligger ikke i repoet.

- Seks SARO-bilder: motellskiltet ved arrayet, kontrollrommet, skrivebordet med feltkamera og S-03-profilen (planlagt 042, enkoder 026, commands received 0, neste spor Sierra Motor Court), servicegården ved S-03 (AZIMUTH 026, NO CONTROL COMMAND RECORDED), konsollen med SIGNAL LOCK og EVENT IN 00:00:47, og spektrumanalysatoren med 1419.900 CAL, 1420.110 RFI, 1420.405 ANOMALY og -39.0 LY.
- Klokkeslett, SNR, asimut og repetisjonstid i bildene avviker fra kanon over (for eksempel 03:14:27, SNR 12.4 dB og "repeats every 47s"). Kanon gjelder, bildene er stemning og stil.
- Et skjermbilde fra et annet spill: håndskrevet dagbok til venstre og ren transkripsjon til høyre. Dokumentvisningen i `UI.document()` har samme oppsett.

## Arkitektur

- `src/main.ts`: oppstart, step/draw-løkke, menyer, adaptiv oppløsning, testkroker på `window.S47` (hold, tick, jump, setQuality, game, room, ext, yard, fcam, ch1, player, renderer, scene, camera). `S47.jump('chapter1')` hopper rett til kapittel 1.
- `src/story/Chapter1.ts`: kapitlet som trinn (`Stage`) med en lagringsbar `CaseState`. Alle regler for når lukkeren kan utløses står i `check()`.
- `src/world/ServiceYard.ts`: gangvei, S-03, B-12, fotolab og detaljer. Gangbare flater er soner (`zones`), hindringer er `colliders`.
- `src/core/FieldCamera.ts`: søkeren gir skjermkameraet det synsfeltet som gjør at rammen viser nøyaktig det fotografiet får med (44,6 grader vertikalt). Eksponeringen tegner scenen på nytt i 960x600.
- `src/story/Prologue.ts`: faser i rekkefølge intro, shift, survey, skip, residual, locked, solving, printing, printed, ringing, call, countdown, event, turning, end. Alle forsinkelser bruker spilltid (`after()`), så pause og omstart virker.
- `src/world/kit.ts`: materialer, byggeklosser, statisk sammenslåing, falske natriumflomlys (`addFlood`, `floodlit`).
- `src/world/Dish.ts`: antenne med asimut- og elevasjonsledd. lod 1 for fjerne antenner.
- `src/core/quality.ts`: bytte mellom High og Low. Kode som setter materialer under spillet skal bruke `materialFor()`.
- Sjekkpunkt: `localStorage` nøkkel `s47.checkpoint`, settes til `residual` ved 02:13.

## Kjente fallgruver

- Spilleren må alltid stå inne i minst én sone. Der to soner møtes, må de overlappe med mer enn spillerens diameter, ellers blir spilleren stående fast i skjøten.
- three.js treffer også usynlige objekter med stråler. Det som skal være skjult for interaksjon, flyttes til lag 31 (`layers.set(31)`). Små eller sammenslåtte ting får en usynlig boks (`proxy()`) som strålen kan treffe.
- `mergeStatic()` slår sammen alt i en gruppe. Objekter som skal kunne brukes, flyttes eller skjules, må få `noMerge()` eller ligge utenfor gruppa.
- Flomlyssett (`floodSet`): ute brukes settet `site` med 20 plasser, fotolaben har sitt eget sett med 5. Et fullt sett gir en advarsel i konsollen.
- I testene vises merkelappen til et objekt et øyeblikk etter et hopp, fordi teksten tones ut med CSS. Det er ikke en feil i spillet.
- Fullskjermlag i `#ui` får `pointer-events: auto` fra `#ui > *`. Lag som dekker skjermen må overstyres eksplisitt, ellers sluker de klikk og berøring.
- Headless Chromium stopper skjermbilder mens pekeren er låst. Testene erstatter `requestPointerLock`.
- Flomlysene var for sterke og gjorde natten til dag i High. Nå har de vindusavtagning og skala 0.07.
- Bygget er reproduserbart. `npm ci` og `npm run build:single` ga 3. oktober en fil som var byte-identisk med den publiserte artefakten (sha256 `8a7dce16c5a6c806...`). Sammenlign sha256 når du vil vite om artefakten og repoet er samme versjon.
- Sluttkortet toner inn med CSS-animasjoner (tittel etter 1 s, knapper etter 6,5 s). I programvare-renderingen går de mye tregere enn vegguret. Tester må vente på synlighet, ikke på fast tid.
- Pages legger spillet i undermappen `/SIGNAL-47/`. `base: './'` i `vite.config.ts` gjør alle stier relative, og det må den fortsette med.
- Alle Toms Pages-sider deler opprinnelsen `tombonator3000.github.io`, også `localStorage`. Nøklene våre har prefikset `s47.`, og det må nye nøkler også ha.
- I Claude Code-skyen ligger Chromium 141 ferdig i `/opt/pw-browsers`. Python Playwright 1.56.0 passer til den. Ikke kjør `playwright install`.
