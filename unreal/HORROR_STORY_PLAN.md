# SIGNAL / 47: historie, oppdrag og psykologisk stealth horror

Dato: 25. oktober 2026. Status: detaljert plan og produksjonsbacklog, klar for senere implementering. Bestilling: kosmisk etterforskning med psychological hide-and-seek / stealth horror, gradvis avsløring av en kommende invasjon, sterkere ubehag og senere jumpscares.

Tom ba deretter om å legge planen i todo slik at innholdet kan settes inn senere. Denne leveransen innarbeider derfor retningen i prosjektdokumentene. Nye oppdrag, bevis, fiender og fluktsekvenser nedenfor er ikke implementert eller spilltestet. Produksjonsoppgavene står åpne i [todo.md](todo.md). Spilltekst som foreslås her er engelsk; den er ikke ferdig innspilt manus.

Nyeste sammenfletting: [MISSION_FLOW_1800_ROSWELL.md](MISSION_FLOW_1800_ROSWELL.md) er én hovedrekkefølge med13 scenarioer og gjelder foran eldre oppdragsrekkefølge her. Alle eksisterende randomfamilier beholdes. Ett tidlig lys blir gradvis flere over himmelen. [Dales kontorvedlegg](DALE_OFFICE_DOCUMENTS.md) viderefører sju eksisterende dokumenter og seks nye frivillige tekstutkast. De tidligere32 arbeidskortene er bevart som [sekundært oppslag](MISSION_DETAILS_REFERENCE.md). Nye produksjonsoppgaver S.1 til S.9 står åpne.

## 1. Anbefalt retning

Skrekkrevisjon R2 etter Toms kritikk: kvelden var for passiv og planen hadde for få reelle alienmøter. [Hovedscenarioets SC01 til04](MISSION_FLOW_1800_ROSWELL.md#sc01-den-første-overflygningen) samler seks nødvendige kveldsbeats. Fem skrevne trusselmøter gjennom natten, tre før midnatt, og synlig bortføring bygger mot fast finale. 00:13:47 er fortsatt første store instrumentelle residual. Fremmed fysisk tilstedeværelse er kjent tidligere. Dette er planlagt produksjon, ikke innhold i levert T01 R2-testpakke.

Detaljerte løsninger er bevart i [MISSION_DETAILS_REFERENCE.md](MISSION_DETAILS_REFERENCE.md) og innpasset i hovedscenarioet. Bruk dem med [engelske forfatterkilder](Narrative/HorrorStory/README.md). T01 R2 er levert; utvidet skrekkreise krever senere integrasjon. E15 er gammelt dinerutklipp uten04:45-port. Mottakeroriginal, påbegynt hendelsesark og ferdige fjernark sikres tidligst04:04 før beleiring; romobservasjoner føres først etter hendelsen.

Spilleren kommer for å holde en radiostasjon i drift. Etter hvert lærer de at avvikene er deler av en systematisk kartlegging av mennesker, steder og forbindelser. Den samme kunnskapen som gjør dem til en god etterforsker, lar dem lese en trussel, finne dekning og komme seg ut. Mot slutten forstår de at SARO bare er ett punkt i en større forberedelse. Invasjonen nærmer seg.

Spillets løfte er: **Undersøk det fremmede. Lær hvordan det finner deg. Bruk kunnskapen til å komme deg videre.**

Kamera, målinger, lydopptak, kart og vitner er hovedverktøyene. Korte sekvenser med skjul og flukt gjør funnene fysisk betydningsfulle. Ingen kamp, våpenprogresjon, crafting, sult, bandasjer eller jakt på batterier. Press kommer fra plassering, lyd, sikt og valg av tidspunkt. Utstyr skal normalt være tilgjengelig når det trengs.

Prioriter fem forskjellige, sammenhengende trusselmøter: trafostasjonens retur, inntrenging i annexet, et nært bilmøte, det nye arrayoppdraget og den store SARO-beleiringen. Korte fysiske tegn, skyggemannen og en faktisk bortføring binder dem sammen. Det guffne skal påvirke hvor spilleren kan gå og hva de må gjøre, i tillegg til hva de leser.

## 2. Grunnlag og konkrete forbedringsbehov

Gjennomgangen tok utgangspunkt i lokal `unreal-port-core`, spillkilde `0be9fbd8`, og eksisterende Unreal-dokumenter og C++-kode. Kildestatus kan endre seg før implementeringen; begynn produksjonen med en ny kontroll.

| Område | Det som finnes | Forbedringen som anbefales |
|---|---|---|
| Etterforskning | Kamera, faktisk fotografisk dokumentasjon, fremkalling, sammenligninger, saksbrett, vitner og kapitler. | La hvert hovedfunn endre en konkret handling eller rute. Reduser gjentatte paneltrinn som bare kvitterer for samme slutning. |
| Nye historier | Kveld/ankomst/Dale og N01/N02/A02/P01 finnes i runtime. Kontor/VHS og senere T01/T02 er helt eller delvis manusgrunnlag. | Fullfør nødvendige oppgaver som sammenhengende spillerreiser; en modell eller skrevet scene er ikke en ferdig oppgave. |
| Skrekkrytme | Felles nattkurve, beskyttede sekvenser, lyd, glimt, lys og bortføring. | Behold randomfamiliene. Reserver hovedmøter og utsett konkurrerende kandidater uten å bruke opp ustartede engangsflagg. Ingen oppsamlet hendelsesbarrage etter dialog. |
| Skyggemenn | Detaljert forslag i ANOMALY_PLAN del 13. Eldre skikkelser kan forsvinne ved blikk, zoom eller lykt. | Innfør den stabile observasjonsregelen før figurene får større betydning. Samme utseende må ikke ha motstridende regler. |
| Sniking | Gange/løp og egen spillerkollisjon. Ingen identifisert komplett mekanikk for huking, dekning, sansing, søk og brutt kontakt. | Bygg et lite faktisk stealth-system; lys og en figur alene gir ikke skjul/flykt-gameplay. |
| Historie | Nora, Tomás, 1947, C, avtrykk og én fast slutt har en sterk personlig sammenheng. | Gjør den kommende invasjonen til et nytt lag over denne historien, med nødvendige bevis i hovedruten. |
| Siste kjøretur | Spilleren vil kontrollere om C fortsatt holder. | Gi avreisen et rimelig arbeidsmål og en forståelig risikovurdering etter at faren er blitt tydelig. |
| Opplevelsesbevis | Automatiske scenarioer og flere visuelle kontroller finnes. | Menneskelig forståelse, frykt, lytting og samlet spilletid er fortsatt egne, uverifiserte spørsmål. |

Prosjektbiblioteket ble søkt først og én gang bredere. [The Dark Mod-profilen](/home/tom/.local/share/prosjektbibliotek/repoer/spill/the-dark-mod.md) var relevant som referanse til synlighet, lyd og stealth-oppdrag, men oppgir C++/id Tech 4 og UNVERIFIED lokal teststatus. Ingen kode eller assets gjenbrukes. Den er ikke en kompatibel Unreal-komponent; prosjektets egne systemer er produksjonsgrunnlaget.

## 3. Referansespill og hva vi tar med

Tabellen skiller dokumenterte beskrivelser fra våre forslag. Referansene beviser ikke at en bestemt scene vil skremme i SIGNAL / 47.

| Referanse og primærkilde | Dokumentert prinsipp | Tilpasning til SIGNAL / 47 |
|---|---|---|
| [P.T., Konami på PlayStation Blog](https://blog.playstation.com/2014/10/31/p-t-on-ps4-survival-tips-for-konamis-horror-hit/) | Gjentatt korridor med endringer; observasjon og lydspor driver undersøkelsen. | Bruk kjente SARO-rom og gjenbesøk som endrer betydning. Behold forståelige oppgaver og hint. Ikke kopier korridoren, Lisa eller gåter basert på tilfeldig prøving. |
| [Alien: Isolation, intervju med Alistair Hope](https://blog.playstation.com/?p=127046) | En reaktiv skapning bruker sanser; løping lager lyd som kan tiltrekke den. | La spilleren forstå syn, lyd, søk og brutt kontakt. Lån det lesbare forholdet mellom handling og risiko. Våpen og ressursøkonomi inngår ikke. |
| [Amnesia: The Bunker, Thomas Grip](https://blog.playstation.com/2023/06/01/amnesia-the-bunker-launches-june-6/) | Kartlegging, planlegging og flere mulige løsninger; støy kan tiltrekke fare. | La en kjent bygning støtte to ruter og enkle, fysiske avledninger. Behovet for drivstoff og forsyningsjakt kopieres ikke. |
| [SOMA, Frictional om Safe Mode](https://frictionalgames.com/2017-11-what-is-somas-safe-mode/) | Skrekk i atmosfære og tema kan bestå selv når frykten for å mislykkes reduseres; skapningenes atferd ble bearbeidet. | La bevis endre spillerens forståelse av hva som skjer med mennesker. Unngå at mange dødsrepetisjoner forstyrrer de viktigste avsløringene. |
| [The Exit 8, KOTAKE CREATE/PLAYISM](https://store.steampowered.com/app/2653790/The_Exit_8/) | En enkel observasjon-regel bestemmer om spilleren skal snu eller fortsette. | Lær noen få pålitelige tegn som får praktiske følger. Et unormalt lysmønster kan varsle at en kjent passasje blir avsøkt. Hele spillet skal ikke bli en katalog over tilfeldige forskjeller. |

Fra P.T. er den viktigste overføringen at et kjent sted kan bli vanskelig å stole på. Fra stealth-referansene er den viktigste overføringen at spilleren likevel må kunne stole på handlingsreglene.

## 4. Historien: fra driftsfeil til forberedelse til invasjon

### Det som beholdes

- New Mexico, kveld og natt 13. til 14. april 1986. For nye kveldssaker: ankomst/skiftstart 18:00 og residual 00:13:47 etter eksisterende tidslinje 2. Gamle saker beholder tidslinje 1.
- 1420.405 MHz, 27 antenner og azimut 026. Byens 1420 AM er en separat lokalradio.
- Nora og Tomás, den bevisste kabelkuttingen i 1947, slettingen av C og de negative avstandene.
- Avtrykk gjentar et bevart forløp og svarer ikke på nye spørsmål. En reaktiv inntrenger er en annen kategori enn et opptak eller avtrykk.
- A og C er forskjellige linjer. Avisen, kartet og oppmålingsbolten beholder sin funksjon.
- Én slutt. THE EVENT er 05:29:00 til 05:29:47, med tom bil og forbindelsen til Reyes i det fullstendige 1947-bildet. Ingen redd-Tomás-slutt eller ny boss.
- Dale finnes bare på telefon, radio eller walkie. Han er en kollega med mangelfull kunnskap, ikke allvitende forteller.

### Det nye laget

**Anbefalt forfatterfasit:** Den ikke-menneskelige registreringsprosessen brukes til å kartlegge og etablere referanser før en større ankomst. Menneskene i 1947 koblet seg til noe som allerede fantes. Nora avbrøt én lokal forbindelse. I 1986 viser uavhengige stasjoner at flere forbindelser er i ferd med å samordnes. Reyes blir selv en ny referanse.

Dette er en bevisst utvidelse av eldre HISTORIE.md, som begrenset hvor tydelig fenomenets omfang skulle bli. Brukerens ønske om en kommende invasjon er den nye retningen. Hvem som kommer, hvorfor de bruker denne prosessen og hva de vil med menneskene kan fortsatt være uavklart. Spilleren skal likevel kunne forstå at noe organisert forberedes, og at det gjelder flere steder.

De 39 årene og avtrykkene beholdes som historiens særpreg. Ikke legg til en ny forklaring om at hvert avtrykk er en alien eller at mennesker nå fritt reiser i tid. Tomás' eksisterende feltkort om å hindre en «organised return» kan få ny betydning, men uttrykket alene beviser ingen invasjon. De nye samtidige registreringene må bære den slutningen.

### Avsløringen i fem trinn

| Trinn | Spillerens første forklaring | Beviset som endrer den | Praktisk følge |
|---|---|---|---|
| 1. Noe er her | Jobben begynner med vanlig drift, men stedet er ikke trygt lenge. | Normal lampetest etterfølges av en reaktiv enhet og reell unnslippelse. | Meld inntrenger, søk hjelp og bruk dekning. En normal prøve forklarer ikke aktøren. |
| 2. Noe blir bevart | Et bilde eller et bånd har en feil. | Kontrollbilder og uendret lydfragment viser at et forløp blir liggende. | Fotografi og gjentakelseskontroll blir pålitelige verktøy. |
| 3. Noe kartlegger | Tilfeldige skikkelser og lys viser seg. | Plasseringer følger faste referanser; kart og fysisk kontroll viser et ordnet mønster. | Spilleren kan forutse et avsøkt område og velge en annen rute. |
| 4. De samordner søket | De tidlige møtene kan fortsatt være lokale og avgrensede. | Sen avsøkning blokkerer ruter i sammenheng med inntrengerens bevegelse. Et gammelt lydfragment fortsetter separat. | Kombiner lærte regler og skil aktør, avtrykk og koordinering. |
| 5. Det er forberedelser | Dette gjelder SARO alene. | Uavhengige, lokale logger fra andre stasjoner viser samme faseorden med egne instrumentdata og en kommende felles fase. | Spilleren varsler andre og innser at en større ankomst er nært forestående. |

De nødvendige bevisene for trinn 4 og 5 legges i hovedreisen. Valgfrie hendelser gir dybde, ikke eneansvar for historiens hovedavsløring. Et manglende valgfritt fotografi skal aldri gjøre invasjonshistorien uforståelig.

Ingen NPC gir en lang invasjonstale. Ward kan bekrefte hva andre stasjoner faktisk rapporterte. Dale kan innrømme hva han forsøkte å undersøke. Nora kjenner den menneskelige kostnaden og det lokale forsøket, ikke hele den fremmede planen.

## 5. Gameplay-loop og progresjon

**Få et konkret spørsmål → undersøk stedet → dokumenter → velg en kontroll → trekk en avgrenset slutning → bruk kunnskapen i verden → kom tilbake med et nytt spørsmål.**

En vanlig undersøkelse bør gi et meningsfullt valg i løpet av få minutter. En lengre oppdragssløyfe kan som prøveanslag vare 8 til 15 minutter, med korte trykktopper og ro etterpå. Tallene er designmål som må måles, ikke påstått nåværende spilletid.

Eksempel: Spilleren finner lys på et rom som skal være frakoblet. De kontrollerer kurs og en annen lampe, tar bilder og ser at lyset kommer utenfra. Senere bruker de samme kunnskap til å velge en innvendig vegg fremfor å stille seg ved vinduet. Beviset har da endret handling, ikke bare åpnet en dokumentknapp.

Progresjonen er bedre dømmekraft: lære normal drift, kjenne igjen et avtrykk, lese en avsøkning, forstå hva som gir skjul og til slutt se nettverket. Utstyr får nye bruksområder fremfor numeriske oppgraderinger. Lommelykten trenger ingen batteriøkonomi. Kameraet er dokumentasjon, ikke et våpen som automatisk stopper inntrengeren.

Saksbrettet beholder store slutninger. Små, åpenbare kontrollresultater kan føres direkte i notatboken når spilleren har gjort den fysiske prøven. Presenter ett hovedspørsmål og høyst noen få relevante oppfølginger samtidig. Ikke vis hele fasiten som en lang sjekkliste før spilleren har undersøkt stedet.

Hver oppgave skal ha et spørsmål, en fysisk handling, en meningsfull kontroll, en konsekvens og en ærlig vei videre ved utilstrekkelig bevis. Fremdrift krever ingen tilfeldig spawn eller skremmekvote.

## 6. Anbefalt oppdragsreise

[Hovedscenarioets SC08](MISSION_FLOW_1800_ROSWELL.md#sc08-de-peker-på-deg) legger ARRAY-01 etter arkiv/bykontroll før STATION01: fast referanse, to selvvalgte ståsteder, faktisk antennerespons og retrett. Et nytt ordinært arrayservicevedlegg må produseres; Dales eksisterende indeks peker til T02s kulvert. Caliche gir hjelp, kildekontroll og sen sammenstilling. Det fjerde aktive møtet ligger på arrayet, ikke en ny lang STATION01-jakt. Nye scener/faseporter er produksjonsforslag.

Presise oppdragsløsninger, feilsituasjoner og farer står i [detaljvedlegget fra 18:00 til Roswell](MISSION_FLOW_1800_ROSWELL.md). Tabellen nedenfor er den overordnede strukturen, ikke en alternativ rekke med ekstra pliktoppdrag.

H-numrene nedenfor er nye planreferanser. De erstatter ikke dagens P-, N-, A- eller T-ID-er i kode og lagring. Kapitlenes eksisterende bevis bevares og omarbeides bare der det trengs for den nye sammenhengen.

| Planoppdrag | Spørsmål og spillerhandling | Ny kunnskap og følge | Skrekk og ferdigkriterium |
|---|---|---|---|
| H01: Vanlig kveld | Ankomst, Dale, normalrunde på SARO. Lær to aktuelle utganger, en innvendig skjermet plass og utstyr. N01 og valgfri byreferanse inngår. | Spilleren forstår jobben og vet hvor de kan gå. | Varm, hverdagslig normal. Ingen påtvunget stor skremmehendelse. Direkte innkjøring uten diner må fungere. |
| H02: Den ordinære feilen og det som står ved bilen | Fullfør T01 ved kontrollfronten. På returen kreves virkelig unnslippelse fra en reaktiv enhet, senere hjelpekontakt og en ny kveldsakt. | Lokal feil forklares; aktøren og de nye hendelsene tas alvorlig. | Nødvendig fangstrisiko før midnatt. Ferdig T01 beholdes ved lokal gjenopptakelse. |
| H03: Den andre eksponeringen | Etter residualen: behold S-03/B-12, velg en kontroll som skiller lampe, motiv og film. | Fremmed tilstedeværelse er kjent; nå undersøkes hvordan en referanse kan vedvare. | Ny mekanisme avsløres gjennom egen prøve. Ingen NPC later som kveldens aktør bare var lampefeil. |
| H04: Det Dale ikke leverte | Finn reservenøkkel gjennom et begrunnet spor. Les utvalgte kontorfunn og se VHS som bevegelig opptak. | Dales plasseringer gir et prøvbart mønster, men han mangler forklaringen. | Et kjent vindusutsyn har endret seg etter undersøkelsen. Nøkkel og film driver en feltoppgave; de løser ikke Nora. |
| H05: Kartleggingen | STATION 01, kabelkutt, fastpunkt og Tomás. Relevant T02 på samme feltreise kontrollerer lyset mot T01. | Det gamle forløpet gjentas; nåtidens plasseringer er systematiske. | Ett kort, lesbart skjulmøte kan lære dekning uten lang jakt. Gjerder og utilgjengelig høyspenningsområde beholdes. |
| H06: Rom 6 | Vis Nora relevante bevis, sammenlign rapportene og still et nytt spørsmål til returlinjen. | Hun kuttet med vilje; stemmen er et avtrykk. Kostnaden får et menneskelig ansikt. | Samtalen er en rolig, beskyttet sekvens. Ingen angrep mens spilleren leser brevet eller snakker. |
| H07: SARO blir undersøkt | Sikre måleoriginal og ferdige fjernark. Samordnet avsøkning og inntrengerens søk avskjærer vante ruter. | Aktøren er kjent; organisering og kontroll over SARO eskalerer. | Den største skjul/flukt-sekvensen bruker alt som ble lært før midnatt. To brukbare ruter og reell unnslippelse. |
| H08: Det samme skjer flere steder | Ved trygg sammenstilling kontrolleres SARO mot uavhengige fjernlogger og den vanlige sendeplanen. Varsle Ward med underlag. | Faseorden og samtidighet peker mot en koordinert kommende ankomst. | Ettervirkning og uro, uten enda en lang jakt. Nødvendig logg kan hentes som bevart utskrift ved sen ankomst. |
| H09: All Night | Avis, vitner og kart løser C ved veien. Avklar et konkret oppdrag for verifikasjon/varsling utenfor SARO. | Personhistorien og det større mønsteret samles. | Dineren gir en reell pustepause. P01 og andre valgfrie hendelser får ikke avbryte kartarbeidet. |
| H10: Roswell Road | Kjør til et begrunnet kontroll-/overleveringspunkt langs den eksisterende ruten, med plan om å passere C uten stopp. | Bilen stanser ufrivillig; fenomenet har allerede registrert spilleren. | Behold 47-sekundersfinalen og 1947-bildet. Ikke presenter dette som en jakt spilleren kunne ha vunnet. |

H07 plasseres etter Room 6 og før den rolige samlingen i All Night. Detaljert fortellertid må innpasses i tidslinje 2, uten å flytte finalen eller gjøre 05:20 til en straffende frist. Eldre saker som allerede har passert sekvensen skal ikke trekkes bakover.

### Motivasjonen for den siste turen må skrives om

Etter en voldsom beleiring er «jeg er nysgjerrig på om linjen holder» en svak grunn til å kjøre inn i et kjent risikoområde. Anbefalt endring: Ward har mottatt varslingen, morgenserien er stanset, og en ekstern kontroll eller overlevering krever at spilleren forlater SARO langs den eksisterende veien. C ligger på ruten, men planen er å passere og holde avstand til referanseflaten.

Nytt manus må gi en troverdig grunn til rutevalget og vise hva spilleren forsøker å oppnå. Kontroller det faktiske veikartet før målet fastsettes. Ingen usynlig sperre eller uforklart radioblokkering kan være eneste begrunnelse. Spilleren skal ha gjort noe nyttig med funnene før slutten. Den endelige fellen fjerner ikke verdien av etterforskningen.

## 7. Trusler og lærbare regler

### Skyggemannen

Følg ANOMALY_PLAN del 13: original helt svart menneskeform, stille under observasjon, bare flytting når avgang og ankomst er usett. Også perifer synlighet, zoom, lykt og aktiv fotosøker fryser figuren. Paneler, dialog, pause og lasting gir ingen tilnærming eller oppspart flytting.

Første SARO-vindu/trafo/retur-pilot er uten skade. Begrens antall flyttinger. Noen ganger står figuren fortsatt på samme sted når spilleren ser tilbake. Den blokkerer ikke eneste vei og tvinger ikke frem endeløs baklengs gange. Samme svarte figur kan ikke senere løpe mot spilleren under direkte blikk.

### Inntrengeren

En annen, gjenkjennelig fysisk kategori med egen positur, bevegelse og lyd. Den kan bevege seg mens den blir sett og undersøker nye lyder. Dagens originale alienressurs kan vurderes etter en egen kontroll av rigg, animasjoner, kollisjon og lesbar silhuett. En eksisterende modell er ikke ferdig fiende-AI.

Første utgave har et lite tilstandsforløp: **ankomst → undersøk siste sansede sted → søk → bekreftet kontakt → forfølgelse → mistet kontakt → avgrenset søk → tilbaketrekning eller fangst**. Et manusstyrt inngangspunkt kan sette scenen; oppdagelse og flukt må deretter følge faktiske handlinger.

Inntrengeren ser langs forståelige synslinjer og hører definerte hendelser. Solide vegger stopper syn og demper lyd. En lukket dør gir tid, men er ikke automatisk varig trygghet. Den kjenner ikke spillerens posisjon gjennom hele bygget. Etter brutt kontakt undersøker den siste kjente sted før andre områder. Avledninger brukes med en ordinær radio eller annen etablert lydkilde; ingen forbruksøkonomi innføres.

Huking må endre faktisk øye-/kroppseksponering bak relevant dekning, ikke bare senke kameraet. Introduksjonen gir tid til å forstå dekning og en kort redningsmargin ved første oppdagelse. Skjul skal støtte bevegelse videre, ikke kreve flere minutter i et skap. Et søk avsluttes også når spilleren lykkes med å holde seg skjult.

### Det fremmede lyset

Et miljøsystem for avsøkning, ikke en tredje fiende. Et bredt, sterkt felt beveger seg over vinduer og gårdsplass. Passasjer, dørkarmer og silhuetter må fortsatt kunne leses. Opake flater og avskjerming har reell effekt. Vanlig mørke er ikke sikkert dersom spilleren står i et vindu.

En strømprøve lærer at det ytre lyset fortsetter selv om en relevant lokal lyskurs er av. K3 blir ikke en magisk bryter som slår av byen eller den fremmede kilden. Ikke bruk usynlige infrarøde sanser i introduksjonen. Lommelyktens risiko gjelder bare når stråle eller lysflekk faktisk kan oppfattes; en av/på-knapp gir ingen global fiendealarm.

## 8. Signatursekvens: SARO under hvitt lys

Arbeidsnavn på engelsk: **The Inspection**. Første samlede prototype anslås til 25 til 35 minutter med normal befaring, undersøkelse, beleiring og etterarbeid. Den aktive forfølgelsen bør være kort, som et prøveutgangspunkt omtrent 1 til 3 minutter. Dette er ikke en målt varighet.

Spilleren kjenner SARO før sekvensen starter. Hovedmålet er å sikre en original registrering og komme til servicebilen. Spillerens nåværende måleoriginal er allerede identifisert før angrepet. Vi legger ikke inn en jakt etter tre nye sikringer mens noe jager dem.

| Fase | Spillerhandling og hendelse | Lesbarhet og kontroll |
|---|---|---|
| 1. Grunnlag | Normal befaring og én relevant kontroll av vindusavskjerming, dør og utgang. | To rutevalg er prøvd i vanlig spilling. En ren, korrekt varslet øvelse kan lære huking/dekning før et ekte angrep. |
| 2. Første varsel | Ventilasjonen stanser, kjent radiolyd faller bort, et hvitt felt treffer fasaden. | Spilleren beholder kontrollen. Ingen umiddelbar fangst eller fullstendig hvitt skjermbilde. |
| 3. Kontroller kilden | Sammenlign lokal lampetilstand med lyset gjennom vinduet. Trekk fysisk avskjerming for. | Kontrollen viser at lyset er eksternt og at opak skjerming virker. Gjennomført tidligere prøve kan gi kortere vei videre. |
| 4. Fronten stenges | Frontutgangen kan ikke brukes mens gårdsplassen avsøkes. En kjent servicevei er tilgjengelig. | Årsaken vises fysisk; alternativet er kjent og markert i miljøet. Utgangen blir ikke bare deaktivert uten forklaring. |
| 5. Noe kommer inn | Dør/lås og faktiske trinn varsler ankomst fra én kjent side. | Første skjulplass og videre rute kan nås. Inntrengeren reagerer på det spilleren gjør, ikke på et usynlig påkrevd tidspunkt for fangst. |
| 6. Velg fremgangsmåte | Ta en skjermet innvendig rute, eller aktiver en kjent lydkilde og bruk en kortere passasje etter at den undersøkes. | Begge løsninger må være gjennomførbare. Risikoen er eksponering eller egen støy, ikke manglende inventargjenstand. |
| 7. Ut og bort | Ta med originalen dersom den ikke allerede er sikret; nå serviceutgangen og bilen. En eventuell siste spurt kommer etter avlest fare. | Førstegangsoppdagelse gir en mulighet til å bryte kontakt. En vellykket flukt fullføres og respekteres. |
| 8. Etterpå | Lyset trekker bort. Spilleren får ro, kontrollerer opptaket og sammenstiller det med fjernloggene. | Ingen ny tilfeldig bortføring umiddelbart etter suksess. Funn og gjennomført flukt lagres én gang. |

Rute A er en roligere vei bak opake flater, med nødvendig stans ved en avsøkt åpning. Rute B bruker en etablert radio/lydkilde og en mer direkte vei. Første versjon trenger ett veivalg og ett tidspunktvalg. Ingen stor fysikksimulator, ventilasjonslabyrint eller generell AI for hele verden kreves.

Romforbindelsene må måles i dagens native SARO. Kontrollrom, annex, korridor, arkiv og lab er kandidater, ikke bevis på en eksisterende sammenhengende løype. Hvis to gyldige ruter ikke finnes, bygg én avgrenset serviceforbindelse eller reduser møteområdet. Ikke skjul en manglende rute bak teleportering. En lukket arkivblindvei kan ikke være eneste anbefalte skjul.

## 9. Fangst, bortføring og fast slutt

Anbefalt første implementering: fangst i SARO avbryter den lokale jaktsekvensen med en kort, særskilt skrekkovergang og tilbyr gjenopptakelse fra et tydelig sjekkpunkt før beleiringen. Dette er vanlig gjenopptakelse av spill, ikke en ny tidsreise i historien. Bevar siste trygge sak og alle funn som var sikret før møtet.

En vellykket flukt må ikke straks etterfølges av en obligatorisk fangst som gjør suksessen meningsløs. Den faste finalen har en annen presentasjon og funksjon: bilen stanser, og THE EVENT skjer. Ikke merk finalen «escape» eller la et skjult feilvalg avgjøre en allerede fast slutt.

En alternativ bortføringsgren med oppvåkning, tapt tid og ny posisjon er en senere, separat vurdering. Den krever egne oppdrag, klokke-/transportregler og manus. Den inngår ikke i første backlog for å unngå å kollidere med den etablerte finalen. Dagens generelle bortføringssystem kan allerede inneholde tapt tid og gjentakelser; det må samordnes før det brukes sammen med den nye reisen.

## 10. Skrekkrytme, lyd og jumpscares

Gi spilleren en kort normal og et fysisk brudd tidlig. Første unnslippelse kommer allerede på kveldens servicetur. Mellom aktive møter bygger låser, kjent utsyn, nye bevegelser og konkrete etterspor ubehaget videre. Som uprøvde mål settes første tydelige brudd innen10 til15 faktiske spilleminutter og første unnslippelse innen25 til40; forståelse og faktisk rute må måles i menneskelig prøve.

De viktigste toppene har forskjellige funksjoner:

1. Trafostasjonens retur blir en virkelig unnslippelse; spilleren lærer at noe kan komme mellom dem og bilen.
2. En tilkalt hjelper blir tatt før midnatt. Det gir et menneskelig tap og en konkret savnetmelding.
3. En blek hånd i annexets dørkarm innleder nær fare og en faktisk innendørsflukt. Dette er et tidlig ordentlig jumpscare, ikke panelstraff.
4. Bilmøtet tvinger spilleren til å lese glass, sikt og lyd på nært hold. Motoren virker; oppgaven er forskjellig fra finalen.
5. Feltpassasjen og den sene beleiringen bruker lærte regler med større avskjæring og koordinering.
6. THE EVENT beholder sitt sene bildespråk. Sluttens hovedavsløring er hva registreringen gjør med Reyes, ikke at fremmede finnes.

Dette er et startutvalg, ingen kvote. Behold alle eksisterende hendelsesfamilier. Dersom takskritt, telefon, bortføring og beleiring konkurrerer, reserver hovedmøtet og utsett kvalifiserte kandidater uten å markere dem opplevd. Ingen garanti om at alt skal spilles i én natt. Allerede påbegynt hendelse trenger korrekt avslutning/gjenopptakelse, ikke falskt observasjonsflagg.

Etabler ventilasjon, insekter, trafo, servoer og lokalradio som normal. Et lokalt bortfall gjør stillhet meningsfull. Trussellyd kommer fra faktisk posisjon og handling. Misvisende lyd kan brukes som et dokumenterbart avtrykk, men må ha samme gjentakelsesregel som resten av historien. Inntrengerens sanseinformasjon skal ikke være ren lyddekor uten sammenheng med hvor den er.

Ikke knytt en skremmehendelse til hver skuff, hver fremkalling eller hvert tilbakeblikk. La noen forventede angrep utebli. Beskytt Noras samtale, brev og viktig sammenstilling. Dineren bør fortsatt gi lettelse før siste kjøretur. Redusert blinking, redusert kamerabevegelse og lesbare lydtekster skal fungere uten at kritisk informasjon forsvinner.

## 11. Teknisk innpassing og lagring

| Område | Eksisterende grunnlag under Signal47/Source/Signal47 | Nødvendig senere arbeid |
|---|---|---|
| Natt og beskyttelse | `Private/S47Curve.cpp`, `S47Night.cpp`, `S47AnomalyDread.cpp`. | La beleiringen reservere én hendelsesplass, sperre konkurrerende topper og avslutte med ro. Ingen separat tilfeldig skrekktimer. |
| Observasjon | `Private/S47Seen.cpp`, `S47Glimpses.cpp`, `S47SaroDark.cpp`. | Egen korrekt skyggemannregel med perifer synlighet, glass, okklusjon og lagret ståsted. Skill den fra inntrengerens sanser. |
| Spiller | `Public/S47Walker.h`, `Private/S47Walker.cpp`, gamepad-ruting. | AS47Walker er APawn med egne gangsoner og kollisjon. Huking, eksponering og frie fluktpassasjer må implementeres i denne løypa. |
| Søk og forfølgelse | Ingen identifisert ferdig generell stealth-AI. | Start med en liten håndtegnet rom-/forbindelsesgraf og kontrollerte siktporter. Mål behov før eventuell innføring av full NavMesh/BehaviorTree. |
| Foto og bevis | `Private/S47FieldCamera.cpp`, `S47Album.cpp`, `S47Anomaly.cpp`, kapitlene og saksbrettet. | Bevar ekte piksler, tid, sted og kontrollresultat. Dagens instantfoto/fremkalling og historiens mørkeromsarbeid er forskjellige løp. Nytt sterkt lys ødelegger ikke automatisk film uten uttrykkelig mekanikk. |
| Bortføring og vern | `Private/S47Abduction.cpp`, `S47World.cpp`, `S47Night.cpp`. | Kontroller dagens Warning/Force og innendørsregler. `World::Indoors` omfatter ikke SARO direkte; den er derfor ikke et ferdig trygghetsflagg for beleiringen. |
| Historie og pakke | `S47Chapter1` til `S47Chapter6`, `S47EveningPack`, `Narrative/EveningShift/modules/`. | Nye engelske scener legges i relevante kilder; ikke håndrediger generert SCRIPT/undertekst. Bevar eksisterende bevis-ID-er. |
| Lys og dører | `S47Web`, native lys/props og aktuelle SARO-modeller. | Fysisk avskjerming, ekstern lyskilde, dører og sikt må følge samme geometri. Unngå global overeksponering og falske åpninger. |
| Lagring | `S47Flow`, `S47Saves`, kapitlenes SaveState og tidslinjeversjon. | Additiv versjonert møtetilstand, migrering og eksakt videreføring. Ingen omskriving av gamle fotografier eller 1947-tider. |

Lagre som minimum møtets ID/versjon, fase, valgte rutevarianter, inntrengerens node og siste kjente spillersted, søkets gjenværende tid, lys-/dør-/avskjermingstilstand, bevisstatus og fullført/utløst-flagg. Skil fakta fra spillerens slutning. Et foto som mangler på disk er ikke godkjent dokumentasjon bare fordi et flagg sier at det ble tatt.

Pause, dokumenter, dialog og notatbok fryser møtet etter eksisterende kontrakt. Å lukke panelet gir ikke oppspart tilnærming. Lyden må følge samme pause. Ekte fotosøker er en handling i verden og skal håndteres uttrykkelig; man kan ikke unngå en inntrenger ved å fotografere den. Skyggemannens egen observasjonsregel gjelder fortsatt.

Første versjon bruker trygt automatisk sjekkpunkt før møtet og etter vellykket flukt. Hvis lasting midt i møtet støttes, må hele tilstanden gjenopprettes. Hvis bare trygg lagring støttes i en kort aktiv fase, køes den tydelig; ingen skjult omstart under «Continue». Å lukke spillet må ha en forståelig gjenopptakelse.

Nye saker kan få en egen innholdsversjon. Gamle saker beholder sine tider, foto og fullførte hendelser; en passert beleiring blir ikke spilt av retroaktivt. Behold en ny-sak-rute for hele den nye oppbyggingen. Alle tilfeldige valg som har betydning for en påbegynt hendelse lagres, slik at Continue ikke trekker en farligere variant.

## 12. Produksjonsrekkefølge og avgrensning

| Etappe | Leveranse | Avhengighet og ferdigport |
|---|---|---|
| H0: Plan | Dette dokumentet, sporbar todo og lenker i overleveringen. | Fullført som dokumentarbeid. Nye spillfunksjoner forblir åpne. |
| H1: Historie og måling | Gjeldende tids-/beviskart, oppdragsmotivasjon, invasjonens nødvendige bevis og inventar over eksisterende skrekktopper. | Ingen scener hevder mer enn bevisene støtter. Rute til finalen har rimelig formål. |
| H2: Undersøkelsesgrunnlag | Nødvendig kontor/VHS og T01/T02 fullført, pluss stabil skyggemannintroduksjon. | Én hel SARO/trafo/retur-reise med ekte foto, fremkalling, fysisk følge og lagring. |
| H3: Stealth-prototype | Lite SARO-område, huking/dekning, én inntrenger, avlest lys og lyd, to ruter, brutt kontakt og fangst/gjenopptakelse. | En spiller kan forklare oppdagelsen og faktisk lykkes med flukt. Ingen fullverden-AI kreves. |
| H4: Hele signatursekvensen | Befaring, kontroll, beleiring, flukt og etterarbeid i samme sak. | 25 til 35 minutter er en hypotese som må måles. Funnet endrer forståelse og neste oppdrag. |
| H5: Nattens historie | Fjernbevis, varsling, All Night og endret begrunnelse for siste tur, med eksisterende finale bevart. | Hovedruten alene bærer invasjonens gradvise avsløring. Valgfrie spor og gamle saker fungerer. |
| H6: Prøving og justering | Kort relevant teknisk kontroll, menneskelig prøve og samlet historiereise. | Dokumenter faktisk kilde, kandidat, bevis og gjenstående forhold før senere integrasjon. |

H2 og H3 kan få avgrensede parallelle kode-/modelloppgaver når arbeidet starter. Felles Story/Main/Flow/Web-endringer eies av én integrator. Tunge eksport-, Unreal-, pakke- og spilljobber kjøres sekvensielt etter gjeldende maskinrutine.

Første tekniske prototype kan fortsatt bruke ett lite romforløp og én aktiv inntrenger. Den fulle reisen krever fem distinkte trusselmøter med samme sanse-/søkelogikk, en passiv skyggemannkategori, avsøkning og en produsert vitnebortføring med egen kveldsperson. Nye store interiører i hele Caliche, romskipinteriør, generell fiende-AI på motorveien, alternative slutter og en spillbar bortføringsverden utsettes. Ingen motormigrering er nødvendig.

## 13. Akseptansekriterier for senere implementering

- **Historie:** En spiller som følger hovedruten kan forklare forskjellen på et avtrykk og en aktør som reagerer nå, og nevne minst to uavhengige tegn på en større forberedelse. Ingen valgfri radiosamtale er eneste kilde.
- **Loop:** Hvert nytt hovedoppdrag har en fysisk handling og minst ett funn som endrer en senere beslutning. Panelbruk og returkjøring måles; fjern trinn som bare gjentar en bekreftet slutning.
- **Skyggemann:** Ingen synlig flytting ved vanlig blikk, periferi, lykt, zoom eller fotosøker. Ingen tilnærming under panel/dialog/pause. Usett flytting krysser ikke solide sperrer. Introduksjonen kan overses uten å låse historien.
- **Inntrenger:** Oppdagelse har dokumentert syns- eller lydårsak. Brutt kontakt virker. Den kjenner ikke spilleren gjennom vegger. Begge ruter kan fullføres med ordinære kontroller, også etter at en avledning er brukt.
- **Lys og dekning:** Vindu, dør, vegg og avskjerming har fysisk samsvar mellom det som sees og det som registreres. High/Low og Clean/filmprofil viser samme nødvendige signaler. Ingen blendende fullskjermseffekt er eneste varsel.
- **Flukt:** Første oppdagelse gir en rimelig reaksjonsmulighet. Å gjemme seg kan lykkes og avslutte søket. Suksess utløser ikke en ny obligatorisk fangst. Ingen låsbar blindvei blir eneste vei videre.
- **Lagring:** Ny sak, gammel sak, før møte, støttet midtfase, etter flukt og etter fangst prøves med faktisk disk-Continue. Foto, dører, valgt variant og engangseffekter bevares. Ingen klokke går bakover.
- **Rytme:** Nora, kartarbeid og etterarbeid får ro. Konkurrerende hendelser holdes tilbake. Nye jumpscares vurderes mot hele nattens eksisterende hendelsesliste.
- **Finale:** Varslingen og reisen har et tydelig formål; C er fortsatt kartlagt, bilen stanser ufrivillig, og THE EVENT beholder sine 47 sekunder. Det fulle 1947-bildet er fortsatt samme bilde med ulike utsnitt.
- **Testomfang:** Kjør bygg og nødvendige selvtester når kode endres, med et kort pakket utvalg for endret loop, foto, pause, lagring, gammel kapittelrute og finale. Utvid bare ved konkret risiko eller feil. Dokumentgjennomgangen her krever ingen GPU-jobb.

Første menneskelige prøve kan være fem personer uten kjennskap til løsningene. Registrer faktisk tid, fastlåste steder, oppdagelser, fangster, venting og forstått neste mål. Spør: «Hvorfor fant den deg?», «Hva hjalp deg ut?», «Når ble du urolig?» og «Når ble det irriterende?». Frykteffekt og målvarighet er UNVERIFIED frem til dette er prøvd. Kamera-/API-scenarioer alene beviser ikke opplevelsen.

## 14. Lokale kilder og status for denne leveransen

- [Gjeldende tidslinjekontrakt](EVENING_TIMELINE.md), [runtimeåpningen](EVENING_RUNTIME.md) og [kveldspakkens integrasjon](Narrative/EveningShift/INTEGRATION.md).
- [Anomaly-planen](ANOMALY_PLAN.md), særlig del 13, [første runtime-pilot](ANOMALY_RUNTIME.md) og [skumlere pilot](ANOMALY_SCARIER.md).
- [Historien med én slutt](../web/HISTORIE.md), brukt som historisk kanonreferanse med senere Unreal-oppdateringer. [Designbibel fra september](../Docs/DesignBible13/design-bible.md) er eldre og overstyrer ikke oktoberhistoriens slutt.
- `Signal47/Source/Signal47/Private/S47Chapter1.cpp` til `S47Chapter6.cpp`, `S47Curve.cpp`, `S47Night.cpp`, `S47Abduction.cpp`, `S47World.cpp`, `S47Walker.cpp`, `S47FieldCamera.cpp`, `S47Anomaly.cpp`, `S47Glimpses.cpp` og `S47SaroDark.cpp`.
- [Feltmodellstatus](FIELD_MODELS.md): native modeller og eksisterende interaksjoner må skilles fra nye oppdragssekvenser.

Dokumentleveransen er kontrollert for konsistens, lenker og diff. Ingen ny spillkode, modell, lydproduksjon, bygg, pakke eller installasjon inngår. Dette dokumentet er gjeldende samlet anbefaling for den bestilte videre retningen; eldre planer er bevis og historikk der de avviker. Det endrer ikke automatisk kjørende innhold eller allerede innspilte manuspakker.
