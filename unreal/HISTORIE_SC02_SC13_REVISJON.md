# Historien SC02 til SC13: gjennomgang og revidert forløp

Claude, 28. oktober 2026, etter Toms bestilling: gå gjennom SC02 til SC13, sjekk om det er bra og logisk eller kan forbedres, skriv dem på nytt og slett det som er unødvendig. Kildene er [MISSION_FLOW_1800_ROSWELL.md](MISSION_FLOW_1800_ROSWELL.md), [HISTORIE_IMPLEMENTERINGSPLAN.md](HISTORIE_IMPLEMENTERINGSPLAN.md) og forberedelsen fra skyøkten (`SC02_TIL_SC13_FORBEREDELSE.md` på grenen `ccr-20021313-mfplx9`, med 16 åpne motstrider).

Dette dokumentet er designbeslutning og regi. Ingenting her er bygget eller prøvd i spillet. Der dette dokumentet og MISSION_FLOW er uenige, gjelder dette dokumentet for SC02 til SC13. SC-ID-ene er de samme, så eksisterende kode, flagg og arbeid på SC02 kan brukes videre.

## Kort dom

Grunnhistorien er god: ett lys som blir mange, en kollega som forsvinner, Dales private samling, arrayet som følger deg, Nora og en slutt der Reyes selv blir en referanse. Spillerløkken «se, kontroller fysisk, bevar, kom deg unna, forstå» passer spillet.

Det som ikke virker er mengden. Planen har tre ulike fienderegler, fem jaktsekvenser, et tidsbudsjett for gamle bortføringer, mange måletall spilleren skal diktere, to nye talende biroller som bare finnes for én opplysning, og en leveringsrutine der hvert av tolv scenarioer skal ha egen gren, full test, fletting og installasjon. Med så mye blir hver skrekkscene svakere, og arbeidet blir aldri ferdig.

Revisjonen kutter omtrent en tredjedel av produksjonen uten å ta bort noen av de sterke øyeblikkene, og den løser de logiske hullene under.

## Logiske hull og hvordan de løses

1. **Hvorfor går Reyes tilbake til SARO etter at Ortiz blir tatt?** Dagens plan sier selv at dette «må prøves». Løsning: Ortiz forsvinner i lyset mens SARO-mottakeren står på. Sheriffen trenger mottakerens originale logg og stripskriver fra 21:00 til savnetsaken, og bare Reyes har nøkler og vet hvor de ligger. Sheriffen kjører bak Reyes til porten, blir kalt bort til Ortiz' bil, og Reyes går inn alene for å hente bevis i en savnetsak. Det er en grunn spilleren forstår uten forklaring.
2. **T01-arkiveringen og den passive residualen (motstrid 1 og 2).** T01-journalen blir en av originalene Reyes sikrer i SC04. Arkiveringen skjer der, én gang. SC04 slutter i det låste kontrollrommet, og en kort fremoverovergang går til 00:13:47. Residualen kommer gjennom passiv mottaking med den automatiske planen fortsatt sperret. Den gamle Survey-utløseren brukes ikke i nye saker.
3. **«Babysit the survey until 06:00» og Wards «Six o'clock, Reyes» (motstrid 3).** Gjelder bare til SC03. Etter SC03 bytter oppgaveteksten til savnetsaken, og Ward sier at serien er stanset.
4. **Antennene på 026 fra 02:15 (motstrid 4).** Ingen motstrid: arrayprøven i SC08 er ferdig før 02:15, og at antennene går tilbake til 026 er nettopp den eksisterende tilstanden. Gjør tidspunktet til slutten av SC08.
5. **Spolen hos Earl startes av et dialogtema (motstrid 8).** Verdensfaktumet kommer fra at spolen faktisk spilles i lobbyen, ikke fra dialogen.
6. **Arrayvedlegget (motstrid 9).** Behold koden: eget ark i kontorskapet, z -75,3. Dokumentene rettes etter koden.
7. **Noras telefon før printene (motstrid 10).** Telefonen kommer etter at begge feltkopiene er fremkalt.
8. **Noras «drive the road» mot H06_CHECK (motstrid 12).** Ny replikk som rommer begge: «Don't finish his observation. Get the originals to the courier, and don't stop at C.»
9. **Ward i telefonen mens veilederlinjen er stille (motstrid 13).** Stillheten gjelder Dales linje. Ward ringer på den vanlige telefonen.
10. **Sjåførens kjørelogg (motstrid 14).** Sjåførrollen fjernes, se under. Veiopplysningen kommer fra sheriffens daterte patruljeark, en fysisk rekvisitt i dineren.
11. **Kapittel 6 lagrer ingenting, og 10.4 kan ikke nås (motstrid 15).** Legg et sikkert lagringspunkt ved avreise fra Caliche. 10.4 vises bare på kart og skilt som målet, og finalen ved C 8.15 avbryter reisen.
12. **EveningShift og kontoret (motstrid 11).** Bare en dokumentfeil. Kontrakten rettes til å si at `FS47Office` i Main eier kontoret.

## Det som slettes eller slås sammen

| Kuttes | Hvorfor | Erstattes av |
|---|---|---|
| Den svarte figuren som står stille når den sees (SC03, SC06) | Tredje fienderegel ved siden av den bleke aktøren og de gamle glimtene. Kjent fra andre spill, krever egen modus i `FS47Glimpses` og forvirrer reglene spilleren nettopp har lært. Motstrid 6 forsvinner. | Under bortføringen i SC03 står flere bleke skikkelser helt stille i kanten av lyset. Ett blikk forteller at de er mange, med samme skapning som spilleren allerede kjenner. |
| Trusselmøte 3 ved bilen (SC04) | Andre jakt samme time, rett etter annexet. Krever huking med kroppseksponering bak bilvindu, som ellers ikke trengs. Svekker finalen, der bilen er det siste trygge stedet. | Ro etter annexet. Bilen forblir trygg til finalen. |
| Huking som eget system | Trengtes bare for bilmøtet. | Dekning med opake vegger, dører og rutevalg. Kan legges til senere om spilltesting viser behov. |
| Sjåføren i SC12 | Ny talende rolle med egen stemme for én opplysning. | Sheriffens patruljeark og en kort replikk fra sheriffen, som allerede er i dineren fra 04:30. |
| Diktering av måletabeller til Ward (SC12) | Lesing av tall i telefonen er kjedelig og feilutsatt. | Reyes leser tre kilde-ID-er og den første grensetiden. Ward leser dem tilbake. Tabellene ligger i dokumentene for den som vil se. |
| Lån av bærbar opptaker hos Earl (SC08) | Eget system for en bisak. | Earl spiller spolen isolert. Kommer Reyes etter 04:00, ligger en datert kopi i lobbyen. |
| H08 og H09 som to kort | Begge ferdigmeldes i samme scenario på samme handling. | Ett kort, H08 «Other stations / All Night». |
| VHS-filmen som krav for SC07 | 72 sekunder originalfilm er dyr produksjon for et valgfritt spor. | Feltmappen bærer hovedsporet. Filmen leveres som valgfritt tillegg når den er laget, uten å holde igjen SC07. |
| Egen gren, full test, fletting og installasjon per scenario | Tolv fulle leveranser med 110-matrise hver gang. | Fire akter, se under. Hvert scenario testes fortsatt for seg, men flettes og installeres per akt. |

Fire jaktsekvenser blir igjen: trafoen (SC02), annexet (SC04), arrayet (SC08) og SARO (SC11). De er ulike steder med ulike løsninger, og de kommer sjeldnere, så hver av dem betyr mer.

**Én sak trenger Toms ja:** gammel tilfeldig bortføring med 17 til 47 minutters tidstap. Den krever et eget tidsbudsjett i `S47Away.cpp` mot alle nye faste tidspunkter, og den kan komme midt i en fortelling som ellers er stramt timet. Anbefaling: behold den i gamle saker, men slå den av i nye saker med historierevisjon 2 og senere. De andre tilfeldige hendelsene (lyder, telefoner, glimt, dyrene, motell-TV-en, lysene langs B) beholdes uendret. Inntil Tom har svart, gjelder dagens regel om at alle tilfeldige familier beholdes.

## Revidert forløp

| SC | Tid | Kort | Hva skjer | Jakt |
|---|---|---|---|---|
| SC02 Returen til bilen | 19:00 | H01/H02 | T01 ved trafoen, den bleke skapningen, flukt til bilen | Ja, trafoen |
| SC03 Hjelpen som kom | 19:30 til 21:15 | H02 | Meld fra i Caliche, Ortiz kommer og blir tatt | Nei |
| SC04 Sikre og kom i dekning | 22:00 til 23:40 | H02 | Hent loggen til savnetsaken, sperr planen, annexet, kontrollrommet | Ja, annexet |
| SC05 Samtalen som ligger foran | 00:13:47 | H02 | Residualen, telefonen, rommet 47 sekunder senere, S-03 | Nei |
| SC06 Det andre motivet | til 00:55 | H03 | B-12, FRAME01/02, fremkalling | Nei |
| SC07 Det Dale ikke leverte inn | 00:55 til 01:15 | H04 | Arkivets rettelse, Dales kontor og feltmappe | Nei |
| SC08 De peker på deg | 01:15 til 02:15 | H04/H05 | Earls spole, arrayet følger Reyes | Ja, arrayet |
| SC09 Den avbrutte oppstillingen | 02:20 til 03:15 | H05 | STATION01, kabelen, Tomás, feltkopiene | Nei |
| SC10 Rom 6 | 03:20 | H06 | Nora | Nei |
| SC11 SARO blir avsøkt | 03:56 til 04:15 | H07 | Fjernarkene, beleiringen | Ja, SARO |
| SC12 Det dere rekker å sende | 04:20 til 05:20 | H08 | Dineren, kildene, varselet, veien | Nei |
| SC13 Siste vei mot Roswell | 05:20 til 05:29:47 | H10 | Kjøreturen og finalen | Nei |

H09 utgår som eget kort. H-tabellen får ni kort.

## Scenarioene, skrevet på nytt

### SC02: Returen til bilen

**Mål:** `Check the lamp at the junction. Return to the truck.`

Reyes gjør den vanlige T01-kontrollen ved Caliche Junction: journal, indikator, lampeprøve og datert normalverdi. Alt er normalt. På vei tilbake står en blek, for høy kropp mellom servicefronten og SARO 07, og et lys går på skrå gjennom stolpene over den. Den står helt stille i ett sekund, så vender hodet seg mot lyden av Reyes' skritt.

Spilleren lærer reglene her og bare her: den hører, den ser, den går dit den sist hørte eller så deg, og den kan miste deg bak noe som ikke er gjennomsiktig. To veier fører til bilen utenfor gjerdet, vest langs gjerdet eller rundt nord og øst. Er bilen klar og veien fri, er det å kjøre en gyldig flukt.

**Fangst:** kort svart overgang, Reyes står ved skuret med T01 bevart. Ingen tapt tid.

**Følge:** I bilen, med motoren i gang og pusten tung, setter målet seg selv: få folk i Caliche til å høre på deg.

Status: SC02 er skrevet som ukompilert kandidat på `ccr-20021313-mfplx9` og stemmer med dette. Ingen endring kreves i koden.

### SC03: Hjelpen som kom

**Mål:** `Report what you saw. Wait for the relief technician.`

Caliche er lys, folk og telefoner. Sheriffens dispatch tar imot meldingen og leser tid og sted tilbake. Ward ringer tilbake: hun sender Mara Ortiz, regional servicetekniker i hvit servicevogn 23, og ber Reyes vente synlig ved bensinstasjonen. Ett vitne har sett et lys, et annet har ikke sett noe. Mens Reyes venter, kan to lys sees samtidig i forskjellige retninger.

Ortiz kommer rundt 21:00. Hun er vanlig, litt irritert over kjøreturen, viser ID og spør hva Reyes faktisk så. Kort samtale. Så treffer et lys bilen hennes. Hun løftes ut av normal bevegelse, henger et øyeblikk uten støtte og er borte. Termosen hennes ruller ut på asfalten. I kanten av lyset står flere bleke skikkelser helt stille. Én av dem vender hodet mot Reyes.

Spilleren kan ikke redde henne og skal ikke få en oppgave som later som. Riktig handling er å komme seg i dekning, inn på stasjonen eller i bilen, og ringe.

**Etterpå:** Sheriffen kommer. Savnetsaken registreres for seg, adskilt fra den første meldingen. Reyes kan undersøke stedet: bilen, termosen og merkene i grusen. Ward stanser 06:00-serien. Sheriffen spør om mottakeren på SARO stod på klokka 21:00. Det gjorde den.

**Følge:** `Retrieve the receiver log for the missing person report.`

### SC04: Sikre og kom i dekning

**Mål:** `Collect the receiver log and the shift originals. Lock the automatic schedule.`

Sheriffen kjører bak Reyes til porten og blir kalt videre til Ortiz' bil. Reyes går inn alene. Oppgaven er konkret og avgrenset: mottakerens logg og stripskriver fra 21:00, skiftjournalen og T01-journalen til savnetsaken. Sperr den automatiske kjøreplanen fysisk, slik at ingenting kjører av seg selv mens ingen er der. Sperren stanser automatisk drift, ikke passiv mottaking. Kontrollarbeid senere er fortsatt mulig.

**Annexet, rundt 22:15:** Fra feil dør kommer et Dale-fragment spilleren har hørt før, med nøyaktig samme pause. Det er et avtrykk, ikke Dale. En blek hånd følger dørkarmen. Et nært hjørneglimt gir kveldens første ordentlige skvett. Så kommer skapningen inn og leter.

To løsninger: lukk en kjent dør for å vinne tid og ta den lange veien bak opake vegger, eller slå på en vanlig lydkilde fra et trygt sted, se at den går dit, og ta den korte veien. Fangst gir ny start før annexet med originalene bevart.

**Etterpå:** Reyes låser seg inn i kontrollrommet, der veien fra parkeringen ble vist tidligere. Ward er på linjen. Rommet er rolig. Reyes velger å bli i dekning mens sheriffen er opptatt, og å la mottakeren stå på passivt. Kort overgang til 00:13:47.

Kuttet: bilmøtet rundt 23:20 og huking.

### SC05: Samtalen som ligger foran

**Mål:** `Locate the residual and compare the call with the room.`

Residualen kommer i passiv mottaking mens den automatiske planen fortsatt er sperret. Notch-prøve på 1420.110 MHz, gain 55, BW 12. Residualen ligger på 1420.405 MHz, gain 82, BW 12, søkeaz 083. Fire pulser, pause, sju pulser. Reyes sikrer utskriften og tar telefonen. Etter bruddet skjer det som ble sagt i rommet 47 spillsekunder senere. Romhandlingen er kontrollen.

S-03 viser 026 der planen sa 042, uten noen kommando som forklarer det. Det er et spørsmål, ikke et svar.

Ward og Dale later ikke som residualen forklarer bort kroppen eller Ortiz. Neste spørsmål er om fotografier bevarer det samme, og det fører til B-12.

### SC06: Det andre motivet

**Mål:** `Test the extra reference at B-12.`

Uendret i innhold fra planen: B-12s ene elfenbensfargede stripe mot R-07s tre, FRAME01, én av to kontroller (passiv med lampe skjermet og motor isolert, eller aktiv med vingen til 042), direkte observasjon, FRAME02, fremkalling og sammenligning. Dårlig print kan lages på nytt fra negativet.

Kuttet: gjenkjenning av den svarte figuren ved gjerdet. Stillheten ute holdes i stedet med lyd og sikt mot feltet.

### SC07: Det Dale ikke leverte inn

**Mål:** `Compare the amended record. Check Dale's field folder.`

E07 har original ABC/C, E06 er rettet. Linjeregister og konvolutt gir STATION01s trekant. Reservenøkkelen ligger i serviceskuffen. På kontoret ligger først det vanlige, så feltmappen, så Dales private samling. `EV_DALE_INDEX` og `EV_DALE_NOTE` er nok for hovedsporet. De seks nye dokumentene er frivillig lesing.

VHS-filmen «Service Road / Field 01» er valgfri og kommer når den er produsert. Når et lesepanel lukkes, kan utsikten fra vinduet ha endret seg.

### SC08: De peker på deg

**Mål:** `Check Earl's old reel. Test what the array follows.`

Earl på 1420 AM har en merket spole fra 1981, natten senderen var av. Reyes velger isolert bånd framfor live monitor, og den gamle stemmen sier det samme som før, uansett hva man spør. Kommer Reyes etter 04:00, ligger en datert kopi i lobbyen. 1420 AM er ikke 1420.405 MHz, og Earl sier det.

På arrayet bruker Reyes servicearket fra kontorskapet med A / ROW 1 WEST, B / ROW 1 EAST og REF / R1. Referanselykten står fast. Reyes velger selv A eller B, flytter til den andre og går kort tilbake. Antennene følger Reyes, ikke lykten. Kommandologgen viser ingen kommando.

Da kommer skapningen inn mellom fundamentene. To veier ut: lang bak opake fundamenter og skap, eller kort kryssing etter å ha lest søket. Etter flukten holder antennene det sist forlatte stedet et øyeblikk og går til 026, den kjente tilstanden fra 02:15.

### SC09: Den avbrutte oppstillingen

**Mål:** `Document the moved reference and the cut cable.`

Uendret fra planen: STATION01 med tom betongfot og flyttet A-post, FRAME03, nullprøve, kabelen med to rene snitt, FRAME04, E10 mot mottakerens «Reference west. No. East. Hold the last reading.» og Tomás. T02 er valgfri. Tilbake på SARO fremkalles FRAME03 og FRAME04, og først når begge er sikret, ringer Nora.

Kort avsøkning ved hytta kan stenge den enkle veien tilbake. Det er en skygge, ikke en jakt.

### SC10: Rom 6

**Mål:** `Ask Nora about the correction, the cut and her brother.`

Uendret fra planen. Ingen fare. Nora avslutter med: «Don't finish his observation. Get the originals to the courier, and don't stop at C.»

### SC11: SARO blir avsøkt

**Mål:** `Secure the records. Get out with them.`

Uendret i kjernen: mottakeroriginal, påbegynt hendelsesark og fjernarkene fra Hondo og Site11, ferdige først 04:04. Huset Reyes kjenner fra SC01 og SC04 blir avsøkt utenfra av flere lys, og skapningen leter inne. Lys gjennom åpninger, dører for tid, lang eller kort vei. Lokal lampe kan slås av, mens lyset utenfra fortsetter. Fluktfaktumet kommer fra den faktiske flukten.

Dette er den største sekvensen, og den eneste der skapningen får hjelp av lyset. Det er grunnen til at de tre første møtene skal ha vært små.

### SC12: Det dere rekker å sende

**Mål:** `Compare the sources, send the warning and plan the road.`

Mesa Diner er trygt. Servitrisen, kaffe og kjente stemmer. Reyes legger SARO-kilden ved siden av de to fjernarkene. De er ikke kopier av hverandre, og de viser samme rekkefølge, ikke nøyaktig samme tid. Noe skjer flere steder, og det nærmer seg.

Reyes ringer Ward og leser tre kilde-ID-er og den første grensetiden. Ward leser dem tilbake og fører dem i en datert logg. Først da er varselet sendt. Bildene blir i mappen.

Sheriffen kommer 04:30 og legger patruljearket sitt på disken: veien nord er kjørt for en time siden, uten hindringer. Kartet på veggen (P13) viser STATION01, nord på E14, C øst til 8.15, og budet som venter ved 10.4. Ingen lover at veien er trygg. Varselet er sendt, originalene har fortsatt verdi, og budet er et konkret mål.

Kuttet: sjåføren og dikteringen av måletabeller.

### SC13: Siste vei mot Roswell

**Mål:** `Carry the originals to the courier. Pass C without taking a reading.`

Lagring ved avreise. Lysene holder posisjon over horisonten. Ved C 8.15 stopper motoren av seg selv, og den faste finalen tar over: tomt førersete, uttrekk og 1947-bildet med Reyes som tredje silhuett. 10.4 nås aldri.

## Levering i fire akter

| Akt | Scenarioer | Slutter med |
|---|---|---|
| 1: Kvelden | SC02 til SC05 | Residualen i kontrollrommet |
| 2: Arkivet og arrayet | SC06 til SC08 | Antennene tilbake på 026 |
| 3: Feltet og Nora | SC09 og SC10 | Noras siste replikk |
| 4: Avsøkning og veien | SC11 til SC13 | Finalen |

Hvert scenario får fortsatt egne testsett etter planens avsnitt 4. Innenfor en akt kan scenarioene ligge på samme gren og testes etter hvert som de blir ferdige. Hele 110-matrisen, fletting og installasjon skjer én gang per akt. Spillet skal være spillbart etter hver akt: nye saker følger ny historie til aktens slutt og går deretter inn i eksisterende fortsettelse, som SC01 gjør i dag.

## Modellen i SC02: vurdering

Skyøkten har laget en original prosedyrisk kropp i seks stive deler, 13 660 trekanter, med forhåndsvisninger i `ArtSource/HorrorSC02/Previews`.

Det som virker: høyden og magerheten, ribbeina og de lange fingrene, og ansiktet med dype øyehuler.

Det som ikke virker ennå:

1. **Den ser ut som en utstillingsdukke, ikke et levende vesen.** Hoftepartiet er glatt og rundt, leddene er synlige ringer, og kroppen står rett og symmetrisk. Skyøkten har selv notert dette.
2. **Hodet leser som en klassisk «grey».** Lang, glatt hodeskalle og store mørke øyne. Det trekker mot en bestemt sjanger og gjør den mindre ukjent.
3. **Huden er jevnt hvit og matt.** I spillets mørke vil den se ut som gips.
4. **Ingen skjelett og vekting.** Stive deler som roteres om leddene gir dukkebevegelse. Det kan fungere for en skapning som beveger seg rykkvis, men da må det være et valg.
5. **Føttene er lange og flate.**

Anbefalt retting, før import:

1. **Positur:** foroverbøyd ryggrad, senkede skuldre og armer som henger forbi knærne. Hodet litt fram og ned, ikke rett opp.
2. **Hofte og ledd:** synlig bekkenkam og hoftekuler under huden, knoklete knær og albuer.
3. **Hodet:** smalere og mindre glatt, med kjeve, kinnben og små, dype øyne, ikke store mørke flekker. Mindre «grey», mer «noe som har vært menneske».
4. **Hud:** svak flekkethet, blålige årer ved ledd og fingre og litt fuktig glans. Teksturen kan lages prosedyrisk i Blender og bakes.
5. **Bevegelse:** enten et enkelt skjelett med vekting, eller et bevisst valg om at skapningen beveger seg rykkvis, ledd for ledd. Det siste passer reglene i SC02 (stillhet, så plutselig hodevending) og koster mindre.
6. **Føtter:** kortere, med tær og en hæl som tar bakken.

Modellen skal også stemme med den bleke hånden og underarmen fra SC01, som spilleren allerede har sett. Bruk samme hudmateriale og samme fingerform.

## Hva som gjenstår for Tom

- Ja eller nei til å slå av gammel tilfeldig bortføring med tidstap i nye saker.
- Stemmer for Ward, Ortiz, Earl og Nora (motstrid 16). Med sjåføren kuttet trengs fire, ikke fem. Ward er kvinne.
