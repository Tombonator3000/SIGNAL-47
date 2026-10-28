# Overtakelse: SC02 og alienens navigasjon

28. oktober 2026. Tom ba Codex hjelpe der Claude stoppet på bruksgrensen, særlig med alien-AI som skal gå rundt hindringer. Claude var fortsatt i SC02; SC03-runtime var ikke påbegynt. SC02 har nå pakket teknisk PASS. Dette er et arbeidscheckpoint for videre akt 1, ikke en ferdig aktleveranse.

## Bevarte kilder og eierskap

Codex fortsetter på `codex/act1-alien-navigation` i `~/scratch-bm/wt/act1-alien`, laget fra oppdatert main `8affc549`. Claudes rene SC02-head `911ce258` er integrert uten å erstatte nyere launcher-/Niagara-/historiedokumenter. Bare de to dokumentkonfliktene er sammenføyd med begge sider bevart.

SC02-kroppen, Blender-kilder, native import og hittil laget historie beholdes. Found footage og Niagara-gnister er fortsatt uferdige i egne Claude-arbeidskopier og er ikke tatt inn i kandidaten. De sju stoppede scenetekstutkastene er kopiert fra `historie-scenes`; originalarbeidskopien er urørt. Eierskap og råbevis ligger i `~/scratch-bm/Signal47-sc02-20261028/codex-takeover`.

Claudes opprinnelige `~/scratch-bm/wt/sc02` var fortsatt ren og er fast-forwardet til de samme rettelsene, uten reset. Den gamle headen er bevart på `claude/sc02-before-codex-20261028`. Fortsett i én av de to SC02-arbeidskopiene om gangen; ikke start en ny kopi fra den gamle headen eller kopier de gamle scenetekstutkastene over den samordnede pakken. Bruk aktgrenen som PR-head. Ingen Claude-prosess er startet på nytt, og ingen bruksgrense er nullstilt.

## Hva som stoppet SC02

Claudes siste editorbygg bestod, men editor-sc02-9 gikk i gjentatt fangst/retry til testens 900 sekunders timeout. Flukttesten brukte gange på 1,9 m/s mens alien jager med 2,1 m/s. Eksisterende løping gir 3,04 m/s. Testen fortsatte gamle veipunkter etter ny fangst, slik at en feil ble en lang løkke.

Navigasjonen hadde også reelle begrensninger: seksten faste punkter, hindringskopi fra møtestart, ingen rute rundt nye props, biler utelatt og avstandsbasert fangst uten ny kontaktkontroll. Disse begrensningene ville også rammet senere møter.

## Endringene

- `S47ThreatNavigation` lager en begrenset visibility graph fra mål, kjente passasjer og hindringshjørner med kroppsklaring. Hver kant prøves. Tom rute er utilgjengelig mål, og brukes aldri som rettlinjet snarvei eller teleport.
- `S47HorrorThreat` oppdaterer områdets aktive kollidere og kjøretøy, validerer hvert faktisk steg og bruker native kapselsweep. Et nytt native hinder gir omplanlegging. ISM/HISM bruker treffinstansens transform og meshbounds, ikke hele instansgruppens bounds. Native ytterbokser er kun hint for nye rutepunkter; faktisk kapselklaring avgjør om kanten kan brukes, slik at en skrå mesh ikke sperrer tom plass inni ytterboksen. Uoppnåelig fotmål gir et kontrollert søk fra tilgjengelig posisjon.
- En bil følges mot en fri posisjon utenfor karosseriet. Spillerens noe mindre kroppsradius gjør ikke en lovlig posisjon ved en vegg immun. Fangst krever faktisk syn og fri hånd-/brystkontaktbane; kontakten prøves igjen før grepet fullføres.
- SC02-flukttesten bruker faktisk løping. Første planlagte fangst beholdes. En ny fangst under forventet flukt gir tydelig FAIL, og løpeinput/veipunkter frigjøres ved pause, restore, etappebytte og slutt.

Dette er et lokalt navigasjonsgrunnlag som kan gjenbrukes. SC02-adapteren er fortsatt målt for trafoområdet. Senere annex-/array-/SARO-møter må koble inn sine egne faktiske passasjer, gulv, dører og sanser; koden gir ikke automatisk ferdig AI i alle områder.

## Scenetekstene videre

Forfatterpakken følger nå den godkjente revisjonen med fire akter: ni hovedkort, tretten scenarioer, 77 scener og 90 eksterne flagg. Den separate svartfiguren, SC04-biljakt/huking og sjåføren er fjernet fra videre plan. Termosen, savnetsakens mottakerlogg og fysisk betjent erstatter de eldre portene. H09-scene-IDer er historisk bevart som kartarbeid under H08, uten eget H09-kort.

SC03s melding avhenger av faktisk SC02-flukt og egen readback, ikke av en valgfri SC01-vindusmelding. Alle H02-/SC02-sceneobjekter og SC01/SC02/H01/H02-flagg er bevart mot `911ce258`. Stemmeopptak, identiteter og proveniens er uendret; Ward, Ortiz, Earl og Nora lages til slutt.

En avgrenset lagringsmigrering godtar de tre pensjonerte framtidssceneflaggene bare som boolsk false, slik gamle ekte SC01-lagringer lagret dem. True, feiltype og andre ukjente felt avvises fortsatt. Nye standardflagg settes til false. Dette er kontrollert mot publisert SC01 og kandidatens SC02-porter.

36 Python-kontraktprøver, HorrorStory `--check` og EveningShift `--check` består. Ti genererte eksporter er konsistente. SC03 og videre er fortsatt `authored_not_integrated`.

## Prøvestatus

Editorbygg og R5-pakking består. Pakket R5 bekrefter **1649 selvtester**, inkludert plannerens omveier/U-form/stengt mål/dynamisk blokk og adapterens simulerte bevegelsessteg, ny hindring, veggkontakt og avbrutt grep. Dette er automatiserte kodeprøver, ikke en menneskelig spillprøve.

Den separate native fysikk-/boundsprøven består **28 kontroller i pakket R5**: querybox, flytting, HISM-treffinstans, individuelle bounds, kollisjon av, lavt trinn i begge retninger, høyere barrierer og opprydding. Bare midlertidige fixtures flyttes; prøven er ikke en hel jakt eller bekreftelse av dynamisk AI-omvei. Den tidligere editorprøven med 22 kontroller er historisk bevis.

Første native editorprøve er bevart som FAIL ved stage 112 etter 387,41 sekunder: kroppen ble stående ved 7885,10 / 522,16 etter native kollisjon, og den planlagte fangsten skjedde ikke. Dette avdekket at en konservativ native ytterboks kunne sperre fri startposisjon. Rettelsen bruker ytterboksen kun som rutehint og prøver faktisk fysikk per kant. Pakket R2 feilet samme planlagte fangst, men loggen identifiserte nå `Prop_substationPad_122`: den omtrent 10 cm høye forkanten på kontrollrampen. En horisontal treffnormal hindret den gamle trinnprøven. Etter blokkert sweep prøves derfor én ny full sweep 30 cm høyere; ingen komponent ignoreres. Native prøver skiller 10 cm trinn fra 40/150 cm barrierer. R4-fokus klarte fangst og flukt, men avdekket at testens parkventing måtte kvittere faktisk dialog før bilen kunne parkere. Begge feil er rettet i R5.

**Full pakket SC02 består 159 kontroller på 483,42 sekunder:** SC01-forløpet, faktisk T01, observert kropp, tilsiktet fangst, lokal retry med T01 bevart, 42,48 m faktisk løping til bilen langs rute A utenfor gjerdet, utkøring, retur og disk-Continue med flukten bevart og kroppen inaktiv. Hele prøven har 219,06 m native gange og 19 disk-/Flow-restores. Tre sluttbilder (kroppen, retry og fluktdialogen) er åpnet og vurdert. Ingen fatal/assert/device-loss eller kontraktfeil ble funnet i de to pakkede sluttloggene.

R5 er bygget fra kodecommit `651dc55897ccc36f96b575c09af26317e086ab9a`. `source-freeze-r5.json` bekrefter 462 uendrede kilde-/konfig-/forfatterfiler før og etter pakking/prøver. `verified-r5.json` har hash for seks payloadfiler; spillbinærens SHA-256 er `3cf5f8bf94a115e4ec2bac9f2586b0d469baa2cd861eb518d6ef56cfb40d4574`.

Lokal bevisrot er `~/scratch-bm/Signal47-sc02-20261028/codex-takeover`:

- `candidate-r5/Linux`: den testede pakken.
- `packed-native-r5/run.log` og `evidence/result.json`: 1649 selvtester og 28 native kontroller.
- `packed-sc02-r5/run.log` og `evidence/`: full SC02, resultat, originale bilder og isolerte diskfixtures.
- `editor-sc02-1`, `packed-sc02-r2`, `editor-resume-r3` og `editor-resume-r4`: bevarte feil og diagnose.

Kjøringene brukte Low, Picture off, 1280×720 og egne `/tmp`-profiler med `-S47HorrorContract=Navigation -S47SelfTest` henholdsvis `-S47HorrorContract=SC02`. Testen bruker faktisk Walker/løpeinput og produksjonens eksisterende autopilot-API for bilturene. Rute B, manuell styring, High-jakt, menneskelig spilling/lytting/frykt og senere områders AI er **UNVERIFIED**. Hele 110-matrisen, main-fletting og installasjon gjenstår for samlet akt 1. Standardspillet og personlige lagringer er uendret.

Kanonisk opprydding er utført etter avsluttede motorjobber: to eldre payloadsett / 14 genererte filer er pensjonert, 8 877 558 180 logiske bytes. R5, feilet R2 for sammenligning og trafostasjon R1 beholdes, samt særskilt beskyttet SC01-R4 og PR174 for andre oppgaver. Kilder, assets, Saved, logger og bilder er bevart. Se `prune-r5-plan.json` og `prune-r5-applied.json`.

## Neste konkrete produksjon: SC03

SC02-forutsetningen nedenfor er nå teknisk oppfylt i R5. Den lokale `sc03-production-checklist.txt` i bevisroten beskriver filansvar, verdenshandlinger, negative observasjonsprøver og savekompatibilitet mer detaljert.

1. Utvid `FS47Horror` og Caliches faktiske `town:payphone` til separat første melding/readback, Ward og savnetsak. Bruk en lagret SC03-fase, og publiser samtalefakta først etter kvittert siste linje.
2. Bygg faktisk Ortiz, hvit servicevogn 23, termose og etterspor, med synlig ankomst, identifikasjon, løft/forsvinning og dekning. Ikke kall den gamle spillerbortføringen som Ortiz-controller. Ingen jakt i SC03.
3. Gi SC03 avgrenset eierskap til betjent/patruljebil så vanlig døgnplan ikke flytter dem midt i sekvensen. Betjenten skal ankomme før loggspørsmålet.
4. Prøv hele SC02 til Caliche-reisen, avbrutt samtale, pause/diskrestore i sekvensene, to gyldige dekninger, faktiske etterspor og bortvendt/okkludert kamera uten falsk observasjon. Deretter SC04-annex og SC05-residual etter revidert regi.

Den avtalte leveransen er **akt 1, SC02 til SC05**. Scenarioer prøves mens de bygges; hele 110-matrisen, fletting og installasjon skjer én gang per akt. Ingen separat fullmatrise eller nye stemmer per scenario er startet her.
