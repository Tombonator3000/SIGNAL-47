# SC01 til SC13: verdensprodusenter, scenematrise og testsett

Generert fra world_producers.json. Dette er planleggingsdata: ingen rad betyr at scenen er integrert, bygget eller prøvd i spillet.

Flagg: 90 (17 finnes i koden, 45 delvis, 28 må bygges). Scener: 77. Tester: 117.

## SC01

SC01 er levert med pakket teknisk aksept og flettet i main (PR #174). Alle seks SC01-flagg og dale_reference_heard produseres allerede i FS47Horror::Context. Gjenstår PR til installasjon og menneskelige prøver.

**Kode:**

- Ingen ny kode for SC01. FS47Horror::Save skriver i dag scenario legacyContinuation når bComplete; dette må endres i SC02-pakken, ikke her.

**Ressurser:**

- Ingen nye. SM_SC01_Hand, glassmerker og skjermspiler er egne Blender-kilder.

**Avhenger av:**

- Ingen.

**Bare på Kubuntu:**

- Installasjon som standard, menneskelig lytting, Xbox, helnatt.

**Risiko og avklaringer:**

- SARO/Sorrow-avvik i streng ASR for ett egennavn.

**Stemmer:** Alle fem SC01/H01-cues er tatt opp (voice/takes.json). Menneskelig lytting er UNVERIFIED.

### Verdensflagg

| Flagg | Status | Produsent | Handling i spillet | Tester |
|---|---|---|---|---|
| `ctx_hr_h01_ready` | existing | FS47Horror::Context via FS47Evening::Pack (handover_done) | Spilleren har faktisk tatt Dales telefonoverlevering ved SARO etter ankomst og parkering; Evening setter handover_done. | SC01-T01, SC01-T05 |
| `ctx_hr_h01_checked` | existing | FS47Horror::NormalComplete via FS47HorrorVisual (lampe/skjerm) og FS47Horror::DoorChanged | Kamera hentet, østdør og annexutgang faktisk åpnet innenfra, kalibrering 1419.900 MHz bestått, lampe sammenlignet direkte og gjennom glass, referansefoto tatt og opak skjerm prøvd. | SC01-T01, SC01-T05 |
| `ctx_hr_h01_insufficient` | existing | FS47Horror::Context (Ready && !NormalComplete) | Overlevering gjort, men minst én fysisk del av normalrunden mangler når spilleren bruker skiftmappen. | SC01-T02, SC01-T05 |
| `ctx_hr_sc01_marks_checked` | existing | FS47HorrorVisual::MarksChecked via FS47Horror::Context | Spilleren har sett glassmerkene fra riktig side med romlampen av og på, med faktisk siktstråle til merkegeometrien. | SC01-T01, SC01-T02 |
| `ctx_hr_sc01_report_readback` | existing | FS47Horror::FinishCall (bReport) | Spilleren ringer dispatch fra kontrollromstelefonen etter faktisk sett hånd og fullfører hele readback-samtalen. | SC01-T01, SC01-T05 |
| `ctx_hr_sc01_window_seen` | existing | FS47HorrorVisual::Step (bHandSeen) via FS47Horror::Context | Den bevegelige hånden har vært gjengitt og i ubrutt sikt i minst 0,2 sekunder utenfor nordvinduet. | SC01-T01, SC01-T02, SC01-T06 |

### Scener

**`H01_BRIEF`**: Første opphold i saro etter handover_done; også fra telefonens rad 'Ask Dale about the evening round'.

- Normalt: Dale gir tre korte linjer om referanse, kamera ved østdøren og utgangene.
- Avbrudd: Ved pause eller panel stopper stemmen; scenen gjenopptas med bResumeRequested og tidligere linje vises igjen uten dobbel kreditt.
- Alternativ: Ingen: scenen er orientering og har ingen valg.
- Stemmecues: 3, innspilt 3
- Modeller: Eksisterende SARO-kontrollrom og telefon
- Lyd: Telefonkanal for Dale
- Dokumenter: ingen
- Tester: SC01-T01, SC01-T03, SC01-T07

**`H01_CHECK`**: Bruk skiftmappen (logbook) i kontrollrommet etter fullført normalrunde.

- Normalt: Beskrivelse bekrefter lampe og prøvde dører; hr_h01_recorded settes.
- Avbrudd: Lukket panel gir ingen kreditt; Context avbryter scenen hvis kravet ikke lenger gjelder.
- Alternativ: H01_INCOMPLETE vises i stedet ved manglende del.
- Stemmecues: 0, innspilt 0
- Modeller: Skiftmappe
- Lyd: ingen nye
- Dokumenter: ingen
- Tester: SC01-T01, SC01-T05

**`H01_INCOMPLETE`**: Bruk skiftmappen før normalrunden er fullført.

- Normalt: Viser neste gjennomførbare handling uten å markere fullført.
- Avbrudd: Kan åpnes på nytt; ingen lagret progresjon endres.
- Alternativ: Spilleren fullfører runden og får H01_CHECK.
- Stemmecues: 0, innspilt 0
- Modeller: Skiftmappe
- Lyd: ingen nye
- Dokumenter: ingen
- Tester: SC01-T02, SC01-T05

**`SC01_WINDOW_CHECK`**: Automatisk i saro etter sett hånd og kontrollerte merker.

- Normalt: To linjer om hånd og merker på utsiden; hr_sc01_window_check_presented.
- Avbrudd: Utsettes mens panel er åpent; vises når modal er lukket.
- Alternativ: Oversett hånd: scenen vises ikke, SC01 kan fullføres uten.
- Stemmecues: 0, innspilt 0
- Modeller: SM_SC01_Hand; glassmerker; opak skjerm
- Lyd: Romtone
- Dokumenter: ingen
- Tester: SC01-T01, SC01-T02, SC01-T06

**`SC01_FIRST_REPORT`**: Telefonrad 'Report the person outside the window to dispatch' etter sett hånd.

- Normalt: Dispatch leser tilbake sted og tid; HR_INITIAL_REPORT arkiveres med ReportStamp.
- Avbrudd: Avbrutt samtale lagres med callActive og callCursor og gjenopptas; ingen readback før siste linje.
- Alternativ: Uten sett hånd finnes ikke raden.
- Stemmecues: 2, innspilt 2
- Modeller: Kontrollromstelefon
- Lyd: Telefonkanal, dispatch
- Dokumenter: HR_INITIAL_REPORT
- Tester: SC01-T01, SC01-T04, SC01-T07

### Testsett

| Test | Type | Dekker | Prøve |
|---|---|---|---|
| SC01-T01 | input | normal | Eksisterende pakket S47HorrorTest High og Low: native gange, begge dører, kalibrering, lampe direkte og gjennom glass, referansefoto, skjerm, sett hånd, merker og dispatchrapport. FAIL hvis en av 120 kontroller eller 17 restores feiler. |
| SC01-T02 | input | alternative, negative | Kjøring med -S47HorrorMissHand: hånden oversees. Må gi window_seen=false, ingen telefonrad for rapport, report_readback=false, og SC01 kan likevel fullføres. FAIL ved oppdiktet kropp eller rapport. |
| SC01-T03 | api | interrupt | Pause, panel og dialog midt i vindushendelsen og H01_BRIEF. Scenen kanselleres av Context ved ugyldige krav og gjenopptas uten dobbel kreditt. FAIL hvis flagg settes under modal. |
| SC01-T04 | input | continue | Fem separate prosesser med vanlig Continue fra før normalrunden, midt i vindushendelsen, etter den, før readback og etter fullføring. FAIL ved tapt eller oppdiktet tilstand. |
| SC01-T05 | api | negative, regression | Dialogvalg kan ikke sette noe ctx_hr-flagg; H01_CHECK kan ikke starte før NormalComplete; FinishCall uten HandSeen gir ikke bReport. Gamle saker uten horror-felt får ingen nye krav. |
| SC01-T06 | image | image | 32 vurderte originalbilder High/Low fra spillerhøyde av hånd, merker, skjerm og ankomster. FAIL hvis hånd eller merker ikke er lesbare der kontrakten krever det. |
| SC01-T07 | api | audio | De fem opptakene (H01_BRIEF_001 til 003, SC01_FIRST_REPORT_001 og 002) ruter telefonkanal med positive source- og routed-samples; daleReferenceHeard settes bare etter full H01_BRIEF_002 med lyd på. FAIL ved hoppet linje som gir hørt. |
| SC01-T08 | input | regression | Regresjon på samme pakke: kontrollrom, T01 53/19, T02 65/15, Dale 191/5, LineLights, feltlab og finale. |
| SC01-T09 | human | audio, image | Menneskelig lytting, stemmelikhet, frykt og tempo samt fysisk Xbox. Står UNVERIFIED til Tom har prøvd. |

## SC02

SC02-kandidaten har eksisterende vert, blek original aktør, native import og live-kontrakt for T01, målt syn, fangst, lokal retry og flukt. Hindringsnavigasjon og hele flukt-/restorekontrollen ferdigstilles før senere scenarioer tar mønsteret i bruk. Dette er kildebeskrivelse, ikke godkjent pakket integrasjon.

**Kode:**

- S47Horror.cpp/.h: innholdsrevisjon 2, fase, AtServiceFront, Context, TriggerSC02 og LocalRetry finnes i kandidaten.
- S47HorrorThreat.cpp/.h: målt syn/lyd, Warn/Investigate/Search/Chase/Capture/Withdrawn, fysisk aktør og rutebevegelse finnes; hindringsnavigasjon skal ha faktisk trinnklarering.
- S47HorrorTest.cpp: faktisk T01 og en tilsiktet fangst, fersk disk-Continue og legitim flukt til SARO 07. Ny utilsiktet fangst skal gi tydelig testfeil, ikke en lang retryløkke.
- Bevar FS47Horror::Busy/US47Main::OneAtATime under møtet så gamle bortføringer holdes unna. SC02s lokale retry bruker ingen gammel fangst-/tidstapsløype.

**Ressurser:**

- Blek reaktiv aktør: original Blender-kilde, FBX med LOD0/LOD1, enkel rigg eller stive ledd; eget hudmateriale avledet fra SC01-hånden. Egen kilde, ingen tredjepartsmodell.
- Lyd: fottrinn på grus, vending og lav pust, egen syntese eller CC0 med kilde ført.
- Eventuell ekstra dekningsflate ved skuret bare hvis rutemåling viser behov; må være faktisk kollisjon.

**Avhenger av:**

- Eksisterende SC01 og T01 R2 i main.
- Kandidatens innholdsrevisjon 2 og lagringskontrakter beholdes.
- To faktisk farbare linjer utenfor gjerdet med oppdatert kroppsklaring.

**Bare på Kubuntu:**

- Blender-modell og FBX-import.
- Editor- og Linux-pakkebygg.
- Pakkede native reiser, High/Low-bilder, Continue i separate prosesser.
- RT-minnemåling etter ny aktør (under 80 MiB AlwaysResident).

**Risiko og avklaringer:**

- T01-arkiveringen knyttes én gang til originalene i SC04. Automatisk plan forblir sperret ved passiv residual i SC05, etter Toms beslutning.
- Fri truckparkering og nye hindringer må inngå i navigasjon og reell fluktprøve.
- Ingen teleport eller gjennomgang av tynn vegg som fastståings-/fangstløsning.
- Den eksisterende H02-walkiesamtalen og alle SC02-scener/lagringsflagg bevares ved den reviderte kildepakken.

**Stemmer:** H02_BRIEF_001, H02_BRIEF_002 og H02_BRIEF_003 (DALE, walkie) har status pending i recording_manifest.json og finnes ikke i voice/takes.json. Ingen andre SC02-scener har stemmer.

### Verdensflagg

| Flagg | Status | Produsent | Handling i spillet | Tester |
|---|---|---|---|---|
| `ctx_hr_h02_ready` | existing | FS47Horror::Context og AtServiceFront i den eksisterende SC02-kandidaten | SC01 er fullført og spilleren står faktisk til fots ved Caliche Junctions merkede kontrollfront utenfor gjerdet. | SC02-T01, SC02-T02, SC02-T07 |
| `ctx_hr_h02_checked` | existing | FS47Substation::HasNormalResult via FS47Horror::Context | Journal lest, tresekunders servicekontroll kjørt ferdig og den faktisk gjengitte lampen sammenlignet med panelresultatet. | SC02-T02, SC02-T07 |
| `ctx_hr_h02_insufficient` | existing | FS47Horror::Use/Context med bH02Attempt fra faktisk uavklart lampeforsøk | Spilleren har lest journalen eller startet kontrollen, men forsøker å avslutte eller sammenligne uten fullført, gjengitt resultat. | SC02-T05, SC02-T07 |
| `ctx_hr_sc02_t01_complete` | existing | FS47Substation::HasNormalResult (lagret med RequestAutosave i FS47Substation::Observe) | Fysisk T01: journal, kontroll og observert lampe er fullført og autolagret før møtet starter. | SC02-T02, SC02-T04, SC02-T07 |
| `ctx_hr_sc02_threat_visible` | existing | FS47HorrorThreat::BodyInView/Think og FS47Horror::StepSC02 | Etter sikret T01 ser spilleren den bleke kroppen gjengitt i ubrutt sikt, bevege seg og vende seg mot en ny lyd, før noen fangst er mulig. | SC02-T02, SC02-T07, SC02-T08 |
| `ctx_hr_sc02_escape_complete` | existing | FS47Horror::StepSC02 leser trusselens tilbaketrekning etter faktisk kjøring | Spilleren når SARO 07 via en av to ganglinjer utenfor gjerdet med brutt sikt og lyd, eller kjører bort på klar rute; aktøren mister kontakt og trekker seg tilbake. | SC02-T02, SC02-T03, SC02-T07 |
| `ctx_hr_sc02_local_retry` | existing | FS47Horror::LocalRetry fra FS47HorrorThreat fangst i SC02-kandidaten | Spilleren blir faktisk fanget i møte 1; spillet gjenoppretter lokalt dekningspunkt ved skuret med T01 bevart, uten gammel bortføring eller tidstap. | SC02-T04, SC02-T06, SC02-T07 |

### Scener

**`H02_BRIEF`**: Spilleren går ut av SARO 07 til fots ved Caliche Junctions kontrollfront etter fullført SC01.

- Normalt: Dale over walkie: klagen gjelder inspeksjonslampen, bruk merket kontroll utenfor gjerdet.
- Avbrudd: Pause eller panel stopper walkie-stemmen; gjenopptas ved neste frie øyeblikk uten å starte på nytt.
- Alternativ: Spiller som allerede har lest journalen får briefen én gang uten å miste T01-fremdrift.
- Stemmecues: 3, innspilt 0
- Modeller: Eksisterende trafostasjon (prop:substation*); SARO 07
- Lyd: Walkie-kanal med eksisterende FS47WalkieSound-klikk
- Dokumenter: EV_TRAFO_CARD
- Tester: SC02-T02, SC02-T05, SC02-T09

**`H02_CHECK`**: Etter FS47Substation::Observe med gjengitt resultat (bekreftelsen kommer ved kontrollfronten).

- Normalt: Beskrivelsen sier at lampe og indikator stemmer og at byforsyningen ikke er fastslått; hr_h02_recorded.
- Avbrudd: Hvis panel er åpent ved fullført kontroll utsettes scenen; T01-resultatet beholdes.
- Alternativ: H02_INCOMPLETE ved uavklart forsøk.
- Stemmecues: 0, innspilt 0
- Modeller: substationStatusLamp; substationLever
- Lyd: Relé/klikk fra servicekontroll
- Dokumenter: EV_TRAFO_CHECK
- Tester: SC02-T02, SC02-T07

**`H02_INCOMPLETE`**: Faktisk forsøk ved kontrollfronten uten fullført gjengitt resultat.

- Normalt: Viser at kalibrering kan fortsette og at kontrollen kan tas opp igjen.
- Avbrudd: Ingen tilstand går tapt; kan vises igjen.
- Alternativ: Spilleren fullfører og får H02_CHECK.
- Stemmecues: 0, innspilt 0
- Modeller: substationStatusLamp
- Lyd: ingen nye
- Dokumenter: ingen
- Tester: SC02-T05, SC02-T07

**`SC02_RETURN_WARNING`**: Etter sikret T01 når den bleke kroppen er faktisk sett mellom kontrollfronten og SARO 07.

- Normalt: Beskrivelsen peker på opak skurvegg og ruten utenfor gjerdet; spilleren trekker seg bak SiteShed.
- Avbrudd: Pause eller panel fryser aktøren uten oppspart tilnærming; scenen vises først når modal er lukket og ikke under aktiv jakt.
- Alternativ: Spilleren kan løpe direkte til bilen dersom kryssingen er klar.
- Stemmecues: 0, innspilt 0
- Modeller: Ny blek reaktiv aktør (original Blender); prop:substationSiteShed; gjerde og porter
- Lyd: Aktørens fottrinn og vending (romlig SFX); Fremmed lys over stolpene (lav drone)
- Dokumenter: ingen
- Tester: SC02-T02, SC02-T05, SC02-T08

**`SC02_ESCAPE`**: Når escape_complete settes etter gang eller kjøring.

- Normalt: Bilen starter, søket er borte; HR_TRAFO_ENCOUNTER arkiveres, neste mål er Caliche.
- Avbrudd: Hvis spilleren er i kjøring vises teksten etter stopp; lagring etter flukt gir ingen ny jakt.
- Alternativ: Rute A, rute B eller direkte kjøring gir samme scene.
- Stemmecues: 0, innspilt 0
- Modeller: SARO 07; blek aktør
- Lyd: Motorstart, avtagende aktørlyd
- Dokumenter: HR_TRAFO_ENCOUNTER
- Tester: SC02-T02, SC02-T03, SC02-T06

**`SC02_RETRY`**: Etter faktisk fangst i møte 1.

- Normalt: Spilleren står ved dekningspunktet før kryssingen; T01 bevart.
- Avbrudd: Fangst under modal er umulig; Continue etter fangst starter på sjekkpunktet.
- Alternativ: Gjentatt fangst gir samme sjekkpunkt uten teller.
- Stemmecues: 0, innspilt 0
- Modeller: dekningspunkt ved skuret
- Lyd: Kort fangstlyd, ingen bortføringstone
- Dokumenter: ingen
- Tester: SC02-T04, SC02-T06

### Testsett

| Test | Type | Dekker | Prøve |
|---|---|---|---|
| SC02-T01 | data | regression, negative | Pakke og register: H02_BRIEF, H02_CHECK, H02_INCOMPLETE og SC02_* er registrert til SC02; ingen on_complete setter ctx_hr-flagg; FS47Horror::Trigger avviser SC02-scener i innholdsrevisjon 1. FAIL ved avvik. |
| SC02-T02 | input | normal | Pakket native reise: SC01 fullført, kjør SARO 07 til Caliche Junction, les journal, kjør kontroll, observer lampe, se blek aktør vende seg mot lyd, trekk bak SiteShed og gå rute A utenfor gjerdet til bilen, start og kjør. Må gi h02_checked, t01_complete, threat_visible og escape_complete i den rekkefølgen og HR_TRAFO_ENCOUNTER én gang. FAIL ved teleport eller flytt etter START. |
| SC02-T03 | input | alternative | To separate pakkede reiser: rute B rundt motsatt side av gjerdet, og direkte kjøreflukt når kryssingen er klar. Begge gir escape_complete uten fangst. FAIL hvis en klar kjørerute blokkeres av usynlig regi. |
| SC02-T04 | input | alternative, negative | Bevisst eksponering til fangst. Må gi local_retry, gjenoppretting ved dekningspunkt, T01 bevart (HasNormalResult uendret, CompletedStamp uendret), ingen FS47Abduction- eller FS47TruckTaken-fase, ingen klokketap. Senere arkivering i skiftmappen skjer nøyaktig én gang. FAIL ved dobbel arkivering eller tidstap. |
| SC02-T05 | api | interrupt | Pause, journalpanel, H02-dialog, walkie og områdebytte midt i møtet og midt i servicekontrollen. Aktøren får ingen oppspart tilnærming, fangst kan ikke skje under modal, Running pauses som i dag. FAIL ved flagg satt under panel. |
| SC02-T06 | input | continue | Separate prosesser med vanlig Continue fra: etter journal, etter T01 før møtet, midt i møtet, etter fangst og etter flukt. Midt i møtet skal gjenopprette sikkert punkt, ikke halv jakt. FAIL ved tapt T01 eller ny jakt etter flukt. |
| SC02-T07 | api | negative | Negative prøver: ingen threat_visible uten gjengitt kropp i fri siktlinje; skurveggen blokkerer; dialog kan ikke sette noe; passasje gjennom høyspenningsgården er umulig (kolliderer) og gir ingen flukt; escape_complete ikke mens søk er aktivt; retry gir ingen bevis; h02_insufficient og h02_checked aldri samtidig. |
| SC02-T08 | image | image | High/Low fra spillerhøyde: aktøren ved forvarsel, skurets opake side, begge ruter, gjerde og porter, bil. Originaler vurderes visuelt. FAIL ved usynlig aktør i forvarsel eller dekning som bare er visuell. |
| SC02-T09 | api | audio | H02_BRIEF_001 til 003 på walkie-kanal når takes finnes; inntil da tekst uten registrert avspilling. Aktørens fottrinn og vending har romlig kilde ved aktøren. FAIL hvis manglende take registreres som avspilt. |
| SC02-T10 | input | regression | Regresjon: S47EveningServiceTest, S47EveningServiceDriveTest (T01 R2), T02-followup, SC01 High og at tilfeldig bortføring og TruckTaken er reservert bort mens møtet pågår (US47Main::OneAtATime). |
| SC02-T11 | human | image, audio | Menneskelig prøve av lesbart forvarsel, reaksjonsmargin, frykt og Xbox. UNVERIFIED til prøvd. |

## SC03

Bygg SC03 i Caliche: egne readback-samtaler, vanlig identifisert Ortiz med vogn 23, synlig løft med stillestående bleke skikkelser, to dekningsmuligheter, fysiske etterspor og savnetsak. Betjentens spørsmål om 21:00-loggen gir returen til SARO.

**Kode:**

- S47Horror.cpp: navngitte verdenssamtaler og egen lagret SC03-fase; readback-fakta først ved fullført samtale.
- S47Caliche.cpp: town:payphone ruter til aktiv SC03 uten å ta over Town->bRinging/OnAnswer fra andre hendelser.
- Ny S47HorrorWorld.cpp: Ortiz/vogn-ankomst, interaksjon, målt synlig løft, bleke figurer i lyskanten, dekning, termose/grusmerker og betjentens faktiske ankomst.
- Begrenset sceneeierskap for eksisterende betjent/patruljebil og dens collidere; ordinær tidsplan gjenopptas etter sekvensen.

**Ressurser:**

- Ortiz: original figur og klær med redigerbar kilde.
- Hvit servicevogn 23 med avklart opphav, fysisk dør og målte collidere.
- Termose med Ortiz-navnelapp og merker i grusen.
- Fremmed lys og stillestående bleke figurer; ingen svartfigur.

**Avhenger av:**

- SC02-kilde og målrettet flukt-/restorekontroll i akt 1.
- Ferdig navngitt samtaleport og fysiske Ortiz-/vognassets.

**Bare på Kubuntu:**

- Blender-/nativeimport, pakket SC02 til SC03-reise og High/Low-bilder.
- Lyttprøve av endelige opptak til slutt.

**Risiko og avklaringer:**

- Egen vogn 23 må skilles fra SARO 07 og eksisterende etterlatt patruljebil.
- Melding og readback må fungere uten valgfri SC01-vindusmelding.
- Den bleke lyskanten gir ingen ny fienderegel eller jakt; safe retreat er oppnåelig både i stasjonen og egen bil.

**Stemmer:** Ward og Ortiz får opptak til slutt etter at historie/runtime er låst. Eksisterende betjent beholdes som tekstfigur; manglende takes er pending, ikke spilt.

### Verdensflagg

| Flagg | Status | Produsent | Handling i spillet | Tester |
|---|---|---|---|---|
| `ctx_hr_sc03_dispatch_readback` | partial | FS47Horror verdenssamtale (bWorldCall-mønsteret) startet fra Caliche-betalingstelefonen | Etter SC02-flukt bruker spilleren betalingstelefonen i Caliche og fullfører meldingen med sted, tid og readback. | SC03-T02, SC03-T06 |
| `ctx_hr_sc03_ortiz_assigned` | partial | FS47Horror Ward-samtale (samme samtaleport) i Caliche | Ward-samtalen fullføres faktisk i telefonen og Ortiz med vogn 23 avtales ved bensinstasjonen. | SC03-T02, SC03-T06 |
| `ctx_hr_sc03_ortiz_identified` | new | Ny FS47HorrorWorld Ortiz-aktør og vogn 23 | Ortiz ankommer synlig med hvit vogn 23 ved bensinstasjonen og spilleren bruker henne fysisk (Talk) innen rekkevidde. | SC03-T02, SC03-T06, SC03-T07 |
| `ctx_hr_sc03_lift_observed` | new | FS47HorrorWorld løftesekvens (lys, Ortiz uten støtte) med siktprøve | Spilleren ser Ortiz løftes i fremmed lys og forsvinne ved vogn 23, med stillestående bleke skikkelser i lyskanten. | SC03-T02, SC03-T06, SC03-T07 |
| `ctx_hr_sc03_safe_retreat` | new | FS47HorrorWorld nærværstilstand (kontakt tapt) pluss dekningssone ved telefon/bil | Etter tapet når spilleren opak dekning inne på stasjonen eller sin egen bil; nærværet i utkanten trekker seg tilbake uten jakt. | SC03-T02, SC03-T06 |
| `ctx_hr_sc03_traces_checked` | new | FS47HorrorWorld rekvisitter (vogn 23, termose og grusmerker) via FS47Interactable | Etter trygg retrett undersøker spilleren den tomme vognen, termosen med Ortiz-navnelapp og merkene i grusen. | SC03-T02, SC03-T06 |
| `ctx_hr_sc03_missing_readback` | partial | FS47Horror samtaleport (ny savnetsak) fra telefon | Spilleren melder Ortiz savnet fra fungerende telefon og hører hele readback, adskilt fra første utkalling. | SC03-T02, SC03-T06 |
| `ctx_hr_sc03_local_order_suspended` | partial | FS47Horror Ward-samtale; ny horror-tilstand localOrderSuspended | Spilleren fullfører Ward-samtalen etter savnetsaken; Ward suspenderer den lokale 06:00-serien. | SC03-T02, SC03-T09 |
| `ctx_hr_sc03_deputy_at_stop` | partial | Ny SC03-ankomst for eksisterende Caliche-betjent eid av FS47HorrorWorld | Etter savnetmelding kommer den kjente sheriffbetjenten fysisk til bensinstasjonen; spilleren bruker ham på stedet. | SC03-T02, SC03-T05, SC03-T06 |

### Scener

**`SC03_DISPATCH_RECEIPT`**: Betalingstelefonen i Caliche (town:payphone) etter SC02-flukt.

- Normalt: Dispatch kvitterer hendelsen og sender hjelp; HR_DISPATCH_RECEIPT.
- Avbrudd: Avbrutt samtale gjenopptas på samme linje; ingen readback før slutt.
- Alternativ: Ingen: samtalen er nødvendig for hjelpen.
- Stemmecues: 2, innspilt 0
- Modeller: Eksisterende payphone-proxy
- Lyd: Telefonkanal
- Dokumenter: HR_DISPATCH_RECEIPT
- Tester: SC03-T02, SC03-T04, SC03-T08

**`SC03_RELIEF_CONTACT`**: Etterfølgende Ward-samtale på samme telefon.

- Normalt: Ward avtaler Ortiz og vogn 23 ved bensinstasjonen.
- Avbrudd: Som over.
- Alternativ: Ingen.
- Stemmecues: 2, innspilt 0
- Modeller: payphone
- Lyd: Telefonkanal
- Dokumenter: ingen
- Tester: SC03-T02, SC03-T08

**`SC03_ORTIZ_IDENTIFY`**: Ortiz ankommer med vogn 23; spilleren bruker 'Talk'.

- Normalt: Ortiz identifiserer seg og ber om å bli vist stedet.
- Avbrudd: Samtalen kan avbrytes; Ortiz venter ved vognen.
- Alternativ: Ingen.
- Stemmecues: 3, innspilt 0
- Modeller: Ortiz (ny); vogn 23 (ny eller ServiceFleet-variant)
- Lyd: Fysisk kanal, romlig
- Dokumenter: ingen
- Tester: SC03-T02, SC03-T07, SC03-T08

**`SC03_HELPER_TAKEN`**: Etter observert løft og trygg retrett.

- Normalt: Ortiz løftes synlig, henger uten støtte og forsvinner. En termose ruller ut. Flere bleke skikkelser står i lyskanten; én vender hodet.
- Avbrudd: Vises ikke under aktiv kontakt; utsatt til ro.
- Alternativ: Spiller som ikke ser: løftet venter på sikt.
- Stemmecues: 0, innspilt 0
- Modeller: Ortiz og vogn 23; termose med navnelapp; bleke figurer i lyskanten
- Lyd: Fremmed lys, lav tone
- Dokumenter: ingen
- Tester: SC03-T02, SC03-T06

**`SC03_HELPER_TRACES`**: Undersøk den tomme vognen, termosen og grusmerkene etter ro.

- Normalt: Beskrivelse av tom vogn og navnelapp; HR_HELPER_TRACE_NOTE.
- Avbrudd: Ingen tidsfrist.
- Alternativ: Kan gjøres etter savnetmeldingen.
- Stemmecues: 0, innspilt 0
- Modeller: vogn 23 med åpen dør; termose med Ortiz-navnelapp; smale merker i grusen
- Lyd: ingen nye
- Dokumenter: HR_HELPER_TRACE_NOTE
- Tester: SC03-T02, SC03-T06

**`SC03_MISSING_RECEIPT`**: Telefon etter trygg retrett.

- Normalt: Dispatch registrerer savnetsak; HR_MISSING_RECEIPT.
- Avbrudd: Avbrutt samtale gjenopptas.
- Alternativ: Ingen.
- Stemmecues: 2, innspilt 0
- Modeller: payphone
- Lyd: Telefonkanal
- Dokumenter: HR_MISSING_RECEIPT
- Tester: SC03-T02, SC03-T08

**`SC03_LOCAL_ORDER_HOLD`**: Ward-samtale etter savnetsaken.

- Normalt: Ward suspenderer 06:00-serien; HR_LOCAL_HOLD_RECEIPT.
- Avbrudd: Avbrutt samtale gjenopptas.
- Alternativ: Ingen.
- Stemmecues: 2, innspilt 0
- Modeller: payphone
- Lyd: Telefonkanal
- Dokumenter: HR_LOCAL_HOLD_RECEIPT
- Tester: SC03-T02, SC03-T09

**`SC03_RECEIVER_QUESTION`**: Betjenten er faktisk ankommet bensinstasjonen etter savnetmeldingen; spilleren snakker med ham.

- Normalt: Han spør om SARO-mottakeren stod på 21:00 og ber om logg/stripskriver til savnetsaken. Dette begrunner SC04.
- Avbrudd: Avbrutt samtale kan fortsettes; ingen logghenteordre før siste replikk.
- Alternativ: Tidligere SC01-vindusmelding er ikke en forutsetning for den nye savnetsaken.
- Stemmecues: 0, innspilt 0
- Modeller: Eksisterende sheriffbetjent og patruljebil, fysisk ankomst ved stasjonen
- Lyd: Fysisk dialog, eksisterende betjent som tekst; nye Ward/Ortiz-opptak tas til slutt
- Dokumenter: ingen
- Tester: SC03-T02, SC03-T05, SC03-T06

### Testsett

| Test | Type | Dekker | Prøve |
|---|---|---|---|
| SC03-T01 | data | regression, negative | Register: åtte reviderte SC03-scener, inkludert betjentens spørsmål om loggen. ORTIZ og DEPUTY beholder egne identiteter; ingen svartfigur eller fjernet eksisterende NPC. |
| SC03-T02 | input | normal | Pakket reise fra SC02-flukt: kjør til Caliche, fullfør dispatch/Ward-samtaler, møt Ortiz ved vogn 23, se løftet, nå dekning, undersøk vogn/termose/grus, meld savnet, motta lokal ordrestans og møt betjenten som ber om SARO-loggen. Egne fakta og dokumenter oppstår én gang. |
| SC03-T03 | input | alternative | Separate kjøringer med dekning inne på stasjonen og i egen bil. Ny SC03-melding skal fungere også om SC01-hånden aldri ble sett og vindusmelding derfor mangler. |
| SC03-T04 | api | interrupt | Pause og panel under telefon, ventetid, Ortiz-ankomst og løft. Ingen oppspart bevegelse; løftet starter ikke under modal og ingen uopplevd observasjon registreres. |
| SC03-T05 | input | continue | Vanlig Continue etter dispatch, etter identifikasjon, etter løft før retrett og etter savnetsak. FAIL ved dobbelt dokument eller tapt fakta. |
| SC03-T06 | api | negative | Lyd eller lys alene gir ikke lift_observed; avbrutt readback gir ingen kvittering; traces krever ro og faktisk undersøkelse; dispatch-ord alene gir ikke deputy_at_stop. Ingen rednings-/jaktport i SC03. |
| SC03-T07 | image | image | High/Low av Ortiz, vogn 23, løftet og etterspor i Caliche om natten. |
| SC03-T08 | api | audio | Dispatch, Ward og Ortiz takes: telefonkanal og fysisk kanal; manglende takes registreres ikke som avspilt. |
| SC03-T09 | input | regression | Regresjon: Caliche-personer (Earl, sheriff, betjent), Glimpses, AnomalyWorld, tilfeldig bortføring reservert under løft, og at senere morgenserie-tekst ikke motsier lokal ordre. |
| SC03-T10 | human | image, audio | Menneskelig vurdering av Ortiz som vanlig person, løftets virkning, de bleke skikkelsene og den konkrete motivasjonen for returen. UNVERIFIED til prøvd. |

## SC04

Sikre mottakerloggen til savnetsaken og originalene, arkiver T01 én gang og sperr automatisk kjøreplan. Ett annexmøte med lang eller lydavledet flukt ender i låsbart kontrollrom og ro; ingen jakt ved bilen.

**Kode:**

- S47HorrorWorld: patruljebil følger til porten og forlater, uten å tvinge spilleren inn.
- S47Evening: behold én fysisk T01-arkivering sammen med sikringen av skiftoriginaler.
- S47HorrorVisual: faktisk plansperrebryter; passiv mottaking og senere kontrollarbeid forblir mulig.
- FS47HorrorThreat: annex-søk, opake dører/vegger og faktisk lokal lydavledning, to gangbare ruter og lokalt sjekkpunkt.
- S47Horror.cpp: samme allerede hørte Dale-fragment fra feil dør, sikkert kontrollrom og Ward på vanlig telefon; overgang til 00:13:47.

**Ressurser:**

- Plansperrebryter og merke (egen modell).
- Radio i annexet (eksisterende eller egen).
- Blek hånd i dørkarm (gjenbruk SM_SC01_Hand).

**Avhenger av:**

- SC03s savnetsak og faktisk betjent-/loggavtale i akt 1.
- FS47HorrorThreat med dokumentert hindringsnavigasjon og lokale sjekkpunkter.

**Bare på Kubuntu:**

- Pakkede annexreiser for lang rute og lydavledning, faktiske bilder og disk-Continue.
- Lyttprøve av samme Dale-fil fra ny posisjon.

**Risiko og avklaringer:**

- T01-filingen skal skje én gang i SC04; kopier/originaler må ikke dupliseres ved retry.
- Plansperren må bevare passiv residual i SC05 uten Survey.
- Å flykte til kontrollrommet skal gi ro; ingen ekstra bilsekvens eller huking.

**Stemmer:** Mangler: SC04_LIMITED_RETURN_001/002 og SC04_SAFE_ROOM_AGREEMENT_001/002/003 (WARD, pending). SC04_ANNEX_IMPRINT_001 gjenbruker H01_BRIEF_002 og trenger ikke nytt opptak.

### Verdensflagg

| Flagg | Status | Produsent | Handling i spillet | Tester |
|---|---|---|---|---|
| `ctx_hr_sc01_dale_reference_heard` | existing | FS47Horror::ObserveVoice (bReferenceHeard) | Spilleren har faktisk hørt hele H01_BRIEF_002 med lyd på, positive source- og routed-samples og full varighet. | SC04-T06, SC04-T08 |
| `ctx_hr_sc04_return_agreed` | partial | FS47Horror Ward-samtale fra Caliche-telefon | Fra trygg posisjon i Caliche fullfører spilleren samtalen med Ward om avgrenset retur. | SC04-T02, SC04-T06 |
| `ctx_hr_sc04_schedule_isolated` | partial | Ny fysisk plansperre i kontrollrommet (FS47HorrorVisual prop) koblet til AS47Story::StagePassed | Spilleren betjener en fysisk lokal auto/surveyplansperre i kontrollrommet og ser kontrollresultatet (lampe/merke) bekreftet. | SC04-T02, SC04-T06, SC05-T02 |
| `ctx_hr_sc04_shift_originals_secured` | partial | FS47Evening::Use (skiftmappe/logbook) utvidet med sikring av originaler | Spilleren tar de daterte skift- og serviceoriginalene fra skiftmappen i kontrollrommet. | SC04-T02, SC04-T06 |
| `ctx_hr_sc04_annex_imprint_playing` | partial | Ny annexavspilling med FS47RecordedDialogue-take H01_BRIEF_002 og US47Audio::Spatial | Mens spilleren er i annexet spiller det samme Dale-fragmentet fra feil dørretning. | SC04-T02, SC04-T08 |
| `ctx_hr_sc04_annex_long_escape` | new | FS47HorrorThreat annexinstans | Spilleren lukker kjent dør, bryter sikt bak opak vegg og fullfører den lange passasjen; søket avsluttes. | SC04-T02, SC04-T06 |
| `ctx_hr_sc04_annex_sound_escape` | new | FS47HorrorThreat lydavledning fra vanlig lydkilde | Fra dekning slår spilleren på en vanlig radio; enheten undersøker lyden og spilleren krysser kort rute. | SC04-T03, SC04-T06 |
| `ctx_hr_sc04_annex_local_retry` | new | FS47Horror lokalt sjekkpunkt (annex) | Spilleren fanges i annexet og gjenopptar ved siste dekningspunkt med sperre og papirer bevart. | SC04-T04, SC04-T05 |
| `ctx_hr_sc04_control_room_safe` | partial | FS47Horror::DoorsSecured pluss trusseltilstand | Spilleren er inne i kontrollrommet med østdør og annexutgang lukket, uten aktiv kontakt eller søk. | SC04-T02, SC04-T06 |
| `ctx_hr_sc04_ward_on_line` | partial | FS47Horror::Telephone (kontrollromstelefon) | Spilleren ringer Ward fra kontrollromstelefonen og samtalen er faktisk koblet opp. | SC04-T02, SC04-T06 |

### Scener

**`SC04_LIMITED_RETURN`**: Ward-samtale fra Caliche etter SC03.

- Normalt: Avtale om avgrenset retur, sikring av plan og skiftlogg.
- Avbrudd: Avbrutt samtale gjenopptas.
- Alternativ: Spilleren kan bli i Caliche; ingen global reisesperre.
- Stemmecues: 0, innspilt 0
- Modeller: payphone
- Lyd: Telefonkanal
- Dokumenter: ingen
- Tester: SC04-T02, SC04-T08

**`SC04_LOCAL_HOLD`**: Etter fysisk plansperre og sikrede originaler i kontrollrommet.

- Normalt: Bekreftelse; HR_LOCAL_ISOLATION_NOTE.
- Avbrudd: Utsettes ved panel.
- Alternativ: Rekkefølgen sperre og originaler er fri.
- Stemmecues: 0, innspilt 0
- Modeller: Ny plansperrebryter; skiftmappe
- Lyd: Bryterklikk
- Dokumenter: HR_LOCAL_ISOLATION_NOTE, log
- Tester: SC04-T02, SC04-T06

**`SC04_ANNEX_IMPRINT`**: Spilleren går gjennom annexet etter sperre.

- Normalt: Identisk Dale-fragment fra feil dør.
- Avbrudd: Pause stopper avspillingen; ingen hørt-kvittering ved hopp.
- Alternativ: Uten tidligere hørt referanse spilles ikke avtrykket.
- Stemmecues: 1, innspilt 0
- Modeller: annex
- Lyd: Romlig radiokanal, feil dørretning
- Dokumenter: ingen
- Tester: SC04-T02, SC04-T08

**`SC04_ANNEX_LONG_ESCAPE`**: Etter lang passasje fullført.

- Normalt: Kontakt brutt.
- Avbrudd: Pause fryser søk.
- Alternativ: Kort rute via lyd.
- Stemmecues: 0, innspilt 0
- Modeller: blek aktør; annexdører
- Lyd: Fottrinn, dørlyd
- Dokumenter: ingen
- Tester: SC04-T02, SC04-T05

**`SC04_ANNEX_SOUND_ESCAPE`**: Etter lydavledning og kort passasje.

- Normalt: Kontakt brutt.
- Avbrudd: Pause fryser søk.
- Alternativ: Lang rute.
- Stemmecues: 0, innspilt 0
- Modeller: vanlig radio i annexet
- Lyd: Radio som romlig lydkilde
- Dokumenter: ingen
- Tester: SC04-T03, SC04-T05

**`SC04_ANNEX_RETRY`**: Etter fangst i annexet.

- Normalt: Tilbake til siste dekningspunkt.
- Avbrudd: Continue starter på sjekkpunktet.
- Alternativ: Gjentatt fangst gir samme punkt.
- Stemmecues: 0, innspilt 0
- Modeller: annex
- Lyd: ingen nye
- Dokumenter: ingen
- Tester: SC04-T04, SC04-T05

**`SC04_SAFE_ROOM_AGREEMENT`**: Kontrollromstelefon når rommet er trygt.

- Normalt: Ward på linjen; avtale om loggsikring.
- Avbrudd: Avbrutt samtale gjenopptas.
- Alternativ: Spilleren kan forlate rommet via prøvd rute.
- Stemmecues: 3, innspilt 0
- Modeller: Kontrollromstelefon
- Lyd: Telefonkanal
- Dokumenter: ingen
- Tester: SC04-T02, SC04-T08

### Testsett

| Test | Type | Dekker | Prøve |
|---|---|---|---|
| SC04-T01 | data | regression, negative | Register: sju SC04-scener. ANNEX_IMPRINT gjenbruker H01_BRIEF_002 uten nytt opptak. Ingen bilsekvens, huking eller scene som opphever plansperren. |
| SC04-T02 | input | normal | Pakket reise etter betjentens loggspørsmål: kjør til SARO med patruljebil til porten, sikre mottakeroriginaler og T01-journal én gang, fysisk plansperre, annexavtrykk og lang flukt, trygt kontrollrom og Ward på vanlig telefon. |
| SC04-T03 | input | alternative | Alternativ annexflukt: slå på en faktisk vanlig lydkilde fra trygg plass, observer at skapningen undersøker den, og ta den korte ruten. Ingen biljakt. |
| SC04-T04 | input | alternative, negative | Fangst i annex gir lokalt sjekkpunkt før annexet med plansperre, originaler og T01 bevart. Ingen gammel bortføringsteller eller tidstap. |
| SC04-T05 | input | continue, interrupt | Vanlig Continue og pause/panel midt i annexmøtet, etter flukt og i kontrollrommet. Mid-møte gjenopptas trygt, uten halv jakt eller doble originaler. |
| SC04-T06 | api | negative | Imprint krever tidligere faktisk hørt Dale-linje; lydavledning krever faktisk lydkilde; glass alene er ikke opak dekning; control_room_safe krever lukket dør. Dialog alene gir ingen fysisk flukt. |
| SC04-T07 | image | image | High/Low av plansperre, annexets to ruter, hånd i dørkarm og trygt kontrollrom. Fysisk skjerming og ruter er lesbare. |
| SC04-T08 | api | audio | Imprint spiller samme fil fra feil retning med positive samples; Ward-takes på telefonkanal; radio som lydkilde har posisjon. |
| SC04-T09 | input | regression | Regresjon: T01-arkivering én gang, kontrollrom, SaroDark og TruckTaken reservert, eksisterende annexdører. |
| SC04-T10 | human | image, audio | Menneskelig vurdering av jumpscare, ro, tro på valget om å bli i rommet og Xbox. UNVERIFIED til prøvd. |

## SC05

Lag en passiv residualovergang fra trygt kontrollrom som ikke starter Survey, og behold eksisterende utskrift, telefon og romhandling 47 spillsekunder senere. Gamle lagringer beholder dagens kjede.

**Kode:**

- S47Story.cpp: ny passiv vei til AS47Story::EnterResidual uten SetPhase(ES47Phase::Survey), RelayFirst og LightsDip; AS47Story::TimeSkip-teksten 'Routine survey completed' må ikke vises i ny rute.
- S47Horror.cpp: SC05-fase setter ctx_hr_sc05_residual_passive når residual vises og sperren er på.

**Ressurser:**

- Ingen nye.

**Avhenger av:**

- SC04 levert med plansperre.

**Bare på Kubuntu:**

- Sammenhengende pakket gjennomspilling SC01 til SC05.

**Risiko og avklaringer:**

- Eksisterende residual er bundet til Survey og TimeSkip; omlegging berører finalekjeden.
- Tekstene 'Babysit the survey until 06:00' i AS47Story::Tasks motsier lokal ordre.

**Stemmer:** SC05_PASSIVE_RESIDUAL har ingen stemmecues. Nye stemmer for Ward, Ortiz, Earl og Nora produseres til slutt, etter øvrig innhold.

### Verdensflagg

| Flagg | Status | Produsent | Handling i spillet | Tester |
|---|---|---|---|---|
| `ctx_hr_sc05_residual_passive` | partial | AS47Story::EnterResidual i ny passiv modus, styrt av FS47Horror | Residualen 00:13:47 vises i passiv mottaking mens plansperren fortsatt er på; spilleren er i kontrollrommet. | SC05-T02, SC05-T06 |

### Scener

**`SC05_PASSIVE_RESIDUAL`**: Kort fremoverovergang i kontrollrommet etter SC04-avtalen.

- Normalt: Residual 00:13:47 i passiv mottaking; spilleren finner 1420.405 MHz, gain 82, BW 12, az 083.
- Avbrudd: Pause og panel stopper ikke klokken feil; Continue gjenoppretter residual.
- Alternativ: Gamle lagringer går via eksisterende Survey.
- Stemmecues: 0, innspilt 0
- Modeller: Eksisterende konsoll
- Lyd: Alarm
- Dokumenter: ingen
- Tester: SC05-T02, SC05-T05

### Testsett

| Test | Type | Dekker | Prøve |
|---|---|---|---|
| SC05-T01 | data | regression, negative | Kanon: residual 00:13:47, 1420.405 MHz, gain 82, BW 12, az 083; notch 1420.110 MHz, gain 55, BW 12. |
| SC05-T02 | input | normal | Fra SC04-avtalen: fremoverovergang, residual i passiv modus, eksisterende utskrift, telefon og romkontroll 47 spillsekunder senere. |
| SC05-T03 | input | alternative | Gammel lagring uten SC04 går via eksisterende Survey-vei; spilleren utenfor rommet ved overgangen. |
| SC05-T04 | api | interrupt | Telefonpaneler er beskyttet; pause under residual og samtale. |
| SC05-T05 | input | continue | Continue fra residual og fra etter samtale i separat prosess. |
| SC05-T06 | api | negative | Survey-fasen settes ikke; residual_passive er false hvis plansperren er opphevet. |
| SC05-T07 | image | image | High/Low av konsoll med residual og sperrelampe. |
| SC05-T08 | api | audio | Alarm og telefon som i dag; ingen nye stemmer kreves. |
| SC05-T09 | input | regression | Regresjon: eksisterende residual til finale, S-03-avvik 042 og 026. |
| SC05-T10 | human | image, audio | Menneskelig gjennomspilling SC01 til SC05 uten utviklerhopp. UNVERIFIED til prøvd. |

## SC06

H03 er i hovedsak kapittel én (US47Chapter1) som allerede har B-12, passiv og aktiv kontroll, to eksponeringer, fremkalling og tåkelagt kopi. Arbeidet er å koble tilstanden inn i FS47Horror::Context og la H03-scenene starte fra faktiske handlinger.

**Kode:**

- Utvid FS47Horror::Context med H03-gren som leser US47Chapter1::S (F1, F2, Dev1, Dev2, Fog1, Fog2, Method, bObserved, bConcluded)
- Generaliser scenegyldighetssjekken i FS47Horror::Context fra SC01 til aktivt scenario
- Legg H03-start ved navngitt handling ved B-12 i FS47Horror::Use
- Utvid S47HorrorTest med passiv, aktiv og tåkelagt kopi

**Ressurser:**

- Ingen nye modeller; eksisterende B-12, mørkerom og feltkamera gjenbrukes

**Avhenger av:**

- SC05 levert med H02 registrert
- Felles horror-lagring fra SC01

**Bare på Kubuntu:**

- Pakket bygg og High/Low-bilder av mørkerom og kopier
- Fersk Continue i separat prosess

**Risiko og avklaringer:**

- Rammebudsjettet: SC01 reserverer siste eksponering; H03 trenger to frie rammer
- Leselighetsterskel for kontrollert detalj er ikke definert i koden

**Stemmer:** H03-scenene er DESCRIPTION uten stemmecues; ingen opptak mangler.

### Verdensflagg

| Flagg | Status | Produsent | Handling i spillet | Tester |
|---|---|---|---|---|
| `ctx_hr_h03_ready` | partial | FS47Horror::Context via US47Chapter1 (Started) og SC05-resultatet | SC05 er faktisk fullført (H02 registrert av verden), residualen 00:13:47 har gått, og spilleren har kapittel ens B-12/S-03-sak i gang ved SARO. | SC06-T01, SC06-T07 |
| `ctx_hr_h03_checked` | existing | FS47Horror::Context via US47Chapter1::S (FS47CaseState) | To faktiske eksponeringer fra B-12-siktlinjen (FRAME 01 og FRAME 02 under passiv lampe/motorisolering eller aktiv 042-kontroll), begge fremkalt og hentet (Dev1 og Dev2 = 3), direkte observasjon registrert og samme detalj sammenlignet på begge kopier. | SC06-T01, SC06-T02, SC06-T08 |
| `ctx_hr_h03_insufficient` | partial | FS47Horror::Context via US47Chapter1 mørkeromstilstand (Fog1/Fog2) og sammenligningsfeil | En kopi er faktisk ødelagt av lys gjennom mørkeromsdøren (negativet beholdes), eller den kontrollerte detaljen er ikke lesbar på kopien som skal sammenlignes. | SC06-T03, SC06-T04 |

### Scener

**`H03_BRIEF`**: Spilleren tar opp feltkameraet ved S-03/B-12 etter at SC05 er fullført.

- Normalt: Startteksten vises én gang og peker på FRAME 01 og B-12.
- Avbrudd: Pause, panel eller dialog stanser visningen uten konklusjon; scenen gjenopptas fra lagret pakkestatus etter at verdensflaggene er gjenopprettet. Områdebytte avbryter uten å sette flagg.
- Alternativ: Ingen: briefen har ett faktisk startpunkt.
- Stemmecues: 0, innspilt 0
- Modeller: Eksisterende B-12, S-03-skap, feltkamera
- Lyd: Ingen ny effekt; UI-kanal
- Dokumenter: ingen
- Tester: SC06-T01, SC06-T07

**`H03_CHECK`**: Spilleren henter FRAME 02 fra fikseringen og sammenligner samme detalj på begge kopier.

- Normalt: Resultatet vises etter faktisk kontroll og H03 registreres.
- Avbrudd: Mørkeromsdør åpnet midt i fremkalling tåkelegger kopien; resultatet utsettes. Lagring midt i fremkalling gjenopprettes uten gratis kopi.
- Alternativ: Passiv lampe/motorisolering eller aktiv 042-kontroll; begge gir samme sluttscene med ulik observasjon.
- Stemmecues: 0, innspilt 0
- Modeller: Eksisterende mørkerom, vått bord, FRAME 01/02-kopier
- Lyd: Lukker, motor/vinge og mørkeromslyd på verdenskanal
- Dokumenter: P01, P02, P03
- Tester: SC06-T01, SC06-T02, SC06-T06

**`H03_INCOMPLETE`**: Spilleren henter en kopi som er tåkelagt av lys eller ulesbar.

- Normalt: Teksten viser at negativet beholdes og at ny kopi lages med lukket dør.
- Avbrudd: Kan vises igjen etter Continue så lenge kopien er ugyldig; forsvinner når gyldig kopi finnes.
- Alternativ: Ulesbar detalj på ellers fremkalt kopi gir samme scene.
- Stemmecues: 0, innspilt 0
- Modeller: Eksisterende mørkeromsdør og negativ
- Lyd: Ingen feil-lyd
- Dokumenter: ingen
- Tester: SC06-T03, SC06-T04

### Testsett

| Test | Type | Dekker | Prøve |
|---|---|---|---|
| SC06-T01 | api | normal | Kjør kapittel én med passiv metode til to hentede kopier og sammenligning. Context må gi ctx_hr_h03_ready og ctx_hr_h03_checked, og H03_BRIEF og H03_CHECK starter én gang. FAIL hvis checked er sann før Dev2 er 3. |
| SC06-T02 | input | alternative | Samme prøve med aktiv 042-kontroll via native input i den pakkede kandidaten. Begge metoder skal gi H03_CHECK; FAIL hvis bare én metode registreres. |
| SC06-T03 | input | interrupt, negative | Åpne mørkeromsdøren under fremkalling av FRAME 02. Forvent insufficient sann, checked usann og H03_INCOMPLETE. Lukk døren og lag ny kopi fra negativet; forvent checked. FAIL hvis dialog eller panel kan sette checked. |
| SC06-T04 | input | continue | Lagre under fremkalling og etter tåkelagt kopi, start ny prosess med vanlig Continue. Tilstand, negativ og dokumenter må være uendret og ikke deles ut på nytt. |
| SC06-T05 | image | image | High og Low fra spillerhøyde ved B-12, mørkerommet og begge kopier. FAIL ved uleselig merke eller lys i mørkerommet. |
| SC06-T06 | api | audio | Kontroller at lukker, vinge/motor og mørkeromslyd rutes på verdenskanal og at H03 ikke krever stemmetakes. |
| SC06-T07 | data | regression | Kjør Chapter1 SelfTest og SC01-prøvene. Eksisterende kapittel ens darkroom-kontroller og SC01-referanserammen må bestå. |
| SC06-T08 | data | negative | Pakkeprøve: H03_CHECK kan ikke startes med checked usann, og ingen scene setter ctx_hr_h03_*. |

## SC07

Kontoret, skuffen, reservenøkkelen og feltmappen finnes i FS47Office, og arkivet (E06/E07/E08, P04/P05) i US47Chapter2. Det mangler vindus/stolpekontroll, den faktiske VHS-filmen og settstatus.

**Kode:**

- Utvid FS47Horror::Context med H04-gren som leser FS47Office og US47Chapter2
- Legg vindus/stolpekontroll i FS47Office med siktstråle og lagring
- Bygg VHS-program for office:fieldDeck på FS47TvProgramme med spill, pause, stopp, spoling, strøm og lagret avspillingssted
- Sett vhs_seen bare fra faktisk avspilte bilder

**Ressurser:**

- Originalfilmen Service Road / Field 01, 72 sekunder, egen produksjon etter VHS_DALE_FIELD_01-storyboardet, med miljølyd; egne Blender-kilder eller CC0
- Vindusendring etter undersøkelsen

**Avhenger av:**

- SC06 levert
- EveningShift-eierskapet til kontor og VHS

**Bare på Kubuntu:**

- Render av VHS-filmen
- Pakket bygg og bilder
- Lytteprøve

**Risiko og avklaringer:**

- Kontrakten sier at EveningShift alene eier kontor, dokumenter og VHS, mens FS47Office ligger i Main: koordiner eierskap
- MISSION_FLOW sier arrayvedlegget ligger i feltmappen, mens koden har det som eget ark i kontorskapet

**Stemmer:** H04_BRIEF_001 til 003 (Dale, telefon, kinski) står som pending i recording_manifest.json og finnes ikke i voice/takes.json. Nye stemmer for Ward, Ortiz, Earl og Nora produseres til slutt, etter øvrig innhold.

### Verdensflagg

| Flagg | Status | Produsent | Handling i spillet | Tester |
|---|---|---|---|---|
| `ctx_hr_h04_ready` | partial | FS47Horror::Context via US47Chapter2 og FS47Office | H03 er registrert etter faktisk fotokontroll, kapittel to er i gang ved SARO, og kontorruten (serviceskuff, reservenøkkel, kontordør) er fysisk tilgjengelig. | SC07-T01, SC07-T07 |
| `ctx_hr_h04_checked` | partial | FS47Horror::Context via FS47Office (feltmappe) og US47Chapter2 (P04/P05) pluss ny vindu/stolpekontroll | Spilleren har tatt feltmappen fra kontoret med faktisk reservenøkkel og åpnet dør, og har holdt Dales daterte ståstedsnotat mot faktisk vindusutsyn og referansestolpe. | SC07-T01, SC07-T02, SC07-T08 |
| `ctx_hr_h04_insufficient` | partial | FS47Horror::Context via FS47Office og ny VHS-avspillingstilstand | Feltmappen er tatt, men VHS-filmen er ikke sett til ende (strømløs spiller, pause eller avbrutt avspilling). | SC07-T03, SC07-T04 |

### Scener

**`H04_BRIEF`**: Spilleren undersøker den låste kontordøren eller serviceskuffen etter H03.

- Normalt: Dale på telefon peker på reservenøkkelen og feltmappen.
- Avbrudd: Telefonen kan avbrytes; uopplevde replikker gir ingen nøkkel. Dialog setter ingen verdensfakta.
- Alternativ: Spilleren kan finne nøkkelen før briefen; briefen vises da fortsatt én gang uten å gjenta nøkkelutdeling.
- Stemmecues: 3, innspilt 0
- Modeller: Eksisterende kontor, serviceskuff, messingnøkkel
- Lyd: Telefonkanal for Dale; skuff og lås på verdenskanal
- Dokumenter: EV_DALE_INDEX, EV_DALE_NOTE
- Tester: SC07-T01, SC07-T06

**`H04_CHECK`**: Spilleren holder Dales ståstedsnotat ved kontorvinduet mot referansestolpen.

- Normalt: Resultatet skiller kulvertplassen (T02) fra arrayservicekortet og STATION 01s historie.
- Avbrudd: Lesepanel åpent stanser kontrollen; vindusendringen skjer først etter at panelet er lukket.
- Alternativ: Spilleren kan ha sett VHS først eller hoppe over den; feltmappen alene bærer hovedløpet.
- Stemmecues: 0, innspilt 0
- Modeller: Kontorvindu, referansestolpe, feltmappe (ny vindu/stolpeinteraksjon)
- Lyd: Romlyd i kontoret
- Dokumenter: EV_DALE_INDEX, E06, E07, P04, P05
- Tester: SC07-T02, SC07-T05

**`H04_INCOMPLETE`**: Spilleren har feltmappen men har ikke sett VHS-filmen til ende.

- Normalt: Teksten fører filmen som usett og viser at feltkontrollen kan gjøres.
- Avbrudd: Strømbrudd eller pause i VHS lagrer avspillingssted; Continue gjenopptar derfra.
- Alternativ: Spilleren ser filmen senere; flagget faller da uten å gi bevis i seg selv.
- Stemmecues: 0, innspilt 0
- Modeller: Eksisterende VHS-spiller office:fieldDeck, ny film FIELD 01
- Lyd: VHS-lyd fra spillerens høyttaler (romkanal)
- Dokumenter: EV_DALE_INDEX
- Tester: SC07-T03, SC07-T04

### Testsett

| Test | Type | Dekker | Prøve |
|---|---|---|---|
| SC07-T01 | api | normal | Kjør skuff, nøkkel, dør og feltmappe. Forvent ready og etter vindu/stolpe checked. FAIL hvis H04_BRIEF gir nøkkel. |
| SC07-T02 | input | alternative | Ta feltmappen uten å se VHS og fullfør vindu/stolpe. Forvent at hovedløpet går videre. |
| SC07-T03 | input | interrupt, negative | Avbryt VHS med strømbrudd, pause og spoling. Forvent at settstatus ikke gis og at insufficient er sann. FAIL hvis spoling eller transkript gir settstatus. |
| SC07-T04 | input | continue | Lagre midt i VHS og etter feltmappen; fersk Continue gjenopptar fra lagret sted. |
| SC07-T05 | image | image | High og Low av kontoret, vindusendringen etter lukket panel og VHS-bildet. |
| SC07-T06 | human | audio | Lytt til Dales tre H04_BRIEF-takes på telefonkanal og VHS-miljølyden. Menneskelig vurdering. |
| SC07-T07 | data | regression | Kjør FS47Office SelfTest, EveningShift-pakkeprøver og T02-prøver. |
| SC07-T08 | data | negative | H04_CHECK kan ikke starte uten checked; ingen dialog setter ctx_hr_h04_*. |

## SC08

BY02 og ARRAY-01 er nesten helt nytt spill. Råvare finnes: Earls reel (FS47ReelTape, FS47LostNights), Earls vakttid (S47Timeline), lobbydøren, arrayservicearket og A/B/REF-tavlene i FS47Office, kommandoregisteret i FS47ArrayBack og følge/hold-logikken i FS47DishesFollow. Arrayets ene trusselmøte og lokal retry må bygges.

**Kode:**

- Ny S47By02.cpp: lobbyutlevering, spoleeske, TAPE/LIVE-velger med frakobling, varige prøver, spørsmål/gjentak, konvolutt etter 04:00
- Ny S47ArrayService.cpp: baseline på terminal, feltlykt, A/B-soner, retur, sammenligning, sluttkontroll, arrayets trusselmøte, hold og lokal retry
- Egen ARRAY-01-modus i FS47DishesFollow uten Warning/Force
- Reservasjon i nattkurven (S47Night/S47Away) og ArchiveDishes/ArrayBack under prøven
- Utvid FS47Horror::Context med alle 22 SC08-flagg

**Ressurser:**

- Spoleeske, off-air-logg, velger, frakoblingskabel, konvolutt, feltlykt og søkende figur; egne Blender-kilder eller CC0

**Avhenger av:**

- SC07-kilde og målrettet prøve i akt 2
- Felles inntrengeraktør fra SC02/SC04
- Felles lokalt sjekkpunkt

**Bare på Kubuntu:**

- Pakket bygg, native gange på arrayet, High/Low-bilder
- Blender-render av nye rekvisitter
- Lytteprøve av Earl og motorer

**Risiko og avklaringer:**

- Lobbyspolens verdensfakta kommer fra faktisk avspilling og sammenligning, ikke et dialogtema.
- SC08 avsluttes før 02:15; antennenes retur til 026 er scenens slutt, ikke en konkurrerende tilstand.
- Arrayservicearket er eget ark i kontorskapet, z -75.3. A/B-ståsteder er egne feltkoordinater, ikke plasseringen av skapet.
- Dynamisk tilgang og hindringsklarering i feltet må prøves.

**Stemmer:** BY02_REQUEST_001 og 002 og BY02_REEL_IDENTIFY_001 og 002 (Earl) er pending. Nye stemmer for Ward, Ortiz, Earl og Nora produseres til slutt, etter øvrig innhold.

### Verdensflagg

| Flagg | Status | Produsent | Handling i spillet | Tester |
|---|---|---|---|---|
| `ctx_hr_by02_dated_lead` | partial | FS47Horror::Context via FS47LostNights (1981-natten bundet på saksbrettet) | Spilleren har funnet arkivbinderens 09/22/81-utskrift og bundet den til et lokalt 1981-spor på saksbrettet. | SC08-T01, SC08-T09 |
| `ctx_hr_by02_earl_present` | partial | FS47Horror::Context via S47Timeline::EarlOnDuty og spillerposisjon i 1420 AM-lobbyen | Spilleren står fysisk i den offentlige lobbyen på 1420 AM før 04:00 mens Earl er på vakt. | SC08-T01, SC08-T03 |
| `ctx_hr_by02_reel_identity` | partial | Ny lobbyutlevering i S47By02 med eksisterende FS47ReelTape som innhold | Spilleren undersøker den merkede 1981-spolen i lobbyen: eske, off-air-logg og Earls initialer sammen. | SC08-T01, SC08-T02 |
| `ctx_hr_by02_tape_isolated` | partial | Ny kildevelger i S47By02 som spiller FS47ReelTape med live-mating fysisk frakoblet | Spilleren setter den merkede velgeren på TAPE, ser at live-monitorens kabel er frakoblet og starter den identifiserte spolen på lobbydekket. | SC08-T02, SC08-T03 |
| `ctx_hr_by02_live_selected` | new | Ny kildevelger i S47By02 mot eksisterende 1420 AM-monitor | Spilleren setter velgeren på LIVE MONITOR; bånddekket er synlig frakoblet og kilden er merket. | SC08-T02, SC08-T03 |
| `ctx_hr_by02_question_compared` | new | Ny spørsmål/gjentaksport i S47By02 | Spilleren trykker på lobbyens talkback og stiller et nytt konkret spørsmål, spiller deretter samme isolerte kilde (original eller kopi) på nytt og hører at fragmentet er uendret. | SC08-T02, SC08-T09 |
| `ctx_hr_by02_tape_sample_verified` | new | Ny varig prøvelogg i S47By02 | En fullført TAPE-avspilling av den identifiserte originalspolen med live frakoblet, lagret som eget resultat. | SC08-T02, SC08-T04 |
| `ctx_hr_by02_live_sample_verified` | new | Ny varig prøvelogg i S47By02 | En fullført LIVE-lytteperiode med bånddekket frakoblet, lagret som eget resultat. | SC08-T02, SC08-T04 |
| `ctx_hr_by02_source_copy_verified` | new | Ny utleveringskonvolutt og lobbydekk i S47By02 etter 04:00 | Etter Earls avgang åpner spilleren den daterte utleveringskonvolutten, kontrollerer kildekopi, off-air-logg og frakoblingsark, og spiller samme kopi isolert på lobbydekket. | SC08-T05, SC08-T09 |
| `ctx_hr_by02_source_insufficient` | new | Ny utfallsvurdering i S47By02 | Spilleren har prøvd å avspille uten identifisert kilde, med begge kilder koblet, eller avbrutt prøven slik at opphav eller signalvei er uavklart. | SC08-T09, SC08-T03 |
| `ctx_hr_array_service_card_acquired` | partial | FS47Horror::Context via FS47Office (EV_DALE_ARRAY_SERVICE) | Spilleren åpner det ordinære arrayservicearket i kontorets skap, og kopien legges i notatboken. | SC08-T06, SC08-T11 |
| `ctx_hr_array_command_checked` | partial | Ny konsollkontroll i S47ArrayService som leser FS47ArrayBack::History | Ved operasjonsterminalen identifiserer spilleren nærgruppens antenner (S-02, S-03, S-04) og kontrollerer kommandoregister og lokal drivtilstand før feltet. | SC08-T06, SC08-T07 |
| `ctx_hr_array_reference_fixed` | partial | Ny feltlykt i S47ArrayService ved eksisterende REF / R1-tavle | Spilleren setter den navngitte feltlykten på REF / R1-merket med registrert innstilling og lar den stå. | SC08-T06, SC08-T07 |
| `ctx_hr_array_a_observed` | partial | Ny ståstedslogikk i S47ArrayService med siktelogikk fra FS47DishesFollow | Spilleren står på A / ROW 1 WEST og ser nærgruppen vende seg mot ståstedet, målt mot faste landemerker. | SC08-T06, SC08-T07 |
| `ctx_hr_array_b_observed` | partial | Ny ståstedslogikk i S47ArrayService med siktelogikk fra FS47DishesFollow | Spilleren står på B / ROW 1 EAST og ser nærgruppen vende seg mot ståstedet, målt mot faste landemerker. | SC08-T06, SC08-T07 |
| `ctx_hr_array_return_observed` | partial | Ny returlogikk i S47ArrayService | Spilleren går tilbake til ståstedet som ble valgt først og ser antennene vende tilbake mot det. | SC08-T07, SC08-T08 |
| `ctx_hr_array_geometry_compared` | new | Ny sammenligning i S47ArrayService | Etter A, B og retur sammenligner spilleren de observerte vinklene mot fast lykt og landemerker på servicearket. | SC08-T07, SC08-T08 |
| `ctx_hr_array_command_history_compared` | partial | Ny sluttkontroll i S47ArrayService mot FS47ArrayBack::History | Etter ruten kontrollerer spilleren kommandohistorikken igjen og ser at ingen vanlig kommando i prøveintervallet forklarer vinkelendringene. | SC08-T07, SC08-T08 |
| `ctx_hr_array_escape_complete` | new | Ny trusselmøte 3-kontroller i S47ArrayService med felles inntrengeraktør | Etter sammenligningen kommer en søkende figur inn i passasjen. Spilleren bryter kontakt bak opake fundament/skapflater eller krysser etter lest avsøkning, og figuren trekker seg tilbake. | SC08-T08, SC08-T10 |
| `ctx_hr_array_empty_hold_observed` | partial | FS47DishesFollow HoldPlace/GoHome i ny ARRAY-01-modus | Fra trygg dekning ser spilleren antennene holde kort på det sist forlatte tomme ståstedet og deretter vende tilbake til 026. | SC08-T08, SC08-T10 |
| `ctx_hr_array_local_retry` | new | Nytt lokalt sjekkpunkt i S47ArrayService | Figuren fanger spilleren i passasjen; spillet gjenoppretter dekket punkt før passasjen med kommando, kilde og geometri bevart. | SC08-T10, SC08-T04 |
| `ctx_hr_array_insufficient` | new | Ny utfallsvurdering i S47ArrayService | Spilleren forlater feltet eller flytter lykten før A, B, retur og kommandokontroll er gjort. | SC08-T09, SC08-T08 |

### Scener

**`BY02_REQUEST`**: Spilleren står i 1420 AM-lobbyen før 04:00 med dateret 1981-spor.

- Normalt: Earl knytter datoen til off-air-spolen og skiller 1420 AM fra 1420.405 MHz.
- Avbrudd: Lukket panel avbryter Earl uten konklusjon; han gjentar ikke etter 04:00.
- Alternativ: Spilleren kommer etter 04:00: scenen vises ikke og BY02_LATE_COPY overtar.
- Stemmecues: 2, innspilt 0
- Modeller: Eksisterende 1420 AM-stasjon og lobbydør; Earl som figur i lobbyen
- Lyd: Romkanal for Earl; lobbyens summer og bærebølge
- Dokumenter: ingen
- Tester: SC08-T01, SC08-T03

**`BY02_REEL_IDENTIFY`**: Spilleren undersøker spoleeske, off-air-logg og initialer i lobbyen.

- Normalt: Earl forklarer velgerens to stillinger.
- Avbrudd: Avbrudd før undersøkelsen er ferdig gir ingen identitet.
- Alternativ: Ingen: spolen må identifiseres før begge prøvene.
- Stemmecues: 2, innspilt 0
- Modeller: Ny spoleeske, off-air-logg, merket TAPE/LIVE-velger
- Lyd: Romkanal for Earl
- Dokumenter: ingen
- Tester: SC08-T01, SC08-T02

**`BY02_TAPE_CHECK`**: Spilleren setter velgeren på TAPE og starter spolen.

- Normalt: Teksten bekrefter avspillingens kilde uten peiling.
- Avbrudd: Avbrutt avspilling gir ingen tape-prøve; velgerstilling lagres.
- Alternativ: LIVE først, så TAPE, er like gyldig.
- Stemmecues: 0, innspilt 0
- Modeller: Lobbydekk, frakoblet monitorledning
- Lyd: Spolens hiss, hull og klikk fra FS47ReelTape på lobbyhøyttaler; velgerklikk
- Dokumenter: ingen
- Tester: SC08-T02, SC08-T03

**`BY02_LIVE_CHECK`**: Spilleren setter velgeren på LIVE MONITOR.

- Normalt: Teksten holder kildene adskilt.
- Avbrudd: Bytte tilbake til TAPE før minste lyttetid gir ingen live-prøve.
- Alternativ: Rekkefølgen er fri.
- Stemmecues: 0, innspilt 0
- Modeller: Lobbymonitor, frakoblet dekk
- Lyd: Nattens 1420 AM-monitor på lobbyhøyttaler
- Dokumenter: ingen
- Tester: SC08-T02, SC08-T03

**`BY02_SOURCE_COMPARE`**: Begge prøvene er lagret og spilleren har stilt nytt spørsmål og spilt spolen igjen.

- Normalt: Kildesjekken gis som dokument.
- Avbrudd: Dokumentet gis atomisk; lasting gjentar ikke utdeling.
- Alternativ: Sen kopi via BY02_LATE_COPY gir parallell vei til H05.
- Stemmecues: 0, innspilt 0
- Modeller: Som over
- Lyd: Ingen ny
- Dokumenter: HR_EARL_SOURCE_CHECK
- Tester: SC08-T02, SC08-T04

**`BY02_LATE_COPY`**: Etter 04:00 åpner spilleren utleveringskonvolutten i lobbyen.

- Normalt: Kopien kontrolleres og spilles isolert; nytt spørsmål gir uendret fragment.
- Avbrudd: Avbrutt avspilling bevarer metadata, men gir ikke resultat.
- Alternativ: Original før 04:00 er den andre gyldige veien.
- Stemmecues: 0, innspilt 0
- Modeller: Ny datert konvolutt, kildekopi, off-air-loggkopi, frakoblingsark
- Lyd: Kopiavspilling på lobbydekk
- Dokumenter: HR_EARL_SOURCE_COPY
- Tester: SC08-T05, SC08-T09

**`BY02_INCOMPLETE`**: Spilleren avspiller uten identifisert kilde eller med begge kilder koblet.

- Normalt: Teksten viser neste konkrete kontroll.
- Avbrudd: Vises igjen etter Continue til gyldig prøve finnes.
- Alternativ: Ingen: dette er selve det uavklarte utfallet.
- Stemmecues: 0, innspilt 0
- Modeller: Som over
- Lyd: Ingen feil-lyd
- Dokumenter: ingen
- Tester: SC08-T09

**`ARRAY_BRIEF`**: Spilleren har åpnet arrayservicearket i kontoret.

- Normalt: Teksten forklarer A, B og REF.
- Avbrudd: Ingen effekt ved avbrudd; arket beholdes i notatboken.
- Alternativ: Ingen: arket er eneste inngang.
- Stemmecues: 0, innspilt 0
- Modeller: Eksisterende office:arrayServiceSheet og feltavlene
- Lyd: Ingen
- Dokumenter: EV_DALE_ARRAY_SERVICE
- Tester: SC08-T06

**`ARRAY_COMMAND_CHECK`**: Spilleren gjør baseline på operasjonsterminalen.

- Normalt: Teksten bekrefter nærgruppe og kommandoregister.
- Avbrudd: Panel lukket før registrering gir ingen baseline.
- Alternativ: Baseline kan gjøres før eller etter at lykten settes ut, men før A/B.
- Stemmecues: 0, innspilt 0
- Modeller: Eksisterende operasjonsterminal
- Lyd: Konsollpip på verdenskanal
- Dokumenter: ingen
- Tester: SC08-T06, SC08-T07

**`ARRAY_FIXED_REFERENCE`**: Spilleren setter feltlykten på REF / R1.

- Normalt: Teksten minner om å flytte seg selv, ikke lykten.
- Avbrudd: Å flytte lykten etterpå fjerner flagget.
- Alternativ: Ingen.
- Stemmecues: 0, innspilt 0
- Modeller: Ny feltlykt, eksisterende REF / R1-tavle
- Lyd: Lyktklikk
- Dokumenter: ingen
- Tester: SC08-T06, SC08-T07

**`ARRAY_POINT_A`**: Spilleren står på A / ROW 1 WEST etter baseline og fast referanse.

- Normalt: Nærgruppen vender seg mot A og teksten ber spilleren registrere det.
- Avbrudd: Pause fryser antennene; ingen oppspart bevegelse.
- Alternativ: Første eller andre ståsted.
- Stemmecues: 0, innspilt 0
- Modeller: Eksisterende antenner og A-tavle
- Lyd: Servomotorer og bremser fra antennene (posisjonert verdenslyd)
- Dokumenter: ingen
- Tester: SC08-T07

**`ARRAY_POINT_B`**: Spilleren står på B / ROW 1 EAST.

- Normalt: Nærgruppen vender seg mot B.
- Avbrudd: Som A.
- Alternativ: Første eller andre ståsted.
- Stemmecues: 0, innspilt 0
- Modeller: Eksisterende antenner og B-tavle
- Lyd: Servomotorer og bremser
- Dokumenter: ingen
- Tester: SC08-T07

**`ARRAY_RETURN`**: Spilleren går tilbake til første ståsted.

- Normalt: Antennene vender tilbake mot første ståsted.
- Avbrudd: Avbrudd før retur gir ingen retur.
- Alternativ: A→B→A eller B→A→B.
- Stemmecues: 0, innspilt 0
- Modeller: Som over
- Lyd: Servomotorer
- Dokumenter: ingen
- Tester: SC08-T07, SC08-T08

**`ARRAY_RESULT`**: Spilleren sammenligner geometri og kommandohistorikk etter retur.

- Normalt: Resultatnotatet gis.
- Avbrudd: Dokumentet gis atomisk.
- Alternativ: Ingen: krever full prøve.
- Stemmecues: 0, innspilt 0
- Modeller: Serviceark, konsoll
- Lyd: Ingen
- Dokumenter: HR_ARRAY_CONTROL_NOTE
- Tester: SC08-T07, SC08-T08

**`ARRAY_RETREAT`**: Spilleren bryter kontakt med den søkende figuren i passasjen.

- Normalt: Teksten bekrefter retrett og bevart sammenligning.
- Avbrudd: Pause gir ingen tilnærming; fangst går til ARRAY_RETRY.
- Alternativ: Lang vei bak opake flater eller kort kryssing etter lest avsøkning.
- Stemmecues: 0, innspilt 0
- Modeller: Ny søkende figur, opake fundament- og skapflater
- Lyd: Fotsteg og søkelyd fra figuren (posisjonert)
- Dokumenter: ingen
- Tester: SC08-T08, SC08-T10

**`ARRAY_EMPTY_HOLD`**: Spilleren ser feltet fra dekning etter retrett.

- Normalt: Antennene holder på tomt sted og går til 026.
- Avbrudd: Ser spilleren bort, utsettes scenen til faktisk observasjon.
- Alternativ: Ingen.
- Stemmecues: 0, innspilt 0
- Modeller: Eksisterende antenner
- Lyd: Bremser og stillhet
- Dokumenter: ingen
- Tester: SC08-T08, SC08-T10

**`ARRAY_RETRY`**: Figuren fanger spilleren.

- Normalt: Spillet går tilbake til dekket punkt med bevart arbeid.
- Avbrudd: Lagring under fangst gjenoppretter sjekkpunktet.
- Alternativ: Ingen.
- Stemmecues: 0, innspilt 0
- Modeller: Ingen nye
- Lyd: Ingen feil-lyd
- Dokumenter: ingen
- Tester: SC08-T10

**`ARRAY_INCOMPLETE`**: Spilleren forlater feltet med ufullstendig prøve.

- Normalt: Teksten viser hva som mangler.
- Avbrudd: Vises igjen etter Continue til prøven er fullført.
- Alternativ: Ingen.
- Stemmecues: 0, innspilt 0
- Modeller: Ingen nye
- Lyd: Ingen feil-lyd
- Dokumenter: ingen
- Tester: SC08-T09

### Testsett

| Test | Type | Dekker | Prøve |
|---|---|---|---|
| SC08-T01 | api | normal | Datert spor, lobby før 04:00 og spoleundersøkelse gir dated_lead, earl_present og reel_identity. FAIL hvis gammel dialogtema 'reel' setter noen av dem. |
| SC08-T02 | input | normal, alternative | Velg TAPE og LIVE i begge rekkefølger, still spørsmål, spill igjen. Forvent begge prøver lagret og BY02_SOURCE_COMPARE. |
| SC08-T03 | input | interrupt | Bytt velger midt i avspilling, lukk panel, forlat lobbyen. Ingen prøve uten fullført avspilling. |
| SC08-T04 | input | continue | Lagre med TAPE-prøve fullført og velger på LIVE; fersk Continue viser prøven bevart og velger LIVE. |
| SC08-T05 | input | alternative | Ankomst etter 04:00: konvolutt og isolert kopi gir BY02_LATE_COPY; Earl sier ingenting. |
| SC08-T06 | api | normal | Servicekort, baseline og lykt gir service_card_acquired, command_checked og reference_fixed. |
| SC08-T07 | input | normal, alternative | A→B→A og B→A→B ved samme klokke gir ulike riktige stillinger og ARRAY_RESULT. FAIL hvis tid alene består. |
| SC08-T08 | input | normal, interrupt | Trusselmøte 3 med begge retrettveier, deretter tomt hold og 026. |
| SC08-T09 | data | negative | Uten identifisert kilde, uten retur eller med flyttet lykt gir ingen positiv scene, bare INCOMPLETE. |
| SC08-T10 | input | continue, interrupt | Fangst i passasjen gir lokal retry med bevart arbeid; Continue fra retry-sjekkpunkt. |
| SC08-T11 | image | image | High og Low av lobbyen, velgeren, A/B/REF-tavlene, antennene og passasjen. |
| SC08-T12 | human | audio | Lytt til Earls fire takes, spolen og antennemotorene. Menneskelig vurdering. |
| SC08-T13 | data | regression | Kjør ArrayBack, DishesFollow, ArchiveDishes, LostNights og ReelTape-prøvene. |

## SC09

STATION 01 er i stor grad ferdig i US47Chapter3 og US47Station. Arbeidet er å koble H05 til SC08, legge inn kort lysavskjæring og ny utskrift fra negativet, og eksponere receiverens avspillingsvindu.

**Kode:**

- Utvid FS47Horror::Context med H05-gren
- Legg FragmentPlaying() i US47Chapter3
- Legg tåkelagt kopi og ny utskrift i US47Chapter3 som i US47Chapter1
- Bygg kort lysavskjæring med opak dekning og samordne US47StationTaken

**Ressurser:**

- Ingen nye modeller utover dekning ved hytta

**Avhenger av:**

- SC08 levert

**Bare på Kubuntu:**

- Pakket bygg, feltreise og bilder

**Risiko og avklaringer:**

- Eksisterende Noras feltringing kommer før fremkalling; kontrakten sier begge kopier før Nora
- US47StationTaken er ikke startet i Unreal i dag

**Stemmer:** H05-scenene er DESCRIPTION og eksisterende receiverfragment; ingen nye cues.

### Verdensflagg

| Flagg | Status | Produsent | Handling i spillet | Tester |
|---|---|---|---|---|
| `ctx_hr_h05_ready` | partial | FS47Horror::Context via FS47Office::DepartureReleased og SC08-resultatene | BY02-original eller verifisert kopi er kontrollert, ARRAY-01-sammenligning og retrett er fullført, og spilleren drar fra kontoret med STATION 01-filen. | SC09-T01, SC09-T07 |
| `ctx_hr_h05_checked` | partial | FS47Horror::Context via US47Chapter3::S (FS47Ch3State) | Ved STATION 01 har spilleren gjort fastmerke A, lokal nulltest, kabelundersøkelse og receiver/E10, tatt FRAME 03 og FRAME 04 og fremkalt dem på SARO før Nora. | SC09-T01, SC09-T02 |
| `ctx_hr_h05_insufficient` | partial | FS47Horror::Context via US47Chapter3 og FS47Substation (T02 uavklart) | Feltkopien er feil eller ubrukelig, eller T02s lampeoppfølging er arkivert som uavklart. | SC09-T03, SC09-T04 |
| `ctx_hr_h05_receiver_playing` | partial | FS47Horror::Context via US47Chapter3::UseReceiver og US47Station | Spilleren slår på receiveren i hytta og det bevarte Tomás-fragmentet spilles faktisk. | SC09-T05, SC09-T02 |

### Scener

**`H05_BRIEF`**: Spilleren ankommer STATION 01 etter SC08.

- Normalt: Startteksten skiller A, C og arraypunktene.
- Avbrudd: Pause, panel eller dialog stanser visningen uten konklusjon; scenen gjenopptas fra lagret pakkestatus etter at verdensflaggene er gjenopprettet. Områdebytte avbryter uten å sette flagg.
- Alternativ: Spilleren kjører hovedvei eller surveyspor.
- Stemmecues: 0, innspilt 0
- Modeller: Eksisterende STATION 01, E09A-transitt
- Lyd: Ingen
- Dokumenter: E09A
- Tester: SC09-T01

**`H05_CHECK`**: Spilleren har P06 til P09 og begge feltkopier fremkalt.

- Normalt: Resultatet peker videre til Nora.
- Avbrudd: Kort lysavskjæring under hjemreisen stopper ikke registrert arbeid.
- Alternativ: T02 kan være uavklart uten å sperre.
- Stemmecues: 0, innspilt 0
- Modeller: Eksisterende hytte, kabel, lampe, receiver, ny opak dekning for lysavskjæring
- Lyd: Lysavskjæringens lyd (posisjonert)
- Dokumenter: E10, FRAME 03, FRAME 04, P06, P07, P08, P09
- Tester: SC09-T01, SC09-T02

**`H05_INCOMPLETE`**: Feil eller ødelagt feltkopi, eller T02 uavklart.

- Normalt: Teksten viser at negativet beholdes.
- Avbrudd: Vises igjen etter Continue til ny kopi finnes.
- Alternativ: T02 uavklart gir SARO-kontroll i H07.
- Stemmecues: 0, innspilt 0
- Modeller: Eksisterende vått bord
- Lyd: Ingen feil-lyd
- Dokumenter: T02
- Tester: SC09-T03, SC09-T04

**`H05_FIXED_FRAGMENT`**: Spilleren slår på receiveren i hytta.

- Normalt: Det eksisterende fragmentet spilles med eksisterende timing.
- Avbrudd: Pause holder avspillingen; avbrudd gir ingen ny replikk.
- Alternativ: Ingen: fast avtrykk.
- Stemmecues: 0, innspilt 0
- Modeller: Eksisterende receiver
- Lyd: Eksisterende Tomás-take via PhoneCall på radiokanal; bærebølge
- Dokumenter: ingen
- Tester: SC09-T05

### Testsett

| Test | Type | Dekker | Prøve |
|---|---|---|---|
| SC09-T01 | api | normal | Fullfør SC08 og dra fra kontoret med STATION 01-filen. Gjør fastmerke A, nulltest, kabel, receiver/E10 og fremkall begge kopier. Forvent ready, checked og H05_CHECK. FAIL hvis ARRAY-resultatet erstatter P06. |
| SC09-T02 | input | alternative, interrupt | Kjør hjemreisen fra STATION 01 med kort lysavskjæring: bryt sikt bak hyttas opake sidevegg og fullfør. Arkiver T02 som uavklart. Forvent at registrert kabel og nullarbeid består og at H05 kan fullføres. FAIL hvis gammel US47StationTaken overstyrer en vellykket retrett. |
| SC09-T03 | input | negative | Lag feil eller ødelagt feltkopi. Forvent insufficient sann, H05_INCOMPLETE og ingen H05_CHECK før ny kopi fra negativet. FAIL hvis case board eller dialog kan sette checked. |
| SC09-T04 | input | continue | Lagre ved hytta, under hjemkjøring og mellom fremkalling av FRAME 03 og FRAME 04. Fersk Continue i separat prosess bevarer P06 til P09 og kopiene uten ny dokumentutdeling. |
| SC09-T05 | api | audio | Slå på receiveren. receiver_playing skal være sann bare mens det eksisterende fragmentet spiller, og H05_FIXED_FRAGMENT starter én gang. Kontroller radiokanal og bærebølge. FAIL ved ny timing eller ny replikk. |
| SC09-T06 | image | image | High og Low fra spillerhøyde ved fastmerke A, kabelkuttet, hytta i mørke under lysavskjæringen og begge feltkopier på SARO. |
| SC09-T07 | data | regression | Kjør Chapter3, Station og StationTaken SelfTest, T02-prøvene og SC01/SC06-prøvene for delt kamera og mørkerom. |

## SC10

SC10 bygger på eksisterende kapittel fire. FS47Horror får en SC10-gren som avleder h06-flaggene fra Ch3/Ch4-tilstand. Returlinjen og brevet knyttes som faktisk sammenligning.

**Kode:**

- Utvid FS47Horror::Context med SC10-gren for h06-flaggene
- Legg utilstrekkelig-felt i US47Chapter4 (Wrong10, LeftRoom)
- Utvid Chapter4 Ending-replikk slik at den ikke inviterer til eksperiment på linjen

**Ressurser:**

- Ingen nye modeller; eksisterende Rom 6 brukes

**Avhenger av:**

- SC09 levert med FRAME03/04
- Lagringsfeltet horror fra SC01

**Bare på Kubuntu:**

- Bygg og pakket test
- High/Low-bilder
- Opptak av Nora-cues

**Risiko og avklaringer:**

- Chapter4 Ending sier 'drive the road' mot H06_CHECK 'Do not finish his observation'
- Hoppede saker (JumpChapter4) må ikke gi ready

**Stemmer:** Alle seks NORA-cues i H06 mangler opptak (pending i recording_manifest.json, ikke i voice/takes.json). Samme stemme som eksisterende Nora. Nye stemmer for Ward, Ortiz, Earl og Nora produseres til slutt, etter øvrig innhold.

### Verdensflagg

| Flagg | Status | Produsent | Handling i spillet | Tester |
|---|---|---|---|---|
| `ctx_hr_h06_ready` | partial | FS47Horror::Context via US47Chapter3 (feltrull hentet) og US47Chapter4 (Rom 6 nådd) | FRAME03/04 er fremkalt, fiksert og faktisk hentet på våtbenken (Dev 3 med F3 og F4), og spilleren går inn gjennom døren til Rom 6 med PHOTO01/02 og E07 tilgjengelig. | SC10-T01, SC10-T02, SC10-T04 |
| `ctx_hr_h06_checked` | partial | FS47Horror::Context via US47Chapter4 (P10 kontroll, returlinje, P12 og brevregelen) | Spilleren legger begge B-12-kopiene og E07 på Noras bord, kontrollsvaret må stemme med faktisk brukt metode (passiv eller aktiv), returlinjen får nytt spørsmål og gir samme 1947-fragment, og linjen sammenlignes med brevet (nøkkelen i kruset) på saksbrettet. | SC10-T02, SC10-T03, SC10-T05 |
| `ctx_hr_h06_insufficient` | partial | FS47Horror::Context via US47Chapter4 (feil plassering, feil kontrollsvar eller forlatt rom uten funn) | Spilleren legger feil kopier på bordet, svarer feil kildefjerning for kontrollen, eller går ut av Rom 6 før P10, P11 og P12 er ført. | SC10-T03, SC10-T04 |

### Scener

**`H06_BRIEF`**: Spilleren går inn døren til Rom 6 med feltkopiene (EnteredRoom), Nora ved bordet.

- Normalt: Nora forteller kort om Tomás, kuttet og returlinjen. Én gang per inngang.
- Avbrudd: Pause, notatbok eller brev stanser linjene; ingen nye farer i Rom 6 (Night holdes). Ved lagring midt i spilles briefen ikke på nytt etter Continue.
- Alternativ: Ingen alternativ; kommer spilleren uten FRAME03/04 sendes han tilbake til fotolaben via objektivet.
- Stemmecues: 3, innspilt 0
- Modeller: world.room6 (eksisterende); Nora og noraHead (eksisterende)
- Lyd: romtone motel, TV-sus fra apparatet (eksisterende)
- Dokumenter: E11, E12, E13
- Tester: SC10-T02, SC10-T04

**`H06_CHECK`**: Etter korrekt P10-kontroll, returlinjen og P12 i samtalen eller på saksbrettet.

- Normalt: Nora gir C østover fra STATION 01, ber om å bevare originalen og ikke stoppe på linjen.
- Avbrudd: Avbrudd før siste linje gir ingen hr_h06_recorded; Continue gjenopptar fra samme node uten dobbel dokumentutdeling.
- Alternativ: Begge B-12-metoder (passiv: lampen, aktiv: ingen) gir riktig variant av kontrollsvaret.
- Stemmecues: 3, innspilt 0
- Modeller: field card, letter, correction på bordet (eksisterende)
- Lyd: telefonklikk og bærebølge fra nattbordstelefonen, romkanal
- Dokumenter: P10, P11, P12
- Tester: SC10-T02, SC10-T03, SC10-T06

**`H06_INCOMPLETE`**: Feil kopier på bordet, feil kontrollsvar eller ut av rommet uten P10/P12.

- Normalt: Stille tekst viser at brev og feltkort fortsatt ligger; spilleren går tilbake til bordet.
- Avbrudd: Teksten forsvinner ved panel og vises på nytt bare ved nytt utilstrekkelig utfall.
- Alternativ: Ingen; brevet er aldri tidsprøve og nøkkelen i kruset er ingen adgangsport.
- Stemmecues: 0, innspilt 0
- Modeller: ingen nye
- Lyd: ingen nye
- Dokumenter: ingen
- Tester: SC10-T03

### Testsett

| Test | Type | Dekker | Prøve |
|---|---|---|---|
| SC10-T01 | data | negative, regression | Kontraktprøve i FS47HorrorPack: H06_BRIEF/CHECK/INCOMPLETE kan ikke starte når ctx_hr_h06_ready eller checked er usann, og ingen on_complete i H06 setter et ctx_hr-flagg. FAIL hvis Start lykkes uten flagg. |
| SC10-T02 | input | normal | -S47HorrorContract=SC10: fremkall og hent FRAME03/04, kjør til motellet, gå inn i Rom 6, legg PHOTO01/02 og E07, svar kontrollen etter faktisk metode, gjennomgå tre emner, returlinjen, P11 og P12. Krev h06_ready, deretter h06_checked og H06_BRIEF/H06_CHECK i rekkefølge. FAIL ved flagg før handling. |
| SC10-T03 | api | alternative, negative | Kjør med passiv og aktiv B-12-metode: riktig svar (lamp, none) gir P10, feil svar øker Wrong10, gir h06_insufficient og H06_INCOMPLETE, og aldri h06_checked. Forlat rommet uten P12 gir insufficient. FAIL hvis brev/nøkkel åpner noe. |
| SC10-T04 | input | interrupt, continue | Åpne brev, notatbok, saksbrett og pause midt i samtalen; gå ut og inn. Lagre i Rom 6 og start Continue i ny prosess med vanlig lagringsløype. Flagg avledes likt, ingen dokumenter deles ut på nytt, briefen gjentas ikke. |
| SC10-T05 | image | image | High og Low fra spillerhøyde: bordet med kopier og brev, Nora, telefonen, PHOTO03 snudd. Vurder lesbarhet. FAIL ved manglende modeller eller uleselige papirer. |
| SC10-T06 | api | audio | Noras nye cues spilles på physical-kanal fra hennes posisjon med undertekst; returlinjens fragment ruter som linjestemme; ingen Night-hendelser eller telefon over replikkene (Night bHeld i room6). Manglende takes registreres ikke som avspilt. |
| SC10-T07 | api | regression | Kjør US47Chapter4::SelfTest og -S47Ch4Drive=talk/p10/line/end samt TheThird face-down. FAIL ved endret eksisterende kapittelfire-atferd. |

## SC11

Hovedarbeidet er nytt: bunt etter 04:04, ytre avsøkning, møte 4 med to ruter og fluktfaktum. SC01-lyskontrollen og telexrullen gjenbrukes.

**Kode:**

- Ny FS47HorrorSiege (S47HorrorSiege.cpp)
- Gjenbruk FS47WindowLights::SetLevel for ytre bånd
- Gjenbruk SC01 FS47HorrorVisual-lyskontroll
- Utvid AS47Story telex med fjernark etter 04:04

**Ressurser:**

- Ytre søkelysrigg, mottakeroriginal, hendelsesark, telexark og overføringsmappe som egne Blender-kilder eller CC0

**Avhenger av:**

- Inntrengersystem fra SC02/SC04
- SC10 levert

**Bare på Kubuntu:**

- Blender-eksport
- Pakket møteprøve
- High/Low-bilder

**Risiko og avklaringer:**

- Story sier at veilederlinjen er stille i ch4/ch5, mens H07_BRIEF er telefon med Ward
- Ingen inntrenger finnes i koden i dag

**Stemmer:** H07_BRIEF_001 og 002 (WARD) mangler opptak. Nye stemmer for Ward, Ortiz, Earl og Nora produseres til slutt, etter øvrig innhold.

### Verdensflagg

| Flagg | Status | Produsent | Handling i spillet | Tester |
|---|---|---|---|---|
| `ctx_hr_h07_ready` | partial | FS47Horror::Context via US47Chapter4 (Rom 6 fullført) og fortellerklokken | Rom 6 er avsluttet (Ch4 Stage complete etter at spilleren har gått ut), og spilleren er tilbake på SARO-området. | SC11-T01, SC11-T02 |
| `ctx_hr_h07_bundle_secured` | partial | FS47HorrorSiege (ny) med AS47Story telexrull og FilePaper | Tidligst 04:04 river spilleren av mottakeroriginalen fra racket, tar påbegynt hendelsesark og begge ferdige Hondo- og Site11-ark fra telexen og legger alt i overføringsmappen før avsøkningen. | SC11-T01, SC11-T02, SC11-T03 |
| `ctx_hr_h07_scan_started` | new | FS47HorrorSiege (ny) med FS47WindowLights for ytre lysbånd | Etter sikret bunt starter den eksterne avsøkningen: et hvitt bånd vandrer over vindusflatene med lesbar geometri, ventilasjonen stopper. | SC11-T02, SC11-T05, SC11-T06 |
| `ctx_hr_h07_checked` | partial | FS47HorrorSiege (ny) som gjenbruker SC01-lyskontrollen i FS47HorrorVisual og inntrengeren fra SC02 | Spilleren slår av lokal lampe og ser at båndet fortsatt krysser glasset, ser at opak skjerm kutter det, og lager en ny radiolyd som inntrengeren faktisk sanser og undersøker mens det gamle fragmentet går uendret. | SC11-T02, SC11-T03, SC11-T05 |
| `ctx_hr_h07_insufficient` | new | FS47HorrorSiege (ny) | Avsøkningen er nådd uten sikret original, lyskontrollen er avbrutt, eller spilleren ble fanget før kontroll. | SC11-T03, SC11-T04 |
| `ctx_hr_h07_captured` | new | Inntrengersystem fra SC02 (FS47HorrorEncounter, ny) via FS47HorrorSiege | Inntrengeren når spilleren etter faktisk sikt eller hørt lyd (dokumentert sanseårsak). | SC11-T04 |
| `ctx_hr_h07_escape_complete` | new | FS47HorrorSiege (ny) med inntrengerens kontakttap fra SC02-systemet | Spilleren når det målte sikre servicebilpunktet via en av to fysisk gyldige ruter, inntrengeren har mistet kontakt og avsluttet søk. | SC11-T02, SC11-T03, SC11-T05 |
| `ctx_hr_h07_original_secured` | new | FS47HorrorSiege (ny), overføringsmappe i servicebilen | Måleoriginalen er plukket opp og ligger i overføringsmappen som er lagt i servicebilen. | SC11-T02, SC11-T03 |

### Scener

**`H07_BRIEF`**: Spilleren er tilbake på SARO etter Rom 6 og tar kontrolltelefonen eller går inn i kontrollrommet.

- Normalt: Ward ber om original og fjernark; ventilasjonen stopper og et hvitt bånd går over yttervegg.
- Avbrudd: Avbrudd før siste linje lar briefen stå åpen; ingen avsøkning starter av briefen.
- Alternativ: Før 04:04 gir samtalen klargjøring og kildekontroll uten ferdige fjernark.
- Stemmecues: 2, innspilt 0
- Modeller: kontrollrom SARO (eksisterende); telex (eksisterende)
- Lyd: ventilasjonsstopp, rombus SARO, ytre båndsurr fra vindusretning
- Dokumenter: ingen
- Tester: SC11-T02, SC11-T03

**`H07_CHECK`**: Etter fullført flukt, i ro ved servicebilen.

- Normalt: Stille tekst oppsummerer lampe av, bånd over glasset, opak skjerm, og samordnet søk.
- Avbrudd: Vises ikke under jakt; panel stopper ikke noe aktivt møte fordi scenen kommer etter flukt.
- Alternativ: Uten fullført flukt eller kontroll vises H07_INCOMPLETE i stedet.
- Stemmecues: 0, innspilt 0
- Modeller: ingen nye
- Lyd: ingen nye
- Dokumenter: HR_SARO_ORIGINAL (gis først i H08_COMPARE_RECORDS)
- Tester: SC11-T02, SC11-T05

**`H07_INCOMPLETE`**: Fangst, manglende original ved scan eller avbrutt kontroll.

- Normalt: Peker til dekket sjekkpunkt før inspeksjonen; ingen fotoport.
- Avbrudd: Vises etter gjenopptakelse, ikke midt i jakt.
- Alternativ: Spilleren kan gjenta kontrollen før faren uten å måtte bruke avledning.
- Stemmecues: 0, innspilt 0
- Modeller: ingen nye
- Lyd: ingen nye
- Dokumenter: ingen
- Tester: SC11-T03, SC11-T04

**`H07_FIRST_WARNING`**: Første avsøkning starter etter sikret bunt, før inntrengerkontakt.

- Normalt: Tekst om bånd mot neste vindu og serviceruten bak skjermen; spilleren beholder kontroll.
- Avbrudd: Pause fryser båndet; ingen oppspart tilnærming etter panel.
- Alternativ: Ingen; scenen er én reaksjonsmulighet.
- Stemmecues: 0, innspilt 0
- Modeller: ytre søkelysrigg (ny, egen Blender-kilde); opak skjerm (fra SC01)
- Lyd: ytre bånd, posisjonell lyd utenfor vinduene
- Dokumenter: ingen
- Tester: SC11-T02, SC11-T06

**`H07_CAPTURE`**: Inntrengeren når spilleren med logget sanseårsak.

- Normalt: Kort overgang og gjenopptakelse fra sjekkpunkt før inspeksjonen med bunten bevart.
- Avbrudd: Lagring under jakt er sperret; Continue går til sjekkpunktet.
- Alternativ: Ingen tapt tid eller ny Abduction-gren.
- Stemmecues: 0, innspilt 0
- Modeller: inntrenger (fra SC02, egen Blender-kilde)
- Lyd: fangststikk, kort, ikke blendende
- Dokumenter: ingen
- Tester: SC11-T04

**`H07_ESCAPE_COMPLETE`**: Spilleren når det sikre servicebilpunktet med original i mappen og inntrengeren har mistet kontakt.

- Normalt: Rolig tekst: original sikret, kontakt tapt, søket avsluttet.
- Avbrudd: Lagres én gang med fluktresultatet; Continue viser den ikke igjen.
- Alternativ: Nås via begge ruter (lang skjermet og kort eksponert).
- Stemmecues: 0, innspilt 0
- Modeller: servicebil (eksisterende); overføringsmappe (ny)
- Lyd: motor tomgang, natt utendørs
- Dokumenter: ingen
- Tester: SC11-T02, SC11-T03

### Testsett

| Test | Type | Dekker | Prøve |
|---|---|---|---|
| SC11-T01 | data | negative, regression | Registry: H07_CHECK krever checked, escape_complete og bundle_secured; H07_FIRST_WARNING krever bundle_secured. Bunt kan ikke bli sikret før 04:04. FAIL hvis noen scene starter uten flagg. |
| SC11-T02 | input | normal | -S47HorrorContract=SC11: etter Rom 6 og 04:04 sikre fire bunt-elementer, avsøkning starter, slå av lokal lampe, se bånd og skjerm, lag radiolyd som inntrengeren sanser, ta lang skjermet rute til servicebilpunktet. Krev escape_complete før H07_CHECK, og HR_SARO_ORIGINAL først etter flukt. |
| SC11-T03 | input | alternative, negative | Ta kort eksponert rute; ankom før 04:04 og kontroller at ingen ferdige fjernark finnes; prøv scan uten original. Krev insufficient og H07_INCOMPLETE i negative tilfeller. |
| SC11-T04 | input | alternative, continue | Bli sett med vilje: captured med logget sanseårsak, H07_CAPTURE, gjenopptakelse fra sjekkpunkt før inspeksjonen med bunten bevart, ingen tapt tid eller bortføring. |
| SC11-T05 | input | interrupt, continue | Pause, panel og områdebytte under avsøkning gir ingen oppspart tilnærming. Lagre etter flukt og Continue i ny prosess: escape_complete og original_secured består, ingen ny jakt. |
| SC11-T06 | image | image | High/Low fra stående og huket øyehøyde: bånd over vinduer, lokal lampe av, opak skjerm, begge ruter, servicebilpunkt. |
| SC11-T07 | human | audio, normal | Menneskelig lytting og gjennomspilling: ventilasjonsstopp, bånd, radiolyd, uendret fragment og Ward på telefonkanal er forståelige; to ruter oppleves lesbare. |
| SC11-T08 | api | regression | Kjør SC01-kontrakten, SaroDark 02:29, Glimpses og Abduction selvtester; bekreft at tilfeldige hendelser holdes borte under møtet og etter flukt. |

## SC12

Dineren og P13 finnes. Ny verifisering av kildeark, kort kilde-ID/readback hos Ward, betjentens daterte patruljeark og bekreftet budavtale skal bygges. H09-kartarbeidet inngår i H08.

**Kode:**

- Ny FS47HorrorSources med faktisk kilde-/provenienssammenligning.
- Utvid eksisterende Ward-telefon med tre kilde-ID-er og første grensetid; tabeller blir i dokumentene.
- Legg betjentens daterte patruljeark fysisk på dinerdisken ved ankomst 04:30, med egen leseinteraksjon.
- Bevar P13 og H09-scene-IDer som del av H08; bud 10.4 og C 8.15 skilles i kart/readback.

**Ressurser:**

- Tre kildeark og betjentens daterte patruljeark som egne lesbare props.

**Avhenger av:**

- SC11 levert med bunt og flukt

**Bare på Kubuntu:**

- Pakket test
- High/Low-bilder
- Opptak

**Risiko og avklaringer:**

- Gamle Ward-replikker om normal 06:00-serie må ikke overstyre registrert lokal ordrestans.
- Patruljearket viser én tur 03:30, ikke en garanti for trygg vei.
- E15 og P13 skal kunne brukes ved sen ankomst, uten ny sjåfør- eller 04:45-port.

**Stemmer:** Ward-opptak tas til slutt. Betjenten beholder eksisterende tekstpresentasjon; ingen ny sjåførstemme eller figur.

### Verdensflagg

| Flagg | Status | Produsent | Handling i spillet | Tester |
|---|---|---|---|---|
| `ctx_hr_h08_ready` | partial | FS47Horror::Context via US47Chapter5 (ankomst Mesa Diner) og h07_escape_complete | Spilleren har kjørt fra SARO etter fullført flukt og stiger ut på dinerens parkering. | SC12-T01, SC12-T02 |
| `ctx_hr_h08_checked` | new | FS47HorrorSources (ny) på bordet i båsen, bygget på FS47RuleBook | Spilleren legger SARO-original, Hondo- og Site11-ark ved siden av hverandre, sammenligner opphav, klokker, faseorden (03:56:55 mot 03:56:56 bevart), råverdier og sendeplan, og fører en avgrenset konklusjon. | SC12-T02, SC12-T03 |
| `ctx_hr_h08_insufficient` | new | FS47HorrorSources (ny) | En fjernkilde mangler eller er uverifisert når spilleren prøver å føre nettverkskonklusjon. | SC12-T03 |
| `ctx_hr_remote_sources_verified` | new | FS47HorrorSources (ny), med kildeark fra SC11-bunten | Spilleren kontrollerer på hvert fjernark lokal logger, eget klokkegrunnlag, råverdier og at Site11-mottakeren var frakoblet lydmatingen. | SC12-T02, SC12-T03 |
| `ctx_hr_warning_acknowledged` | partial | US47Chapter5 betalingstelefon med ny kilde-ID/readback-fase i FS47HorrorSources | Spilleren ringer Ward fra betalingstelefonen og leser tre kilde-ID-er og første grensetid. Ward leser dem tilbake, fører dem i datert logg og bekrefter at 06:00-serien fortsatt er stanset. | SC12-T02, SC12-T05 |
| `ctx_hr_h09_ready` | partial | FS47Horror::Context via US47Chapter5 (i dineren) og warning_acknowledged | Spilleren er inne i Mesa Diner etter kvittert varsel. | SC12-T01, SC12-T04 |
| `ctx_hr_h09_checked` | existing | US47Chapter5::Answer (P13 på kartet) | Spilleren legger E09A-tracingen med transit på STATION 01, nord mot nord, og velger bolten ved 8.15. | SC12-T04 |
| `ctx_hr_h09_insufficient` | partial | US47Chapter5::Answer (feil bolt eller ikke justert) | Spilleren prøver å svare uten justert tracing eller velger feil bolt. | SC12-T04 |
| `ctx_hr_sc12_courier_104_confirmed` | new | Ward-samtale med kartvalg (FS47HorrorSources, ny) etter P13 | Etter kartlagt C ved 8.15 markerer spilleren 10.4 på kartet og avtaler bud og retning med Ward på betalingstelefonen; Ward bekrefter i datert logg. | SC12-T04, SC12-T05 |
| `ctx_hr_sc12_patrol_sheet_read` | new | Sheriffbetjentens daterte patruljeark ved dinerdisken, eid av ny FS47HorrorSources | Etter betjentens ankomst 04:30 leser spilleren hans daterte patruljeark fra 03:30: nordveien og gamle Roswell Road uten hindring under den ene turen. | SC12-T04, SC12-T05, SC12-T08 |

### Scener

**`H08_BRIEF`**: Spilleren tar betalingstelefonen i dineren etter ankomst fra SARO.

- Normalt: Ward skiller Hondos strøm fra SAROs mørklegging og ber om sammenligning.
- Avbrudd: Lagt på før slutt gir ingen brief-flagg; Continue gjenopptar ved telefonen.
- Alternativ: Sen ankomst: Ward peker på bevart telexkopi.
- Stemmecues: 3, innspilt 0
- Modeller: betalingstelefon (eksisterende)
- Lyd: myntinnkast og klikk, telefonkanal
- Dokumenter: ingen
- Tester: SC12-T02, SC12-T05

**`H08_CHECK`**: Kildearkene er sammenlignet og verifisert ved båsbordet.

- Normalt: Tekst om felles faseorden og fortsatt tilnærming; ring Ward med begrensningene.
- Avbrudd: Panelavbrudd lar sammenligningen stå; ingen konklusjon uten ført kobling.
- Alternativ: Én kilde mangler gir H08_INCOMPLETE.
- Stemmecues: 0, innspilt 0
- Modeller: tre kildeark på bordet (nye props)
- Lyd: papir, dinerens kjøler dempet
- Dokumenter: HR_HONDO_LOCAL, HR_SITE11_LOCAL
- Tester: SC12-T02, SC12-T03

**`H08_INCOMPLETE`**: Forsøk på nettverkskonklusjon med uverifisert kilde.

- Normalt: Send lokal varsling straks og be Ward om originalark.
- Avbrudd: Vises på nytt bare ved nytt forsøk.
- Alternativ: Sen ankomst bruker bevart lokalt stempel på telex.
- Stemmecues: 0, innspilt 0
- Modeller: ingen nye
- Lyd: ingen nye
- Dokumenter: ingen
- Tester: SC12-T03

**`H08_COMPARE_RECORDS`**: Etter remote_sources_verified og fullført SC11.

- Normalt: Tekst om ulike lokale målinger og samme rekkefølge; dokumentene utdeles.
- Avbrudd: Utdeling skjer én gang; lasting gjentar ikke dokumenter.
- Alternativ: Ingen; uten flukt finnes ingen komplett SARO-kilde.
- Stemmecues: 0, innspilt 0
- Modeller: ingen nye
- Lyd: ingen nye
- Dokumenter: HR_SARO_ORIGINAL, HR_HONDO_LOCAL, HR_SITE11_LOCAL, HR_NETWORK_CHECK
- Tester: SC12-T02, SC12-T05

**`H08_WARNING_ACK`**: Etter opplesning av tre kilde-ID-er og første grensetid, med fullført readback på betalingstelefonen.

- Normalt: Ward bekrefter varsel, suspendert 06:00-serie, loggførte kilde-ID-er og budet øst for referansen.
- Avbrudd: Avbrutt samtale før readback gir ingen kvittering.
- Alternativ: Ingen tidsutløp kan erstatte kvitteringen.
- Stemmecues: 3, innspilt 0
- Modeller: betalingstelefon (eksisterende)
- Lyd: telefonkanal, klikk ved avslutning
- Dokumenter: HR_WARNING_RECEIPT
- Tester: SC12-T02, SC12-T05, SC12-T07

**`H09_BRIEF`**: Spilleren undersøker E15 ved dinerdøren og den løse tracingen etter kvittert varsel.

- Normalt: E15 fastslår fotografiets historiske sted, mens P13-kartet må justeres fysisk. H09 er del av H08 og ingen egen leveranse.
- Avbrudd: Pause og panel stanser linjene; ingen ny fare i dineren (Night holdt i ch5-diner).
- Alternativ: E15 er tilgjengelig uten 04:45-port. Et ujustert kart får eksisterende tips.
- Stemmecues: 0, innspilt 0
- Modeller: E15 ved dinerdøren (eksisterende); P13-kart med E09A-tracing (eksisterende)
- Lyd: dinerrom, kaffe, kjøler
- Dokumenter: E15
- Tester: SC12-T04, SC12-T07

**`H09_CHECK`**: Etter P13 og avtalt 10.4 på betalingstelefonen.

- Normalt: Ward bekrefter C 8.15 og overlevering 10.4, og gir kjøreinstruks uten avlesning.
- Avbrudd: Avbrudd lar avtalen stå om den er loggført; ellers ny samtale.
- Alternativ: Ingen alternativ overleveringsrute.
- Stemmecues: 3, innspilt 0
- Modeller: kartoverlegg (eksisterende)
- Lyd: telefonkanal
- Dokumenter: P13
- Tester: SC12-T04, SC12-T05

**`H09_INCOMPLETE`**: Svar uten justert tracing eller feil bolt.

- Normalt: Tekst om å legge transit på STATION 01 først.
- Avbrudd: Kartposisjon er lagret (S.Tracing) og bevares.
- Alternativ: E15 har ingen 04:45-port.
- Stemmecues: 0, innspilt 0
- Modeller: ingen nye
- Lyd: ingen nye
- Dokumenter: ingen
- Tester: SC12-T04

**`SC12_PATROL_SHEET`**: Betjenten kommer til dineren 04:30 og legger sitt daterte ark på disken; spilleren leser det.

- Normalt: 03:30: én tur over nordveien og gamle Roswell Road uten hindring da. Ingen påstand om dagens sikkerhet.
- Avbrudd: Lesing og pause flytter ikke tidsstempelet eller gir ufortjent kvittering.
- Alternativ: Arket blir liggende tilgjengelig ved sen ankomst og er ikke avhengig av sjåførdialog.
- Stemmecues: 0, innspilt 0
- Modeller: Datert patruljeark på dinerdisken; Eksisterende sheriffbetjent
- Lyd: Fysisk betjentreplikk som tekst, dinerrom
- Dokumenter: ingen
- Tester: SC12-T04, SC12-T05, SC12-T08

**`SC12_ROUTE_READBACK`**: Ny samtale med Ward etter P13-kontroll, lest patruljeark og bekreftet budavtale.

- Normalt: Ward leser tilbake betjentens ene daterte tur og budet ved 10.4; dineren er fortsatt tilflukt og ingen rute er lovet trygg.
- Avbrudd: Lagt på før slutt: ingen dokument; Continue tilbyr samtalen igjen.
- Alternativ: Spilleren kan bli i dineren; reisen er et valg.
- Stemmecues: 3, innspilt 0
- Modeller: betalingstelefon (eksisterende)
- Lyd: telefonkanal
- Dokumenter: HR_ROUTE_READBACK
- Tester: SC12-T04, SC12-T09

### Testsett

| Test | Type | Dekker | Prøve |
|---|---|---|---|
| SC12-T01 | data | negative, regression | Register: H08_CHECK krever remote_sources_verified, rute/readback krever lest datert patruljeark og faktisk budavtale. H09 er del av H08, uten sjåførfigur. E15 har ingen tidsport. |
| SC12-T02 | input | normal | Pakket SC12-reise: ankom diner, sammenlign tre kilder, verifiser fjernark og ring Ward. Les tre kilde-ID-er og første grensetid og fullfør readback. checked, verified og warning_acknowledged kommer i rekkefølge og HR_WARNING_RECEIPT bare én gang. |
| SC12-T03 | input | alternative, negative | Bare ett fjernark eller sen ankomst: verified forblir usann, H08_INCOMPLETE vises, lokal varsling kan sendes, ingen nettverkskonklusjon. |
| SC12-T04 | input | normal, alternative, negative | Les betjentens daterte patruljeark etter 04:30, løs P13 etter et ujustert forsøk, skill C 8.15 fra bud 10.4, og fullfør SC12_ROUTE_READBACK. Replikk alene gir ikke lest ark, og gammel sjåførhistorie er ingen port. |
| SC12-T05 | input | interrupt, continue | Legg på midt i kilde-ID/readback-samtalen: ingen kvittering. Pause og kartpanel under arbeid. Lagre ved dineren og Continue i ny prosess: faktiske flagg og dokumenter består uten duplisering. |
| SC12-T06 | image | image | High/Low: båsbord med tre ark, P13-kart med 8.15 og 10.4, betalingstelefon og betjentens patruljeark på disken. |
| SC12-T07 | api | audio | Ward på telefonkanal; eksisterende betjent som tekst ved disken. Dinerambience overstyrer ikke dialog. Ward/Ortiz/Earl/Nora-opptak produseres til slutt; manglende take regnes aldri som avspilt. |
| SC12-T08 | api | regression | US47Chapter5::SelfTest, -S47Ch5Drive=map/p13/truck, LostNights (1985-kvittering forblir historisk), Town-timer (deputy 04:30). |
| SC12-T09 | human | normal | Menneskelig vurdering: spilleren forstår hvorfor reisen velges fremfor å bli, og at ruteopplysningene ikke er trygghetsgaranti. |

## SC13

Finalen finnes og skal ikke endres. Det nye er avreisekontroll, oppdatert turvalg og kobling av h10_stalled.

**Kode:**

- Avreisekontroll og sikkert lagringspunkt i Caliche med fullført varsel, kart og originaler.
- Koble faktisk US47Chapter6::StartEvent til h10_stalled uten å endre finalen.
- Formålet med turen er budet ved 10.4, mens C ved 8.15 er en passering uten stopp; 10.4 er bare kart/skiltmål og nås ikke.

**Ressurser:**

- Mappe i førerhus som prop

**Avhenger av:**

- SC12-kilde og målrettet prøve i akt 4

**Bare på Kubuntu:**

- Pakket test av hele finalen på High og Low

**Risiko og avklaringer:**

- Sikker disk-Continue må bygges ved avreise; kapittel 6 har ellers ingen ny lagring.
- Ingen ekstra kjørbar strekning eller overleveringsscene ved 10.4 skal bygges.

**Stemmer:** Ingen stemmecues i H10.

### Verdensflagg

| Flagg | Status | Produsent | Handling i spillet | Tester |
|---|---|---|---|---|
| `ctx_hr_h10_ready` | partial | FS47Horror::Context via US47Chapter5 (Stage road) og bilen på dinerens plass | Spilleren går ut til servicebilen på dinerens parkering etter løst P13. | SC13-T01, SC13-T02 |
| `ctx_hr_h10_checked` | new | Avreisekontroll i servicebilen (FS47Horror SC13-gren, ny) | Ved bilen kontrollerer spilleren at overføringsmappen med originalene ligger i førerhuset, at Ward-kvitteringen finnes, og at kartet viser C 8.15 og overlevering 10.4 som adskilte punkter. | SC13-T02, SC13-T03 |
| `ctx_hr_h10_insufficient` | partial | US47Chapter5::TruckTrip og ny avreisekontroll | Spilleren prøver å kjøre før kart, kvittering eller originaler er i orden. | SC13-T03 |
| `ctx_hr_h10_stalled` | existing | US47Chapter6::StartEvent og Event (motorstopp ved C) | Spilleren kjører gamle Roswell Road og når C ved 8.15; motoren stopper ufrivillig og THE EVENT starter. | SC13-T02, SC13-T06 |

### Scener

**`H10_BRIEF`**: Spilleren åpner servicebilen på dinerens plass etter readback.

- Normalt: Tekst: originalene i førerhuset, overlevering 10.4, C 8.15, passér uten stopp.
- Avbrudd: Panel lukkes uten å starte kjøring; turvalget står.
- Alternativ: Ingen.
- Stemmecues: 0, innspilt 0
- Modeller: servicebil og mappe i førerhus (ny prop)
- Lyd: bildør, motor start
- Dokumenter: HR_WARNING_RECEIPT, HR_ROUTE_READBACK
- Tester: SC13-T02

**`H10_CHECK`**: THE EVENT har startet ved C etter fullført avreisekontroll.

- Normalt: Tekst om ufrivillig motorstopp og at varslet allerede er sendt, uten å bryte den faste finalen.
- Avbrudd: Kapittel seks lagrer ingenting; Continue går til dineren med avreisekontroll bevart.
- Alternativ: Ingen alternativ slutt.
- Stemmecues: 0, innspilt 0
- Modeller: eksisterende old road, truck, Ending
- Lyd: motorstopp, solenoid, pust (eksisterende)
- Dokumenter: ingen
- Tester: SC13-T02, SC13-T04, SC13-T06

**`H10_INCOMPLETE`**: Kjøreforsøk uten kvittering, original eller ferdig kart.

- Normalt: Tekst om konkret handling før avreise (betalingstelefon).
- Avbrudd: Ingen tekst under finalen.
- Alternativ: Ingen falsk redningsgren.
- Stemmecues: 0, innspilt 0
- Modeller: ingen nye
- Lyd: ingen nye
- Dokumenter: ingen
- Tester: SC13-T03

### Testsett

| Test | Type | Dekker | Prøve |
|---|---|---|---|
| SC13-T01 | data | negative, regression | Registry: H10_BRIEF krever readback-flaggene, H10_CHECK krever stalled; ingen H10-scene setter ctx_hr-flagg. |
| SC13-T02 | input | normal | -S47HorrorContract=SC13 på High og Low: avreisekontroll ved bilen, kjør gamle vei, CB forbi sju mil, ufrivillig stopp ved C 8.15, THE EVENT, uttrekk, 1947-bildet og sluttmeny. Klokken går ikke bakover. |
| SC13-T03 | input | alternative, negative | Kjøreforsøk uten kvittering eller original gir H10_INCOMPLETE; stopp og rygg før C; ingen alternativ slutt eller redningsgren. |
| SC13-T04 | input | interrupt, continue | Lagre sikkert ved avreise fra Caliche. Pause under kjøring og THE EVENT følger hendelsesklokken; lagring på selve finalestrekningen er sperret. Continue gjenopptar avreisen, uten å fullføre 47-sekundershendelsen i menyen. |
| SC13-T05 | image | image | High/Low bilder av førerhus med mappe, kart med adskilte punkter og finalens nøkkelbilder. |
| SC13-T06 | human | audio | Lytt: motorstopp, solenoid, pust og CB er uendret og hørbare; ingen ny stemme over finalen. |
| SC13-T07 | api | regression | US47Chapter6::SelfTest, SS47Ending og Flow Finale kjøres uendret. |
