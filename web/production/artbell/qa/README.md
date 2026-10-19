# Art Bell-radio: testbevis

Status 19. oktober 2026, 18:28 UTC. Gjelder arbeidsgrenen `codex/art-bell-radio`, med base `aa38e2b`. Nye funksjonsprøver og den parvise lydkostmålingen består. Resterende kompatibilitet, ny `soundcheck` med grense per bilde og den fulle regresjonsrunden er fortsatt ventende. Denne filen er ikke en ferdigmelding for hele leveransen.

## Beståtte kontroller

| Kontroll | Resultat | Bevis |
| --- | --- | --- |
| Art Bell i Pages-undermappen `/SIGNAL-47/` | PASS, 49/49 | [Logg](artbell-pages.log), [lyd- og tilstandsdata](pages/artbell-radio.json) |
| Art Bell i enkeltfil med streng CSP | PASS, 50/50 | [Logg](artbell-single.log), [lyd- og tilstandsdata](single/artbell-radio.json) |
| Radiomotor og resolver i Node | PASS, 46/46 | [Logg](artbell-core.log) |
| Parvis lydkost, samme førerhus med On/Off/On | PASS, 17/17 | [Logg](audio-cost/artbell-cost.log), [målinger](audio-cost/artbell-cost.json) |
| Eksisterende Kinski-motor og tekstkorpus | PASS, 55/55 og 26/26 | [Logg](readaloud-core.log) |
| Eksisterende telefonmotor | PASS, 28/28 | [Logg](phonecalls-core.log) |
| Lagring | PASS, 19/19 | [Logg](saves.log) |
| Styring | PASS | [Logg](steercheck.log) |
| TypeScript, Pages-bygg og enkeltfilbygg | PASS | [Typecheck](typecheck.log), [Pages](build-pages.log), [enkeltfil](build-single.log) |
| Gammel radio, Pages og enkeltfil | PASS, 36/36 og 34/34 | [Pages](radio-pages.log), [enkeltfil](radio-single.log) |
| Vanlige kjøreradiohendelser | PASS, 35/35 | [Logg](tripradio.log), [resultatdata](tripradio/tripradio.json) |

[CPU-oversikten](cpu-results.json) og [funksjonsoversikten](feature-results.json) bevarer kommandoer, returkoder og kjøretider. Kompatibilitetsoversikten oppdateres av den pågående sluttrunden; den eldre `soundcheck`-feilen er forklart nedenfor og bevart separat.

Nodeprøven bruker kontrollerte Web Audio-dobler til planlegging og avbrudd. Bevis for ekte MP3-dekoding og PCM kommer fra nettleserprøvene og filkontrollen, ikke fra disse doblene.

Uttrekket dekker 38 statiske vertslinjer og seks klokkemaler med åtte ordrette tekstfragmenter og 69 unike tidsfraser, totalt 115 klipp. [Uttrekksloggen](extract.log), [korpuset](../corpus.json) og [filkontrollen](../verification.json) dokumenterer tekst, ID-er og filer. Filkontrollen består for alle 115 MP3-filer, 2 065 028 byte og 247,98 sekunder. Ingen droppede chunker, manglende filer, foreldreløse MP3-filer eller dekodede samples som klipper er rapportert. Alle 1555 eksisterende Kinski-MP3-filer og gammel katalog er faktisk sammenlignet mot Gitbasen og bevart, se [filbevis](kinski-preservation.json).

## Hva nettleserprøvene beviser

`tools/artbell-radio.py` observerer den ekte dekoderen og de ekte Web Audio-kildene. Lyden erstattes ikke. R-tasten vekker bilradioen på 1420, og klippets bufferidentitet, MP3-header, opprinnelige avspillingshastighet, faktiske varighet og planlagte start/slutt blir kontrollert. PCM måles ved stasjonsbussen og hovedbussen. Forbindelsen går gjennom mottakerens eksisterende AM-filter, romlige lyd og `AudioSys.radioBus`; radiovolum null gir stillhet ved hovedbussen når de øvrige bussene er dempet.

Prøvene kontrollerer full tekst og captionvarighet, en kort replikks naturlige slutt, to mottakere med én felles avspilling, klokkemalene 02:03 og 02:00, samt uavhengige lagrede Art Bell- og Kinski-valg. Pause varer lenger enn den opprinnelige replikkens fulle varighet. Den samme caption-noden skal fortsatt være tilkoblet og uten `.out` mens HUD er skjult av menyen, og bli synlig igjen ved Resume. Både lydklokken og stasjonens posisjon står stille under pausen.

BOY, avmelding og kjøreinnslag har merkede manusbaserte prøver gjennom den ekte stasjonsplanleggeren. Disse beviser valg og avspilling av de aktuelle opptakene. De vanlige radio- og kjøreradioprøvene ovenfor prøver hendelsesutløserne separat i den opprinnelige pratelydmodusen, fordi de spoler spilltid raskt. De er ikke et bevis på at hele lange programmer er lyttet gjennom.

Innringere og Roswells morgenvert beholder prosedyrisk lyd. Ukjent tekst og en tilsiktet dekoderfeil gir komplett prosedyrisk fallback. Off, stasjonsbytte, reset, lasting og tittel stanser både aktiv tale og fremtidige planlagte kilder. En gate etter ekte innlasting/dekoding kontrollerer at et sent resultat ikke starter gammel lyd etter avbrudd. Enkeltfilen består med `default-src 'none'` og uten `connect-src`; vertsklippene krever ingen eksterne MP3-forespørsler. Pages-prøven har ingen mislykkede vertsklippforespørsler. Begge sluttkjøringene har tom feilliste.

Skjermbilder av desktop- og 844 × 390-berøringsmenyer ligger i [pages](pages/) og [single](single/). Testene bruker ekte trykk og kontrollerer horisontal plass og lagring etter reload. Bildene og den emulerte berøringsprøven erstatter ikke en kontroll på fysisk mobil.

## Lydkost og den gamle grensen på 1500/s

Den første ordinære `soundcheck` ga **FAIL, 3/4**, med den gamle grensen på 1500 parameterkall per sekund. Førerkabinen målte 1744,8/s stående og 1762,2/s under kjøring. Samme kjøring hadde ingen målte lydutfall og beskjedent antall nye noder. Feilen er bevart i [soundcheck-old-rate-limit](soundcheck-old-rate-limit/), med [logg](soundcheck-old-rate-limit/soundcheck.log) og [rådata](soundcheck-old-rate-limit/soundcheck/soundcheck.json). Den regnes ikke om til PASS.

Deretter målte `tools/artbell-cost.py` samme stående førerhus og samme lange manusreplikk med vanlig rammeløkke, seks sekunder per tilstand:

| Tilstand | Målte bilder/s | Parameterkall/s | Parameterkall per bilde |
| --- | ---: | ---: | ---: |
| On-1 | 59,94 | 1752,20 | 29,2306 |
| Off | 59,91 | 1767,37 | 29,5028 |
| On-2 | 59,90 | 1767,01 | 29,4972 |

Rådata og kategorier er i [audio-cost/artbell-cost.json](audio-cost/artbell-cost.json). Lytter gir ni kall per bilde, motor åtte, dekk fem og mottaker fire, totalt 26 før øvrig lydarbeid. Ved 60 bilder/s tilsvarer 1500/s bare 25 kall per bilde. Grensen var dermed lavere enn den observerte grunnkostnaden også med Art Bell Off. On/Off/On viser tilnærmet samme arbeid per bilde, uten ny dekoding eller stasjonssynkronisering i målevinduene. Ingen målte hovedbussutfall eller løpsk kilde-/nodevekst ble funnet. Den kontrollerte radiomotorprøven består også etter denne undersøkelsen, se [audio-cost/artbell-core.log](audio-cost/artbell-core.log).

Den oppdaterte `soundcheck` bruker **under 36 parameterkall per faktisk rendret bilde**, med krav om at bilder faktisk er telt. Dette gir omtrent 20 prosent margin over målt grunnarbeid på rundt 29,5 per bilde. Antall noder per sekund og lydutfall kontrolleres fortsatt separat. Dette er en endring av måleenhet med et målt On/Off-grunnlag, ikke en påstand om at den gamle 1500/s-prøven besto. Sluttresultatet fra den oppdaterte firestedsprøven var **ventende ved denne oppføringen**.

## Historiske forsøk og rettinger i testverktøyene

- [attempt-1](attempt-1/) bevarer første funksjonskjøring. Pausesjekken krevde synlig høyde mens pausemenyen bevisst skjulte hele HUD, selv om teksten kom tilbake ved Resume. Sjekken følger nå den opprinnelige caption-noden mens menyen står oppe og krever synlighet etterpå. En løs assignment ved gjenoppretting av dekoderen returnerte også en funksjon som Playwright automatisk kalte med `null`. Den er erstattet med en blokk uten retur. Begge sluttkjøringene er gjennomført på nytt.
- [legacy-harness-attempt-1](legacy-harness-attempt-1/) bevarer et tilsvarende Playwright-problem i den gamle radiotestens toast-wrapper. En funksjonsverdi fra assignment ble utilsiktet kalt med `null`. Radiotesten er rettet og kjørt på nytt, med resultatene i tabellen.
- [audio-cost/attempt-1](audio-cost/attempt-1/) bevarer første parvise måleforsøk. Probeinstallasjonen feilet med `Illegal invocation`, og tellere ble null. Disse tallene er ugyldige som kostbevis. Sluttproben kontrollerer at de native tellerne faktisk er installert og i bruk, og den nye kjøringen består 17/17.
- Den reelle caption-fristen bruker nå den pausefølsomme lydklokken. Toast-prober videresender det tredje klokkeargumentet, slik at instrumenteringen beholder denne oppførselen. Gaten for sen innlasting installeres direkte før riktig manusplan, så den ikke forbrukes av en gammel sending ved oppvåkning.

Ingen av de historiske feilforsøkene slettes eller presenteres som vellykkede sluttkjøringer.

## Ventende og begrensninger

- **VENTENDE:** sluttkjøring av oppdatert `soundcheck`, resterende kompatibilitetsprøver for eksisterende Kinski-funksjoner og den fulle pakken med 13 spillregresjoner. Nye logger eller uferdige filer i denne mappen er ikke automatisk PASS. Sluttansvarlig oppdaterer status når kjøringene er ferdige.
- **UNVERIFIED:** menneskelig lytting, likhet med Art Bell, klang, innlevelse, Caliche-uttalen og hørbar sømløshet mellom tidsfraser. ASR er en ufullkommen ordkontroll. Se [produksjonsoversikten](../README.md), [ASR-prøver](../verification.json) og [sammensatte tidsprøver](../clock-asr.json).
- **UNVERIFIED:** fysisk lyd fra PC-/mobilhøyttalere, ytelse og forsinkelser på spillerens maskinvare. PCM-, graf- og livssyklusmålinger beviser det som foregår i testnettleseren. Målte bilder/s i den avgrensede kostprøven er ikke en generell ytelsesgaranti.
- **Avgrenset dekning:** de lange opptakene er kontrollert som filer, planer og avbrudd. Nettleseren fullfører én kort naturlig replikk, og prøver representative klokkemaler. Den spiller ikke ferdig alle 115 opptak eller alle klokkeslett.
