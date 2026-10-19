# Art Bell-radio: testbevis

Status 19. oktober 2026, 18:48 UTC: **lokal sluttkandidat validert**. Gjelder `codex/art-bell-radio`, med opprinnelig base `aa38e2b` og nyere main `d6dc68b` integrert gjennom `66667c3`. Privatbanken fra `SIGNAL-47-assets` på `12afec3` er med i begge sluttbyggene, slik den hentes i CI. Funksjonsprøvene, alle 12 kompatibilitetsprøver, den oppdaterte firestedsprøven og alle 13 spillregresjoner består. De nyeste funksjonsprøvene inkluderer rettingen for retur til en allerede dekodet radiosending. Publiseringsstatus føres separat fra denne lokale valideringen.

[Byggmanifestet](../build-manifest.json) identifiserer kildefilene og begge byggene. Enkeltfilkandidaten er **98 514 754 byte**, SHA-256 `fb62e9c4894fad42752f55fa929493805896f0c5285f61b6f348641e2b0f12e7`. Den inneholder den komplette banken med 15 lisensierte lyder, 24 telefonklipp og 47 ordinære lyder. Et eget bygg uten privatbanken er bare brukt til fallback-prøven nedenfor.

## Beståtte kontroller

| Kontroll | Resultat | Bevis |
| --- | --- | --- |
| Art Bell i Pages-undermappen `/SIGNAL-47/` | PASS, 51/51 | [Logg](artbell-pages.log), [lyd- og tilstandsdata](pages/artbell-radio.json) |
| Art Bell i enkeltfil med streng CSP | PASS, 52/52 | [Logg](artbell-single.log), [lyd- og tilstandsdata](single/artbell-radio.json) |
| Radiomotor og resolver i Node | PASS, 48/48 | [Logg](artbell-core.log) |
| Parvis lydkost, samme førerhus med On/Off/On | PASS, 17/17 | [Logg](audio-cost/artbell-cost.log), [målinger](audio-cost/artbell-cost.json) |
| Eksisterende Kinski-motor og tekstkorpus | PASS, 55/55 og 26/26 | [Logg](readaloud-core.log) |
| Eksisterende telefonmotor | PASS, 28/28 | [Logg](phonecalls-core.log) |
| Lagring | PASS, 19/19 | [Logg](saves.log) |
| Styring | PASS | [Logg](steercheck.log) |
| TypeScript, Pages-bygg og enkeltfilbygg | PASS | [Typecheck](typecheck.log), [Pages](build-pages.log), [enkeltfil](build-single.log) |
| Gammel radio, Pages og enkeltfil | PASS, 36/36 og 34/34 | [Pages](radio-pages.log), [enkeltfil](radio-single.log) |
| Vanlige kjøreradiohendelser | PASS, 35/35 | [Logg](tripradio.log), [resultatdata](tripradio/tripradio.json) |
| Komplett lisensiert privatbank i sluttbygget | PASS, 38/38 | [Logg](licensed-with.log), [resultatdata](licensed-with/licensed_sfx.json) |
| Separat bygg uten lisensiert privatbank | PASS, 22/22 | [Logg](licensed-without.log), [resultatdata](licensed-without/licensed_sfx.json) |
| Eksisterende Kinski-opplesning, Pages og enkeltfil | PASS, 37/37 og 38/38 | [Pages](readaloud-pages.log), [enkeltfil](readaloud-single.log) |
| Eksisterende telefonstemmer, Pages og enkeltfil | PASS, 49/49 og 50/50 | [Pages](phonevoices-pages.log), [enkeltfil](phonevoices-single.log) |
| Eksisterende beskrivelser, Pages og enkeltfil | PASS, 115/115 og 116/116 | [Pages](descriptions-pages.log), [enkeltfil](descriptions-single.log) |
| Firesteds lydprøve med måling per bilde | PASS, 4/4 | [Logg](soundcheck.log), [målinger](soundcheck/soundcheck.json) |
| Streng CSP, fonter, lyd og oppstartsbilder | PASS | [Logg](csptest.log), [kjøringsoversikt](csp-results.json) |

[CPU-oversikten](cpu-results.json), [funksjonsoversikten](feature-results.json), [kompatibilitetsoversikten](compatibility-results.json) og [CSP-sluttkjøringen](csp-results.json) bevarer kommandoer, returkoder og kjøretider. Den eldre `soundcheck`-feilen og feil i testverktøyene er forklart nedenfor og bevart separat.

Nodeprøven bruker kontrollerte Web Audio-dobler til planlegging og avbrudd. Bevis for ekte MP3-dekoding og PCM kommer fra nettleserprøvene og filkontrollen, ikke fra disse doblene.

Uttrekket dekker 38 statiske vertslinjer og seks klokkemaler med åtte ordrette tekstfragmenter og 69 unike tidsfraser, totalt 115 klipp. [Uttrekksloggen](extract.log), [korpuset](../corpus.json) og [filkontrollen](../verification.json) dokumenterer tekst, ID-er og filer. Filkontrollen består for alle 115 MP3-filer, 2 065 028 byte og 247,98 sekunder. Ingen droppede chunker, manglende filer, foreldreløse MP3-filer eller dekodede samples som klipper er rapportert. Alle 1555 eksisterende Kinski-MP3-filer og gammel katalog er faktisk sammenlignet mot Gitbasen og bevart, se [filbevis](kinski-preservation.json).

## Hva nettleserprøvene beviser

`tools/artbell-radio.py` observerer den ekte dekoderen og de ekte Web Audio-kildene. Lyden erstattes ikke. R-tasten vekker bilradioen på 1420, og klippets bufferidentitet, MP3-header, opprinnelige avspillingshastighet, faktiske varighet og planlagte start/slutt blir kontrollert. PCM måles ved stasjonsbussen og hovedbussen. Forbindelsen går gjennom mottakerens eksisterende AM-filter, romlige lyd og `AudioSys.radioBus`; radiovolum null gir stillhet ved hovedbussen når de øvrige bussene er dempet.

Prøvene kontrollerer full tekst og captionvarighet, en kort replikks naturlige slutt, to mottakere med én felles avspilling, klokkemalene 02:03 og 02:00, samt uavhengige lagrede Art Bell- og Kinski-valg. Pause varer lenger enn den opprinnelige replikkens fulle varighet. Den samme caption-noden skal fortsatt være tilkoblet og uten `.out` mens HUD er skjult av menyen, og bli synlig igjen ved Resume. Både lydklokken og stasjonens posisjon står stille under pausen.

Den siste rettingen kontrolleres med ekte R-tast bort fra og tilbake til 1420 mens samme opptak ligger i cache. Radioen skal gjenoppta på riktig forskyvning med én kilde og gjenopprette nøyaktig én levende caption. Disse to nye nettleserkontrollene og motorens tilsvarende kontroller er med i **51/51, 52/52 og 48/48** ovenfor. De tidligere resultatene på 49/49, 50/50 og 46/46 var før denne rettingen og er historiske.

BOY, avmelding og kjøreinnslag har merkede manusbaserte prøver gjennom den ekte stasjonsplanleggeren. Disse beviser valg og avspilling av de aktuelle opptakene. De vanlige radio- og kjøreradioprøvene ovenfor prøver hendelsesutløserne separat i den opprinnelige pratelydmodusen, fordi de spoler spilltid raskt. De er ikke et bevis på at hele lange programmer er lyttet gjennom.

Innringere og Roswells morgenvert beholder prosedyrisk lyd. Ukjent tekst og en tilsiktet dekoderfeil gir komplett prosedyrisk fallback. Off, stasjonsbytte, reset, lasting og tittel stanser både aktiv tale og fremtidige planlagte kilder. En gate etter ekte innlasting/dekoding kontrollerer at et sent resultat ikke starter gammel lyd etter avbrudd. Enkeltfilen består med `default-src 'none'` og uten `connect-src`; vertsklippene krever ingen eksterne MP3-forespørsler. Pages-prøven har ingen mislykkede vertsklippforespørsler. Begge sluttkjøringene har tom feilliste.

Seks nye skjermbilder av desktop- og 844 × 390-berøringsmenyer ligger i [pages](pages/) og [single](single/). Testene bruker ekte trykk og kontrollerer horisontal plass og lagring etter reload. Bildene og den emulerte berøringsprøven erstatter ikke en kontroll på fysisk mobil. De 282 større regresjonsbildene er bevart utenfor repoet i samtalens `outputs/artbell/integration-qa/`; logger og resultatoversikter er beholdt her.

## Full spillregresjon

Alle 13 prøver består på den ferdige kandidaten. [Kjøringsoversikten](regression-results.json) bevarer returkoder og kommandoer.

| Prøve | Resultat | Logg |
| --- | --- | --- |
| Walkthrough | PASS, 57/57 | [Logg](walkthrough.log) |
| Kapittel 1 | PASS, 75/75 | [Logg](chapter1.log) |
| Kapittel 2 | PASS, 76/76 | [Logg](chapter2.log) |
| Kapittel 3 | PASS, 72/72 | [Logg](chapter3.log) |
| Kapittel 4 | PASS, 69/69 | [Logg](chapter4.log) |
| Kapittel 5 | PASS, 55/55 | [Logg](chapter5.log) |
| Kapittel 6 | PASS, 44/44 | [Logg](chapter6.log) |
| Utviklermeny | PASS, 14/14 | [Logg](devmenu.log) |
| Kjøreturer | PASS, 23/23 | [Logg](drives.log) |
| Naturlyd | PASS, 7/7 | [Logg](ambience.log) |
| Diner | PASS, 15/15 | [Logg](diner.log) |
| Musikk, Pages | PASS, 12/12 | [Logg](music-pages.log) |
| Musikk, enkeltfil | PASS, 5/5 | [Logg](music-single.log) |

## Lydkost og den gamle grensen på 1500/s

Den første ordinære `soundcheck` ga **FAIL, 3/4**, med den gamle grensen på 1500 parameterkall per sekund. Førerkabinen målte 1744,8/s stående og 1762,2/s under kjøring. Samme kjøring hadde ingen målte lydutfall og beskjedent antall nye noder. Feilen er bevart i [soundcheck-old-rate-limit](soundcheck-old-rate-limit/), med [logg](soundcheck-old-rate-limit/soundcheck.log) og [rådata](soundcheck-old-rate-limit/soundcheck/soundcheck.json). Den regnes ikke om til PASS.

Den første parvise On/Off/On-målingen ga 29,2306/29,5028/29,4972 parameterkall per bilde. Det er historiske resultater fra før siste retting; de er bevart i [before-warm-retuning-fix/audio-cost](before-warm-retuning-fix/audio-cost/). De brukes ikke som sluttmålingen for den integrerte kandidaten.

`tools/artbell-cost.py` er kjørt på nytt mot sluttkandidaten med privatbank og rettingen for retur til en varm radiocache. Samme stående førerhus og samme lange manusreplikk måles med vanlig rammeløkke, seks sekunder per tilstand:

| Tilstand | Målte bilder/s | Parameterkall/s | Parameterkall per bilde |
| --- | ---: | ---: | ---: |
| On-1 | 59,89 | 1758,58 | 29,3639 |
| Off | 60,06 | 1765,49 | 29,3961 |
| On-2 | 59,91 | 1772,36 | 29,5861 |

Rådata og kategorier er i [audio-cost/artbell-cost.json](audio-cost/artbell-cost.json), med [sluttlogg](audio-cost.log) og [kjøringsoversikt](cost-results.json). Lytter gir ni kall per bilde, motor åtte, dekk fem og mottaker fire, totalt 26 før øvrig lydarbeid. Ved 60 bilder/s tilsvarer 1500/s bare 25 kall per bilde. Grensen var dermed lavere enn den observerte grunnkostnaden også med Art Bell Off. On/Off/On viser tilnærmet samme arbeid per bilde, uten ny dekoding eller stasjonssynkronisering i målevinduene. Ingen målte hovedbussutfall eller løpsk kilde-/nodevekst ble funnet. Den kontrollerte radiomotorprøven består også etter integrasjonen, se [artbell-core.log](artbell-core.log).

Den oppdaterte `soundcheck` bruker **under 36 parameterkall per faktisk rendret bilde**, med krav om at bilder faktisk er telt. Dette gir omtrent 20 prosent margin over målt grunnarbeid på rundt 29,5 per bilde. Antall noder per sekund og lydutfall kontrolleres fortsatt separat. Dette er en endring av måleenhet med et målt On/Off-grunnlag, ikke en påstand om at den gamle 1500/s-prøven besto.

Sluttkjøringen består **4/4**, med **null målte lydutfall** på alle fire steder. Kontrollrommet målte 12,45 parameterkall per bilde, uteområdet 12,43, stående førerhus med radio 29,93 og kjøring med radio 29,52. Alle verdier er under 36; nodearbeidet er beskjedent og feillisten er tom. Se [sluttlogg](soundcheck.log) og [rådata](soundcheck/soundcheck.json).

## Historiske forsøk og rettinger i testverktøyene

- [attempt-1](attempt-1/) bevarer første funksjonskjøring. Pausesjekken krevde synlig høyde mens pausemenyen bevisst skjulte hele HUD, selv om teksten kom tilbake ved Resume. Sjekken følger nå den opprinnelige caption-noden mens menyen står oppe og krever synlighet etterpå. En løs assignment ved gjenoppretting av dekoderen returnerte også en funksjon som Playwright automatisk kalte med `null`. Den er erstattet med en blokk uten retur. Begge sluttkjøringene er gjennomført på nytt.
- [legacy-harness-attempt-1](legacy-harness-attempt-1/) bevarer et tilsvarende Playwright-problem i den gamle radiotestens toast-wrapper. En funksjonsverdi fra assignment ble utilsiktet kalt med `null`. Radiotesten er rettet og kjørt på nytt, med resultatene i tabellen.
- [audio-cost/attempt-1](audio-cost/attempt-1/) bevarer første parvise måleforsøk. Probeinstallasjonen feilet med `Illegal invocation`, og tellere ble null. Disse tallene er ugyldige som kostbevis. Sluttproben kontrollerer at de native tellerne faktisk er installert og i bruk, og den nye kjøringen består 17/17.
- [before-warm-retuning-fix](before-warm-retuning-fix/) bevarer tidligere funksjons-, motor-, kost- og kompatibilitetsresultater samt det daværende byggmanifestet. Dette er et arkiv fra før rettingen for retur til en allerede dekodet sending. Sluttkandidatens logger og manifest ligger på de ordinære stiene, og har de nye resultatene og filhashen ovenfor.
- [phonecalls-core-pre-licensed-mock.log](phonecalls-core-pre-licensed-mock.log) bevarer en CPU-feil etter integrasjonen av den lisensierte banken. Den isolerte modulprøven måtte også isolere den nye `LICENSED`-globben. Web Audio-doblene i radiomotorprøven ble tilsvarende tilpasset. Rettede prøver består nå 28/28 og 48/48. Walkthrough skiller 47 ordinære, 24 telefon- og 0/15 lisensierte bufferoppføringer; beskrivelsesprøven venter på den ekte preload-promisen fremfor et gammelt samlet antall.
- [csptest-pre-licensed-count.log](csptest-pre-licensed-count.log) bevarer første CSP-feil på den integrerte kandidaten. Den gamle testen telte alle ikke-telefonklipp som 45 ordinære lyder, mens sluttbygget har 47 ordinære og 15 lisensierte lyder. Nå venter den på ferdig lydoppstart og kontrollerer de tre navnerommene separat. CSP, font- og bildepredikater er uendret. Halley-plakaten var allerede forventet blant bildene som lastes etter oppstartsbanken; 31 lastede bilder mot 30 startbilder er korrekt når denne plakaten er med. Ny sluttkjøring består med fire fonter, 86 dekodede lyder, riktig bildebank og ingen feil eller advarsler.
- Den reelle caption-fristen bruker nå den pausefølsomme lydklokken. Toast-prober videresender det tredje klokkeargumentet, slik at instrumenteringen beholder denne oppførselen. Gaten for sen innlasting installeres direkte før riktig manusplan, så den ikke forbrukes av en gammel sending ved oppvåkning.

Ingen av de historiske feilforsøkene slettes eller presenteres som vellykkede sluttkjøringer.

## Begrensninger

- **UNVERIFIED:** menneskelig lytting, likhet med Art Bell, klang, innlevelse, Caliche-uttalen og hørbar sømløshet mellom tidsfraser. ASR er en ufullkommen ordkontroll. Se [produksjonsoversikten](../README.md), [ASR-prøver](../verification.json) og [sammensatte tidsprøver](../clock-asr.json).
- **UNVERIFIED:** fysisk lyd fra PC-/mobilhøyttalere, ytelse og forsinkelser på spillerens maskinvare. PCM-, graf- og livssyklusmålinger beviser det som foregår i testnettleseren. Målte bilder/s i den avgrensede kostprøven er ikke en generell ytelsesgaranti.
- **Avgrenset dekning:** de lange opptakene er kontrollert som filer, planer og avbrudd. Nettleseren fullfører én kort naturlig replikk, og prøver representative klokkemaler. Den spiller ikke ferdig alle 115 opptak eller alle klokkeslett.
