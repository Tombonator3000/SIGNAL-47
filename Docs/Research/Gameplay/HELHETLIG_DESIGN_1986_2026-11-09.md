# Helhetlig design: én løkke, ett designspråk, og 1986 som mekanikk

**Status:** Avvist av Tom 9. november 2026 (nye mekanikker ødelegger mer enn de hjelper); erstattet av [SAMMENFLETTING](SAMMENFLETTING_2026-11-09.md). **Gjelder:** Freeform (jobb, etterforskning, skrekk, økonomi, ukesaker). **Sist kontrollert:** 9. november 2026.

Tom ba 9. november om en syntese som fletter alt sammen til ett bilde og ett designspråk, og som bruker 1986 mer enn i dag. Dokumentet bygger på [gjennomgangen av d38](GJENNOMGANG_D38_2026-11-09.md), [sammenlignbare spill](SAMMENLIGNBARE_SPILL_2026-11-09.md), [helhetlig løkke](HELHETLIG_LOKKE_2026-11-09.md), [ukesaker](UKESAKER_2026-11-09.md), [saker og OPS](SAKER_OPS_2026-11-09.md), [økonomi](OKONOMI_LOKKE_2026-11-09.md), [notatboka](NOTATBOK_2026-11-09.md), spillbibelen og [DECISIONS](../../Active/DECISIONS.md). Vedtatte føringer er holdt. Koden er lest, ikke kjørt. Toms egne testnotater fra d38 er ikke kommet ennå og må flettes inn.

Arbeidsantakelse som Tom må bekrefte: **å beholde stasjonen krever natta ute.**

## 1. Visjon og designspråk

**Visjon:** Du er alene på nattskift i 1986, og alt du vet, står på papir, bånd og film som du selv har hentet, stemplet og båret hjem.

1986 er ikke kulisser. 1986 betyr tre ting som spillet kan bygge mekanikk på:

- **Ingenting kommer med en gang.** Film må fremkalles, telex kommer over natta, brev tar dager, avisa kommer torsdag.
- **Ingenting vet hvor du er.** Ingen GPS, ingen mobil. Du har kompass, tripteller, papirkart og din egen klokke.
- **Du kan bare nå én kanal der kanalen står.** Telefonen henger på veggen, CB-en sitter i bilen, walkien har rekkevidde.

### Sju pilarer

Hver pilar har en regel og en prøve. Ny kode og nytt innhold skal bestå prøven.

| # | Pilar | Regel | Ja | Nei |
|---|---|---|---|---|
| 1 | **Jobben er instrumentet** | Alt uforklarlig merkes først som et avvik i rutinen. | Nålen faller ikke til ro på kort 2. | En alien dukker opp uten at noe på stasjonen merket det. |
| 2 | **Svaret kommer etterpå** | Det viktigste du tok med hjem, ser du først senere. | Bildet fra skjulet kommer opp i fremkallerbadet neste dag. | Et varsel sier «bevis sikret» idet du trykker. |
| 3 | **Tid og sted fører du selv** | Bevis er et klokkeslett og et sted du selv har skrevet ned. | Du setter minuttet ditt mot strimmelen. | Kartet viser en markør der hendelsen er. |
| 4 | **Én kanal, ett sted** | Hjelp og informasjon finnes bare der apparatet står. | Dale når deg ikke når du er utenfor walkiens rekkevidde. | Dale snakker i hodet ditt overalt. |
| 5 | **Vanlig forklaring først** | Verden har alltid en troverdig jordisk årsak, og noen ganger er den riktig. | Lyset var en konvoi fra basen, og loggen viser det. | Hvert avvik er en alien. |
| 6 | **Merkes, og får følge** | Spilleren legger merke til det, og noe endrer seg etterpå. | Leteren fant hatten din, og Ledger skriver om den. | En hendelse fyres bak en vegg og logges bare. |
| 7 | **Natta har klokkeslag** | Tidsnivåene høres og ses gjennom sendedøgnet i 1986. | TV-en går til prøvebilde ved midnatt, og Earl går av lufta kl. 04. | Et HUD-merke sier «fase 2». |

Pilar 1 er det som binder jobb og skrekk. Pilar 2–4 er 1986. Pilar 5 er etterforskningen. Pilar 6 er Toms «felt, not fired». Pilar 7 gjør tidsnivåene til noe spilleren lærer seg å frykte.

### Felles uttrykk

- **Alt spilleren vet, er en ting.** Et ark, en strimmel, et negativ, en kassett, en lapp. Det har stempel, klokkeslett og sted, og det kan ligge i en skuff, henge på brettet eller ligge igjen i bilen.
- **Bekreftelse er et stempel.** En sak avgjøres når et ark skrives ut og stemples (`RULED OUT / D04`), ikke når en linje skifter farge.
- **Ord fra 1986.** OPS skriver som et terminalsystem, Ward som et fylkeskontor, Dale som en mann som roper i telefonen, og notatboka som blyant.
- **Tre tempo.** Minutter (hendene), natta (planen) og uka (kravet fra Ward).

## 2. 1986 som mekanikk

Tabellen skiller det som finnes fra det som er nytt. «Finnes» betyr at modulen ligger i koden. «Delvis» betyr at tingen finnes, men ikke gjør det som står her.

| 1986-ting | Mekanikk | Binder | Status |
|---|---|---|---|
| **Armbåndsur og tidssignal** | Stasjonsklokka er fasit. Uret ditt driver litt. Du stiller det ved racket før du drar ut. «Samme minutt» teller bare når uret var stilt samme kveld. Etter tapt tid er uret det første som viser det. | Jobb ↔ bevis ↔ bortføring | Nytt (S–M). Tapt tid finnes. |
| **Papirkart, kompass og tripteller** | Peilelappen gir en retning fra SARO. Radiopeileren i bilen gir en retning til. Du tegner begge på kartet med blyant, og krysset er stedet. Avstand leses på triptelleren («9,0 miles forbi ferista»). | Jobb → felt | Delvis: peilelapp, kart, RDF og tripteller finnes, kryssingen gjør du ikke selv. |
| **Film som må fremkalles** | En rull har et fast antall bilder. Du ser ingenting før fotolaben neste dag. Bildet kan vise noe du ikke så. | Felt → SARO neste dag | Finnes (kamera, lab, album). Rullens grense og «noe du ikke så» må sjekkes og bygges. |
| **Kassett og spolebånd** | Én side varer en fast tid. Å ta opp over noe sletter det. Båndet kan ha noe på seg som bare høres på båndpulten. | Felt → SARO | Finnes (`S47Tape`, `S47ReelTape`, $1 per opptak). Sidelengde er nytt. |
| **Telex** | Wards bestilling kommer om morgenen. Nattarket går ut kl. 06. Svaret kommer morgenen etter. Du river av papiret. | Uke → natt | Finnes. Ukebestillingen er ny. |
| **Fasttelefon og telefonkiosk** | Vitner nås bare til faste tider og bare fra en telefon. Kiosken i Caliche ringer noen ganger når du går forbi. | By ↔ sak | Finnes (`S47Phones`, `S47StationPhone`). Ringetider er delvis. |
| **Walkie** | Dale har rekkevidde. Ute på flata er du alene, og det hører du på støyen før han faller ut. | SARO ↔ felt, skrekk | Finnes. Rekkevidde som følelse må sjekkes. |
| **CB i bilen** | Sjåførene er et bevegelig varslingsnett. Et rop har frist: «noe står på veien ved mile 14, nå». | Vei → sted, dødtid | Finnes (`S47CB`). Rop med frist er nytt. |
| **AM 1420 med Earl** | Innringerne gir tips, og ditt eget tips leses opp. Sender fra midnatt til fire. | Felt → by → felt | Finnes (`S47RadioTips`, `S47EarlDialogue`). |
| **AM-støy** | Bilradioen knitrer før øyet ser noe. Tørt lyn gir samme knitring, så varselet kan være falskt. | Instrument før øye | Nytt (S). Værsystemet finnes. |
| **TV med sendeslutt** | Motell- og kontor-TV går til prøvebilde ved midnatt. Etter kl. 02 kan snøen på skjermen oppføre seg rart. | Tidsnivå 1 → 2 | Delvis: TV og prøvebilde finnes, koblingen til tidsnivåene er ny. |
| **Ukeavisa Ledger** | Torsdag står det du meldte, fant eller mistet. Byen svarer på det. Rubrikkannonser er spor. | Uke, konsekvens | Finnes. Rubrikker som spor er nytt. |
| **Post** | Sky Watch-bulletinen kommer i posten. En prøve sendt ut tar tre døgn. | Flere døgn | Finnes. |
| **Åpningstider** | Butikken stenger kl. 18 og holder stengt søndag. Mangler du film i natt, mangler du film. | Dag → natt, planlegging | Delvis (`S47Openings`, `S47TownDay`). |
| **Fylkesstrøm og aggregat** | Brune kutt, sikringer og kanner. Aggregatet tvinger deg ut. | Jobb ↔ skrekk | Finnes, og virker. |
| **Kald krig** | Konvoier, jetfly, smell og raket bakke er alltid en mulig forklaring. Militæret rydder spor før deg hvis du venter. | Vanlig forklaring, spor kjølner | Delvis (militæret i uke 2, THE RAKED GROUND). |
| **Geigerteller fra sivilforsvaret** | Et brukt instrument med tydelig terskel og åpen mening. | Instrument, butikk | Finnes i butikk og verksted. |
| **Sky Watch og UFO-miljøet** | Entusiaster med kikkert står ute om natta. De er både vitner og feilkilder, og de sender trykte spådommer. | Vitner, falske lys | Finnes (varebil, bulletin). |
| **Måne og vær** | Månefasen avgjør hvor mye du ser uten lykt. Støvvind sletter spor. | Felt, valg av natt | Finnes (`S47Weather`). Spor som slettes er nytt. |
| **Kirke, kino, sheriff** | Søndagsgudstjeneste, plakater som byttes og betjentens runde gir byen en uke. | Ukerytme, ukesaker | Finnes. |

**Tre nye 1986-grep som gir mest:**

1. **Uret.** Det gjør «samme minutt» til noe du gjør selv, det gir et ritual før du drar ut, og det gjør tapt tid til et øyeblikk: du kommer tilbake til racket, og uret ditt går 2 timer og 11 minutter feil.
2. **Blyantkrysset på kartet.** Det erstatter den automatiske peilingen. To retninger gir et sted. Én retning gir bare en linje du må kjøre langs.
3. **Sendedøgnet som klokkeslag.** TV-ens sendeslutt ved midnatt, Earl fra 00 til 04 og stillheten etter 04 gjør Toms tidsnivåer til noe som høres.

**Tidsfarge (krever Toms ja, se spørsmål 3):** kometen som hele våren har fått folk ut med kikkert, nyhetene om atomulykken i slutten av april som gjør geigertelleren til et samtaleemne, oljeprisfallet som forklarer hvorfor fylket vil stenge stasjonen, og omleggingen til sommertid som gir én natt der klokkene ikke stemmer. Det brukes bare som nyheter på radio og i avisa, uten virkelige personer. Freeform-kalenderens datoer må sjekkes mot dette.

## 3. Én løkke for natt, uke og måned

### Natta

| Tid | Hva spilleren gjør | Hva som kan skje |
|---|---|---|
| 18–19 | Stiller uret, laster film og kassett, leser telexen og velger utstyr til bilen. | Ingenting. Dette er ritualet. |
| 19–23:30 | Jobbkortene. Ett kort har et avvik som kan være noe. Avviket gir en peilelapp. | Bare hint som kan bortforklares. |
| 23:30–24 | Valget: dra nå mens sporet er varmt og natta trygg, eller ta kort 3. | TV-en nærmer seg sendeslutt. |
| 24–02 | Ute eller inne. Earl er på lufta. | Glimt, aldri nært. Også synlige fra vinduene på SARO. |
| 02–04 | Minst ett arbeidsærend ligger her hver uke: en bestilling fra Ward, aggregatet, en drive som må resettes. | Fulle møter. Høyst ett farlig per natt, og ro etterpå. |
| 04–06 | Earl går av lufta. Hjem, lagre, legge filmen i laben. | Stille. Kl. 06 går nattarket ut på telex. |
| Dag | Fremkalling, by, telefon, butikk, søvn. Sporet fra i natt er kaldt ved middag. | Svaret på i går kommer. |

Regelen som bærer natta: **hver natt har ett spor som kjølner.** Det er aldri null signal, og en tom tur utelukker noe og føres som det.

### Uka

- **Mandag:** Wards notat med **ett konkret krav** for uka. Kravet kan bare oppfylles med noe fra feltet.
- **En kveld i uka:** Wards bestilling, en måling med vindu etter kl. 02.
- **Natt 4–5:** ukesaken.
- **Torsdag:** Ledger skriver om det du har gjort.
- **Søndag:** ukearket. Butikken er stengt, kirka er åpen.

### Måneden

Dag 22 skriver inspektøren hva han vil se. Dag 30 avgjør Ward. **Stasjonen beholdes bare med minst én sak som har et tidsstempel fra feltet, og med kravene fra minst to av de fire ukene oppfylt.** Rutine alene holder stasjonen i drift, men redder den ikke.

### Tre netter sett fra spilleren

**Uke 1, natt 3 (den store natta).**
- 18:40: stiller uret ved racket, tolv bilder igjen på rullen.
- 19:10–21:30: to kort. På kort 2 faller ikke nålen til ro. OPS skriver ut en peilelapp: 026.
- 21:40: tegner linja på kartet. Den går over flata nord for gården. Én linje gir ikke noe sted.
- 22:00–23:30: kjører langs linja. Bilradioen knitrer ved ferista. Finner ingenting, og fører «ingenting ved ferista 22:50». Det utelukker den nære halvdelen.
- 00:00: tilbake. Prøvebilde på TV-en. Earl kommer på.
- 02:14: hele arrayet snur seg. Lysene flimrer. En lyssøyle står ni sekunder, i retning 026. Spilleren tar tre bilder og skriver minuttet fra uret.
- 02:30: kjører ut. Finner glassert grunn og små fotspor. Leser triptelleren.
- 06:00: nattarket går ut. Filmen ligger i laben.
- Dagen etter: Dale roper kl. 08:30. To av bildene viser søylen. Det tredje viser noe ved foten av den som spilleren ikke så. Minuttet på uret stemmer med strimmelen, og sak 8 får sitt første stempel.

**Uke 2, natt 11 (kirka).**
- Mandag ba Ward om «én observasjon med to uavhengige klokker».
- 19–23: kortene. Himmelkildene ligger en halv time tidligere enn i uke 1, og spilleren er ferdig før.
- 23:10: en sjåfør på CB-en: «klokka på kirketårnet i Caliche står, og det lyser der inne.» Fristen er til midnatt.
- 23:40: i Caliche. Tårnuret står på 02:14. Spilleren fotograferer det sammen med sitt eget ur.
- 00:20: glimt i enden av gata, borte når lykta kommer opp.
- 02:10–02:50: Wards bestilling på SARO. Spilleren må velge: bli i kirka eller rekke vinduet. Velger vinduet.
- Følge: bestillingen betaler. Sporet i kirka er kaldt ved middag, men bildet med de to klokkene oppfyller ukekravet.

**Uke 4, natt 24 (antennefeltet).**
- Grensene har krøpet: glimt fra kl. 23, fulle møter fra kl. 01.
- 18:30: inspektøren har bedt om gren C i drift. Den ligger lengst ute.
- 19–22: kortene. Automatloggeren tar kort 2 mens spilleren laster bilen.
- 22:30: radiopeileren og peilelappen krysser hverandre på kartet, midt i antennefeltet.
- 23:20: arbeider på gren C. Walkien faller ut i støy. Nålen på feltmåleren stiger før noe er å se.
- 01:10: noe går mellom antennene. Spilleren har ett bilde igjen og tar det, gjemmer seg i koblingsskapet og venter.
- 01:40: stille. Tilbake til racket. Uret går fire minutter feil, og ingen kan si hvorfor.
- Følge: gren C er oppe (inspektørens krav), saken har tidsstempel fra feltet (Wards krav), og Ledger skriver torsdag om lys over observatoriet.

## 4. Systemene flettet sammen

| System | Mater | Endres fra i dag |
|---|---|---|
| **Jobbkort** | Avvik → peilelapp. Kvalitet → penger. | Himmelkildene driver 3 min 56 s per natt, slik telexen lærer bort. Kortene flytter seg dermed gjennom måneden. Ett kort per kveld har en detalj som kan være noe. Kort som ikke tas, faller ut ved døgnskiftet. Ingen fjerde kilde på kortene. |
| **Wards bestilling** | Arbeid etter kl. 02. Ukekravet. | Ny. Eget ark på telex, ikke et jobbkort, så `Valid` og `Runs()` røres ikke. |
| **Peiling** | Jobb → sted. | Målet følger sakens sannhet, ikke `NextUnknown`. Én retning gir en linje, to gir et kryss. Radiopeileren kommer dag 8. |
| **Steder** | Felt → funn. | `Places` og `EventPlaces` slås sammen til én liste med tilstand per natt. Sporet er varmt, kaldt eller ryddet av militæret. |
| **Saker** | Funn → stempel → Ward. | «Spå, så test»: du velger forklaring og sier hva du venter, før brettet viser hva prøven sier. Mysterier får tre spor der to støtter hverandre. Du setter selv minuttet mot strimmelen. Riktig svar teller mer enn levert svar. |
| **Bevis** | Sak → ukekrav → dag 30. | Bevis har klokkeslett, sted og stilt ur. Tråder på brettet er fortsatt ikke bevis. |
| **Fare** | Natt → kostnad → neste natt. | Uendret tak på ett farlig møte per natt. Nytt: faren kommer også mens du gjør et arbeidsærend, og instrumentet varsler først. Noen varsler er falske. |
| **Notatboka** | Alt → neste steg. | NOW bygges som TONIGHT. CASES har mapper per sak. WHO & WHAT har skriblerier som gir kartmerker. Venter på Toms ja. |
| **Radio, CB, Earl** | By og vei → sted. Dødtid → valg. | CB-rop med frist. Tipset ditt på lufta gir en innringer natta etter. |
| **Dale** | Reaksjon på det du gjorde. | Uendret rolle. Han er årsaken til arbeidsærender etter kl. 02, og han faller ut når du er for langt ute. |
| **Ledger** | Uke → by. | Skriver om utfall, også det du mistet. Rubrikker kan åpne et sted. |
| **Økonomi** | Penger → rekkevidde, tid, viten, trygghet. | Se del 7. |

Den røde tråden: **avvik på kortet → retning → sted → funn med klokkeslett → fremkalling og sammenligning dagen etter → stempel → ukekrav → dag 30.** Hvert ledd er en fysisk ting, og hvert ledd kan svikte på en måte spilleren forstår.

## 5. Ukesakene som serie

Hver uke bruker én 1986-ting som nøkkel, og hver sak lærer bort noe den neste krever.

| Uke | Sted | Nøkkel fra 1986 | Hva som skjer | Hva spilleren lærer | Følge |
|---|---|---|---|---|---|
| 1 | Skjulet på gården | Walkien og bilen | Dale melder interferens. Skjulet banker tilbake, lyser, og noe bryter seg ut og leter. Bilen harker. | Gjemme seg, snike, lese varsel. | Utfall A–E til sak 8, telex, Ledger og Ward. |
| 2 | Kirka i Caliche | Klokker | Tårnuret står på et klokkeslett spilleren kjenner. Orgelet har luft uten at noen spiller. Noe er i galleriet etter kl. 02. | To klokker som bevis. Byen som sted for skrekk. | Presten og betjenten sier forskjellige ting, og Ledger velger side. |
| 3 | Motellet | TV og telefon | Et rom som ingen har leid, har TV-en på etter sendeslutt. Sentralbordet ringer rom 6 fra et rom som er tomt. | At «trygge» steder ikke er det. Film som viser det du ikke så. | Motellrommet er ikke lenger sikker søvn den uka. |
| 4 | Antennefeltet | Uret og kartet | Kryss på kartet midt i eget anlegg. Arbeid på grenen lengst ute mens grensene har krøpet. | Alt: peiling, ur, film, skjul. | Inspektørens krav og Wards avgjørelse. |

Uke 1 trenger patch 4 (leteren og gjemmestedene) og patch 5 (følgene). Utfall C må sjekke hvor spilleren er. Storys mørke minutt 02:20–02:40 unngås i alle fire. Høyst ett eller to kameragrep per sak. Ingenting viser hvordan noen ble tatt.

## 6. Eskalering over 30 netter

| Uke | Nytt for spilleren | Skrekk | Mysterium | Aldri tomt fordi |
|---|---|---|---|---|
| 1 | Kort, peilelapp, ur, film | Hint, så den store natta, så skjulet | «Noe pekte ut et sted» | Hvert kort har en detalj. Glimt ses fra vinduene. |
| 2 | Bestilling etter kl. 02, radiopeiler, bilens oppgraderinger, militæret | Første møter under arbeid. Falske varsler. | «Noen rydder etter dem» | CB-rop med frist. Militæret tar sporet hvis du venter. |
| 3 | Sene saker, fjerne steder, Reyes fase 2 | Trygge steder svikter | «Det har skjedd før, og folk mangler» | Nye steder åpnes av tank og dekk. Rubrikker i Ledger. |
| 4 | Grensene kryper, inspektøren | Eget anlegg | «Det peker hit» | Kravene fra Ward og inspektøren konkurrerer om samme netter. |

**Mot forutsigbar skrekk:** varselet foran skrekken byttes (nål, knitring, walkie-støy, lys som flimrer, hunder), omtrent hvert fjerde varsel er falskt og har vanlig årsak, og enkelte hendelser kommer uten varsel. **Mot tomme netter:** hver natt har ett spor som kjølner, og en rolig natt ender alltid med noe på film eller bånd som først vises dagen etter.

## 7. Økonomien

Toms rammer står: $12–14 per kort pluss kvalitet, tid som valuta, kostnader som vokser med uka, saldo $0–150, bilen oppgraderes.

**Hva penger kjøper**

| Kjøper | Eksempel | Åpner |
|---|---|---|
| Rekkevidde | Ekstratank, dekk | Fjerne steder som ellers ikke nås og hjem igjen på én natt |
| Tid | Automatlogger som tar ett kort mens du er ute | Natta ute uten å miste lønna |
| Viten | Radiopeiler (dag 8, ikke 15), logger nr. 2, geigerteller | Kryss på kartet, to målesteder |
| Trygghet | Overføringsbryter, gårdsalarm, lyskaster | Varsel og strøm |
| Forbruk | Film, kassetter, batterier | Valg av motiv |

**Rettinger**

- **Å lykkes straffes ikke.** Vedlikehold regnes per antenne som står *nede*. En reparert antenne gjør arket billigere.
- **Kvalitet slår kort nr. 3.** Bonusen for et lesbart opptak med avvik ført gjennom kontrollen skal være større enn grunnlønna på det tredje kortet.
- **Ett navn på pengene.** Det er stasjonens konto (STATION ACCOUNT), og brevet sier «the station needs it», ikke «you».
- **Taket på $35 får en forklaring.** Ukearket har linja «FIELD EXPENSES, county limit $35».
- **Brevet og koden sier det samme** om overført beløp.
- **Tid koster synlig.** OPS viser hva natta ute kostet i kort, og hva kortene kostet i natt.

**Regnemodellen** fra pakke E sjekkes inn i `Tools/econ/` og kjøres med tre spillere: en som kjøper alt, en som ikke kjøper noe, og en som tar ett kort og er ute resten. Alle tre skal ligge på $0–150 ved hver ukegjennomgang.

## 8. Arbeidsplan

Størst endring i opplevelsen først. Hvert trinn er en egen leveranse.

| # | Trinn | Str. | Filer og systemer | Tegn på at det virker |
|---|---|---|---|---|
| 1 | **Ward krever feltet.** Stays krever en sak med tidsstempel fra feltet. Ett konkret mandagskrav per uke. | M | `S47Predecessor.cpp` (`Points`, `Decide`), `S47WardReport`, telex | Selvtest: en spiller som aldri forlater SARO, får ikke beholde stasjonen. |
| 2 | **Peilingen følger sannheten.** Mål fra sakens årsak. Én liste med steder og tilstand per natt. | M | `S47Places.cpp:584-624`, `S47EventPlaces` | Tre frø: lappen peker dit sporet faktisk ligger. Steder brukes ikke opp i uke 3. |
| 3 | **Spå, så test.** Skjul prediksjonen til valget er gjort. Tre spor i mysterier. Riktig teller mer. | M | `S47FreeformCases.cpp:590-597`, OPS-brettet | En spiller som gjetter, får lavere uttelling enn en som leser sporene. |
| 4 | **Arbeid etter kl. 02.** Wards bestilling på telex. Dales ærend. | M | `S47FreeformTelex`, `S47NightTiers`, regissøren | Hver uke har minst én natt der spilleren er på jobb når fulle møter er lov. |
| 5 | **Ukesak 1 ferdig.** Leteren, gjemmestedene, følgene. Utfall C sjekker posisjon. | L | `S47WeekCase.cpp:216`, patch 4–5 | Alle fem utfall kan nås, og hvert av dem står i Ledger eller på telex. |
| 6 | **Uret.** Stilles ved racket. Driver. Bevis krever stilt ur. Viser tapt tid. | S–M | Ny liten modul, `S47LostNights`, saksregelen for samme minutt | Spilleren ser avviket på uret etter en bortføring uten at noe forteller det. |
| 7 | **Sendedøgnet og varslene.** TV til prøvebilde ved midnatt, Earls sendeslutt, AM-knitring, falske varsler, glimt synlige fra vinduene. | S–M | `S47TvProgramme`, `S47TripRadio`, `S47Weather`, `S47NightTiers` | Tom kan si hvilket tidsnivå det er uten å se på klokka. |
| 8 | **Kortene lever.** Siderisk drift. Én detalj per kveld. Kort som ikke tas, faller ut. | M | `S47FreeformJobs.cpp:16-21` | To netter med en uke mellom har ulike klokkeslett og ulik detalj. |
| 9 | **Økonomien.** Rettingene i del 7. Radiopeiler dag 8. Automatlogger. Regnemodell i repoet. | M | `S47Outfit`, `S47Openings`, ukearket, `Tools/econ/econ.py` | Tre modellspillere ligger på $0–150. |
| 10 | **Blyantkrysset.** Spilleren tegner retninger på kartet. | M | Kartet, `S47Places`, radiopeileren | To retninger gir et sted uten markør fra spillet. |
| 11 | **Ukesak 2–4.** Kirka, motellet, antennefeltet. | L×3 | `S47WeekCase` som mal | Én forfattet topp per uke. |
| 12 | **Notatboka** etter Toms ja, og en headless «typisk spiller»-uke som måler dødtid. | M | `SS47Notebook.cpp`, `S47TonightNote` | NOW og TONIGHT viser de samme linjene. Dødtid per natt er et tall. |

Trinn 1–4 kan gå som fire parallelle patcher uten overlapp i filer. Trinn 5 er størst og bør starte samtidig. Trinn 6–7 er små og gir mest 1986 per arbeidstime.

## 9. Spørsmål bare Tom kan avgjøre

1. **Skal det å beholde stasjonen kreve natta ute?** Hele planen hviler på ja.
2. **Uret:** vil du ha et ur som må stilles og som kan gå feil, eller blir det pirk?
3. **Virkelige hendelser fra våren 1986** (kometen, atomulykken, oljeprisfallet, sommertid) som nyheter på radio og i avisa, uten virkelige personer: ja eller nei?
4. **Begrenset film:** hard grense per rull, slik at du må velge motiv, eller bare en pris per bilde som i dag?
5. **Blyantkrysset:** skal spilleren tegne retningene selv, eller skal kartet gjøre det når to retninger finnes?

Åpne fra før: notatbokas nye faner, og om ukesak 1 ligger natt 4–5 eller tidligere.
