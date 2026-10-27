# SC02 til SC13: forberedelse fra skyøkt 27. oktober 2026

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

## SC02 i denne økta

Se avsnittet SC02-kandidat nedenfor når koden og modellen er ført inn.

## Slik brukes dette på Kubuntu

```sh
python3 unreal/Narrative/HorrorStory/build_pack.py --check
python3 unreal/Narrative/HorrorStory/test_pack.py
python3 unreal/Narrative/HorrorStory/voice/generate.py --scenario SC02 --limit 3
```

Planens regel gjelder fortsatt: hvert SC får egen gren, testet pakke, PR mot main, fletting og installasjon før neste starter. Matrisen og testene er utgangspunktet for hvert SC, ikke en kvittering. En rad med status existing betyr at faktumet produseres i koden, ikke at det er koblet inn i `FS47Horror` for annet enn SC01.
