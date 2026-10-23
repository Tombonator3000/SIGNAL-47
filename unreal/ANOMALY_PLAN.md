# Plan for anomaly spotting og UFO-horror i SIGNAL / 47

Dato: 23. oktober 2026. Gjelder bare Unreal-versjonen.

Tom har sluttet seg til retningen med aktiv etterprøving, en nyttigere by og gradvis fremmed tilstedeværelse. Detaljene nedenfor er anbefalt design og rekkefølge for gjennomføring. De er ikke implementert. Spillet skal gi spilleren stadig bedre forståelse av hvordan et avvik kan undersøkes, samtidig som det blir vanskeligere å føle seg trygg på hva som observerer dem.

Første produksjonsmål er to bybesøk med aktive sammenligninger og bevis som følger samme sak. Begge delstrekninger prøves med ekte input før samme opplegg utvides gjennom resten av natten. Hele nattens transport- og tidskontinuitet må deretter prøves uten testhopp.

## 1. Spillerens rolle og historiens rammer

Spilleren er en nattoperatør som blir nødt til å etterforske sitt eget arbeidssted og omgivelsene. Et vellykket funn skal komme fra noe spilleren faktisk oppdaget, dokumenterte og sammenlignet. Ny informasjon skal gi en forstått grunn til neste handling.

Fem prinsipper styrer endringene:

1. Spilleren må få lære en normaltilstand før en endring krever hukommelse.
2. Undersøkelser må kunne skille vanlige feil fra uforklarlige funn.
3. Hvert sentralt sted i byen må bidra med en egen type kontroll eller bevis.
4. Opptrapping betyr større sikkerhet, nærhet og personlig betydning, med pustepauser mellom toppene.
5. Viktig fremdrift må ha en pålitelig vei videre selv om en valgfri hendelse overses.

Følgende rammer beholdes:

- Én natt i New Mexico, natt til 14. april 1986, og én slutt.
- THE EVENT begynner 05:29:00 og varer til 05:29:47. Gryet og soloppgangen beholder sin eksisterende plass.
- 1947, Nora, Tomás, den kuttede kabelen, de negative avstandene og C er historiens ryggrad.
- Et avtrykk gjentar et registrert forløp og kan ikke svare på nye spørsmål. Levende vitner kan reagere på det spilleren viser dem.
- Den tredje personen i 1947-bildet skal ikke forklares som en tilfeldig alien. Bildet må fortsatt bære forbindelsen til spilleren og slutten.
- 1420.405 MHz, 27 antenner og azimut 026 beholdes. Byens 1420 AM er en annen frekvens og funksjon.
- A gjennom byen og C ved den gamle veien er forskjellige linjer. Byens observasjoner skal ikke alene løse hvor C krysser veien.
- Betydningen av 47 sekunder konsentreres om de viktige hendelsene. Nye små avvik får andre varigheter.
- Spilltekst er engelsk. Plan og arbeidslogg er norsk.

Kanonens grunnlag er `../web/HISTORIE.md` med oppdateringene i oktober, kontrollert mot den nåværende Unreal-koden. Web- og Unity-versjonene videreutvikles ikke.

## 2. Undersøkelsen som går igjen gjennom spillet

Den gjennomgående handlingen er:

**Utfør en vanlig oppgave → legg merke til et mulig avvik → dokumenter → velg en kontroll → sammenlign → følg opp resultatet.**

Et eksempel er å fotografere et merke, ta en kontroll med annen belysning og oppdage at det samme merket fortsatt finnes på filmen. En annen undersøkelse kan bruke en bergingskvittering og en sjåfør som uavhengige kilder.

Første læringssituasjon bruker dagens B-12- og fotolabarbeid. Deretter gir byen spilleren anledning til å anvende det samme prinsippet på andre ting. Kamera, album, dokumenter, saksbrett og samtaler får tydelige roller gjennom hele natten.

### Registrering og tilbakemelding

En automatisk lapp kan fortelle hva spilleren hørte eller så. Den skal ikke konkludere med at et fenomen er bekreftet.

I notatboken kan spilleren knytte et bilde, et dokument eller en eksisterende avskrift til en observasjon. Deretter tilbys en konkret sammenligning, for eksempel «Compare position» eller «Compare the timestamps». Spilleren peker ut forskjellen eller velger «Insufficient evidence». Første versjon bruker et lite utvalg slike handlinger i dagens grensesnitt.

Et funn kan ende som:

| Resultat | Tilbakemelding | Hva spilleren kan gjøre videre |
|---|---|---|
| Vanlig forhold | En konkret forklaring, som planlagt radioslutt eller kjent lampefeil. | Avslutte sporet og bruke det som kontrollreferanse. |
| Teknisk feil | Kontrollen viser en feil som kan rettes eller isoleres. | Gjenta målingen etter retting. |
| Uavklart | Dokumentasjonen støtter mistanken, men mangler en kontroll. | Følge et sted, vitne eller dokument som kan hjelpe. |
| Etterprøvd avvik | To relevante kilder eller en kontrollert prøve viser noe som ikke stemmer. | Åpne et oppfølgingsspørsmål, en sammenligning eller et fysisk etterspill. |

Spilleren får ingen usynlig poengsum for å gjette. En feilrapport ødelegger ikke saken og koster ikke liv. Samme rapport kan korrigeres uten å samle alt på nytt. Tilbakemeldingen sier hva kontrollen viste, og lar årsaken være åpen når den faktisk er ukjent.

Målteksten skal uttrykke undersøkelsens hensikt. Forslag til engelsk spilltekst er «Check whether Caliche lost power before the SARO printout», «Compare the shop window with your earlier photograph» og «Use the records to locate where C crosses the road». En oppgave er ferdig når spilleren har en begrunnet vurdering eller har fastslått hva som fortsatt mangler. En ny dokumentknapp alene er ikke en ny gameplayhandling.

### Tre typer avvik

| Type | Hva spilleren lærer å kontrollere | Eksempel |
|---|---|---|
| Sted og plassering | Sammenligne en kjent detalj fra samme ståsted. | En gjenstand er et annet sted, eller en form dekker bestemte stjerner. |
| Tid og gjentakelse | Sammenligne dato, rekkefølge og identiske lydfragmenter. | Et opptak finnes før hendelsen; en setning og host gjentas likt. |
| Tilstedeværelse | Sammenholde direkte syn, film, lyd og fysiske spor. | En skikkelse finnes på bildet, og senere observeres noe tilsvarende ute. |

Fotografier må kunne brukes som støtte til hukommelsen. Hele spillet skal ikke kreve at spilleren husker små endringer uten å ha fått en rimelig sjanse til å registrere normalen.

## 3. Revidert reise gjennom natten

Det første bybesøket flyttes frem i forhold til det første muntlige forslaget. Det legges i The Second Exposure eller tidlig The Amended Record, med ønsket ankomst omtrent 02:40–02:55. Dette er et mål for fortellingen, ikke en nedtelling spilleren må rekke.

Trucken kan allerede brukes gjennom fri kjøring fra ch1 etter prologen. Det trengs likevel ny oppdrags- og returhåndtering. Å kjøre til Caliche skal ikke aktivere All Night, fullføre gjeldende kapittel eller gi tilgang til den gamle veiens avslutning.

| Del | Spillerens viktigste handling | Ny oppbygging | Overlevering til neste del |
|---|---|---|---|
| Night Shift, 23:41–02:16 | Lære normal drift, håndtere feil og følge registreringene. | Én forståelig feil gir trygghet i prosedyren; tidsrekkefølgen begynner å svikte. | Et konkret registreringsproblem fører til feltkontroll. |
| The Second Exposure, 02:16–02:55 | Gjennomføre kontrollene i gården og fotolaben. Første korte bytur legges etter at kameraet er lært. | Filmen beholder en referanse. I Caliche møter spilleren en lokal normalitet og henter en uavhengig kontroll. | En tids-/driftsopplysning tas tilbake til samme SARO-undersøkelse. |
| The Amended Record, 02:55–03:35 | Sammenligne rapportversjonene og opplysninger fra byen. | Instrumentforklaringen blir svakere. Noen har fjernet C med vilje. | STATION 01 blir et begrunnet mål. Ved lang første bytur kan returen inngå her. |
| The Survey Station, 03:40–04:30 | Kjøre, undersøke kabelen og knytte stemmen til feltdokumentene. | En fjern form eller lysgeometri får et fysisk sammenlignbart spor. Tomás' avtrykk følger sin låste sekvens. | Dokumentasjon og Nora fører til Room 6. En eventuell byavstikker er valgfri. |
| Room 6, 04:35–05:00 | Legge frem bevis og forstå Noras handling. | Den kosmiske hendelsen får menneskelig kostnad. Etter samtalen kan en fremmed skikkelse observeres på avstand. | Spilleren forstår hvorfor C og den gamle veien må undersøkes. |
| All Night, 05:00–05:20 | Gjenbesøke byen, etterprøve et funn og samle kart/vitner i dineren. | De kjente stedene har endret seg. Et større himmelfenomen og personlige gjentakelser øker nærheten. | P13 og sjåførens bekreftelse peker på riktig kryss. Spilleren velger selv å sette i gang siste kjøretur. |
| Roswell Road, 05:20–05:29 | Kjøre mot det utledede punktet. | Tidligere lydfragmenter faller sammen med det som nå skjer rundt trucken. | Den eksisterende hendelsen utløses ved C. |
| THE EVENT, 05:29:00–05:29:47 | Observere fra trucken slik den eksisterende finalen er skrevet. | Spillerens rolle som observatør snus. | Én slutt og fotografiets eksisterende etterspill. |

### Første bybesøk

SARO trenger en uavhengig kontroll av hva som skjedde rundt registreringen 02:13. Et nytt, kort oppdrag ber spilleren sammenligne tidspunktet med nattlukas driftsjournal og en bevart opplysning fra Night Line. Dette gjelder hendelsesrekkefølge og klokkeslett; det påstår ingen frekvenslikhet mellom AM-radioen og SAROs mottaker.

Spilleren gjør to meningsfulle handlinger: undersøker journalen ved bensinstasjonen og får en tidsfestet sending/avskrift hos Earl. På veien registreres tre referanser: suvenirutstillingen, gatetelefonen og studioets normale drift. Bruk kameraet og notatboken spilleren nettopp har lært.

Journalen og den nye korte Earl-utvekslingen må skrives og produseres. De finnes ikke som dette oppdraget i dagens spill. Ikke legg den nye kontrollen på en gammel, valgfri reellånsregel uten å kontrollere forutsetningene.

Baselinen beskyttes mens referanseoppgaven pågår: pappfiguren står i utstillingen, og gateautomaten spiller ikke sin store anomalihendelse før normalbruken er etablert. Andre gamle triggere må omfattes av samme scenehensyn. Etterpå kan den første eksisterende telefonhendelsen bli tilgjengelig som valgfritt tillegg; ingen må stå og vente på den.

Ved retur er samme SARO-oppgave fortsatt aktiv. Spilleren sammenligner materialet og får en kort, nøktern konklusjon om hva kontrollen utelukker. Ved lang tidsbruk går neste kapittel aldri bakover i klokketid.

### Gjenbesøket i All Night

Bybesøket starter mens spilleren fortsatt er til fots før den avsluttende kart-/veibeslutningen. Spilleren velger ett av flere kjente spor å etterprøve. Den nye hovedreisen presenterer én pålitelig tilgjengelig sammenligning før P13. «Insufficient evidence» er et gyldig avslutningsresultat. Ingen bestemt anomalirapport eller valgfri telefon blir et ekstra krav for å kjøre videre; øvrige observasjoner gir mer kontekst og kan følges opp frivillig.

Dineren forblir samlingsstedet for kartgåten, avisen og vitnene. Dersom spilleren vil gå videre, skal det være tydelig hva som allerede er tilstrekkelig for P13, og hvilke spor som fortsatt er valgfrie. Ingen skjult kvote på to eller tre tilfeldige anomalier sperrer den gamle veien.

Undersøkelsene påvirker hvilke sammenhenger spilleren forstår, hva som ligger i saken og hvilke vitner som kan følges opp. Slutten beholdes. Oppdragsteksten skal gi en troverdig grunn til å kontrollere C fra veien; den skal ikke love at grundig dokumentasjon lar spilleren velge en annen slutt.

### Tre historier som gradvis blir én

SARO-sporet handler om en institusjon som har gjort avvik til vedlikeholdsfeil. Gjentatte relébytter og den endrede rapporten viser hva som ble gjort for å holde driften i gang. Spilleren kommer videre ved å kontrollere en forklaring som først virker rimelig.

Bysporet handler om folk som lever med hendelser de har gitt hverdagslige navn. Et nytt batteri, en uvanlig lang bergingstur eller en merkelig radiosamtale kan hver for seg avskrives. Spilleren ser sammenhengen fordi de har adgang til både SAROs materiale og byens erfaringer.

Det personlige sporet begynner med at spilleren samler referanser og slutter med at spilleren selv blir en referanse. De sene frempekene skal kunne gjenkjennes etterpå. De skal ikke tidlig fortelle at en bestemt skikkelse i et gammelt bilde er spilleren. Den tredje personen og bildets eksisterende avsløring bevares.

Disse sporene kobles gjennom handling: journalen fører tilbake til utskriften, kvitteringen fører til sjåføren, kartene fører til C, og den kjente motorlyden fører frem mot finalen. Et nytt dokument må tilføre en slik forbindelse eller et menneskelig perspektiv. Forklaringer på alt det ukjente holdes tilbake.

De levende personene får ulike roller. Earl hjelper med hva som faktisk ble sendt; sheriffen hjelper med hvor hendelser ble rapportert; ekspeditøren gir hverdagslig trygghet og en tvetydig gjentakelse. Nora gir den menneskelige kostnaden og en konkret erfaring med kabelen. Servitrisen og sjåføren gir et siste jordnært holdepunkt. Ingen av dem trenger en monolog om utenomjordiske skapninger.

## 4. Byens funksjoner og tilgjengelighet

| Sted | Første besøk | Senere bruk | Forutsetning og alternativ |
|---|---|---|---|
| Bensinstasjonen | Undersøke ny driftsjournal og møte ekspeditøren. | Sammenligne bergingskvitteringen, den gamle veien og eventuelt gjentakelsen. | Ekspeditørens identiske replikk krever minst to timer siden første møte i dagens kode. Det er et valgfritt spor. |
| 1420 AM | Earl tidsfester en bevart sending/avskrift. Normal studioaktivitet er synlig. | Spilleren bruker materialet som allerede ble tatt med, og kan undersøke studioet uten vert. | Earl er tilgjengelig før 04:00. Et sent førstebesøk får en fysisk, datert avskrift ved et nytt utleveringspunkt; ingen NPC blir værende kunstig. |
| Gatetelefonen | Vanlig ringetone og myntbruk blir kjent. | Printerlyden, 1947-mynten og senere motorfragment kan etterprøves. | Første hendelse slutter å være tilgjengelig 04:45. Den senere krever tidligere hendelse og er tilgjengelig til 05:19. Ingen av dem er eneste vei gjennom hovedhistorien. |
| Sheriffkontoret | Kartet kan observeres uten å love en samtale. | Etter 04:30 kan sheriffen kommentere relevante faktiske observasjoner. Egne markeringer sammenlignes med kartet. | Sheriffen er ute 02:45–04:30. Kartet og etterlatte opplysninger må kunne brukes når døren er låst. |
| Suvenirbutikken | Spilleren tar eller undersøker et referansebilde av utstillingen. | Figurens plassering, prismerke og fysiske spor sammenlignes. | Førstegangsreferansen må være registrert før en hukommelsesoppgave. Gamle direkte-spawn må tilpasses. |
| Vaskeriet | En fungerende maskin og lysrøret gir en vanlig rytme. | Valgfri lyd-/rytmekontroll og et sted for fysisk etterspill. | Kvinnen går 04:50. Sen hovedfremdrift kan ikke kreve henne. |
| Mesa Diner | Utsiden kan kjennes igjen på første tur; kapittel 5-historien holdes tilbake. | Avis, sjåfør, servitrise og P13 samler trådene. Dinertelefonen har Ward. | Tidlig besøk må ikke sette ch5-flagg. Beskytt kartarbeidet mot konkurrerende skremmehendelser. |

Suvenirmannen går 04:10. Valgfrie tidlige vitnesamtaler gir senere kontekst, men skal ikke gjenoppstå som nødvendige møter etter fem. Kino, kirke og fôrbutikk beholdes som stedsankre i første produksjonsrunde. Nye fullverdige interiører prioriteres først når de gir en nødvendig handling.

## 5. Foreslåtte undersøkelser og hendelser

ID-ene nedenfor er planreferanser, ikke eksisterende runtime-ID-er. «Utvidelse» betyr at deler finnes, men at den beskrevne spillerhandlingen må bygges.

| ID | Innhold og spillerens handling | Kontroll og tilbakemelding | Rolle |
|---|---|---|---|
| N01 | Første bytur: sammenligne driftsjournal, sending og SARO-utskrift. | Vis rekkefølgen uten å forklare fenomenet. Registrer byens normalreferanser. | Ny kort hovedsekvens. |
| N02 | En svak lampe eller refleks ser ut som en skikkelse. Spilleren endrer ståsted og tar en kontroll. | Formen følger den vanlige lyskilden og kan forklares. | Nytt, kort kontrolltilfelle; viser at alt merkelig ikke er et avvik. |
| A01 | B-12-merket fotograferes under de eksisterende kontrollene. | Nullprøve og fremkalling beholder betydningen. | Eksisterende hovedsekvens; mal for resten. |
| A02 | Spilleren sammenligner suvenirfigurens prismerke og skader med referansefotoet etter at plasseringen endrer seg. | Samme gjenstand, annet sted; hvem som flyttet den er fortsatt uavklart. | Utvidelse av pappalien-sporet. |
| A03 | Gateautomaten spiller SARO-printeren og gir en 1947-mynt etter fullført samtale. | Sammenlign kjent printerlyd og mynten med et levende vitnes opplysninger. | Eksisterende hendelse; utvid aktiv etterprøving. |
| A04 | Ekspeditørens ord, avbrudd og host gjentas etter minst to timer. | Sammenlign med en avskrift fra første møte. Det gir et uavklart mønster, ikke bevis for at mannen er et avtrykk. | Eksisterende spor; valgfri fordypning. |
| A05 | Et fjernt område på himmelen dekker kjente stjerner. Spilleren registrerer ståsted og sammenligner to bilder. | Skillet mellom filmfeil, landskap og en himmelform blir lesbart. | Ny fjern UFO-observasjon i feltetappen. |
| A06 | Lys og hundelyder går gjennom byen i en bestemt rekkefølge. Spilleren markerer observasjonen på kartet. | Et relevant ettersport kan undersøkes; retningen tilhører byens motiv og gjør ikke A til C. | Utvidelse av eksisterende bølge. |
| A07 | En skikkelse opptrer i et dokumentert utsnitt og kan senere observeres på avstand ute. | Samme gjerde, bygning eller stolpe gir målestokk. Delvis dekning og en begrenset bevegelse gjør figuren fysisk. | Sen sekvens med den egenlagde alienmodellen. |
| A08 | Etter Room 6 kan den senere gatetelefonen spille motoren som hoster og stopper. | Sammenlign med feltradioens kjente fragment. Begge er låste opptaksforløp. | Eksisterende frempek; valgfri personlig opptrapping. |
| A09 | En større mørk form over mesaen kan ses fra en opparbeidet siktlinje i All Night. | Kjente hustak og stolper gir størrelse; et bevart bilde gir spor hvis observasjonen ble kort. | Ny sen UFO-observasjon; ingen loreforklaring. |
| A10 | Dinerens bredere avisbilde sammenlignes med saksmappens utsnitt. | Den eksisterende tredje personen og himmelen får gradvis betydning. | Eksisterende sentralt spor; bevar sluttreferansen. |
| P01 | Etter bildesammenligningen høres en kort, kjent printerrytme fra det stengte studioet. Spilleren kan gå nærmere og undersøke den normale fasaden. | Det gir et spørsmål om lydens kilde, uten å bekrefte at et nytt vesen finnes i rommet eller starte en samtale. | Nytt, avgrenset etterspill i første prototype. |

Kontrolltilfeller skal være meningsfulle. Studioets normale avslutning 04:00, vanlig morgenlys og at noen går hjem er ikke automatisk anomalier. Ett eller to slike tilfeller tidlig gir spilleren grunn til å teste en mistanke. De skal ikke fylle byen med vilkårlige lureoppgaver.

### Eksempel på gjenbesøk

Spilleren kjenner suvenirutstillingen. På returen er figurens plassering endret. De tar et bilde og sammenligner prismerket med første foto. Dermed vet de at det er samme pappfigur. Hos ekspeditøren får de en opplysning som kan kontrolleres, uten at noen forklarer fenomenet.

I første prototype følger P01, printerrytmen fra det stengte studioet, etter undersøkelsen. Den begynner først når panelet er lukket og ingen annen viktig hendelse eller samtale pågår. Den er kort og bruker eksisterende lyd; ingen ny figur eller jakt kreves.

I trinn 2 kan lys-/lydbølgen knyttes til en senere undersøkelse og gi en siktlinje mot et fjernt objekt. Da må kapittelbetingelsen, dagens grense 04:55 og daggryslukking endres samlet. Hvis bølgen allerede er opplevd eller lysene har slukket, beholdes den historikken og et undersøkbart etterspill brukes. Hendelsen skal ikke gjentas bare for å fylle sekvensen.

Spilleren går inn i dineren med et konkret spørsmål og et dokumentert funn. Der får de ro til å sammenholde byen, avisen og kartene før P13. Et svakt kjent lydfragment ved trucken kan avslutte pausen og føre tilbake til siste kjøretur.

## 6. Opptrapping av UFO-er og aliens

Fremmed tilstedeværelse øker langs tre akser: hvor sikkert den er observert, hvor nær den kommer, og hvor tydelig den angår spilleren.

| Nivå | Opplevelse | Utførelse |
|---|---|---|
| 1. Mulig feil | «Jeg kan ha misforstått dette.» | Kontrollerbare lampe-, film- og instrumentforhold. Ingen tydelig alien. |
| 2. Etterprøvbart avvik | «Det samme finnes i to kilder.» | Fotografier, utskrifter, avskrifter og vitner samsvarer på en ubehagelig måte. |
| 3. Fysisk tilstedeværelse | «Det er noe her ute.» | Stjerner dekkes, lys svikter lokalt, en form har størrelse og delvis skjuling bak landskapet. |
| 4. Nær observasjon | «Noe har sett meg.» | En sen skikkelse ved gjerdet eller siktlinjen. Begrenset bevegelse, et lesbart forhold til omgivelsene og et etterspill. |
| 5. Personlig sammenfall | «Opptaket handler om det som skjer med meg.» | Motor, radio og eksisterende registreringer faller sammen på Roswell Road. |

Den egenproduserte alienmodellen brukes først til en begrenset sekvens med idle, én orientering og en enkel tilbaketrekning bak faktisk dekning. Kilden i Blender og den gamle referansemodellen bevares. Gangen og overgangen må animeres og prøves; eksportert idle alene er ikke ferdig fiendeatferd.

Første produksjonsrunde trenger ingen kamp, stor jakt-AI eller mange alienvarianter. Senere kan én kort trusselsituasjon prøves dersom den har tydelige regler og tilfører en ny handling. Eksisterende bortføringer, opptaksvandring og glimt må vurderes samlet med den nye stigen, slik at de ikke avslører nivå 4 før spilleren har lært nivå 1–3.

De fremmede skal ikke forklare C, Noras valg eller hele fenomenet i dialog. Tilstedeværelsen kan bli tydelig mens årsakssammenhengen forblir åpen.

## 7. Rytme, lyd, lys og eksisterende rekvisitter

`S47Curve` og regelen om én større hendelse om gangen beholdes som felles koordinator. De utvides med kunnskap om normalreferanser, besøk og gjennomførte kontroller. Det lages ikke en konkurrerende tilfeldig skremmemotor.

Skillet mellom aktivitet og intensitet er viktig. Rolig arbeid og utforskning kan vare lenge når spilleren vet hva de prøver å finne ut. Passiv venting på at en tilfeldig hendelse skal starte, skal ikke være nødvendig for fremdrift.

For første prototype prøves følgende rytme:

- En undersøkelse gir en liten respons eller ny opplysning innen en forståelig arbeidssekvens.
- Ingen obligatorisk fremdrift krever mer enn to minutters ren venting. Kjøring, lesing og selvvalgt undersøkelse regnes separat.
- Ett større opptrinn får et etterspill før neste topper. Eksisterende 8–12 minutters rest er en hypotese som vurderes mot faktisk aktivitet, ikke en verdi som halveres overalt.
- Lesing, samtaler, Room 6 og kartarbeid beskyttes. Permanente spor kan undersøkes etterpå. Ingen viktig endring skjer ubemerket bak et modalpanel og regnes deretter som oppdaget.
- Minst én kjent trygg arbeidssekvens og en tydelig pustepause i dineren må overleve helt frem til slutten.

Lyd etablerer først vanlige referanser: rør, kjøleskap, printer, radio og motor. Et låst avtrykksfragment beholder sin identitet. Subtil endring i avstand og romklang kan skape uro uten at alle hendelser får et høyt lydstøt.

Lys skal gjøre observasjonen mulig. Bruk stabile armaturer og kjente siktlinjer før et avvik. Senere kan et avgrenset lysområde eller en skygge endres. Fast eksponering og støtte i foto/tekst må hindre at en oppgave blir en test av skjermens svartnivå. Kontroller også VHS/Clean og lavere grafikkinnstillinger.

KELVORA-TV-ene spiller fortsatt levende Unarius-film med lyd. Filmenes normale løp er en stemningsreferanse, og et eventuelt avbrudd håndteres av dagens film-/strømkontrakt. En film erstattes ikke med et stillbilde for å simulere avspilling. Et bevart fotografi av en TV-hendelse er et eget bevis og skal presenteres som det.

RANGER-radioen kan gi en kjent lyttestasjon og referanselyd. Den blir ikke automatisk en ny bærbar opptaker. Første prototype bruker eksisterende opptak og avskrifter; en fri lydopptaker er en egen funksjon med egne kostnader og inngår ikke i grunnomfanget.

## 8. Unreal-implementering og lagring

### Ansvar i dagens systemer

| System | Anbefalt endring | Grense |
|---|---|---|
| `S47Chapter1` / `S47Chapter2` | Introdusere første kontrolltur, ta imot materialet og gjenoppta samme SARO-steg. | Ikke starte ch5 ved tidlig ankomst til byen. |
| `S47World` | Egen oppdragsankomst og retur med kontrollert tidsregning og eksisterende kjøring. | Ikke gjenbruke `ArrivedDiner()` som generell ankomst eller resette kapittelklokka. |
| `S47TownLayer` / `S47TownEvents` | Besøksstadier, referanser, fysiske varianter og etterspill. | Ett fysisk objekt og én eier per tilstand; ingen doble triggere. |
| `S47Caliche` | Nye korte journal-/tidsreferanser og målrettet fremleggelse av bevis. | Respektere arbeidstid, eksisterende vitnespor og skillet mellom levende vitne og avtrykk. |
| `S47Curve` / `S47Night` | Bruke eksponering og undersøkelsesstatus sammen med kapittel og nattklokke. | Beholde én større hendelse om gangen og viktige pauser. |
| `S47Seen` / `TownSeen` | Registrere faktisk eksponering separat fra spillerens etterprøving. | Gamle `ev:*`-markører alene betyr ikke at en sammenligning er løst. |
| Kamera, album, notebook og board | Knytte et lite antall bevis til konkrete sammenligninger. | Gjenbruke grensesnittet; ikke kreve generell friobjektgjenkjenning. |
| `S47Chapter5` | Koble byens funn inn før kart-/veivalget. | P13 beholder sin funksjon. En byrapport kaller aldri `Finish()` eller setter `road`. |
| `S47Flow` / `S47Saves` | Lagre referanser, variant, undersøkelsesstatus og rekkefølge. | Gamle saker skal kunne lastes uten tvungen omstart. |

### Tilstander og data

Et lite sett eksplisitte undersøkelsesdata lagres sammen med saken. Bruk prosjektets JSON-mønster og en versjonert utvidelse. Endelig feltnavn velges ved implementering.

| Data | Formål |
|---|---|
| ID, kapittelkrav og relevans | Gjør hendelsen sporbar og hindrer feil rekkefølge. |
| Normalreferanse og eventuell foto-ID | Viser hva spilleren faktisk fikk anledning til å lære. |
| Besøk og første møteklokke | Styrer gjenbesøk og tidsavhengige samtaler. |
| Aktiv variant og valgt startbetingelse | Hindrer ny trekning ved lasting. |
| Eksponering | Skiller aktivert, hørt, sett og oversett. |
| Bevislenker, spillerens vurdering og kontrollresultat | Gjør etterprøvingen vedvarende og mulig å korrigere. |
| Etterspill og fullført/avsluttet | Hindrer dobbel belønning eller gjentatte topper. |

Eksponering og løsning er separate forhold. Et fotografi kan oppdages i albumet selv om spilleren ikke så skikkelsen direkte. En scene kan avsluttes med et undersøkbart etterspill selv om selve bevegelsen ble oversett. Et bevis forsvinner ikke når den midlertidige hendelsen er over.

Ved lasting må objektets plassering, lys, samtaletilgang og fotografiets innhold samsvare. Fotografier skal bevare motivet fra eksponeringstidspunktet; et gammelt bilde skal ikke gjengis med dagens levende scenevariant. Bruk eksisterende bildelagring og kontroller dette eksplisitt.

### Klokke og migrering

Transport i `DriveToDiner` legger i dag til 0,19 sekunder per kjørt meter i tillegg til ordinær tid. En tidlig kontrolltur trenger en uttrykkelig valgt tidsregel. Første implementasjon bruker fri kjøring med vanlig klokketid og en egen oppdragsankomst, eller et avgrenset turvalg med samme regel. Ikke bland de to tidsmodellene utilsiktet.

Det er ingen tidsstraff for å lese eller sammenligne mens spillets eksisterende panelpause er aktiv. Nye byoppdrag og overganger før Roswell Road skal ikke flytte klokka bakover eller forlenge vitnenes åpningstider.

Dagens kapittel 6 har en egen, regissert tidsmodell: avreisen setter 05:20, og kjøreturen tilpasses 05:29. Dette kan allerede gi et tilbakesprang ved et langt All Night-besøk og er ikke løst av denne planen. Pilotarbeidet endrer ikke denne tidsmodellen. THE EVENT beholdes 05:29:00–05:29:47.

Trinn 0 må registrere klokka ved start og slutt på de nye byoppdragene, ved P13 og ved avreise, både med rask rute og lang frivillig utforskning. Prøv særlig avreise etter 05:20 og dokumenter faktisk tidsdiskontinuitet. Hvis den utvidede normalruten overskrider tidsbudsjettet, må innhold forkortes eller flyttes, eller en avgrenset regel for overgangen til finalen avklares før fullnatten godkjennes. Dette skal ikke bortforklares som en ny anomalihendelse.

For eldre lagringer uten normalreferanse brukes tilgjengelig, datert dokumentasjon som støtte til sammenligning. En manglende tidlig oppgave sperrer ikke et allerede påbegynt senere kapittel. Hvis den gamle saken allerede har opplevd pappfiguren eller bølgen, beholdes den historikken og et passende annet spor velges. Ikke skriv om gamle `TownSeen`-funn til nye, fullførte undersøkelser uten grunnlag.

## 9. Feiltilfeller og spillerhjelp

| Situasjon | Ønsket håndtering |
|---|---|
| Første bybesøk skjer etter 04:00 | Earl er gått. Ny fysisk avskrift gir kontrollopplysningen. Normalreferanser etableres før en ny endring kreves. |
| Spilleren går direkte til dineren ved gjenbesøk | Kartarbeidet kan fortsatt begynne. En kort sammenligning med medbrakt dokumentasjon gir den reviderte undersøkelsen uten å sende spilleren på en lang henteomvei. |
| Spilleren overser en skikkelse | Ikke registrer at den er forstått. Et bilde eller et fysisk etterspill kan gi en senere mulighet. Ikke spam samme skremmeøyeblikk. |
| Ingen relevant NPC er til stede | Bruk bevart materiale og de sene vitnene som faktisk finnes. Ingen låst dør blir en skjult fremdriftssperre. |
| Første telefonsamtale aldri ble opplevd | Ikke start samtale nummer to som om den første er hørt. Andre spor bærer undersøkelsen. |
| Lysbølgen er foreslått etter dagens grense 04:55 | Endre betingelsen uttrykkelig for den nye sekvensen. Ikke anta at dagens trigger kjører etter fem. Ved dagslukket belysning brukes et annet etterspill. |
| Spilleren velger feil sammenligning | Gi et konkret hint om manglende kontroll, behold bevis og tillat ny vurdering. |
| Film eller lyd mangler | Vis en tydelig avspillingsfeil. Et nødvendig faktum har en bevart avskrift; ingen usynlig automatisk fullføring. |
| Spilleren trenger hjelp | Gi først et sted-/kontrollhint og tilby deretter en datert referanse. Ikke marker alle avvik på forhånd. |
| Spilleren bruker undertekster eller lavere grafikk | Samme vesentlige informasjon er tilgjengelig. Tekst beskriver relevante lyder uten å røpe en skjult visuell figur før den er oppdaget. |

## 10. Produksjonsrekkefølge og avgrensninger

Hvert trinn omfatter sammenhengende handling, tilbakemelding, lagring og kontroll. Grafikkarbeidet som allerede pågår beholdes som separat produksjon; denne planen starter ingen nye tunge jobber.

| Trinn | Leveranse | Avhengighet | Når trinnet er godt nok |
|---|---|---|---|
| 0. Baseline | Måle dagens rute, klokke, triggere og passive venting med ekte input. Bevare relevante opptak og lagringer. | Nåværende fungerende pakket utgave. | Rute og begrensninger er dokumentert uten å bruke testhopp som bevis på vanlig spillflyt. |
| 1. Første og andre Caliche-besøk | N01, N02 og A02: tidlig kontrolltur, tre normalreferanser, ett forklarlig kontrolltilfelle og én aktiv bildesammenligning. P01 gir et kort etterspill. Retur til riktig historie og bevis gjennom lagring. | Trinn 0 og avgrensede endringer i tur/kapittel/evidens. | Begge delstrekninger og beviskontinuiteten kan fullføres, lagres og forstås av nye spillere. |
| 2. Hele byens etterprøving | Knytte kvittering, sheriffkart, telefon og relevante vitner til funnene. Vurdere en ny, besøksstyrt betingelse for lysbølgen med riktig lampetilstand. P13 samler sporene uten å bli overflødig. | Trinn 1 har lesbar kjernehandling. | Hvert valgt sted endrer spillerens kunnskap eller neste handling; stedslisten er ikke bare samleobjekter. |
| 3. Felt og fremmed tilstedeværelse | Fjern himmelform, én sen aliensekvens og ett større UFO-øyeblikk. Rebalansere gamle glimt/bortføringer. | Referanser, siktlinjer og riktig hendelsesrekkefølge. | Opptrappingen oppleves som økende nærhet gjennom natten; modellen fungerer i pakket gameplay. |
| 4. Hele natten | Kontrollere alle overganger, pauser, etterspill og koblingen til finalen. | Trinn 2–3. | Sammenhengende menneskelig gjennomspilling, regresjon og målinger av påvirkede scener. |

Første Gauntlet-prøve sikter mot 20–30 minutter samlet i to avgrensede delstrekninger. Første del starter før den tidlige kontrollturen og spilles tilbake til SARO. Andre del starter ved All Night fra en testfixture som viderefører den første delens faktiske referanser, bilder og undersøkelsesstatus gjennom vanlig lagring. Tidskuttet merkes tydelig for testspilleren. En slik fixture er testdata og ingen snarvei i den leverte hovedhistorien.

Dette prøver handlingene og beviskontinuiteten, mens trinn 4 må godkjenne hele kapittel-, transport- og tidsforløpet uten testhopp. I full natt ligger andre besøk etter de mellomliggende kapitlene; faktisk total varighet må måles. 20–30 minutter er en målsetting for prøven, ikke et løfte om hele spillets lengde eller en ferdig måling.

Pilotens omfang er fast: N01, N02, A02 og P01. Telefon 1 kan eksistere som et valgfritt sidespor, men kreves ikke. A04–A09, flytting av lysbølgen, en ny lydopptaker og nye UFO-/alienanimasjoner inngår ikke i trinn 1. Dagens lysbølge og grensen 04:55 beholdes i dette trinnet.

Etter denne prøven vurderes om ett sted må kuttes, om sammenligningen er for tung, eller om et mellomliggende spor bør flyttes. Først deretter utvides katalogen. Ingen ny økonomi, sult-/søvnsimulering, stor inventory, kamp eller åpen verdensstruktur inngår i grunnplanen.

## 11. Gauntlet og akseptkriterier

Fem mål brukes med fem nye spillere. Tallene er foreslåtte akseptgrenser for første prøve.

1. Minst 4 av 5 kan forklare handlingen «oppdag, dokumenter, sammenlign, følg opp» uten muntlig instruksjon fra testleder.
2. Minst 4 av 5 oppdager ett avvik og kan etterpå beskrive hva som var annerledes enn normalreferansen.
3. Minst 3 av 5 bruker minst to steder i byen av en forstått praktisk grunn. Ingen trenger tilfeldig spawn eller et utilgjengelig vitne for å fortsette.
4. Begge pilotdelstrekninger og bevisoverføringen kan fullføres med ekte input og lagring/lasting uten feil kapittel, tapte bevis eller endrede fotografier. Ingen obligatorisk ren venting overstiger to minutter. Full natt uten testkutt er et eget krav i trinn 4.
5. Testen viser økende forventningsuro før toppen og en merkbar avlastning i dineren. Registrer ubehag 0–10 ved fire definerte punkter; sikt mot minst 3 av 5 som beskriver denne utviklingen. Bruk forklaringene deres sammen med tallene.

Logg start på normalreferanse, første mistanke, dokumentasjon, kontroll, konklusjon, neste selvvalgte handling og tid uten forstått mål. Frivillig utforskning skal skilles fra å være fast. Spør om ubehag ved naturlige pauser eller etter gjennomspillingen, slik at målingen ikke stadig avbryter opplevelsen.

Teknisk kontroll ved implementering:

- Tilstander og migrering: ny sak, gammel sak, sent førstebesøk, manglende første telefon og allerede opplevd byhendelse.
- Lagring før og etter dokumentasjon, kontroll og etterspill. Gjenåpne fra disk, ikke bare fra minnet.
- Kapittelregresjon gjennom SARO, STATION 01, Room 6, P13, veivalg, THE EVENT og Continue.
- Faktisk filmbevegelse og lyd på TV-ene etter pause, sceneendring og lasting.
- Bildelesbarhet i VHS/Clean og berørte kvalitetsnivåer. Kontroller ekte input og lyd i pakket spill.
- Relevant selvtest og kort scenario-utvalg per kodeendring. Hele prosjektets påkrevde sett før fletting til main.
- Sammenlign ytelse mot trinn 0 med samme kandidat, maskin, oppløsning, preset, rute og varme shadercacher. Første mål er høyst 10 % høyere p95-bildetid i de berørte scenene. Dette er en foreslått regresjonsgrense; absolutt spillbarhet må også vurderes, og vedvarende hakking under selve observasjonen må rettes.

En kodelogikk-test kan vise at en hendelse skjedde. Den kan ikke alene vise at spilleren oppdaget, forsto eller ble urolig av den.

| Gate ved denne planleveransen | Status | Grunnlag |
|---|---|---|
| Planens dekning og kildekobling | PASS | Dagens kapittel-, transport-, by- og observasjonssystemer er lest; fallgruver er tatt inn. Dette er dokumentkontroll. |
| Ny spillerreise i runtime | UNVERIFIED | Planen er ikke implementert. |
| Lesbarhet og opptrapping i pakket spill | UNVERIFIED | Krever produksjon og menneskelig gjennomspilling. |
| Lagring og regresjon for endringene | UNVERIFIED | Krever de nye tilstandene og faktiske tester. |
| Ytelse etter endringene | UNVERIFIED | Ingen ny runtime eller måling i planarbeidet. |

## 12. Kilder og overlevering

Vurderingen bygger på prosjektets nåværende Unreal-kode og historien. Opplevelsesdiagnosen om passiv observasjon og lange pauser er en hypotese fra kodegjennomgang og Toms tilbakemelding. Den er ikke en komparativ spilltest.

Viktige lokale kilder:

- `../web/HISTORIE.md`: natt, kapitler, avtrykk, 1947, valgfrie spor og slutt.
- `Signal47/Source/Signal47/Private/S47Curve.cpp` og `S47Night.cpp`: rytme, eksponering og beskyttelse av viktige sekvenser.
- `Signal47/Source/Signal47/Private/S47World.cpp`: tidlig kjøring, ankomst, tidsregning og transport.
- `Signal47/Source/Signal47/Private/S47Caliche.cpp`, `S47Town.cpp`, `S47TownEvents.cpp` og `S47TownLayer.cpp`: vitner, arbeidstid, telefoner, lysbølge og byspor.
- `Signal47/Source/Signal47/Private/S47Chapter3.cpp`, `S47Chapter4.cpp` og `S47Chapter5.cpp`: turvalg, Room 6, P13 og avreise til gammel vei.
- `Signal47/Source/Signal47/Public/S47Seen.h`: eksisterende skille mellom hendelse og faktisk blikk/eksponering.

Eksterne referanser undersøkt i forrige vurdering:

| Kilde | Hva beskrivelsen støtter | Designlesning brukt her |
|---|---|---|
| [Voices of the Void, utviklerside](https://mrdrnose.itch.io/votv) og [utviklerlogg](https://mrdrnose.itch.io/votv/devlog/1474386/voices-of-the-void-alpha-090) | Signalinnsamling og behandling, vanlige og uvanlige kilder; vedlikehold og fysiske hendelser omtales i loggen. | Vanlig arbeid gir et sted og en rytme som fremmed tilstedeværelse kan bryte inn i. |
| [I'm on Observation Duty, offisiell beskrivelse](https://store.steampowered.com/app/1046820/Im_on_Observation_Duty/) | Observere kameraer, huske normalen og rapportere type/sted for avvik. | En oppdagelse må ha en forståelig handling og respons. |
| [The Exit 8, offisiell beskrivelse](https://store.steampowered.com/app/2653790/The_Exit_8/) | Snu ved avvik og fortsette når det ikke er avvik. | Tydelig undersøkelsesregel kan sameksistere med usikker årsak. |
| [Signal Simulator, offisiell beskrivelse](https://store.steampowered.com/app/839310/Signal_Simulator/) | Signalbehandling, antenner, kalibrering og vedlikehold. | La tekniske kontroller gi spilleren ansvar for konklusjonen. |

Prosjektbiblioteket ble søkt med behov og teknologi, deretter ett bredere søk. Ingen relevant løsning ble gjenbrukt. Brainstorming og gauntlet-loop er brukt fra `../Automation/Skills/`. `game-design` var ikke tilgjengelig som egen skill.

Neste produksjonsarbeid starter med trinn 0 og hele trinn 1: mål baseline, bygg den tidlige kontrollturen og gjenbesøket med aktive sammenligninger, og kontroller reisen før resten av katalogen produseres. Planen endrer ingen kildekode, assets, lagringer, pakket utgave eller standardstarter.
