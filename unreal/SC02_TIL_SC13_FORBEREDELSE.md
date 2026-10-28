# SC02 til SC13: forberedelse fra skyøkt 27. oktober 2026

**Oppdatert status 28. oktober:** Dette dokumentet bevarer skyøktens opprinnelige omfang og prøvestatus. [SC02/AI-overleveringen](SC02_AI_HANDOFF.md) har gjeldende kandidatstatus: SC02 er bygget, importert og pakket prøvd, og de videre kildene følger den godkjente revisjonen med 77 scener/90 eksterne flagg. Eldre skyggeregel, SC04-bilmøte og tellinger nedenfor er historikk. SC03 og videre er fortsatt uintegrert.

Tom ba om at SC02 og resten fram til SC13 forberedes mens Kubuntu-maskinen ikke er tilgjengelig. Arbeidet er gjort i en skysesjon på grenen `ccr-20021313-mfplx9`, laget fra `origin/main` (`1c309d11`). Skyen har verken Unreal eller GPU. Ingenting her er bygget, pakket eller prøvd i spillet. Alt runtimearbeid er UNVERIFIED til det er kjørt på Kubuntu.

## Hva som er levert

1. **Verdensprodusenter for alle 96 flagg.** Planen (avsnitt 2) krever at hvert `ctx_hr_*`-flagg har en navngitt produsent og test før scenen kan ferdigmeldes. [world_producers.json](Narrative/HorrorStory/world_producers.json) navngir nå produsenten for alle 96, med fil og symbol i dagens kode. `build_pack.py` kontrollerer at hvert symbol faktisk står i filen det peker på, så kartet kan ikke vise til kode som ikke finnes. Status: 12 flagg produseres allerede av koden, 48 delvis, 36 må bygges.
2. **Scenematrise.** Én rad per scene (79) med startsted eller spillerhandling, normalforløp, avbrudd, alternativ, modeller, lyd, dokumenter og tester. Stemmecues og opptaksstatus hentes fra opptaksmanifestet. Lesbar versjon: [generated/SCENE_MATRIX.md](Narrative/HorrorStory/generated/SCENE_MATRIX.md).
3. **Testsett per SC.** 117 navngitte prøver. Hver SC dekker alle åtte kategoriene fra planens avsnitt 4 (normal, alternativ, avbrudd, Continue, negativ, bilde, lyd, regresjon). Testtypene skiller data, API, faktisk input, bilde og menneskelig vurdering.
4. **Arbeidspakker per SC.** Kodeendringer med fil og klasse, ressurser, stemmer, avhengigheter, hva som bare kan gjøres på Kubuntu og risiko. De står i samme fil og i matrisen.
5. **Validering.** `build_pack.py` avviser flagg uten produsent, oppdiktede kodesymboler, planlagte filer som allerede finnes, feil SC-tilhørighet og testsett som mangler en kategori. Sju nye negative prøver i `test_pack.py` (31 totalt, alle består). Runtimeeksportene, inkludert `HorrorPackData.inl`, er byteidentiske; spillet ser ingen ny data.
6. **Stemmegenerator per scenario.** `voice/generate.py --scenario SC02` velger cuene for ett scenario. Uten valget lages de fem SC01-opptakene som før. Mangler en rolle stemmeprofil i `casting.json`, stopper kjøringen i stedet for å gi rollen en ny stemme.
7. **SC02.** Se eget avsnitt under.

## Status per scenario

| SC | Flagg finnes / delvis / nye | Tester | Stemmecues uten opptak | Største byggearbeid |
|---|---|---|---|---|
| SC01 | 6 / 0 / 0 | 9 | 0 | Levert; installasjon og menneskelige prøver gjenstår |
| SC02 | 2 / 2 / 3 | 11 | 3 (Dale) | Blek reaktiv aktør, rutegraf ved trafoen, lokal retry, revisjon 2 |
| SC03 | 0 / 5 / 4 | 10 | 11 (Ward, dispatch, Ortiz) | Mara Ortiz, servicevogn 23, synlig løft, savnetsak, skyggeregel |
| SC04 | 1 / 7 / 7 | 10 | 5 | Plansperre, annexmøte med lydavledning, bilmøte, huking |
| SC05 | 0 / 1 / 0 | 10 | 0 | Passiv residualvei som ikke starter Survey |
| SC06 | 1 / 2 / 0 | 8 | 0 | Koble H03 til eksisterende B-12 og FRAME01/02 |
| SC07 | 0 / 3 / 0 | 8 | 3 (Dale) | Original 72-sekunders VHS med transport og settstatus |
| SC08 | 0 / 12 / 10 | 13 | 4 (Earl) | BY02 med TAPE/LIVE, ARRAY-01 med A/B/retur, møte 4 |
| SC09 | 0 / 4 / 0 | 7 | 0 | Koble arraykunnskap til STATION01 og receiveren |
| SC10 | 0 / 3 / 0 | 7 | 6 (Nora) | Nye Rom 6-scener mot eksisterende P10 til P12 |
| SC11 | 0 / 3 / 5 | 8 | 2 | Fjernark etter 04:04, møte 5 med ytre lys og dørtilstand |
| SC12 | 1 / 4 / 5 | 9 | 14 (Ward, sjåfør) | Diktering til Ward, varselkvittering, sjåførens kjørelogg |
| SC13 | 1 / 2 / 2 | 7 | 0 | Koble avreise, kart og CB til eksisterende finale |

Opptak: 48 av 53 påkrevde cues mangler fortsatt. SC02 kan tas opp med Dales eksisterende profil. Fra SC03 trengs profiler for Ward, Ortiz, Earl, Nora og sjåføren, se avklaringene.

## Avklaringer Tom må ta

Hjelperne som leste koden, fant disse motstridene mellom manus, plan og dagens kode. Ingen av dem er løst på egen hånd.

**Tid og plan**
1. H02_INCOMPLETE krever T01-arkivering før overgangen til ubetjent survey, mens SC04 og SC05 krever at automatisk plan forblir sperret og residualen kommer passivt. Hvordan skal arkiveringen kobles til den passive residualen?
2. Residualen nås i dag bare gjennom Survey, `RelayFirst`, `LightsDip`, `TimeSkip` og `EnterResidual`. SC05 trenger en passiv vei uten Survey.
3. `AS47Story::Tasks` viser «Babysit the survey until 06:00», og Ward sier «Six o'clock, Reyes» i dineren. Begge strider mot at Ward suspenderer 06:00-serien i SC03.
4. Kodekanon har antennene på 026 fra 02:15, mens SC08 er lagt mellom 01:15 og 02:15 og forutsetter retur til 026.

**Fangst og fiender**
5. `FS47Abduction`, `FS47TruckTaken` og `US47StationTaken` gir alle tidstap. Planen forbyr tidstap ved fangst, så møtene får eget lokalt sjekkpunkt. `US47StationTaken` startes dessuten ingen steder i Unreal-bygget.
6. `FS47Glimpses` skjuler seg når den blir sett. Skyggeregelen i SC03 krever det motsatte: stille under blikk, flytting bare usett. Den trenger en egen modus.
7. Det finnes ingen inntrenger og ingen huking i koden. SC04 og SC11 avhenger av at møtesystemet fra SC02 blir generelt nok.

**Kilder og rekkefølge**
8. Earls eksisterende spole startes av et dialogtema i studioet (`FS47Caliche`), ikke i lobbyen. Det kan ikke telle som verdensfaktum for BY02.
9. MISSION_FLOW legger arrayvedlegget i feltmappen, men i koden er det et eget ark i kontorskapet. Tavlene står på z -75,3 i koden og -74 i servicearket.
10. I `US47Chapter3` kommer Noras felttelefon før printene er fremkalt, mens SC09 krever begge feltprintene før Nora.
11. Kontrakten sier at EveningShift alene eier kontor og VHS, men `FS47Office` ligger i Main.
12. Noras avslutning i kapittel 4 («drive the road») strider mot H06_CHECK («Do not finish his observation»).
13. Story sier at veilederlinjen er stille i kapittel 4 og 5, mens H07_BRIEF er en Ward-samtale på telefon.
14. I H09_BRIEF sier sjåføren selv at han nylig kjørte veien, men flagget kan ikke komme fra dialog. Det trengs en datert kjørelogg som ny rekvisitt.
15. Kapittel 6 lagrer ingenting, så `h10_stalled` finnes bare mens hendelsen pågår, og Continue går tilbake til dineren. Veien slutter ved C 8.15, så overleveringspunktet 10.4 kan ikke vises som et sted man når.

**Stemmer**
16. Ward, Ortiz, Earl, Nora og sjåføren har ingen profil i HorrorStorys `casting.json`. Hvilke eksisterende profiler skal de bruke? Ward er kvinne (memory.md). Ingen navngitt rolle får ny stemme uten Toms valg.

## SC02-kandidat i denne økta

SC02 er skrevet som kilde, men ikke kompilert, pakket eller prøvd. Status i planen er «Under arbeid».

**Kode** (commit `3c560e3e` og rettelsene i `6966b644`):

- `FS47Horror` har innholdsrevisjon 2. Nye saker går fra SC01 videre til SC02 i stedet for `legacyContinuation`. Saker med revisjon 1 skal oppføre seg som før; en uavhengig gjennomgang av diffen fant ingen endring der.
- Nytt versjonert lagringsfelt `horror.sc02` med fasene `sc01`, `t01`, `encounter`, `escaped`, `unwitnessed` og `legacy`. Gamle lagringer uten feltet laster uendret. Continue midt i møtet gir sikkert punkt ved skuret, ikke halv jakt.
- Sju verdensflagg settes fra faktiske fakta: T01 fra `FS47Substation::HasNormalResult`, synlig aktør fra gjengitt kropp i ubrutt siktstråle i 0,35 s mens den vender hodet, flukt fra brutt kontakt og kjøring, lokal retry fra faktisk fangst.
- Ny `FS47HorrorThreat` (`S47HorrorThreat.h/.cpp`): tilstandene Idle, Warn, Investigate, Search, Chase, Capture og Withdrawn. Forvarsel med 1,6 s stillstand og hodevending mot faktisk lydkilde (skritt og SARO 07). Sikt med siktstråle mot spillerens øye, stoppet av opake flater. Rutegraf med 16 punkter rundt trafogården, målt fra gjerdekollideren i `substation.json`, kontrollskapet og skuret. Rute A langs vestgjerdet, rute B rundt nord og øst, og ingen kant gjennom gården. Fangst gir lokal gjenopptakelse ved skuret uten tidstap og uten de gamle bortføringssystemene.
- `Busy()` holder gamle hendelser unna mens møtet pågår. Returarkiveringen i skiftmappen er urørt og skjer fortsatt én gang. SC02_ESCAPE gir `HR_TRAFO_ENCOUNTER`.
- Selvtester for revisjon 1 uendret, overgang til SC02, lagring rundtur, at SC02-scenene bare leser vertens flagg, rutegrafen og tilstandsovergangene.
- `Config/DefaultGame.ini` pakker `/Game/Signal47/HorrorSC02`.

**Uavhengig gjennomgang.** En egen hjelper leste diffen som en kompilator og en kritisk kollega. Den fant ingen sikre kompileringsfeil og ingen endring for revisjon 1. Disse funnene er rettet: møtet kunne bli stående fast hvis spilleren satt i bilen før aktøren viste seg; ventende H02-scener kunne gå tapt ved Continue fordi posisjonen gjenopprettes etter `Horror::Begin`; aktørens bakkehøyde ble regnet ut før posisjonen var gjenopprettet; SC02-scener kunne starte langt fra stedet; målteksten manglet under kjøring på stedet; og en sak i et bygg uten meshene ble lagret som permanent `legacy`. Ikke rettet: hvis T01 arkiveres før SC01 er ferdig, får spilleren ingen egen måltekst tilbake til Caliche; møtet starter når spilleren likevel kommer til fronten. Det lagrede `checkpoint`-feltet leses ikke, sjekkpunktet regnes ut på nytt.

**Modell.** `ArtSource/HorrorSC02`: original prosedyrisk Blender-kilde med seks leddede deler og to LOD-er, bygget og rendret med bpy 5.1.2 i skyen. Hele kroppen 13 660 trekanter i LOD0 og 4 098 i LOD1. Forhåndsvisninger i `ArtSource/HorrorSC02/Previews`. Ansiktet og magerheten fungerer; hoftepartiet ligner fortsatt en utstillingsdukke, og hodet minner om en klassisk «grey» fra siden. Prosjektets Blender er 4.5 LTS; skriptet må kjøres der før endelig eksport.

**Må prøves først på Kubuntu:**

1. Kjør `build_scene.py` i Blender 4.5 og `import_assets.py` i UnrealEditor-Cmd, så `/Game/Signal47/HorrorSC02` finnes før bygget. Uten meshene slår SC02 seg av og saken fortsetter i eksisterende historie.
2. Bygg editor og Linux og rett kompileringsfeil. Mest usikre punkter ifølge forfatteren: `FVector2D::Equals`, `TPair`-initialisering med klammer og negativ skala på barnekomponenter.
3. Selvtesten: linjene «SC02 route» og «SC02 threat» skal være PASS.
4. SC01-kontrakten og en gammel revisjon 1-lagring: fortsatt `legacyContinuation` og ikke noe `sc02`-felt.
5. Ny sak til Caliche: loggen «SC02 route graph» viser hvilke punkter som faktisk er farbare. Deretter rute A, rute B, direkte kjøring, fangst med retry ved skuret, pause og panel under jakt, Continue midt i møtet og etter flukt, og at T01 arkiveres én gang.
6. Bilder av aktøren i spillet (ledd, speilet høyre side, hodevending, bakkehøyde), T01- og T02-regresjon, RT-minne under 80 MiB og hele `screenshots.sh` før fletting.
7. Stemmer: `voice/generate.py --scenario SC02` for de tre H02-replikkene med Dales profil.

## Slik brukes dette på Kubuntu

```sh
python3 unreal/Narrative/HorrorStory/build_pack.py --check
python3 unreal/Narrative/HorrorStory/test_pack.py
python3 unreal/Narrative/HorrorStory/voice/generate.py --scenario SC02 --limit 3
```

Planens regel gjelder fortsatt: hvert SC får egen gren, testet pakke, PR mot main, fletting og installasjon før neste starter. Matrisen og testene er utgangspunktet for hvert SC, ikke en kvittering. En rad med status existing betyr at faktumet produseres i koden, ikke at det er koblet inn i `FS47Horror` for annet enn SC01.
