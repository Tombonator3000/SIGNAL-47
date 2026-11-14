# Feltbevis som skiller de ukjente (Freeform)

**Status:** Under arbeid. Besluttet av Tom 13. november 2026. Trinn 1 er installert i d66; trinn 2 og 3 er kodet, ikke bygget og ikke spilt av Tom. **Gjelder:** Freeform, feltarbeid, notatboka, arkivpermen og Wards vurdering. **Sist kontrollert:** 14. november 2026 mot kilden på `claude/freeform-20261112` (lest og skrevet, ikke bygget eller spilt).

Toms idé 13. november: bevis av ulike slag fra feltet (bilder, geiger, målinger, prøver) logges og legges i arkivet med bilder og tegninger, slik at spilleren gradvis forstår hva slags vesener han har med å gjøre. Inspirasjon er løkka i UFOPHILIA (måle → utelukke → fotografere). Vi tar bare den generelle ideen om å utelukke ut fra bevis. Arter, navn, bevistabell, grensesnitt og ordlyd lages fra det spillet vårt allerede har.

Gjeldende regler som dette må følge: [DECISIONS](../../Active/DECISIONS.md) (tidsnivåer 9. november, Freeform etter Story, vev inn det som finnes), [milepælene](../../Active/TODO.md) og [game bible](../GAME_BIBLE.md) (vanlige forklaringer kan være riktige; et avvik fastslår ikke art eller hensikt).

## Besluttet av Tom 13. november

Tom godkjente forslaget 13. november 2026 med disse svarene på de fem spørsmålene i del 6:

1. **Veiglimtets «crawler» er den lave, og «tallfigure» er den høye.** De deler mappe med den lave og den høye fra stedsbesøkene.
2. **Sølvmannen og den som lyser får egne mapper som stopper på MARKS.** De kan aldri arkiveres som ferdige.
3. **Skyggen får aldri en mappe.** Den blir stående uløst under SHADOWS.
4. **Navnet er spillerens egen tekst,** med et forslag fylt inn på forhånd. Kodet i trinn 3.
5. **Ward gir ett poeng for to arkiverte mapper og to poeng for alle tre som kommer nær.** Kodet i trinn 3.

### Trinn 1 slik det er kodet (13. november, ikke bygget, ikke spilt av Tom)

Hva spilleren får:

- **Stempel der beviset tas:** `UNKNOWN 2 / GROUND.`, med «New file in the notebook.» første gang og «The file is at MARKS now.» når trinnet stiger. Stempelet sier bare nummer og bevistype.
- **Notatboka (TAB › Knowledge › Who & what, høyre side THEM):** én ramme per mappe øverst på siden, med nummer, trinn, seks blyantbokser (SHAPE, TAPE, GROUND, LAMP, GEIGER, ANIMALS), blyantnotat for de fylte boksene og én linje om hva neste trinn krever.
- **Permen (skillearket UNKNOWNS etter FIELD + BENCH):** de samme mappene maskinskrevet, bare lesing, med bildene henvist til albumets original.
- **Ward:** én setning i ukenotatet fra første mappe på MARKS.
- **OPS-kvitteringen for bilder:** nøytrale titler («small figure», «low shape on the ground», «still figure»).

Hvor det ligger:

- Ren modell og selvtest: `Public/S47Dossier.h`, `Private/S47Dossier.cpp`.
- Øktas del (merke, stempel, lagring, gjenoppbygging, permens ark): `Private/S47FreeformDossier.cpp`.
- Merker ved kilden: bilde i `FS47FreeformDirector::Photo` og `ClaimEnlargement`, veispor i `S47Happenings.cpp` (`road_tracks_d<natt>`), `rule-still-held`, `rule-pale-broken` og feltsiden `<sted>_visit` i `S47FarmSite.cpp`.
- Visning: `SS47Notebook.cpp` (THEM), `S47RecordsBinder.h/.cpp` og `SS47Binder.cpp` (UNKNOWNS), `S47PredecessorText.cpp` (`Apart`).
- F10: gruppen «Evidence dossier (M3)» med prøvebevis for tre mapper (`S47DossierDev.cpp`).
- Lagring: `dossier: { order: [...], tags: { "<nøkkel>": { k, t, d } } }`. Feltet skrives først når noe er merket. En eldre lagring bygges opp igjen fra fotokravene (`strange.<gruppe>.n<natt>`), `rule-still-held` og veisporene.

Avvik fra forslaget, og hvorfor:

- **Første boks heter SHAPE og fylles av bilde eller syn.** Et fotografi gir «on film», og stedsbesøkets feltside gir «seen». Forslaget sa at feltsiden skulle merkes, men ikke med hvilken bevistype. Sidens ord om lykta er like for alle vesener (`S47VisitAttention`) og kan derfor ikke være et LAMP-trekk. I lagringen heter typen `shape`; et bilde kjennes på nøkkelen.
- **`rule-pale-broken` merkes bare når det var den lave.** Den høye kan også bryte regelen i koden (N1110), men det er ikke dens LAMP-trekk i matrisen. I en eldre lagring merkes funnet ikke, fordi verken nøkkel eller tekst sier hvem det var.
- **Veisporene registreres i `S47Happenings.cpp`,** ikke i `S47RoadGlimpse.cpp`, som bare har teksten. Stedet husker bare «low» eller «tall». Lange smale spor går derfor til den høye også når kroppen som krysset veien, var sølvmannen eller den som lyser.
- **Stempelet bruker skråstrek,** slik spillets andre stempler gjør.
- **Feltsiden skrives én gang per sted.** Et senere besøk på samme sted gir derfor ikke noe nytt merke i trinn 1.
- **Wards setning står sammen med de andre setningene hennes** (`LeverWords`). Den kommer derfor også når spilleren spør henne på telefonen om vurderingen.
- **Kvitteringer som alt er lagret, beholder teksten sin.** Nye kvitteringer og fotokrav lest fra eldre lagringer får nøytral tittel.
- **Ikke med i trinn 1:** betaling per bevistype under ukas tak, boksen satt til «gir ikke noe», UNPLACED og tilordning. Sammenligningen passer, strider eller ukjent finnes i modellen og er prøvd, men vises først i trinn 2.
- **Med trinn 1 alene kan bare den lave nå MARKS** (form, veispor og regelbruddet). De små har to bevistyper, og den høye, sølvmannen og den som lyser har én eller to. Trinn 2 gir resten.

### Følger av trinnene (13. november)

Tom godkjente 13. november kritikernes funn: et trinn som bare gir et stempel, er ikke ferdig. Hvert trinn får nå én følge med det spillet har fra før, og én linje under trinnet på THEM og på permens UNKNOWNS-ark sier hva det ga. Kodet 13. november, ikke bygget, ikke spilt av Tom.

| Trinn | Hva spilleren får | Hvor han ser det |
| --- | --- | --- |
| OUTLINE | Stedet der mappa først ble logget ute, blyantført med «UNKNOWN n» ved siden av. Over måneden viser kartet hvor hver av dem har vært. | Feltkartet i notatboka; linjen «Where it was first logged is on the map.» |
| MARKS | Én peiling mer enn nattens ene, én gang per mappe, med den vanlige rekkefølgen av mål (forbi det kveldens lapper alt har pekt på). Aldri på den store natta, aldri for første dags gratis lapp; tidsnivåene og alle andre avslag gjelder. Gitt for natta trinnet nås, fra kl. 05 for kommende natt, og står åpen til den er skrevet ut. | Lappen og toasten («The station knows what to listen for now: one more bearing tonight.»), kveldens plan til den er brukt, og linjen under trinnet. |
| HABITS | Timene mappa er logget på så langt, i nattens rekkefølge, uten forklaring. | Én linje i kveldens plan: «UNKNOWN 2 / Logged so far at 23:00, 01:00, 03:00.» |

Regler som holder: spillet sier aldri hva noe er; ingen ord fra Freeforms forbudsliste; sølvmannen og den som lyser stopper på MARKS (de gir også peilingen); skyggen har ingen mappe. Trinn 1 når ikke HABITS ennå, men regelen og selvtesten finnes.

Lagring: hvert merke kan ha `h` (timen) og `x`, `z` (der spilleren sto, kartets egne meter), og `dossier` kan ha `extra` (natta peilingen gjelder fra) og `spent`. Alt er valgfritt. En eldre lagring leses som før: uten timer og kartmerke, og en mappe som alt står på MARKS får sin ene peiling ved lasting. En forstørring i mørkerommet gir verken time eller sted.

Kode: `S47Dossier` (`Gave`, `HoursLine`, `ExtraNightFor`, `FirstWhere`, `HoursOf`, `Grant`, `Spend`), `S47FreeformDossier.cpp` (`TagEvidence`, `DossierMap`, `DossierTasks`, `DossierExtraOpen`), `FS47Places::SlipAllowed` og `AfterRegistered`, `S47FieldMap::AddFirstLogged`, THEM i `SS47Notebook.cpp`.

### Trinn 2 slik det er kodet (14. november, ikke bygget, ikke spilt av Tom)

Hver kanal som var lik for alle, har nå et eget trekk per vesen. Alt er små varianter av det spillet har. Ingen nye mekanikker og ingen nye lydfiler.

| Kanal | De små | Den lave | Den høye | Hvor | Merkes når |
| --- | --- | --- | --- | --- | --- |
| BÅND (feltbåndet) | tonestiger som ender i et spørsmål | dobbelt hjerteslag, raspende pust | dype pulser, to toner som gnisser, metall som knirker | `FS47Tape`: `OddKind` fra kilden (`S47Dossier::KindOfOmen`), signaturen fra `S47AlienKinds::Render` spilles ved pulten (`Follow`, nøkkel `tape`) i stedet for den vanlige lyden; ord i funnet fra `OddWordsOf` | båndet er spilt av forbi stedet (`HeardThrough`), med time og sted fra opptaket |
| BAKKE (dagslys på stedet) | små bare spor, steiner snudd med fuktig side opp | spor i firer, en hånd fra hverandre, ut av ringen i løp | lange smale spor, et langt steg, nesten fire fot | `FS47FarmSite::PrintsText`; stedet husker hvem som kom til merkene (`MarksBy`, lagret `marksBy`/`marksByNight`) | siden åpnes ved dagslys |
| GEIGER | litt over (×0,55) | ingenting (bakgrunn) | klar stigning (×1,6) | `S47Dossier::GeigerFactor` i `FS47Happenings::DoseAt`, over merkene og nær kroppen under besøket | telleren er ute innen 12 m i 3 s (`GeigerReads`) |
| DYR | hundene bjeffer og tier | coyoten stirrer og løper | fuglene letter | `FS47AnimalWarn::ForVisit`, `FS47FarmSite::WarnAnimals` (covey og coyote med sine egne skript; hundene med lydene spillet har) | 3 s inn i varselet, til fots |
| LYKT | (regelen fra trinn 1) | (regelen fra trinn 1) | fryser i første lyskjegle: nytt funn `rule-tall-froze` | `FS47FarmSite`, der den høye fryser | når den fryser, sett i lykta |

De to fjerne får bare kanalene matrisen gir dem. Sølvmannens og den som lyser sitt bånd er det uvanlige opptaket på mottakeren (`DossierHeardTake`, hørt gjennom, ikke før midnatt). Lykta kommer fra figuren som kommer nærmere (`figure_near`): sølvmannen er borte når han blir sett, den som lyser synes uten lykt på seg. Skyggen får fortsatt ingenting.

**Mapper som spilleren kan knytte sammen.** En mappe er ikke lenger én per vesen. Et funn går i en mappe som allerede har det samme i samme boks, eller som har noe fra samme natt innen 250 m. Uten sted og av samme vesen er samme natt nok. Finnes ingen slik mappe, åpner funnet sin egne, også når spillet vet at det er det samme vesenet. Et bånd av noe som aldri er fotografert, blir altså sin egen UNKNOWN. Når ett funn knytter to mapper, slår spillet dem ikke sammen. THEM viser dem side om side, boks over boks, med et rødt blyant-likhetstegn der de har det samme. Permen skriver «KEPT BESIDE UNKNOWN n.». Når et funn fra samme natt og sted motsier det som står i en boks, strekes den gamle blyantlinjen over og den nye blir stående. Permen skriver den som «STRUCK / …». Spillet forklarer aldri hva merkene betyr.

**Tempo (M7).** HABITS venter på første uke i kurven som har fare i seg (`director.json`, `curve.weeks`: dag 8). Selvtesten leser portene fra fila (`site_visit`, `field_visit`, `road_glimpse`, `horizon_figure`, `figure_near`, regelbruddet fra natt 4) og regner det beste tilfellet. Ingenting kan nå HABITS i uke 1. De tre som kommer nær, kan nå HABITS fra natt 8, altså innen uke 4. De to fjerne stopper på MARKS, tidligst natt 10. Første MARKS kan komme natt 3, altså godt innen uke 2.

Lagring, alt valgfritt: `files` (vesenet som åpnet hver mappe, bare når et vesen har flere), `f` på et merke (mappa, når det ikke er vesenets første), `n` (rekkefølgen det ble logget i), `same` (parene side om side), og `oddKind` på et bånd. En lagring fra trinn 1 leses som før, og en eldre lagring bygges opp igjen med én mappe per vesen.

Avvik fra planen, og hvorfor:

- **Ingen tilordning ennå.** Planen hadde UNPLACED med spillerens egen tilordning. Her åpner et funn spilleren ikke kan knytte, sin egen mappe, og to mapper som viser seg å være det samme, står side om side. Å legge dem sammen er `FILE AS ONE` i trinn 3.
- **HABITS-sperren fra kurven er ny.** Portene alene stopper ikke HABITS i uke 1, fordi stedsbesøk finnes fra natt 2 og gir mange bevistyper på én gang. Sperren bruker spillets egen kurve, der uke 1 har fare 0.
- **Mottakerens bånd teller bare for de to fjerne.** Matrisen sier at båndet til de tre nære kommer fra feltbåndet.
- **Lykta for de to fjerne kommer bare fra `figure_near` (fra dag 10),** ikke fra figuren på høyden. Ellers kunne de nå MARKS i uke 1.
- **Den ekstra peilingen ved MARKS gis én gang per vesen, ikke per mappe.** Ellers kunne en delt mappe gi flere.
- **Dyrevarselet før besøket bruker ikke skriptenes odds.** Det kommer ved hvert besøk til fots, der det er plass til dyret.
- **Geigeren krever røret fra OPS.** Tempoprøven regner beste tilfelle og antar at telleren er bygget fra første besøk.
- **Lyden er ikke sendt som WAV.** Signaturene er de samme som mottakeren alt spiller (`S47AlienKinds::Render`), og ingen bygg er kjørt.

Kode: `S47Dossier` (`Tag`, `Together`, `Agreement`, `BesideWords`, `Earliest`, `FGates`, `GeigerFactor`, `KindOfOmen`, `AlienKindOf`, `GeigerReads`, `TakeHeard`, `LintWords`), `S47FreeformDossier.cpp` (`TagEvidenceAt`, `DossierHeardTake`, `DossierHabitsFrom`), `S47Tape`, `S47FarmSite`, `S47Happenings` (`DoseAt`, `StepGeiger`, `StepFigure`), `S47AnimalWarn::ForVisit`, THEM i `SS47Notebook.cpp` (`UnknownPairBox`, strøket blyant) og `S47Flow.cpp`. Selvtest: linjene `EVIDENCE-DOSSIER: slice 2`, `tape: EVIDENCE-DOSSIER slice 2`, `animal warning: EVIDENCE-DOSSIER slice 2` og `Freeform THEM (slice 2)`. F10: «Evidence dossier (M3)» fyller alle kanaler, viser arbeidsnavnene og tempoet fra portene.

### Trinn 3 slik det er kodet (14. november, ikke bygget, ikke spilt av Tom)

Hva spilleren får:

| Del | Hva han ser og gjør | Hva det fører til | Hvor |
| --- | --- | --- | --- |
| Navn | Ved permen i arkivrommet har hver mappe et navnefelt, fylt inn med et forslag fra det mappa holder (SHAPE og GROUND, ellers første fylte boks: «Big head, bare feet», «Folded one, in fours», «Far-off one, long stride», «Lit inside»). Han skriver sitt eget og trykker Enter, eller velger linja «WRITE THE NAME ON THE FILE» (også med håndkontroll). | Navnet står ved siden av «UNKNOWN n» på THEM, i permen, på kartmerket, i stempelet der beviset tas og i timelinja på kveldens plan. En mappe med navn på HABITS er FILED: én gang $5 i feltgodtgjørelse under ukas tak, og den teller hos Ward. | `S47Dossier::CleanName`, `Suggest`, `Titled`, `FS47Dossier::SetName`; `FS47FreeformSession::DossierName`; navnefeltet i `SS47Binder::SheetPage` |
| Arkiver som én | Når en mappe er valgt ved permen, står linjene «FILE AS ONE WITH UNKNOWN n» under den, de som står side om side først (merket «KEPT BESIDE»). Han kan også legge sammen to som spillet vet er ulike. | Laveste nummer blir stående med boksene fra begge; det andre nummeret forsvinner fra listene. Navnet beholdes, eller arves når den overlevende ikke hadde noe. Der de to hadde ulikt i samme boks, strekes blyanten over, og boksen teller ikke for trinnet. Ingenting sier om det var riktig. «TAKE UNKNOWN n OUT AGAIN / UNTIL 08:00» angrer til neste 08:00. | `FS47Dossier::Merge`, `TakeOut`, `StageConsistent`; `DossierFileAsOne`, `DossierTakeOut`; radene i `S47Binder::FModel::Rows` |
| Ward | Ett poeng i vurderingen for to av de tre nære som er FILED hver i sin egen mappe (riktig holdt fra hverandre eller riktig lagt sammen), to poeng for alle tre. Én linje i ukenotatet når de teller: «Two of your files are filed complete, each kept to itself. That is archive work, and it counts.» | En feil sammenlegging gir ingen poeng for vesenene i den før den tas ut igjen. Terskelen på 6 står. | `FS47Dossier::KindsFiled`, `S47Dossier::WardPoints`, `FS47Predecessor::Points` (`FTally::Archived`), `S47PredecessorText::Archived` i `LeverWords` |
| Vaner som tanker | En mappe på HABITS kommer som spillerens egen tanke, én gang per natt per mappe: «Near 03:00. I logged it about this hour.» når en mørk time mappa er logget på nærmer seg (20 minutter før) og han er ute av bygget, og lykt-regelen den har fått når han slår på lykta innen 40 m fra et vesen med samme svar på lykta («Lamp off. Keep still. It looks past you.», «Standing still won't do. Get indoors.», «Lamp on it first. It stops. Then back off.»). | Kunnskapen merkes i spillet, ikke bare på papiret. Samme nedkjøling som de andre tankene. | `FS47Dossier::HabitHourFile`, `HabitLampFile`; `DossierHabitStep` (fra `S47Thoughts::Step`), `DossierHabitLamp` (fra `FS47FarmSite::StepAttention`); `S47Thoughts::ThinkLine` |
| Tegninger | THEM har én liten blyanttegning per trinn mappa har nådd: OUTLINE en grov kropp fra SHAPE-boksen (liten med stort hode, lav på fire, høy og lang; for de to fjerne et lyspunkt og et lyst omriss; en stiplet ring når ingenting er sett), MARKS sporene fra GROUND-boksen, HABITS en urskive med timene skravert. | Spilleren ser mappa vokse. Ingen detaljer og ingen forklaring. | `S47Dossier::SketchOf`, `PrintsOf`; `SS47NbSketch` i `SS47Notebook.cpp` |

Lagring, alt valgfritt: `dossier.names` (etter nummer: `n` navnet, `d` natta), `dossier.merged` (i den rekkefølgen det ble gjort: `into`, `from`, `night`, `was` trinnet den overlevende hadde, og navnet den hadde, så angringen blir eksakt) og `dossier.paid` (numrene som har fått godtgjørelsen). En lagring fra trinn 1 eller 2 har ingen av dem og leses som før. Et navn eller en sammenlegging som ikke holder i lagringen, utelates. Tankenes «én gang per natt» lagres ikke.

Avvik fra listen og planen, og hvorfor:

- **Ordsjekk på spillerens navn.** Oppdraget sa bare lengde og tegn for spillerens tekst. Dokumentet sier «egen tekst, maks 24 tegn, ordsjekk» og prøven «navnet går gjennom ordsjekk». Dokumentet vant: navnet tar bokstaver, mellomrom og `, . - '` (ingen sifre, så verken årstall eller klokke), 1 til 24 tegn, og må bestå `LintWords`. Et navn som ikke tas, gir en nøytral lapp («NOT WRITTEN: THE BINDER KEEPS PLAIN WORKING NAMES. TRY OTHER WORDS.»).
- **FILED krever navn.** Dokumentet sier at spilleren gir mappa et navn og arkiverer den. Derfor teller en mappe hos Ward først når den både står på HABITS og har navn fra spilleren. Forslaget alene teller ikke før det er skrevet.
- **Alle to mapper kan legges sammen, ikke bare parene.** Parene står først og er merket. Ellers kunne spilleren nesten aldri ta feil, fordi et par stort sett gjelder samme vesen.
- **Angring bare til neste 08:00.** «Tas ut igjen» i Wards regel er den samme angringen. Etter 08:00 står sammenleggingen.
- **Trinnet kan synke ved sammenlegging.** Gjennomgangen av trinn 2 sa at et trinn aldri synker. Her er det eneste unntaket: en sammenlagt mappe får trinnet fra det som faktisk stemmer. Det som alt er gitt (kartmerket, peilingen), tas ikke tilbake.
- **$5 er med** etter dokumentets «En arkivert mappe gir $5 én gang», gjennom `EarnSide` under ukas tak.
- **Tankene sier ikke mappas navn.** En tanke er høyst 48 tegn, og navnet kunne bli for langt.

Prøver: linjene `EVIDENCE-DOSSIER: slice 3` i `S47Dossier::SelfTest` (navnets regler, forslagene, navn og tilbakefall over lagring, sammenlegging og angring over lagring, feil sammenlegging med strøket blyant og trinn, Wards poeng for 0, 1, 2 og 3 og for feil sammenlegging, tankene én gang per natt og bare på HABITS, tegningenes byggemåte, lagringer fra trinn 1 og 2), `DossierSelfTest` (navn og sammenlegging bare ved permen, godtgjørelsen én gang, permens linjer, gjennom en Continue), `RECORDS-BINDER: slice 3`, `Freeform THEM (slice 3)` og Wards poeng i forgjengerens selvtest. F10, «Evidence dossier (M3)»: navngi alle med forslaget, legg sammen første par eller ta det ut igjen, hent en vane fram nå; «what is in the files now» viser navn, FILED, sammenlegginger og Wards tall.

## 1. Mål og løkke

**Mål.** Det spilleren finner om natta, skal samle seg til egne mapper for hvert ukjent vesen, slik at han over 30 dager selv ser at det er flere slag og hvordan de skiller seg fra hverandre. Spillet gir aldri et navn, en opprinnelse eller en forklaring. Spilleren setter sammen bildet, gir hver mappe sitt eget navn, og kunnskapen gjør nettene tryggere og teller hos Ward.

**Løkka i fem steg:**

1. **Merk:** noe skjer ute (lyd på walkie, spor, lys, dyr som blir stille, et glimt).
2. **Sikre:** ta bilde, ta opp på bånd, les geigeren, se på bakken, se hva den gjør når lykta treffer.
3. **Logg på stedet:** et kort stempel (toast) og en linje i notatboka med det samme, i riktig mappe hvis spillet vet hvilken, ellers som «ukjent tilhørighet».
4. **Sammenlign:** i notatboka (TAB) eller i permen på SARO viser mappene hva som passer og hva som er utelukket. Spilleren legger løse funn i en mappe eller åpner en ny.
5. **Konkluder:** når en mappe har nok ulike bevis, gir spilleren den et navn og arkiverer den. Det gir en linje hos Ward, litt betaling under ukas tak og en vane som hjelper resten av måneden.

## 2. De ukjente og hva som skiller dem

### Kinds i koden (arbeidsnavn, vises aldri i spillet)

| Arbeidsnavn | I koden | Kan møtes | Mappe |
| --- | --- | --- | --- |
| De små | `ES47AlienKind::Roswell`, besøkskode 0, `ES47Taker::Roswell` | Stedsbesøk etter mørkets frembrudd, dørbesøk, innendørs titting; jakter etter 38 s (`S47AlienKinds.cpp:175`) | Ja |
| Den lave | `Pale`, besøkskode 1 (krabbekroppen), og veiglimtets «crawler» | Stedsbesøk når som helst etter mørket, mest på åpen mark (`S47FarmSite.cpp:185`); bryter stillhetsregelen fra natt 4 etter 02 (`S47FarmSite.cpp:716-737`) | Ja |
| Den høye | `Tall`, besøkskode 2, og veiglimtets «tallfigure» | Bare fra 03:00 (`S47AlienKinds.cpp:200`, `S47FarmSite.cpp:185`) | Ja |
| Sølvmannen | `Nordic`, bare figur (`VisitCode` −1, `S47AlienKinds.cpp:192`) | Høydedrag, til fots, i lyskjeglen, over veien; figur nær fra uke 2 | Ja, men stopper ved TREKK |
| Den som lyser | `Cosmic`, bare figur, egen glød (`S47Happenings.cpp:1687`) | Som sølvmannen, sjeldnere (vekt 0,08) | Ja, men stopper ved TREKK |
| Skyggen | `""`/`"shadow"`, nesten svart (`S47Happenings.cpp:1582`) | Standard figur på høy- og lav-netter | **Nei.** Forblir uløst under SHADOWS |

Mannen med hatten ved pumpa er humor og holdes utenfor.

### Matrise: hvert vesen × seks bevistyper

**F** = finnes og skiller allerede i koden. **U** = liten utvidelse (samme system, ny variant per vesen). **–** = gir ikke noe (det er også et spor).

| | BILDE (form) | BÅND (lyd) | BAKKE (spor) | LYKT (reaksjon) | GEIGER | DYR |
| --- | --- | --- | --- | --- | --- | --- |
| De små | Liten, stort hode, ofte flere (1–6, `S47RoswellGroup.cpp:142`) **F** | Tonestiger som stiger og ender i et spørsmål, pause etter (`S47AlienKinds.cpp`, `SynthRoswell`) **U** på feltbåndet | Små bare fotspor, steiner snudd med fuktig side opp (bibelen, stedet) **U** | Trekker seg unna lykta; lykt av og stå stille, så ser den forbi deg (`S47FarmSite.cpp:698-705`) **F** | Svak økning **U** | Hundene tier (`dogs_hush`) **U** |
| Den lave | Lav, foldet, blek, rask (`S47FarmSite.cpp:586`, `:741`) **F** | Dobbelt puls som et hjerte, raspende pust (`SynthPale`) **U** | Avtrykk i firer, tett, utfor kanten i løp (`S47RoadGlimpse.cpp:116-121`) **F** på vei, **U** på stedet | Bryr seg ikke om lykta; fra natt 4 etter 02 stopper den ved deg selv om du står stille (`S47FarmSite.cpp:716-737`) **F** | Ingenting **U** | Kuer og coyoter flykter (`animals_flee`) **U** |
| Den høye | Høy, lang, langt unna, sitter iblant (`S47FarmSite.cpp:547-553`) **F** | Dype langsomme pulser, to toner som slår mot hverandre, metall som knirker (`SynthTall`) **U** | Lange smale avtrykk, et langt steg mellom (`S47RoadGlimpse.cpp:116-121`) **F** på vei, **U** på stedet | Fryser i den første lyskjeglen; «shh» i lyset innen 12 m (`S47FarmSite.cpp:556-567`) **F** (ny finding **U**) | Klar økning **U** | Fuglene letter (`S47AnimalWarn` Birds) **U** |
| Sølvmannen | Menneskelik, for stille, på høyden (`S47Marginalia.cpp` «THE SILVER MAN») **F** | Rene akkorder som sakte går ut av stemning (`SynthNordic`) **F** på mottakeren | – | Borte når den blir sett **F** | – | – |
| Den som lyser | Lyser innenfra, «stjerner inni» **F** | Pip i grupper som teller, så et svar (`SynthCosmic`) **F** på mottakeren | – | Synes i eget lys (`S47Happenings.cpp:4574`) **F** | – | – |

Kontroll: ingen to rader er like. Sølvmannen og den som lyser deler bare tomme felt og skilles på bilde og bånd. De tre som kommer nær, skilles på hver kolonne.

**Ingen ny mekanikk:** alle kolonnene er systemer spilleren allerede bruker (kamera og mørkerom, kassettspiller, dagslyslesing av stedet, lykta, geigeren, dyrevarslene). «U» betyr at systemet får en variant per vesen der det i dag er likt for alle.

Fysisk prøve er bevisst utelatt fra matrisen. Fragmentet og jordprøven har fast svar «origin remains unassigned» (`S47FreeformWorld.cpp:1094-1096`, `S47Happenings.cpp:5321-5355`), og et mikroskop skal ikke bli en artsidentifikator (bibelen). Tapt tid er også utelatt, fordi Freeform ikke skal forklare hvordan noen blir borte.

## 3. Logging uten tur tilbake til en datamaskin

### Der beviset tas

- **Stempel:** kort toast med bare mappe og bevistype, for eksempel `UNKNOWN 2 — GROUND`. Aldri en egenskap eller et navn.
- **Notatboka (TAB) med en gang:** KNOWLEDGE › Who & what › THEM får mappene, side om side med blyantnotatene som finnes (`S47Marginalia`). Mappa er alltid med i lomma. Hele poenget er at spilleren aldri må til SARO for å sammenligne.
- **Funn uten kjent tilhørighet** (spor neste morgen, bånd tatt langt unna, viltkamera) havner under `UNPLACED` og kan legges i en mappe der og da.

### I arkivet (permen på SARO)

`S47RecordsBinder` får skilleark **UNKNOWNS** etter FIELD + BENCH. Der ligger hver mappe som en ark-dossier med:

- spillerens egne bilder (albumutskrifter knyttet til mappa, forstørrelser foran),
- en blyanttegning som spillet tegner selv i samme stil som dokumenttegningene (`S47Drawings`). Spilleren kan ikke tegne i dag, så tegningen fylles ut med det han faktisk har sett,
- målinger og båndlinjer med dato og sted.

Permen er stedet for å **arkivere** en mappe (gi navn, sende til Ward). Å lese og sammenligne kan gjøres overalt.

### Mappa fylles i tre trinn

| Trinn | Krav | Hva tegningen og teksten viser |
| --- | --- | --- |
| OUTLINE | 1 bevistype | Bare en form eller en lyd: et omriss langt unna |
| MARKS | 3 bevistyper fra minst 2 netter | Høyde, gange, spor og lyd |
| HABITS | 4 bevistyper der LYKT er én av dem, fra minst 3 netter | Én praktisk vane i blyant: hva den gjør med lys, stillhet og lyd |

### Utelukking vises, spilleren konkluderer

- Hver mappe har seks bokser i en rad (BILDE … DYR): fylt, tom eller satt til «–» når spilleren har sett at den ikke gir noe.
- Når et løst funn åpnes, får hver mappe et blyantmerke: **passer** (✓), **strider mot det du har** (strek), **ukjent** (?). Merkene sammenligner bare med det spilleren allerede har funnet. Databasen er hans egen og ikke en ferdig liste.
- Spillet godkjenner ikke tilordningen med en gang. Et feil plassert funn blir strøket når et senere funn motsier det, slik `StruckBy` virker for blyantnotatene. Bekreftelsen kommer samlet når mappa når neste trinn (lærdom fra Obra Dinns tre om gangen: samlet tilbakemelding gjør gjetting dyrt).
- Konklusjon: når mappa er på HABITS, kan spilleren skrive et navn (egen tekst, maks 24 tegn, ordsjekk) og arkivere den (`FILE AS ONE`). Spillet sier aldri om navnet er «riktig». Det sjekker bare at bevisene i mappa ikke strider mot hverandre.

### Følger (merkes og leses av andre systemer)

- **Ward:** ukenotatet nevner antall atskilte mapper med nok bevis («Keep them apart. Three separate things, by your own notes.»). I 30-dagersvurderingen gir arkiverte mapper poeng (se spørsmål 5).
- **Penger:** første gang hver bevistype kommer i en mappe, betales den som feltregistrering under ukas felles tak på $35 (ingen ny pengestrøm). En arkivert mappe gir $5 én gang.
- **Tryggere netter:** når en mappe er på HABITS, kommer vanen som tanke og som linje i notatboka når dette vesenets tegn høres eller ses igjen:
  - de små: «Lamp off. Keep still. It looks past you.» (regelen finnes, `S47FarmSite.cpp:698-705`)
  - den lave: «Still won't do with this one. Get indoors, or get up high.» (tar innen 1,9 m og følger over tak bare innen 14 m, `S47FarmSite.cpp:653`, `:749`)
  - den høye: «Put the lamp on it first. It stops. Then back away.» (fryser i første lys, `S47FarmSite.cpp:556-561`)
- Mappene sier aldri hva de er, hvor de kommer fra eller hva de gjør med folk.

### Ord i spillet

Spilltekst må bestå ordsjekkene som finnes (`S47Thoughts.cpp:169`, `S47FreeformCases.cpp:910`, `S47Marginalia::NamesKind`). Det betyr ikke «alien», «ufo», «saucer», «grey», «silhouette», «taken», «third», 47, 1947, 8.15 eller «cable». **Mappene nummereres derfor 1–5 og ikke med bokstaver**, fordi en frittstående stor C er forbudt. Nummeret deles ut i den rekkefølgen spilleren først logger dem, så nummeret sier ikke hvem det er.

## 4. Tempo over 30 dager

| Uke | Tidsnivåer og sperrer i koden | Hva mappene typisk når |
| --- | --- | --- |
| 1 (natt 1–7) | 18–24 hint, 00–02 glimt, fulle møter etter 02; stor natt 2 eller 3; de små og den lave på stedsbesøk; den høye fra 03 | 1–2 mapper på OUTLINE, ofte bare bånd eller bilde langt unna. Ingen mappe kan nå HABITS (3 netter) |
| 2 (natt 8–14) | Figur nær fra uke 2 (`S47Happenings.h:35`); den lave bryter regelen fra natt 4; veiglimt én gang i uka | De små på MARKS og kanskje HABITS; den lave på MARKS; den høye eller sølvmannen på OUTLINE |
| 3 (natt 15–21) | Dyre oppgraderinger fra dag 15 (radiopeiler); fylkesinspektøren skriver dag 22 | Første arkivering mulig; den høye på MARKS; sølvmannen eller den som lyser på OUTLINE |
| 4 (natt 22–30) | Grensene kryper en time (glimt fra 23, fulle møter fra 01) | De tre som kommer nær, kan nå HABITS; de to fjerne stopper på MARKS. Vurderingen ved dag 30 |

Kobling til milepæler:

- **M3 Etterforskning:** skive 1 og 2 (merker på funn, mapper i notatboka og permen, båndet og forstørrelsen som bevis).
- **M4 Folk og steder:** rykter fra Caliche (servitøren, Baca, innringere på 1420) kan legges som «hearsay» i UNPLACED. De teller aldri som bevistype, men peker mot et sted.
- **M7 Freeform 1.0:** balanse av trinnkravene mot 30 dager med `-S47NaturalStand=tour`, Wards poeng og vanene som tryggere netter.

## 5. Gjenbruk og nye skiver

### Hva som gjenbrukes

| Del | Hvor |
| --- | --- |
| Vesenenes temperament, lyd og hvilken som dominerer natta | `Public/S47AlienKinds.h:20-49`, `Private/S47AlienKinds.cpp:175-215` |
| Hvem som kommer på stedet, og atferden per vesen | `Private/S47FarmSite.cpp:183-187`, `:543-567`, `:698-749` (`VisitWho`, `S47FarmSite.h:169`) |
| Fotoregistrering per gruppe | `Private/S47FreeformDirector.cpp:1756-1773`, `:1924-1929` (`strange.<gruppe>.n<natt>`) |
| Fotspor på veiskulderen per figur | `Private/S47RoadGlimpse.cpp:116-121`, `S47Happenings.cpp:4586-4593` |
| Funnlageret | `RecordEvidence`, `Private/S47Freeform.cpp:1651-1686`; lagret som `findings`/`findingDays` (`:4930`) |
| Blyantnotater om dem, med StruckBy og ordsjekk | `Public/S47Marginalia.h:9-60`, `Private/S47Marginalia.cpp:69-96`, `:165` |
| Kassettspilleren og det rare på båndet | `Public/S47Tape.h:27-34`, `FS47Tape::OddSound` (`Private/S47Tape.cpp:53-60`) |
| Geigeren | `Private/S47Happenings.cpp:5321-5355` |
| Dyrevarsler | `Public/S47AnimalWarn.h:28` (`EKind{Birds,Coyote}`) |
| Permen | `Public/S47RecordsBinder.h:10-45`, `Private/SS47Binder.h`, `Private/S47FreeformBinder.cpp:112-128` |
| Notatboka | `Private/SS47Notebook.cpp:591`, `:900` |
| Tegnestil | `Public/S47Drawings.h:8-24` |
| Wards vurdering | `Private/S47Predecessor.cpp:115-131` |

**Avvik som må rettes:** OPS-kvitteringen skriver i dag «Nordic figure», «Cosmic figure» og «grey/pale/tall visitor» (`S47FreeformDirector.cpp:1767-1773`, `:2032`). Det gir spilleren navnet og bryter `NamesKind`. Titlene må bli nøytrale beskrivelser («a figure on the rise», «something low»).

### Skive 1: merker på funnene som finnes, og mappe i notatbok og perm (minste som merkes)

- **Hva:**
  - Ved `RecordEvidence` og fotoregistrering legges et merke `{vesen, bevistype, dag}` på funn der vesenet allerede er kjent i øyeblikket:
    - fotoregistrering per gruppe (`roswell`, `pale`, `tall`, `nordic`, `cosmic`, `crawler`→den lave, `tallfigure`→den høye),
    - veispor (`tracks:low`, den høye),
    - `rule-still-held` (de små, LYKT) og `rule-pale-broken` (den lave, LYKT),
    - stedsbesøkets feltside (`VisitWho` når siden skrives, `S47FarmSite.cpp:860`).
  - Nummer deles ut ved første merke.
  - Toast `UNKNOWN n — TYPE`.
  - THEM-siden i notatboka viser mappene med seks bokser og trinn.
  - Permen får skillearket UNKNOWNS (lesing, ingen arkivering ennå).
  - Wards ukenotat får én linje om antall atskilte mapper.
  - OPS-titlene gjøres nøytrale.
- **Filer:** `S47Freeform.h/.cpp` (nytt `FS47Dossier` i økta), ny ren `S47Dossier.h/.cpp` (merker, nummer, trinn, selvtest), `S47FreeformDirector.cpp` (merke og titler), `S47FarmSite.cpp` (merke ved feltsiden og reglene), `S47RoadGlimpse.cpp` (merke på spor), `SS47Notebook.cpp` (THEM), `S47RecordsBinder.h/.cpp` og `SS47Binder.cpp` (skilleark), `S47PredecessorText.cpp` (ukelinje).
- **Lagring:** `dossier: { order: ["pale","roswell"], tags: { "<findingKey>": { "k": "pale", "t": "ground", "d": 9 } } }`. Interne id-er vises aldri. Eldre lagringer uten feltet laster tomt, og merker kan bygges opp igjen fra eksisterende funn der nøkkelen avslører gruppen (`strange.*`, `rule-*`).
- **Prøver (selvtest, ingen motormatrise):**
  - rekkefølgen er stabil,
  - ingen streng i mappene feiler `NamesKind` eller `S47Script::LintText`,
  - ingen to vesener har lik signatur i tabellen,
  - lagring tur-retur, og en gammel lagring gir tomt resultat,
  - trinn regnes riktig (OUTLINE/MARKS/HABITS med nattkrav).
  - Én rask sjekk i spill: F10-hendelse med stedsbesøk og bilde gir toast og en linje i THEM.
- **Størrelse:** omtrent 500–700 linjer C++, ett bygg og én rask sjekk. Tom merker det med en gang: stempelet i felt og mappa som vokser i TAB.

### Skive 2: egne signaturer der alt er likt i dag, og løse funn

- **Hva:**
  - Feltbåndet (`OddSound` for kropp/besøk) spiller vesenets egen signatur fra `S47AlienKinds::Render`, lagret som `OddKind` i `FTake`.
  - Dagslysteksten på stedet får en variant per vesen (`S47FarmSite.cpp:874-887`).
  - Geigeren på et besøkssted får en faktor per vesen.
  - Dyrevarselet velges etter vesen (Birds for den høye, Coyote/kuer for den lave, hundene tier for de små).
  - Den høye som fryser, blir eget funn `rule-tall-froze`.
  - UNPLACED med ✓/strek/? og tilordning, og StruckBy-stryk ved motsigelse.
- **Filer:** `S47Tape.h/.cpp`, `S47FarmSite.cpp`, `S47Happenings.cpp` (geiger, dyr), `S47AnimalWarn.h/.cpp`, `S47Dossier.cpp`, `SS47Notebook.cpp`.
- **Lagring:** `tape[].oddKind`; `dossier.tags[].assigned` (spillerens valg) mot `sure` (kjent ved opptak); `dossier.struck[]`.
- **Prøver:**
  - signaturene skilles (eksisterende `S47AlienKinds::SelfTest` utvides til båndnivå),
  - én tilordning blir strøket av en motsigelse,
  - lyd sendes til Tom som WAV (regelen om lydprøver).
- **Størrelse:** omtrent 700–900 linjer, ett bygg og en lytteprøve.

### Skive 3: arkivering, vaner og tempo

- **Hva:**
  - Navn og `FILE AS ONE` i permen.
  - Ward-poeng og $5.
  - Vanene som tanke og notatlinje når tegnene kommer igjen (lest av `S47FieldRadio`/walkie og stedets varsler).
  - Blyanttegning per trinn (`S47Drawings`-stil, tre trinn × fem mapper).
  - Balanse over 30 dager.
- **Filer:** `S47Dossier.cpp`, `S47FreeformBinder.cpp`, `S47Predecessor.cpp` (`FTally` + `Points`), `S47Thoughts.cpp`, `S47FarmSite.cpp`, `S47Drawings2.cpp`.
- **Lagring:** `dossier.filed: { "<kind>": { "name": "...", "day": 19 } }`.
- **Prøver:**
  - navnet går gjennom ordsjekk,
  - Wards poeng endrer ikke terskelen på 6 for en spiller uten mapper,
  - vanen kommer bare etter HABITS,
  - flernatt med `-S47NaturalStand=tour`,
  - kritikerrunde før installasjon.
- **Størrelse:** omtrent 800–1 000 linjer og tegninger, to bygg og en flernattsprøve.

## 6. Risiko, hva som må unngås, og åpne spørsmål

### De tre klagene på UFOPHILIA

- **Bratt læringskurve og uklar opplæring:** første bevis kommer av seg selv fra noe spilleren uansett gjør (bilde, bånd). Den første vanen (de små ser forbi deg i mørket) finnes allerede som lærdom. Ingen manual. Guide-fanen får høyst én prosedyrelinje om hvordan mapper føres, aldri hva de inneholder.
- **Tilbake til laptopen:** mappene og sammenligningen ligger i notatboka. Permen trengs bare for å arkivere.
- **Kjedelig fram og tilbake:** ingen sjekk krever reise. Tegnet logges der det skjer.

### Andre farer

- **Skrekk blir sjekkliste:** seks bokser er nok. Ingen prosent og ingen «2/6 found». Boksene fylles med blyant, ikke haker. Mange netter gir ingenting nytt. Skyggen forblir uløst. Det fjerne paret når aldri helt fram.
- **Fasit i teksten:** spillet setter aldri navn, opprinnelse eller hensikt. Tittelen på OPS-kvitteringen rettes i skive 1.
- **Bryte Freeforms regler:** ingen forklaring på hvordan noen ble borte, tapt tid er ikke bevis, ingen forbudte ord, og tidsnivåene styrer fortsatt når noe kan ses (mappa kan aldri vise mer enn tidsnivået viste, som `TierOf` i blyantnotatene).
- **Belønning for fare:** penger bare under det eksisterende taket, så det ikke lønner seg å oppsøke bortføring.
- **For mange vesener:** vi lager ingen nye. Crawler og tallfigure slås sammen med den lave og den høye.

### Spørsmålene til Tom (besvart 13. november, se «Besluttet av Tom 13. november»; anbefalt standard i parentes)

1. **Er veiglimtets «crawler» den lave og «tallfigure» den høye, altså samme mappe?** (Ja. Det gir færre vesener og bedre spor på veien.)
2. **Skal sølvmannen og den som lyser ha egne mapper som stopper på MARKS uten arkivering?** (Ja. De kommer aldri nær av seg selv, så de forblir rare.)
3. **Skal skyggen aldri få en mappe?** (Ja, den blir under SHADOWS. Én ting skal være uløst.)
4. **Skal navnet være spillerens egen tekst eller et valg blant blyantord spillet foreslår?** (Egen tekst med ordsjekk, og et forslag fylt inn fra trekkene.)
5. **Hvor mye skal arkiverte mapper telle hos Ward?** (+1 for to arkiverte og +2 for alle tre som kommer nær. Terskelen på 6 står, så rutine alene fortsatt ikke holder.)

## Kilder

- UFOPHILIA-anmeldelser:
  - The Xbox Hub, laptopen bare i campingvogna, «the constant back-and-forth is annoying»: https://www.thexboxhub.com/ufophilia-review/
  - Use a Potion, «bare-bones tutorial», «a tense investigation into a tedious commute»: https://www.useapotion.com/2026/02/ufophilia-impressions-close-encounters-of-the-clunky-kind/
  - Playfront, løkka i fire faser og analyse-PC-en: https://playfront.de/en/?p=1154234
- Utelukking og lesbarhet:
  - NME om Phasmophobias journal, der fra-liste med alle kandidater erstattet avkrysning: https://www.nme.com/news/phasmophobia-devs-share-improvements-to-the-journal-system-3158844 (lest gjennom søkesammendrag, ikke åpnet)
  - Film Stories om Obra Dinns tre om gangen, der samlet bekreftelse demper gjetting: https://filmstories.co.uk/?p=83249
