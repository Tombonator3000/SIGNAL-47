# Implementeringsplan: hele historien fra SC01 til SC13

**Revidert 28. oktober 2026:** [HISTORIE_SC02_SC13_REVISJON.md](HISTORIE_SC02_SC13_REVISJON.md) gjelder for SC02 til SC13 der den avviker fra denne planen og fra MISSION_FLOW. Den kutter den svarte figuren, bilmøtet i SC04, huking, sjåføren og dikteringen i SC12, slår sammen H08 og H09 og leverer i fire akter i stedet for per scenario.

Godkjent av Tom 26. oktober 2026. Denne planen er arbeidsgrunnlaget for gjennomføringen. Første dokumentasjonsleveranse lagrer planen, kobler den fra README og todo og registrerer arbeidet. Kode, pakket kandidat, fletting og installasjon føres med egne faktiske kvitteringer.

## 1. Mandat, kilder og leveranserekkefølge

**Målfil i repoet:** `unreal/HISTORIE_IMPLEMENTERINGSPLAN.md`.

Målet er å gjøre hele hovedhistorien spillbar i Unreal: **13 sammenhengende scenarioer, alle 79 eksisterende enkeltscener, nødvendige verdenshandlinger, modeller, stemmer, film, lagring og progresjon.**

Avtalte leveranseregler:

- Gjennomfør **SC01 → SC13** i denne rekkefølgen.
- Hvert scenario får egen gren, kodecommits, testet spillpakke, PR, fletting og lokal installasjon.
- Full produksjon kreves før scenarioet ferdigmeldes. Midlertidig tekst, stillbilder eller manglende fiendelogikk oppfyller ikke ferdigkravet.
- Mens senere scenarioer fortsatt mangler, fortsetter spillet gjennom den eksisterende historien.
- Hver lagret sak beholder innholdsrevisjonen den startet med. Nye scenarioer prøves gjennom en ny natt; eksisterende saker får ingen bakoverrettede krav.

**Kildehierarki:**

1. Denne gjennomføringsplanen styrer arbeidsrekkefølge, testing og levering.
2. [MISSION_FLOW_1800_ROSWELL.md](MISSION_FLOW_1800_ROSWELL.md) styrer historiens rekkefølge og dramaturgi.
3. [Narrative/HorrorStory](Narrative/HorrorStory/README.md) inneholder engelske scenetekster, H-kort, scenariokontrakter og krav til verdensfakta.
4. Eldre detaljvedlegg brukes for kontrollverdier og bevis, underordnet hovedscenarioet.
5. Faktisk kode og siste testkvittering avgjør hva som allerede er implementert.

Ved motstrid mellom kildene registreres konflikten og avklares før den berørte mekanikken bygges. Eksisterende ID-er og fungerende spillerhandlinger bevares.

**Kontrollert utgangspunkt:** Integrasjonsgrenen er `unreal-port-core`. Baseline er `1c929b76`, med installert NP1. Manuspakken består 20 kontrakttester og har 10 H-kort, 13 scenarioer og 79 unike scener. Opptaksplanen har 54 cues som krever 53 separate opptak.

### Sammenhengen mellom scenarioer og oppdragskort

| H-kort | Tilhørende scenarioer | Kan ferdigmeldes etter |
|---|---|---|
| H01: Vanlig kveld | SC01 til SC02 | SC02 |
| H02: Den ordinære feilen | SC02 til SC05 | SC05 |
| H03: Den andre eksponeringen | SC06 | SC06 |
| H04: Det Dale ikke leverte | SC07 til SC08 | SC08 |
| H05: Kartleggingen | SC08 til SC09 | SC09 |
| H06: Rom 6 | SC10 | SC10 |
| H07: SARO blir undersøkt | SC11 | SC11 |
| H08: Det samme skjer flere steder | SC12 | SC12 |
| H09: All Night | SC12 | SC12 |
| H10: Roswell Road | SC13 | SC13 |

En enkeltscene med navnet `H01_CHECK` eller `H02_CHECK` kvitterer for sin konkrete kontroll. Den ferdigmelder ikke automatisk hele H-kortet.

## 2. Felles implementasjon og faste kontrakter

Det nødvendige grunnlaget bygges som del av SC01 og utvides bare når neste scenario trenger det.

### Manus, scenekobling og verdensfakta

- Lag en deterministisk C++-eksport av HorrorStory, med eksisterende EveningShift-datatyper som grunnlag.
- Innfør `FS47HorrorPack` for dialog, valg, dokumenter og manusstatus. EveningShift beholder eierskapet til overlevering, kontor, T01, T02 og VHS.
- Innfør `FS47Horror` som koordinator for scenario, mål, verdenshendelser, dialogkø og overganger.
- Koble koordinatorens start, stopp, oppdatering og lagring til eksisterende Main/Flow.
- Alle verdensfakta skal komme fra kontrollerte handlinger eller observerte hendelser. Dialogvalg kan registrere hva som er sagt, men kan ikke produsere fotografier, flukt, antennerespons eller gjennomført inspeksjon.

Opprett en maskinlesbar scenematrise med én rad per enkeltscene. Hver rad skal angi:

- SC- og H-tilhørighet.
- Konkret startsted eller spillerhandling.
- Verdenskrav og ansvarlig produsent.
- Lyd, modeller og dokumenter scenen trenger.
- Normalforløp, avbrudd og relevant alternativ.
- Test og bevis som dekker scenen.

Seks eksisterende scener mangler SC-kobling og innpasses eksplisitt:

- `H01_BRIEF`, `H01_CHECK`, `H01_INCOMPLETE` til **SC01**.
- `H02_BRIEF`, `H02_CHECK`, `H02_INCOMPLETE` til **T01-delen av SC02**.

Alle 96 eksisterende eksterne kravflagg skal ha en navngitt verdensprodusent. Et flagg uten produsent eller test blokkerer ferdigmelding av den berørte scenen.

### Lagring og overgang mellom gammel og ny historie

Legg til et eget versjonert `horror`-felt i GameState med innholdsrevisjon, scenariofase, manusstatus, hendelsestilstand og sikre sjekkpunkter. Eksisterende `timelineVersion` og gamle bevis beholdes.

- Gamle lagringer uten dette feltet fortsetter gjennom sin eksisterende rute.
- Nye saker får den siste ferdige, sammenhengende scenariorevisjonen.
- Når en slik sak går over i eksisterende fortsettelse, lagres denne grensen. En senere installasjon trekker den ikke tilbake.
- Verdensmodulene gjenopprettes før manusets eksterne krav avledes på nytt.
- Lagring av dialog og valg skal være atomisk. Lasting skal ikke gjenta dokumentutdeling eller engangseffekter.
- Fangst gjenoppretter et sikkert lokalt sjekkpunkt og bevarer ferdig undersøkelsesarbeid.
- Vanlig Continue skal bruke samme lagringsløype som ordinær spilling, uten utviklerflagg.

### Tid, farer og beskyttede situasjoner

- Én koordinator eier den nye historiens fortellertid og overganger.
- Tid går fremover. Transport og undersøkelser får plass før neste faste hendelse.
- Residualen beholdes ved **00:13:47**, komplette fjernark tidligst **04:04**, og finalen ved **05:29:00 til 05:29:47**.
- Automatisk kjøreplan og passiv mottaking holdes atskilt.
- Eksisterende tilfeldige hendelser bevares, men reserveres bort fra aktive møter, Rom 6, nødvendig kildekontroll og kartarbeid.
- En vellykket flukt etterfølges av ro. Den overstyres ikke av gammel tilfeldig bortføring.

De fem aktive møtene plasseres i **SC02, SC04 to ganger, SC08 og SC11**.

### Fysiske aktører, dekning og medier

Inntrengeren får reell sikt, definerte lydkilder, siste sansede sted, søk, forfølgelse, kontakttap og tilbaketrekning. Bevegelsen følger et målt rutegrafgrunnlag med faktisk kollisjon og dørtilstand.

Huking skal endre øyehøyde og kroppens eksponering, også ved bilvindu. Opake vegger stopper sikt. Lydavledning må komme fra en faktisk lydkilde på riktig sted.

Den svarte menneskeformen beholder en separat regel: stille under observasjon og bare flytting når både avgang og ankomst er usett. Paneler og pause gir ingen oppspart tilnærming.

Nye modeller produseres som redigerbare originalressurser med fungerende import. Stemmer følger eksisterende produksjonsløype med stabil rollefordeling, målte varigheter og riktige telefon-, radio- og romkanaler. Manglende takes kan ikke registreres som avspilt.

## 3. Gjennomføring av de 13 scenarioene

### SC01: Den første overflygningen, H01

**Implementer:**

- Behold direkte ankomst og valgfritt dinerbesøk, faktisk parkering og Dales overlevering.
- Koble H01-dialogen til normalrunden og kameraet ved østdøren.
- Gjør østdøren og tilgjengelig korridorutgang fysisk prøvbare fra innsiden.
- Produser en faktisk opak skjerm og kontroll av lampe sett direkte og gjennom glass.
- Bruk normalreferansen **1419.900 MHz, az 042, gain 55, båndbredde 48**.
- Koble første himmellys til utvendig runde uten kameratvang.
- Produser bevegelig blek hånd/underarm utenfor et kjent vindu, skjult bak virkelig veggkant, med etterlatte merker.
- Gi undersøkelse av glassets utside og en begrenset rapportmulighet.

**Aksept:** Begge ankomstruter, de to dørene, skjermen, normalreferansen, vindushendelsen og rapporten fungerer med vanlig input og Continue. Oversett himmellys blokkerer ikke progresjon. Rapporten påstår bare det spilleren faktisk har observert.

**Levering:** SC01 ferdigmeldes. H01 forblir åpent frem til SC02.

### SC02: Returen til bilen, H01/H02

**Implementer:**

- Gjenbruk T01s journal, separate indikator, lampeprøve og normalresultat.
- Koble H02-dialog og resultat til den eksisterende fysiske kontrollen.
- Start møte 1 først etter sikret T01-resultat.
- Produser blek reaktiv aktør, lesbart forvarsel, opak dekning og to farbare linjer til bilen utenfor gjerdet.
- La en klar kjørerute være gyldig flukt.
- Fangst gir lokal gjenopptakelse med T01 bevart. Returarkivering skal fortsatt skje én gang.

**Aksept:** Begge gangruter, direkte kjøreflukt, fangst, avbrudd og Continue prøves. Ingen passasje gjennom høyspenningsområdet eller dokumentert dekning som bare finnes visuelt.

**Levering:** SC02 og H01 ferdigmeldes. H02 forblir åpent.

### SC03: Hjelpen som kom, H02

**Implementer:**

- BY01 med konkret inntrengermelding, datert readback og faktisk utkalling.
- Produser Mara Ortiz, hvit servicevogn 23, vanlig identifikasjon og kort samtale før tapet.
- Innfør den svarte figurens observasjonsregel som et valgfritt spor.
- Produser synlig løft/bortføring, etterlatt kjøretøy og navngitt personlig gjenstand.
- Gjør trygg retrett og etterfølgende undersøkelse mulig.
- Registrer savnetsak separat fra første hendelsesmelding.
- Ward suspenderer den lokale 06:00-serien.

**Aksept:** Utkalling, identifikasjon, synlig tap, etterspor og savnetmelding har egne fakta. Skyggefiguren kan overses. Nødvendige eksisterende personer beholdes.

### SC04: Sikre og kom i dekning, H02

**Implementer:**

- Avtal den avgrensede returen fra Caliche.
- Produser fysisk auto-/surveyplansperre med synlig kontrollresultat; passiv mottaking består.
- Sikre daterte skift- og serviceoriginaler.
- Møte 2 i annexet med lang skjermet rute og kortere rute via virkelig lydavledning.
- Bruk et identisk, tidligere avspilt Dale-fragment som avtrykk. Skipping av dialog gir ingen falsk hørthetskvittering.
- Legg inn faktisk ro og kontakt etter annexflukten.
- Møte 3 ved bilen med kroppseksponering, vinduslinje, skjermet parkering og fungerende motor.
- Produser trygg rute til kontrollrommet og avtalen som leder videre til passiv residual.

**Aksept:** Begge annexløsninger, bilflukt, fysisk skjerming, lokal gjenopptakelse og sikre funn prøves. Ingen oppgave ber spilleren oppheve plansperren for å fortsette.

### SC05: Samtalen som ligger foran, H02

**Implementer:**

- Koble fullført sikkerhetsakt og T01-arkivering til passiv residualovergang.
- Behold notch **1420.110 MHz, gain 55, BW 12** og residual **1420.405 MHz, gain 82, BW 12, søkeaz 083**.
- Behold faktisk utskrift, telefonsamtale og romhandling 47 spillsekunder senere.
- Koble S-03s avvik mellom 042 og 026 til den nye motivasjonen.
- Oppdater mål og dialog slik at tidligere kropp og bortføring tas alvorlig.

**Aksept:** SC01 til SC05 kan spilles sammenhengende uten utviklerhopp. Residualen starter med automatisk plan sperret. Telefonpaneler er beskyttet.

**Levering:** H02 ferdigmeldes.

### SC06: Det andre motivet, H03

**Implementer:**

- Koble H03 til eksisterende B-12, FRAME01/02, fremkalling og P01 til P03.
- Støtt både passiv lampe-/motorisolering og aktiv 042-kontroll.
- Krev faktiske eksponeringer og kontrollerte sammenligninger.
- Behold negativet ved dårlig kopi og gi konkret fremgangsmåte for ny kopi.

**Aksept:** Begge kontrollmetoder, dårlig fremkalling, ny kopi, dokumentbevaring og Continue prøves. Dialog erstatter ingen fotografisk handling.

**Levering:** H03 ferdigmeldes.

### SC07: Det Dale ikke leverte inn, H04

**Implementer:**

- Koble E06/E07, lineage, Nora-kontakt, serviceskuff, reservenøkkel og eksisterende kontor.
- Behold feltmappens korte hovedrute og de frivillige lesedokumentene.
- Produser originalfilmen **«Service Road / Field 01»**, 72 sekunder, etter eksisterende storyboard, med bevegelse og miljølyd.
- Bruk den eksisterende TV-pakkingen som grunnlag for et eget program med spill/pause, stopp, spoling, strøm og lagret avspillingssted.
- Sett settstatus fra faktisk avspilte bilder. Spoling og åpning av transkript gir ingen settstatus.
- Produser vindusendringen etter undersøkelsen, etter at lesepanelet er lukket.

**Aksept:** VHS er faktisk bevegelig og hørbar. Strømbrudd, pause, spoling og fersk Continue prøves. Feltmappen fører hovedhistorien videre også når filmen hoppes over.

**Levering:** SC07 ferdigmeldes. H04 forblir åpent.

### SC08: De peker på deg, H04/H05

**Implementer:**

- BY02 med identifisert 1981-spole, separat TAPE/LIVE-valg og fysisk frakobling av den andre kilden.
- Bevar fullførte prøver separat fra aktuell bryterstilling.
- Produser legitim datert utleveringskopi og avspillingsmulighet etter Earls avgang 04:00.
- Bruk eget arrayserviceark med **A / ROW 1 WEST**, **B / ROW 1 EAST**, **REF / R1**.
- Produser fast referanselykt, innledende kommandokontroll, fritt A/B-valg, kort retur og etterfølgende kontroll av kommandohistorikken.
- La antennene reagere på faktisk spillerposisjon.
- Gjennomfør møte 4 med to retrettmuligheter og bevart undersøkelsesarbeid.
- Etter flukt holdes det sist forlatte stedet kort før retur til 026.

**Aksept:** A→B→A og B→A→B gir riktige forskjellige stillinger. Tid alene kan ikke bestå prøven. Tidlig original, sen kopi, kildeveksling, fangst og Continue prøves.

**Levering:** H04 ferdigmeldes. H05 forblir åpent.

### SC09: Den avbrutte oppstillingen, H05

**Implementer:**

- Koble ny arraykunnskap til STATION01s eksisterende fastmerke, nulltest, kabel, receiver og Tomás.
- Behold skillet mellom arraypunktene, STATION01s geometri og T02s kulvert.
- Legg inn kort lysavskjæring med faktisk opak dekning.
- Samordne dette med gammel betinget bortføring uten å overstyre en vellykket ny retrett.
- Behold faktisk feltretur og fremkalling av FRAME03/04 før Nora.

**Aksept:** Hele felt-/returreisen, begge feltkopier, dårlig kopi, uavklart T02 og Continue prøves. T02 forblir valgfri for hovedprogresjonen.

**Levering:** H05 ferdigmeldes.

### SC10: Rom 6, H06

**Implementer:**

- Koble nye scener til Nora, bevisene og eksisterende P10 til P12.
- Tilpass respons til valgt B-12-kontroll.
- Gjør nytt spørsmål til returlinjen og uendret gammelt fragment til en faktisk sammenligning.
- Behold brevet og krusnøkkelen som historisk opplysning.
- Avslutt med konkret motivasjon om originaler og varsling.

**Aksept:** Alle tre emner, begge B-12-metoder, avbrutt samtale og Continue prøves. Hele samtale- og brevforløpet er beskyttet mot nye farer.

**Levering:** H06 ferdigmeldes.

### SC11: SARO blir avsøkt, H07

**Implementer:**

- Produser SARO-original, påbegynt hendelsesark og komplette Hondo-/Site11-ark tidligst 04:04.
- Krev at bunten er sikret før avsøkningen starter.
- Bygg møte 5 med samordnet ytre lys, reaktiv aktør, dørtilstand og to fysisk gyldige fluktruter.
- La lokal lampe kunne slås av mens den eksterne lyskilden fortsetter.
- La faktisk verdensflukt produsere fluktstatus før rapportsamtalen.
- Lag den samlede SARO-dokumentasjonen først etter kontroll og fullført flukt.

**Aksept:** Begge ruter, lokal lyskildekontroll, sanset reaksjon, fangst, sikker gjenopptakelse og Continue prøves. Tidlig ankomst gir ingen fremtidsdata. Spilleren trenger ikke gå inn igjen etter et siste ark.

**Levering:** H07 ferdigmeldes.

### SC12: Det dere rekker å sende, H08/H09

**Implementer:**

- BY03 med beskyttet sammenstilling i dineren.
- Gjør opphav, uavhengige kanaler, klokker, faseorden, råverdier og frakoblet feed etterprøvbare.
- Bevar forskjellen mellom Hondo-grensene og Site11s midtgrense.
- Produser faktisk diktering av kilde-ID-er og måletabeller til Ward, datert logg og readback.
- Gi varselkvittering først etter gjennomført samtale.
- Behold E15 som gammelt dinerutklipp uten 04:45-port og P13 som faktisk kartoppgave.
- Produser fersk stedfestet utfartsobservasjon, sjåførens nylige gjennomkjøring og budavtale ved 10.4.
- Skill C ved 8.15 fra overleveringsmålet.

**Aksept:** Manglende kilde, sen ankomst, uferdig kart, avbrutt telefon og Continue prøves. Varslingen lykkes før avreise. Ruteopplysningene gir begrenset grunnlag, ingen trygghetsgaranti.

**Levering:** H08 og H09 ferdigmeldes separat.

### SC13: Siste vei mot Roswell, H10

**Implementer:**

- Oppdater avreisemål, kart og CB til overlevering ved 10.4.
- Kontroller originaler, varselkvittering og kartløsning før avreise.
- Bruk eksisterende vei og finale ved C8.15.
- Behold ufrivillig motorstopp, THE EVENT, uttrekk og hele 1947-bildet.
- Overleveringspunktet forblir et planlagt mål som finalen avbryter.

**Aksept:** Vanlig kjøring, stopp og rygging, sikkert Continue og hele finalen til sluttmeny prøves på High og Low. Fortellertiden går ikke bakover. Ingen alternativ slutt innføres.

**Levering:** H10 ferdigmeldes. Deretter gjennomføres samlet sluttaksept.

## 4. Test- og ferdigkrav

Hvert scenario skal ha et avgrenset, navngitt testsett som dekker:

1. Normal spillerreise med vanlige interaksjoner.
2. Vesentlige alternative løsninger og uavklarte utfall.
3. Avbrudd gjennom pause, dialog, dokumenter og områdebytte.
4. Faktisk lagring og fersk Continue i separat prosess.
5. Negativ prøve: manglende bevis kan ikke gi positivt resultat.
6. High/Low-bilder fra spillerhøyde, kontrollert visuelt.
7. Nye stemmer, undertekster, kanalruting og relevante lydeffekter.
8. Et kort utvalg tidligere berørte systemer.

Testene skal skille mellom datakontrakt, API-prøve, faktisk input, bildebevis og menneskelig vurdering.

**Et scenario kan teknisk ferdigmeldes når:**

- Alle tilhørende enkeltscener har fungerende start og krav.
- Nødvendig geometri, aktører og medier er produsert.
- Handlinger og konsekvenser virker i den pakkede kandidaten.
- Fortsettelse, avbrudd og lagring består.
- Bilder er vurdert, og kjente feil eller begrensninger er ført.
- Kandidatens kilde, binær og testbevis er entydig bundet sammen.

**Samlet sluttaksept etter SC13:**

- Alle 79 opprinnelige scener er dekket, inkludert alternative og uavklarte grener.
- Alle fem aktive møter har gyldig flukt og lokal gjenopptakelse.
- Alle ti H-kort oppfyller kravene på tvers av scenarioene.
- En ny natt kan gjennomføres til slutt uten utviklerhopp eller injiserte bevis.
- Gamle lagringer og tidligere innholdsrevisjoner fortsetter korrekt.
- Hele eksisterende regresjonsmatrisen kjøres én gang ved sluttintegrasjonen.
- Menneskelig gjennomspilling med lyd, forståelse av mål, tempo og fysisk Xbox registreres separat. Slike vurderinger kan ikke erstattes av automatiske tester eller erklæres bestått uten faktisk prøve.

## 5. Arbeidsflyt, fletting og installasjon

For hvert SC følges samme rekkefølge:

1. Opprett scenarioets gren fra kontrollert, oppdatert `unreal-port-core`.
2. Registrer eierskap, avhengigheter og forventet scenedekning.
3. Implementer en komplett spillerreise med nødvendige ressurser.
4. Kjør manus-/datakontroll, bygg og målrettet pakket test.
5. Rett feil og gjenta bare berørte kontroller.
6. Før testresultater, bilder og resterende begrensninger i repoet.
7. Commit og push scenarioets endringer.
8. Opprett PR med konkret før/etter-beskrivelse og testgrunnlag.
9. Verifiser PR-head og påkrevde kontroller; flett til `unreal-port-core`.
10. Installer nøyaktig samme testede pakke med full rollbackbackup.
11. Kontroller payload, personlige lagringer og oppstart via standardstarter.
12. Før faktisk merge- og installasjonskvittering i en dokumentasjonscommit via PR før neste scenario starter.

Bare dokumentasjon kan endres mellom testet kandidat og installasjon uten ny spilltest. Endres kode, ressurser eller pakkeinnhold, må den berørte kandidaten testes på nytt.

Planen skal ha en løpende statustabell med **SC-ID, H-kort, kodecommit, pakket test, PR, mergecommit, installasjon og åpne begrensninger**. Status går gjennom:

`Planlagt → Under arbeid → Kilde ferdig → Pakket test bestått → Flettet → Installert`

Subagenter får avgrensede filansvar for eksempelvis manusadapter, modeller, lyd og uavhengig gjennomgang. Én integrasjonseier håndterer felles kode, scenariooverganger og leveranser. Unreal-bygg, pakking og grafiske prøver kjøres sekvensielt på maskinen.

Etter hver levert installasjon brukes prosjektets eksisterende prune-rutine. Gjeldende verifiserte bygg og maksimalt to eldre komplette testbygg beholdes. Kilder, Blender-filer, personlige lagringer og testbevis bevares.

**Leveringsregelen er revidert 28. oktober:** scenarioene produseres og prøves fortløpende innen hver akt. Hele matrisen, fletting og installasjon skjer én gang per akt, som beskrevet i [gjeldende revisjon](HISTORIE_SC02_SC13_REVISJON.md).

## 6. Løpende gjennomføringsstatus

Opprettet 26. oktober 2026, oppdatert 28. oktober. SC01 er flettet via PR #174 og installert som standard 27. oktober 2026, og er med i Linux-testutgaven 28. oktober 2026. Codex har hjulpet Claudes SC02 videre på Kubuntu med hindringsnavigasjon og pakket teknisk aksept; se [overlevering](SC02_AI_HANDOFF.md). Verdensprodusenter, scenematrise og testsett for SC01 til SC13 ligger i `Narrative/HorrorStory/world_producers.json`. Senere scenetekster er samordnet med revisjonen, men runtime er fortsatt planlagt. `Ikke utført` er en faktisk status, ikke en planlagt kvittering.

| SC-ID | Scenario | H-kort | Status | Kodecommit | Pakket test | PR | Mergecommit | Installasjon | Åpne begrensninger |
|---|---|---|---|---|---|---|---|---|---|
| SC01 | Den første overflygningen | H01 | Flettet og installert | dbae3177 | 21 kjøringer PASS, 32 vurderte bilder; full matrise 110/110 mot main | PR #174 | `a032ed0e` | Installert standard | Menneskelig lytting/tempo/Xbox/helnatt UNVERIFIED |
| SC02 | Returen til bilen | H01/H02 | Pakket teknisk PASS, akt 1 under arbeid | 651dc558, inkluderer Claudes 911ce258 | R5: 159 kontroller, 19 disk-/Flow-restores, 1649 selvtester, 28 native kontroller | Arbeidscheckpoint | Ikke flettet | Ikke utført | Rute B/manuell styring/High-jakt/menneskelig prøve UNVERIFIED; SC03–05 og full aktmatrise gjenstår |
| SC03 | Hjelpen som kom | H02 | Planlagt | Ikke ført | Ikke utført | Ikke opprettet | Ikke flettet | Ikke utført | Avventer levert SC02, BY01 og Ortiz-produksjon |
| SC04 | Sikre og kom i dekning | H02 | Planlagt | Ikke ført | Ikke utført | Ikke opprettet | Ikke flettet | Ikke utført | Avventer levert SC03, plansperre og møte 2/3 |
| SC05 | Samtalen som ligger foran | H02 | Planlagt | Ikke ført | Ikke utført | Ikke opprettet | Ikke flettet | Ikke utført | Avventer levert SC04 og passiv residualovergang |
| SC06 | Det andre motivet | H03 | Planlagt | Ikke ført | Ikke utført | Ikke opprettet | Ikke flettet | Ikke utført | Avventer levert SC05 og H03-integrasjon |
| SC07 | Det Dale ikke leverte inn | H04 | Planlagt | Ikke ført | Ikke utført | Ikke opprettet | Ikke flettet | Ikke utført | Avventer levert SC06 og original 72s VHS |
| SC08 | De peker på deg | H04/H05 | Planlagt | Ikke ført | Ikke utført | Ikke opprettet | Ikke flettet | Ikke utført | Avventer levert SC07, BY02, ARRAY-01 og møte 4 |
| SC09 | Den avbrutte oppstillingen | H05 | Planlagt | Ikke ført | Ikke utført | Ikke opprettet | Ikke flettet | Ikke utført | Avventer levert SC08 og ny feltkobling |
| SC10 | Rom 6 | H06 | Planlagt | Ikke ført | Ikke utført | Ikke opprettet | Ikke flettet | Ikke utført | Avventer levert SC09 og ny samtalekobling |
| SC11 | SARO blir avsøkt | H07 | Planlagt | Ikke ført | Ikke utført | Ikke opprettet | Ikke flettet | Ikke utført | Avventer levert SC10, fjernark og møte 5 |
| SC12 | Det dere rekker å sende | H08/H09 | Planlagt | Ikke ført | Ikke utført | Ikke opprettet | Ikke flettet | Ikke utført | Avventer levert SC11, BY03, varsling og rutegrunn |
| SC13 | Siste vei mot Roswell | H10 | Planlagt | Ikke ført | Ikke utført | Ikke opprettet | Ikke flettet | Ikke utført | Avventer levert SC12 og sluttaksept |

### Separat ferdigmelding av H-kort

H-status følger alle kravene gjennom de oppgitte scenarioene. Eksisterende delsystemer som T01, T02 og kontor er fortsatt levert innen sine tidligere dokumenterte grenser; det ferdigmelder ikke den nye H-kjeden.

| H-kort | Nødvendige scenarioer | Tidligste ferdigmelding | Status | Ferdigkvittering |
|---|---|---|---|---|
| H01 | SC01 og SC02 | Etter SC02 | Åpent | Ikke ført |
| H02 | SC02, SC03, SC04 og SC05 | Etter SC05 | Åpent | Ikke ført |
| H03 | SC06 | Etter SC06 | Åpent | Ikke ført |
| H04 | SC07 og SC08 | Etter SC08 | Åpent | Ikke ført |
| H05 | SC08 og SC09 | Etter SC09 | Åpent | Ikke ført |
| H06 | SC10 | Etter SC10 | Åpent | Ikke ført |
| H07 | SC11 | Etter SC11 | Åpent | Ikke ført |
| H08 | SC12 | Etter SC12 | Åpent | Ikke ført |
| H09 | SC12 | Etter SC12 | Åpent | Ikke ført |
| H10 | SC13 | Etter SC13 | Åpent | Ikke ført |
