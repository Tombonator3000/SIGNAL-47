# Plan for anomaly spotting og UFO-horror i SIGNAL / 47

Dato: 23. oktober 2026. Oppdatert med innkjøring før skiftet, Dale med Kinski-stemme, hemmelig kontor, trafostasjon og bedre bybygninger. Gjelder bare Unreal-versjonen.

Tom har sluttet seg til retningen med aktiv etterprøving, en nyttigere by og gradvis fremmed tilstedeværelse. Detaljene nedenfor er anbefalt design og rekkefølge for gjennomføring. Åpningen/tidslinjen og første N01/N02/A02/P01-pilot er siden implementert; [ANOMALY_RUNTIME.md](ANOMALY_RUNTIME.md) gir gjeldende kilde-, pakke- og teststatus. Senere plansteg gjenstår. Spillet skal gi spilleren stadig bedre forståelse av hvordan et avvik kan undersøkes, samtidig som det blir vanskeligere å føle seg trygg på hva som observerer dem.

Tom har valgt at skiftet starter 18:00 søndag 13. april ved ankomst til SARO. Før dette får spilleren en rolig, spillbar kjøretur gjennom byen med lystig radiomusikk og valgfritt matstopp i dineren. Dale er kollegaen som overleverer skiftet på telefon og senere kan nås på telefon eller tjenesteradio/walkie, med prosjektets Kinski-stemme. Han vises aldri fysisk. Nøkkelen til kontoret hans og hans private UFO-undersøkelser blir en del av etterforskningen.

Den første store signalhendelsen anbefales fortsatt flyttet fra 02:13 til 00:13 natt til mandag. En ny trafostasjon gir en vanlig arbeidsoppgave og en senere kontroll av det uforklarlige. Alle bybygningene får en utvendig oppgradering; de viktigste spillestedene får også gjennomarbeidede, synlige eller tilgjengelige rom.

Manuspakken ligger i [Narrative/EveningShift](Narrative/EveningShift/README.md), med engelsk dialog, dokumenter, filmmanus og maskinlesbare betingelser. Innkjøring, telefonoverlevering og N01 er koblet til runtime. Senere kontor-/trafo-/VHS-scener er fortsatt forfatterinnhold.

Dette erstatter tidsplanen fra første planutgave. Første produksjonsmål er en fungerende kveld og to bybesøk med aktive sammenligninger og bevis som følger samme sak. Trafostasjonens to forskjellige besøk prøves som neste sekvens. Hele kveldens og nattens transport- og tidskontinuitet må deretter prøves uten testhopp.

## 1. Spillerens rolle og historiens rammer

Spilleren er en nattoperatør som blir nødt til å etterforske sitt eget arbeidssted og omgivelsene. Et vellykket funn skal komme fra noe spilleren faktisk oppdaget, dokumenterte og sammenlignet. Ny informasjon skal gi en forstått grunn til neste handling.

Fem prinsipper styrer endringene:

1. Spilleren må få lære en normaltilstand før en endring krever hukommelse.
2. Undersøkelser må kunne skille vanlige feil fra uforklarlige funn.
3. Hvert sentralt sted i byen må bidra med en egen type kontroll eller bevis.
4. Opptrapping betyr større sikkerhet, nærhet og personlig betydning, med pustepauser mellom toppene.
5. Viktig fremdrift må ha en pålitelig vei videre selv om en valgfri hendelse overses.

Følgende rammer beholdes:

- Én sammenhengende kveld og natt i New Mexico, fra 13. til 14. april 1986, og én slutt. Skiftstart 18:00 og innkjøringen før dette er valgt av Tom; de er ikke implementert. Flytting av signalhendelsen er fortsatt anbefalt design.
- THE EVENT begynner 05:29:00 og varer til 05:29:47. Gryet og soloppgangen beholder sin eksisterende plass.
- 1947, Nora, Tomás, den kuttede kabelen, de negative avstandene og C er historiens ryggrad.
- Et avtrykk gjentar et registrert forløp og kan ikke svare på nye spørsmål. Levende vitner kan reagere på det spilleren viser dem.
- Den tredje personen i 1947-bildet skal ikke forklares som en tilfeldig alien. Bildet må fortsatt bære forbindelsen til spilleren og slutten.
- 1420.405 MHz, 27 antenner og azimut 026 beholdes. Byens 1420 AM er en annen frekvens og funksjon.
- A gjennom byen og C ved den gamle veien er forskjellige linjer. Byens observasjoner skal ikke alene løse hvor C krysser veien.
- Betydningen av 47 sekunder konsentreres om de viktige hendelsene. Nye små avvik får andre varigheter.
- Spilltekst er engelsk. Plan og arbeidslogg er norsk.

Kanonens grunnlag er `../web/HISTORIE.md` med oppdateringene i oktober, kontrollert mot den nåværende Unreal-koden. Tidsendringene og trafostasjonen nedenfor er uttrykkelige utvidelser av denne historien. Web- og Unity-versjonene videreutvikles ikke; den nye fortellerplanen vedlikeholdes i Unreal-prosjektet.

## 2. Undersøkelsen som går igjen gjennom spillet

Den gjennomgående handlingen er:

**Utfør en vanlig oppgave → legg merke til et mulig avvik → dokumenter → velg en kontroll → sammenlign → følg opp resultatet.**

Et eksempel er å fotografere et merke, ta en kontroll med annen belysning og oppdage at det samme merket fortsatt finnes på filmen. En annen undersøkelse kan bruke en bergingskvittering og en sjåfør som uavhengige kilder.

Kvelden begynner med en enkel kamerareferanse og en forståelig teknisk kontroll. Dagens B-12- og fotolabarbeid blir den første grundige etterprøvingen av et uforklarlig avvik etter midnatt. Kamera, album, dokumenter, saksbrett og samtaler får tydelige roller gjennom hele natten. Tidlig kameratilgang skal ikke fullføre B-12 eller aktivere sene fotohendelser.

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

Målteksten skal uttrykke undersøkelsens hensikt. Forslag til engelsk spilltekst er «Bring back the local service log and a transmission schedule», «Check whether Caliche lost power before the SARO printout», «Compare the shop window with your earlier photograph» og «Use the records to locate where C crosses the road». Den første oppgaven gjøres før hendelsen; sammenligningen med utskriften blir aktuell etterpå. En oppgave er ferdig når spilleren har en begrunnet vurdering eller har fastslått hva som fortsatt mangler. En ny dokumentknapp alene er ikke en ny gameplayhandling.

### Tre typer avvik

| Type | Hva spilleren lærer å kontrollere | Eksempel |
|---|---|---|
| Sted og plassering | Sammenligne en kjent detalj fra samme ståsted. | En gjenstand er et annet sted, eller en form dekker bestemte stjerner. |
| Tid og gjentakelse | Sammenligne dato, rekkefølge og identiske lydfragmenter. | Et opptak finnes før hendelsen; en setning og host gjentas likt. |
| Tilstedeværelse | Sammenholde direkte syn, film, lyd og fysiske spor. | En skikkelse finnes på bildet, og senere observeres noe tilsvarende ute. |

Fotografier må kunne brukes som støtte til hukommelsen. Hele spillet skal ikke kreve at spilleren husker små endringer uten å ha fått en rimelig sjanse til å registrere normalen.

## 3. Revidert reise gjennom kvelden og natten

Skiftet starter **18:00 søndag 13. april 1986 ved ankomst til SARO**, etter Toms valg. Den spillbare innledningen begynner før dette, på vei til jobben. Spilleren får lære Caliche som et vanlig sted med folk og radiomusikk. Tidligere start skal også spre undersøkelsene utover natten. En lang innledning foran den gamle, tettpakkede avslutningen vil ikke løse tidsproblemet.

Som lysreferanse gir USNO ved Roswell, 33,39 N / 104,52 V og MST, solnedgang omtrent 18:27 og slutt på borgerlig skumring 18:53 den 13. april. Det gjør en start 18:00 egnet for en synlig overgang til natt. Den fiktive SARO-horisonten og lysforløpet må kalibreres i spillet. Dette brukes ikke til å flytte finalens klokkeslett. [USNO-beregning](https://aa.usno.navy.mil/api/rstt/oneday?date=1986-04-13&coords=33.39%2C-104.52&tz=-7&dst=false).

Den første store residualen anbefales flyttet fra 02:13:47 til **00:13:47 mandag 14. april**. Datoen for nattens hovedregistrering beholdes, mens nesten to timer frigjøres mellom etterforskningens deler. Telefonens forløp i forkant og de øvrige relative sammenhengene følger hendelsen. Finalen beholdes 05:29:00 til 05:29:47.

Dette er en foreslått revisjon av historien, ikke dagens implementerte tider. Klokkeslettene nedenfor angir fortellingens kveld og natt, ikke antall timer faktisk spilletid. Reisetid, meningsfull spilltid og fortellerklokke skal måles hver for seg før vinduene låses.

| Del | Foreslått tid | Hva spilleren gjør | Oppbygging |
|---|---|---|---|
| På vei til skiftet | Før 18:00 | Kjøre gjennom Caliche med lystig radio; kunne parkere ved dineren, kjøpe mat og kjøre videre til SARO. | Normal kveld, mennesker og kjørerute. Ingen påtvunget anomalihendelse. |
| Overlevering og første rutine | 18:00–19:00 | Få telefonoverlevering fra Dale, tilgang og instruks til vanlig drift, lære kameraet og sjekke stasjonen. | En troverdig kollega og et arbeidssted å bli kjent med. Dales eget kontor holdes låst. |
| Night Shift, første del | 19:00–20:00 | Starte målinger og samle N01s ordinære driftsjournal/sendeskjema. | Et forståelig strømproblem viser seg. |
| Trafostasjonen, første besøk | 20:00–21:00 | Følge serviceveien, undersøke lokal driftslogg og løse en avgrenset, vanlig feil. | Ørkenen og arbeidslydene blir kjent mens spilleren fortsatt har kontroll. |
| Rutine og små tegn | 21:00–00:13 | Fortsette SARO-arbeidet, kontrollere at rettingen virker og eventuelt undersøke et begrunnet sidespor. | Uro kommer i små doser. En merket overgang til neste observasjon kan hoppe over uinteressant rutinetid. |
| Den store signalhendelsen | 00:13–00:30 | Håndtere residualen og telefon-/registreringsforløpet. | Hendelsesrekkefølgen lar seg ikke lenger forklare med den vanlige strømfeilen. |
| The Second Exposure | 00:30–01:30 | Feltkamera, B-12, kontrollprøver og fotolab. Bruke kveldens materiale til sammenligning. | Filmen beholder en referanse øyet ikke ser. |
| The Amended Record | 01:30–02:20 | Sammenligne rapportene, finne Dales kontornøkkel og undersøke hans private filer og opptak. | Noen fjernet C med vilje. Den hjelpsomme kollegaen har undersøkt mer enn han fortalte. |
| The Survey Station | 02:20–03:20 | Kabelen og Tomás på STATION 01. En relevant oppfølging ved trafostasjonen kan inngå på feltetappen. | Strømforklaringen prøves på nytt; en fjern form får fysisk betydning. |
| Room 6 | 03:20–04:10 | Legge frem bevis for Nora og forstå kabelkuttet. | Menneskelig kostnad. En sen skikkelse kan observeres etter samtalen. |
| All Night | 04:10–05:10 | Gjenbesøk i Caliche, etterprøving, avis og kartarbeid i dineren. | Kjente steder avviker. Vitner, lys og et større himmelfenomen peker mot samme natt. |
| Samle og velge avreise | 05:10–05:20 | Avslutte en kontroll, orientere seg og gjøre klar siste kjøretur. | Tidsrommet er en buffer for lesing og ettertanke, ikke en ventekø. |
| Roswell Road | 05:20–05:29 | Kjøre til det utledede krysset med C. | Opptakene faller sammen med det som skjer rundt trucken. |
| THE EVENT | 05:29:00–05:29:47 | Den eksisterende finalen. | Observatøren blir selv en referanse. |

### Innkjøring, radio og matstopp

Innledningen begynner i bilen på veien mot Caliche, i sen ettermiddag. Spilleren kjører selv, lærer radioen og ser bygningene før de senere skal undersøkes i mørket. Radioen spiller lystig, tidsriktig musikk og en ordinær lokal sending. Bruk eksisterende egnet musikk eller et eget produsert spor; en bestemt kommersiell låt er ikke et krav for scenen.

Dineren er et valgfritt stopp. Spilleren kan parkere, gå inn, møte servitrisen og velge en enkel matbestilling, for eksempel matpakke eller burger med kaffe. Bestillingen må ha en synlig respons og kunne tas med eller spises. Et enkelt kjøp er nok; dette innfører ingen sultmåler eller generell økonomisimulering. Det er like gyldig å kjøre forbi.

Tidlig dinerbruk har egne samtaler og tilstander. Den aktiverer ikke ch5, Ward-samtalen, avisfunnet som hovedoppgave, P13 eller avreise til gammel vei. Den varme, vanlige opplevelsen gir senere gjenkjennelse. Ingen skjult hovedhistoriegjenstand kan bare fås ved å kjøpe mat.

Innkjøringen har en egen innledningsfase uten løpende skiftklokke. Den vises som sen ettermiddag og «before the shift», uten et minuttsatt HUD som først kan passere 18 og deretter stilles tilbake. Når spilleren ankommer SARO, avsluttes innkjøringen, saken får et ankomstsjekkpunkt og skiftet starter 18:00. Spilleren får ingen bot eller mistet historie fordi matstoppet tok tid. Denne fasegrensen må være tydelig og lagres, ikke skjules som et vanlig tidshopp.

Innkjøringen kan bruke en av prosjektets eksisterende 1980-tallsbilmodeller og den fungerende kjørekontrolleren. Reisekjøretøyet parkeres ved ankomst, mens servicepickupen står tilgjengelig på SARO til skiftets oppgaver. De to kjøretøyenes tilstand må holdes adskilt, slik at innledningen ikke flytter eller fullfører en senere servicetur.

### Overlevering og vanlige referanser

Dale har allerede avsluttet dagvakten når spilleren kommer til SARO. Overleveringen skjer på telefon. Vi beholder hans eksisterende navn og rolle og bruker Kinski-stemmen i en rolig, tørr og menneskelig tone. Han kan spørre om matstoppet og reagere på det spilleren forteller, vise til skiftloggen og forklare vanlig utstyr. Etter Toms korreksjon finnes rollen bare på telefon, radio eller walkie-talkie: ingen fysisk Dale, ingen møtefigur og ingen Kinski-forteller på VHS.

Etter overleveringen får spilleren lære kameraet uten at B-12 eller senere fotohendelser fullføres. N01 er fortsatt ordinært arbeid før signalhendelsen: dagens driftsjournal og sendeskjema/tidsgrunnlag kontrolleres under den tidlige driften. Grunnmaterialet finnes i overleveringsmappen på SARO, med en datert byreferanse som kan støtte senere bildesammenligning. En ekstra bytur er en begrunnet, valgfri oppfølging. Innkjøringen blir ikke gjort om til en obligatorisk papirjakt.

Gjennomkjøringen og matstoppet gjør byen kjent, men teller ikke automatisk som fotografisk dokumentasjon. En senere oppgave som krever referansefoto må sørge for at bildet faktisk er tatt, eller gi en tilgjengelig, datert referanse. Spilleren kan registrere suvenirutstillingen, gatetelefonen og studioet i denne vanlige fasen.

N01 undersøker ikke en fremtidig 00:13-hendelse. Materialet samles som rutine på kvelden og får en ny funksjon når utskriften kommer etter midnatt. Det kreves ikke en ekstra obligatorisk bytur bare for å hente samme papir igjen.

Earl har Night Line fra midnatt til fire. Kveldsbesøket bruker en vanlig kveldssending og et fysisk utleveringspunkt eller en kort vanlig ekspeditørutveksling. Earl blir ikke flyttet til klokken 18. En senere, valgfri oppfølging med ham kan skje etter midnatt dersom spilleren har en konkret grunn til å komme tilbake.

Den tidlige byen trenger egne åpningstider og bemanning. Nåværende nattbetingelser gir ikke automatisk en fungerende kveldsby. La noen virksomheter være i drift, andre stenge og folk gå hjem gradvis. Radiostudioets synlige ON AIR, stemmen i radioen, dørtilgangen og sendeskjemaet må være enige.

Baselinen beskyttes mens referanseoppgaven pågår: pappfiguren er i utstillingen, og store telefon-/bortføringshendelser holdes tilbake. Det kan finnes én ufarlig, kontrollerbar feil. Ingen må vente på en tilfeldig hendelse for å avslutte ærendet.

Trucken kan i dag brukes gjennom fri kjøring fra ch1 etter prologen. Kveldens nye tilgang må derfor bygges uttrykkelig, med eget oppdrag og retur. En tidlig ankomst til Caliche skal ikke aktivere ch5, Nora, P13 eller den gamle veiens avslutning.

### Gjenbesøket i All Night

Gjenbesøket starter tidligere, rundt 04:10, med god tid til både byen og dineren. Spilleren velger et kjent spor å etterprøve. Den nye hovedreisen presenterer én pålitelig tilgjengelig sammenligning før P13. «Insufficient evidence» er et gyldig avslutningsresultat; ingen tilfeldig anomalikvote sperrer veien videre.

Ekspeditøren og sheriffen gir ulike oppfølginger. Kvinnen kan fortsatt være der frem til 04:50. Earl er gått klokken fire, og suvenirmannen kan allerede ha gått. Tidlige samtaler eller bevarte opplysninger må bære deres spor etter stengetid.

Avisbildet og kartgåten får ro i dineren. Ved sen ankomst kan en sammenligning med medbrakt materiale brukes uten en lang henteomvei. Undersøkelsene endrer hva spilleren forstår og tar med seg, mens én slutt beholdes. Oppdragsteksten skal ikke love at grundig dokumentasjon lar spilleren velge en annen slutt.

### Tre historier som gradvis blir én

SARO-sporet handler om en institusjon som har gjort avvik til vedlikeholdsfeil. Gjentatte relébytter og den endrede rapporten viser hva som ble gjort for å holde driften i gang. Spilleren kommer videre ved å kontrollere en forklaring som først virker rimelig.

Bysporet handler om folk som lever med hendelser de har gitt hverdagslige navn. Et nytt batteri, en uvanlig lang bergingstur eller en merkelig radiosamtale kan hver for seg avskrives. Spilleren ser sammenhengen fordi de har adgang til både SAROs materiale og byens erfaringer.

Det personlige sporet begynner med at spilleren samler referanser og slutter med at spilleren selv blir en referanse. De sene frempekene skal kunne gjenkjennes etterpå. De skal ikke tidlig fortelle at en bestemt skikkelse i et gammelt bilde er spilleren. Den tredje personen og bildets eksisterende avsløring bevares.

Disse sporene kobles gjennom handling: journalen fører tilbake til utskriften, kvitteringen fører til sjåføren, kartene fører til C, og den kjente motorlyden fører frem mot finalen. Et nytt dokument må tilføre en slik forbindelse eller et menneskelig perspektiv. Forklaringer på alt det ukjente holdes tilbake.

De levende personene får ulike roller. Dale er kollegaen som kjenner den daglige driften og skjuler sin egen etterforskning. Ward beholder rollen som vakthavende med krav om korrekt rapport. Earl hjelper med hva som faktisk ble sendt; sheriffen hjelper med hvor hendelser ble rapportert; ekspeditøren gir hverdagslig trygghet og en tvetydig gjentakelse. Nora gir den menneskelige kostnaden og en konkret erfaring med kabelen. Servitrisen og sjåføren gir et siste jordnært holdepunkt. Ingen av dem trenger en monolog om utenomjordiske skapninger.

### Dale, telefonene og det låste kontoret

Dale bruker prosjektets eksisterende Kinski-stemmeprofil bare via telefon, radio eller walkie-talkie, også ved overleveringen 18:00. Eventuelle faste Dale-fragmenter bruker de samme kanalene. VHS har miljølyd og skriftlige merknader uten Kinski-forteller. Ward, Nora, Tomás og Earl beholder sine identiteter. Innstillingen for generell dokumentopplesning skal ikke avgjøre om Dale kan snakke: rollebasert dialog knyttes til karakterstemmene med egne Dale-klipp, talernavn og undertekster. At en Kinski-bank allerede finnes, betyr ikke at de nye replikkene er spilt inn.

Hans utvikling gjennom natten anbefales slik:

| Trinn | Spillerens møte med Dale | Hva det tilfører |
|---|---|---|
| Overlevering 18:00 | Praktiske råd og tørr humor på telefon fra kollegaen som allerede har gått hjem. | Et kjent menneske å stole på; ingen forhåndsadvarsel om romvesener. |
| Første strømproblem | En kort, levende telefonsamtale om den vanlige rutinen ved trafostasjonen. | Han hjelper spilleren til å mestre en reell arbeidsoppgave. |
| Etter residualen | Når spilleren nevner en bestemt detalj, stopper han litt opp og ber dem beholde utskriften. | Det merkes at noe er kjent for ham, men han forklarer ikke alt. |
| Arkiv og nøkkelfunn | Spillerens sammenligning av serviceopplysninger fører til en merket reservenøkkel i arkivets serviceskuff. | En pålitelig oppdagelse som åpner et tidligere sett, låst sted. |
| Det private kontoret | Spillerens funn viser at Dale har samlet hendelser og gjort egne kontroller lenge før denne natten. | Den tidligere hjelperen får en skjult side. Materialet gir en konkret oppfølging ute. |
| Eventuell senere samtale | Spilleren kan legge frem ett bestemt funn. Dale kan svare unnvikende eller erkjenne at han undersøkte det. | En menneskelig reaksjon som skiller en levende samtale fra et fast avtrykk. |

Kontoret er den eksisterende Operations office-døren i arkivfløyen, som i dag bare sier at Dale tok nøkkelen hjem. Dette er en konkret utvidelse av den døren og rommet bak. Tidlig informasjon kan si at han tok hovednøkkelen; den senere reservenøkkelen får en fysisk, navngitt plass og registreres i saken. Den dukker ikke opp etter en tilfeldig hendelse. Den holdes adskilt fra Noras nøkkel til STATION 01.

Inngangen til kontoret er del av den reviderte arkivreisen. Spilleren trenger ett relevant dokument eller kontaktark for den videre kontrollen; hele samlingen og alle videoene er ikke en obligatorisk lese-/seerkvote. Rommet åpner seg i tre lag:

1. **Vanlig arbeidskontor:** servicepermer, kaffekopp, skiftmateriale og en avslått TV/VHS-spiller. Dette er et rom en kollega faktisk bruker.
2. **Privat samling:** avisutklipp fra tidligere hendelsesår, kommenterte servicekopier, fotografier og mapper/disketter som viser egne UFO-/alienundersøkelser. De kan sammenlignes med datoene spilleren allerede har. Den bredere 1947-avsløringen fra dineren og P13 beholdes til sin plass.
3. **Egne feltopptak:** et kort VHS-opptak av en gjenkjennelig kraftlinje eller trafostasjon og et uavklart lys-/formavvik. Et kontaktark eller notat viser et ståsted spilleren kan prøve selv. Det gir en grunn til T02 og senere observasjon ute, uten å bevise hele forbindelsen til C.

TV/VHS skal vise faktisk bevegelig film med lyd, med enkle kontroller for avspilling, stopp og spoling. Et filmklipp må ha en observerbar forandring, for eksempel at noe passerer bak en stolpe, slik at det ikke oppleves som et stillfoto på TV-en. Første opptak sikter mot omtrent ett minutt med mulighet til å stoppe. Et bevart stillbilde kan brukes som eget bevis etterpå. Undertekster og et bevart kontaktark støtter spillere som ikke får med seg lyden eller hele klippet.

Eksisterende Unarius-film kan inngå i Dales samling av offentlig UFO-/romtro-materiale og fortsatt spilles som film. Egne, daterte feltopptak får en annen bevisrolle. En offentlig film blir ikke behandlet som et faktisk SARO-feltopptak bare fordi den står i samme rom.

Det nye rommet krever egne bygningsflater, kollisjon, lys, mapper, nøkkeltilstand, TV/VHS-binding og lagring. Nøkkel funnet, dør åpnet, dokument tatt og avspillingsposisjon bevares. Strømbrudd, pause og lasting skal følge samme mediekontrakt som de øvrige TV-ene.

Dale er ikke allvitende og får ikke forfatterfasiten om slutten. Han kan ha sett et mønster og vært redd for å rapportere det uten å forstå fenomenet. Han gjøres heller ikke automatisk til hovedskurk eller avtrykk. Dersom stemmen hans senere dukker opp som et fast lydfragment, kan det fragmentet bare gjenta sitt registrerte forløp.

## 4. Byens funksjoner og tilgjengelighet

| Sted | Første besøk | Senere bruk | Forutsetning og alternativ |
|---|---|---|---|
| Bensinstasjonen | Hente dagens driftsjournal og møte ekspeditøren i ordinær drift. | Sammenligne bergingskvitteringen, den gamle veien og eventuelt gjentakelsen. | Ekspeditørens identiske replikk krever minst to timer siden første møte; beregningen må tåle midnatt. Det er et valgfritt spor. |
| 1420 AM | En vanlig kveldssending, studioaktivitet og et datert sendeskjema gir referansen. | Materialet brukes etter residualen; Earl kan følges opp under Night Line. Senere kan det tomme studioet undersøkes. | Earl har Night Line 00:00–04:00. Kveldsbemanning og synlig ON AIR må defineres separat; sent førstebesøk bruker en fysisk avskrift. |
| Gatetelefonen | Vanlig ringetone og myntbruk blir kjent. | Printerlyden, 1947-mynten og senere motorfragment kan etterprøves. | Første hendelse slutter å være tilgjengelig 04:45. Den senere krever tidligere hendelse og er tilgjengelig til 05:19. Ingen av dem er eneste vei gjennom hovedhistorien. |
| Sheriffkontoret | Kartet kan observeres uten å love en samtale. | Etter 04:30 kan sheriffen kommentere relevante faktiske observasjoner. Egne markeringer sammenlignes med kartet. | Sheriffen er ute 02:45–04:30. Kartet og etterlatte opplysninger må kunne brukes når døren er låst. |
| Suvenirbutikken | Spilleren tar eller undersøker et referansebilde av utstillingen. | Figurens plassering, prismerke og fysiske spor sammenlignes. | Førstegangsreferansen må være registrert før en hukommelsesoppgave. Gamle direkte-spawn må tilpasses. |
| Vaskeriet | En fungerende maskin og lysrøret gir en vanlig rytme. | Valgfri lyd-/rytmekontroll og et sted for fysisk etterspill. | Kvinnen går 04:50. Sen hovedfremdrift kan ikke kreve henne. |
| Mesa Diner | Valgfritt matstopp på innkjøringen: parkere, bestille og ta med/spise mat, møte servitrisen og se en vanlig diner. | Avis, sjåfør, servitrise og P13 samler trådene. Dinertelefonen har Ward. | Tidlig besøk må ikke sette ch5-flagg eller kreves for et senere historiespor. Beskytt kartarbeidet mot konkurrerende skremmehendelser. |

Suvenirmannen går 04:10. Valgfrie tidlige vitnesamtaler gir senere kontekst, men skal ikke gjenoppstå som nødvendige møter etter stengetid. Kino, kirke og fôrbutikk får også bedre utvendige bygninger; fullverdige nye interiører prioriteres etter de viktigste undersøkelsesstedene. Kveldstilstandene skal ikke arve alle de gamle nattfigurene og åpningstidene ukritisk.

### Trafostasjonen i ørkenen

Arbeidsnavn: **Caliche Junction Substation**. Dette er et nytt, avgrenset serviceområde på en kort grusavstikker fra den eksisterende kjøreruten. Nøyaktig plassering velges etter kart-/rutemåling. Stasjonen får egen identitet og kan ikke forveksles med STATION 01, som beholder transiten, kabelen, Tomás og forbindelsen til C.

Det synlige anlegget består av grusdekke, nettinggjerde, port, transformatorer med kjøleribber og isolatorer, kabel-/stolpeføring, en liten betjeningsbod og få fysiske arbeidslys. Byens og SAROs forsyning fremstilles med tydelig adskilte utgående kurser. SAROs K3 er fortsatt en intern mottaker-/surveykrets og blir ikke bryteren for hele byen.

**Første besøk, rundt 20:00:** En vanlig lokal feil må undersøkes. Spilleren leser en kort driftsjournal, bruker en avgrenset kontroll på betjeningsfronten og kontrollerer en synlig virkning på den aktuelle kursen. Oppgaven gir en forståelig forklaring og en målbar normaltilstand. Det skal føles som at spilleren lærer arbeidet. Ingen tydelig UFO eller nærfigur belønner enhver reparasjon.

**Senere oppfølging på feltetappen:** Et nytt funn gir en konkret grunn til å kontrollere samme sted igjen. En kjent lampe eller registrering oppfører seg nå annerledes enn ved den vanlige feilen. Spilleren sammenligner journal, foto og kontrollresultat. En mulig nødforsyning eller vanlig indikatorfeil må kunne utelukkes før et avvik regnes som etterprøvd. Første forslag er at én dokumentert lyskilde fortsetter å følge sitt gamle forløp selv om den relevante servicekontrollen viser frakoblet kurs. Detaljen er fiksjonell spillatferd som må vises entydig i scenen.

Andre besøk kan gi et fjernt lys mellom stolpene eller en form som dekker himmelen, etter at undersøkelsen er gjort. Forløpet følger spillerens handling og riktig opptrappingsnivå; det utløses ikke hver gang noen åpner porten.

Den første oppgaven er ordinært arbeid i hovedreisen. Den andre er en konkret oppfølging av et feltfunn med en alternativ bevart referanse for sen ankomst eller eldre saker. Ingen tilfeldig spawn kreves. Reparasjon fjerner ikke det kosmiske fenomenet. Første versjon bruker serviceområdet og betjeningsfronten, uten en egen detaljert høyspenningssimulator eller et stort ekstra bygg å utforske.

Nytt område krever kjørbar avstikker, parkering, gangflater, kollisjon, siktlinjer, kamerasubjekter, lyd/lys, oppdragsretur og lagring. Visuell modell alene er ikke ferdig spillested. Kartmarkøren sier trafostasjon, og stedet gis ikke automatisk A- eller C-tilhørighet bare fordi en kraftlinje går forbi.

### Bedre bygninger i Caliche

Målet er en troverdig liten by som har vokst over tid og fortsatt brukes i 1986. Konseptbildene av Caliche og Mesa Diner brukes som eksisterende visuell retning. De er konseptkunst, ikke bevis på dagens runtime-grafikk. Behold den nåværende byens orientering og sentrale siktlinjer mens det visuelle bygningsskallet bygges bedre.

En felles oppgradering omfatter volum og proporsjoner, veggtykkelse, dør-/vindusnisjer, sålebenker, karmer, takkanter, avløp, ventilasjon, lokale materialer og troverdig overgang til fortau/grus. Store flater får variasjon i materiale og overflate, mens rust og skitt plasseres der vær og bruk gir mening. Noen bygninger er vedlikeholdte, andre slitte. Hele byen skal ikke fremstå som en forlatt ruin før noe har skjedd.

| Bygning | Konkret visuell forbedring | Spillfunksjon som må bevares |
|---|---|---|
| Bensinstasjon | Eget tak over pumpene med synlig konstruksjon, inntrukket nattluke, glass med synlig butikkdybde, disk, driftslapper, ulik slitasje på sokkel og tak. | Nattluke, journal, ekspeditør, mynt og telefon i lesbar avstand. |
| 1420 AM | Tydelig studiovindu, et faktisk lite rom med mikrofon/spoler/telefon, egen inngang, takantenne og ON AIR fra samme tilstand som sendingen. | Kveldssending, Night Line, senere tomt studio og lydetterspill. |
| Mesa Diner | Egen 3D-fasade med brystning, dype vinduer, ekte inngang og neonskilt. Innvendig: båsrekker, disk, kjøkkenåpning, gulv og materialer med målestokk. | Avis, saksbord, sjåfør, servitrise, Ward-telefon og fri vei til trucken. |
| Sheriffkontor | Pusset eller murt fasade med dørnisje, diskret skilt, vinduer med innvendig dybde, skrivebord og faktisk synlig kart. | Kartnåler og bemanning kan leses gjennom vinduet. Patruljebilen har plass begge steder. |
| Suvenirbutikk | Håndmalt skilt, en dyp utstillingsnisje, hyller, butikkdisk og egne fremviste småting. | Pappfigur, prismerke og referansefotografi har samme gjenkjennelige plass. |
| Vaskeri | Troverdig glassfront, et enkelt rom bak, maskiner med faktisk dybde, benk og egne lysrør. | Normal rytme, kvinnens plass og relevante lyd-/synskontroller. |
| Kino, fôrbutikk og kirke | Felles kvalitetsløft på hele bygningsvolumet: innganger, tak, sokler, vinduer og egne skilt/materialer. Kinoen får en lesbar gammel marquee; fôrbutikken lasterampe og varer; kirken en enkel inngang og vindusdybde. | Landemerker, gangrute og viktige siktlinjer beholdes. Stengte dører krever ikke fullstendige interiører. |
| Øvrige byhus | Samme målestokk og materialfamilie, med individuelle tak-/vindusvariasjoner og noen enkle, synlige rom. | De skal gi en sammenhengende gate, uten å gjøre alle dører til oppdragsmål. |

Første visuelle leveranse er en sammenhengende gatebit ved bensinstasjonen, suvenirbutikken og radiostudioet, med alle tre fra spillerhøyde både på kvelden og om natten. Deretter ferdigstilles dineren, sheriffkontoret og vaskeriet; resten av byens utvendige skall følger samme kvalitetsnivå. Ingen av disse erstattes av et rent lys-/filterpass.

Nye egne bygninger modelleres med bevarte Blender-kilder, UV-er, PBR-kart og egnede kollisjonsmesher. Bruk delbare materialer, trimteksturer og instanser der det passer. Unike detaljer prioriteres ved innganger og steder spilleren undersøker. Felles råteksturer skal ikke dupliseres som en stor ny pakke for hver vegg.

Unreal er det redigerbare målet. Nye native assets og eksisterende `S47Props`-/materialmønster kan brukes til visuell utskifting, med eksplisitt binding til de bevarte ankerpunktene. Gamle synlige skall fjernes først når erstatningen har korrekt kollisjon og interaksjon. Ikke skjul en logikkrot slik at dens nye barn, hendelsesdrivere eller trefflater også forsvinner. Web-geometrien videreutvikles ikke for å gjennomføre dette passet.

Kontrollbildene må vise samme kamerasted før/etter, en normal og en avvikende tilstand og riktig kveld/natt/gry. Glass, skilt og lys skal ikke skjule selve forskjellen spilleren skal finne. Nye materialer og renderbaner krever ny vurdering av den leverte shader-/PSO-oppvarmingen og ytelsen.

## 5. Foreslåtte undersøkelser og hendelser

ID-ene nedenfor er planreferanser, ikke eksisterende runtime-ID-er. «Utvidelse» betyr at deler finnes, men at den beskrevne spillerhandlingen må bygges.

| ID | Innhold og spillerens handling | Kontroll og tilbakemelding | Rolle |
|---|---|---|---|
| N01 | Første rutine på SARO: kontrollere driftsjournal, sendeskjema og referanser fra overleveringsmappen. Etter midnatt sammenlignes materialet med utskriften. | Først en vanlig oppgave, deretter bevismateriale som utelukker en forklaring. Et valgfritt byærend kan utdype kontrollen. | Ny hovedsekvens i to deler; innkjøring og matstopp brukes ikke som obligatorisk papirjakt. |
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
| T01 | Første tur til trafostasjonen: en vanlig feil kontrolleres og rettes med et synlig resultat. | Journal, servicekontroll og berørt lyskilde blir en normalreferanse. | Ny ordinær hovedoppgave før den store signalhendelsen. |
| T02 | Senere tur: samme kontroll og referanse prøves mot et uforklarlig forløp. | Vanlig strømfeil, nødforsyning og misvisende indikator må skilles fra det som blir igjen. | Ny oppfølging som knytter strømforklaringen til mysteriet uten å erstatte STATION 01. |
| I01 | Innkjøring gjennom Caliche med radio og valgfritt matstopp før skiftet. | Begge valg fører videre til SARO og tydelig skiftstart 18:00; matkjøpet har en synlig respons. | Ny normal innledning med egen fase og lagring. |
| D01 | Snakke med Dale på telefon om drift, deretter om den merkelige registreringen. | Samme Kinski-karakterstemme og navn, med konkrete svar på levende samtaler. | Ny rollebasert dialog som bygger tillit og senere tvil. |
| D02 | Finne reservenøkkelen og undersøke kontorets filer, avisutklipp og bevegelige VHS-opptak. | Ta med et konkret referansebilde/notat og etterprøve ståsted eller kontroll i felt. | Ny del av arkivreisen, knyttet til T02 og byens tidligere hendelser. |

Kontrolltilfeller skal være meningsfulle. Studioets normale avslutning 04:00, vanlig morgenlys og at noen går hjem er ikke automatisk anomalier. Ett eller to slike tilfeller tidlig gir spilleren grunn til å teste en mistanke. De skal ikke fylle byen med vilkårlige lureoppgaver.

### Eksempel på gjenbesøk

Spilleren kjenner suvenirutstillingen. På returen er figurens plassering endret. De tar et bilde og sammenligner prismerket med første foto. Dermed vet de at det er samme pappfigur. Hos ekspeditøren får de en opplysning som kan kontrolleres, uten at noen forklarer fenomenet.

I første prototype følger P01, printerrytmen fra det stengte studioet, etter undersøkelsen. Den begynner først når panelet er lukket og ingen annen viktig hendelse eller samtale pågår. Den er kort og bruker eksisterende lyd; ingen ny figur eller jakt kreves.

Med All Night fra 04:10 kan lys-/lydbølgen nå passe før dagens grense 04:55. I trinn 2 kontrolleres kapittelbetingelse, faktisk ankomst og daggryslukking samlet. En sen spiller får et undersøkbart etterspill. Hvis bølgen allerede er opplevd, beholdes den historikken; hendelsen gjentas ikke bare for å fylle sekvensen. Første pilot krever fortsatt ikke denne bølgen.

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
| `S47Story` og kapittelportene | Egen innkjøringsfase, ankomstsjekkpunkt og overlevering 18:00; tidlig utstyr og felles tidsplan. | Valgfritt matstopp gir ingen skjult tidsstraff. Nye kveldsoppgaver gir ikke fullført senere kapittel. Historiske 1947-tider flyttes ikke. |
| `S47Chapter1` / `S47Chapter2` | Bruke kveldens materiale etter residualen og bevare B-12-/arkivoppgavene. | Ikke gjøre N01 til en kontroll av et fremtidig utskriftstidspunkt. |
| `S47World` | Egen oppdragsankomst og retur med kontrollert tidsregning og eksisterende kjøring. | Ikke gjenbruke `ArrivedDiner()` som generell ankomst eller resette kapittelklokka. |
| `S47TownLayer` / `S47TownEvents` | Besøksstadier, referanser, fysiske varianter og etterspill. | Ett fysisk objekt og én eier per tilstand; ingen doble triggere. |
| `S47Caliche` | Nye korte journal-/tidsreferanser og målrettet fremleggelse av bevis. | Respektere arbeidstid, eksisterende vitnespor og skillet mellom levende vitne og avtrykk. |
| `S47Curve` / `S47Night` | Bruke eksponering og undersøkelsesstatus sammen med kapittel og nattklokke. | Beholde én større hendelse om gangen og viktige pauser. |
| `S47Seen` / `TownSeen` | Registrere faktisk eksponering separat fra spillerens etterprøving. | Gamle `ev:*`-markører alene betyr ikke at en sammenligning er løst. |
| Kamera, album, notebook og board | Knytte et lite antall bevis til konkrete sammenligninger. | Gjenbruke grensesnittet; ikke kreve generell friobjektgjenkjenning. |
| `S47Chapter5` | Koble byens funn inn før kart-/veivalget. | P13 beholder sin funksjon. En byrapport kaller aldri `Finish()` eller setter `road`. |
| `S47Flow` / `S47Saves` | Lagre referanser, variant, undersøkelsesstatus og rekkefølge. | Gamle saker skal kunne lastes uten tvungen omstart. |
| Ny trafostasjon og `S47World` | Ny avstikker, serviceområde, forsyningstilstand, T01/T02 og retur til eksisterende oppdrag. | Egen rolle og ID; ikke erstatte STATION 01 eller utvide K3 til byens strømnett. |
| Nye bygningsassets, `S47Props` og material-/lysbindinger | Eget bygningsskall med bevarte ankre og tilstandsstyrte vinduer, skilt og rom. | Kollisjon, referansefoto, NPC-plass og eksisterende trefflater verifiseres per bygg. |
| Ny innkjøring, `S47World` og diner | Reisekjøretøy, normal radiosending, matbestilling, parkering og overlevering til SARO. | Dinerens ch5-logikk og servicepickupens turtilstand holdes adskilt fra innledningen. |
| Dale-dialog, stemmebank og kontor | Egne Dale-klipp med Kinski-profil, levende samtaler, reservenøkkel, mapper, kontaktark og TV/VHS. | `bKinskiCalls`/generell opplesning er ikke ferdig karakterruting. Nøkkelen er ikke Noras nøkkel, og VHS er faktisk film. |

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

Ny kveld trenger én tidsakse som skiller søndag 13. april fra mandag 14. april. Ordning, varigheter og totimerskravet beregnes fra sammenhengende natte-/datotid. Lokal HH:MM og dato vises ved behov. Eksisterende nattomregning i `S47Story` gjenbrukes der den passer, men beviser ikke at alle modulo- og klokkesammenligninger tåler starten 18:00.

Før retiming lages en avgrenset tidsreferanseoversikt:

| Referanse | Dagens verdi | Foreslått regel |
|---|---|---|
| Spillstart og overlevering | 23:41, med blant annet K3-logg 23:44 og tekst om at Dale allerede har gått. | Ny innkjøringsfase før skiftklokka. Ankomst og Dales telefonoverlevering starter 18:00; notater følger den nye sekvensen, og han har allerede forlatt SARO. |
| Residual og tilhørende 1986-registrering | 02:13:47. | 00:13:47 den 14. april. Oppdater utskrifter, konsolltekst, telefon/printerreferanser, decoderens etterfølgende klokkeslett, stemmenotater og gjenopprettede logger. |
| Oppbyggingens start og nattstyrke | Blant annet `NIGHT0=02:16`. | Knyttes til ny fase og faktisk eksponering; ikke la 18:00 bli tolket som slutten av natten eller gi toppintensitet. |
| Kapittel 3, 4 og 5 | Minst 03:40, 04:35 og 05:00. | Nye mål 02:20, 03:20 og 04:10; alle minimumsporter, måltekster og underhendelser kontrolleres. |
| Forsøket i 1947 | 02:17:00–02:17:47. | Behold historisk tid inntil en eksplisitt historieendring eventuelt krever noe annet. Den flyttes ikke sammen med 1986-residualen. |
| Roswell Road og THE EVENT | Avreise 05:20, hendelse 05:29. | Behold siste kjøreturs tid og 47 sekunders finale. |

Nye relevante tider samles i ett avgrenset tidsplanoppsett med adaptere til eksisterende systemer. Det er ingen grunn til å skrive om hele historiemotoren. Men kvelden krever at radiosending, synlig studio, NPC-er, butikker, lampetilstand og tekster om «today/yesterday» bruker samme dato og plan. Dagens `Caliche`/`Town` har ulike kveldsvilkår for ON AIR og må bringes i samsvar.

Transport i `DriveToDiner` legger i dag til 0,19 sekunder per kjørt meter i tillegg til ordinær tid. En tidlig kontrolltur trenger en uttrykkelig valgt tidsregel. Første implementasjon bruker fri kjøring med vanlig klokketid og en egen oppdragsankomst, eller et avgrenset turvalg med samme regel. Ikke bland de to tidsmodellene utilsiktet.

Det er ingen tidsstraff for å lese eller sammenligne mens spillets eksisterende panelpause er aktiv. Nye byoppdrag og overganger før Roswell Road skal ikke flytte klokka bakover eller forlenge vitnenes åpningstider.

Dagens kapittel 6 har en egen, regissert tidsmodell: avreisen setter 05:20, og kjøreturen tilpasses 05:29. Dette kan allerede gi et tilbakesprang ved et langt All Night-besøk og er ikke løst av denne planen. Pilotarbeidet endrer ikke denne tidsmodellen. THE EVENT beholdes 05:29:00–05:29:47.

Trinn 0 må registrere klokka ved start og slutt på de nye byoppdragene, ved P13 og ved avreise, både med rask rute og lang frivillig utforskning. Prøv særlig avreise etter 05:20 og dokumenter faktisk tidsdiskontinuitet. Hvis den utvidede normalruten overskrider tidsbudsjettet, må innhold forkortes eller flyttes, eller en avgrenset regel for overgangen til finalen avklares før fullnatten godkjennes. Dette skal ikke bortforklares som en ny anomalihendelse.

Nye saker får en eksplisitt fortellertidslinje i lagringen. Anbefalt kompatibilitet er at eksisterende saker fortsetter med sine gamle historiske klokkeslett gjennom samme tidsplanoppsett, mens nye saker bruker kvelden. En migrering må være bevisst og testet; ikke endre gamle bildestempler, lydreferanser og rapporter fra 02:13 til 00:13 i stillhet.

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

Hvert trinn omfatter sammenhengende handling, tilbakemelding, lagring og kontroll. Det leverte material-/lyspasset er et grunnlag, mens de nye bygningsskallene og trafostasjonen er egne kommende leveranser. Denne planoppdateringen starter ingen tunge jobber og endrer ikke det installerte spillet.

| Trinn | Leveranse | Avhengighet | Når trinnet er godt nok |
|---|---|---|---|
| 0. Baseline og tidskontrakt | Måle dagens rute og venting, kartlegge referanser til hendelsen og prøve datoaksen gjennom midnatt. Vurdere trafostasjonens avstikker og velge den første gatebitens bevarte ankre. | Nåværende fungerende pakket utgave og eksisterende konseptbilder. | Rute, gammelt/nytt tidsgrunnlag og visuelle kontrollsteder er dokumentert uten testhopp som bevis på vanlig spillflyt. |
| 1. Innkjøring, overlevering og Caliche | I01 og D01: normal radio, valgfritt matstopp og Dale med Kinski-stemme ved skiftstart 18:00. N01/N02/A02/P01 og faktiske referanser. Første bedre gatebit og en diner som fungerer ved tidlig besøk, med lagring og adskilte kapitteltilstander. | Trinn 0 og eksplisitte innlednings-/datoporter. | Både direkte kjøring og matstopp fører riktig til SARO. Dales fjernsamtaler og de nye byhandlingene kan oppleves med ekte input; bevis følger saken. |
| 2. Trafostasjon, Dales kontor og byens etterprøving | T01/T02, D02 med nøkkel, private filer og faktisk bevegelig VHS. Kvittering, sheriffkart, telefon og P13. Ferdigstill diner, sheriffkontor og vaskeri. | Trinn 1 har lesbar kjernehandling og tidsgrunnlag. | Vanlig strømfeil kan skilles fra senere avvik; kontorfunn gir en konkret feltkontroll uten å overta Noras eller P13s funksjon. Nye steder og bevis lagres. |
| 3. Felt, resten av byen og fremmed tilstedeværelse | Fjern himmelform, sen aliensekvens og større UFO-øyeblikk. Rebalansere gamle glimt/bortføringer. Utbedre øvrige utvendige bygg med samme kvalitetsnivå. | Referanser, siktlinjer og riktig hendelsesrekkefølge. | Hele gaten har bedre bygninger, opptrappingen oppleves som økende nærhet, og de nye modellene fungerer i pakket gameplay. |
| 4. Hele natten | Kontrollere alle overganger, pauser, etterspill og koblingen til finalen. | Trinn 2–3. | Sammenhengende menneskelig gjennomspilling, regresjon og målinger av påvirkede scener. |

Første Gauntlet-prøve sikter mot 20–30 minutter avgrenset byinnhold fordelt på kveldens besøk og All Night. Kveldsoverlevering, ny trafostasjon og de mellomliggende kapitlene kommer i tillegg; total aktiv spilletid må måles. Den sene delprøven bruker en testfixture som viderefører kveldens faktiske referanser, bilder og undersøkelsesstatus gjennom vanlig lagring. Tidskuttet merkes tydelig. En slik fixture er testdata og ingen snarvei i den leverte hovedhistorien.

Mellom de to bydelene prøves også N01s korte SARO-kontroll etter 00:13:47 med samme lagrede journal og sendeskjema. Den kan inngå i første testøkt etter et merket tidskutt. Piloten består ikke før materialet faktisk har fått den lovede funksjonen mot utskriften; det er utilstrekkelig å bare ta med papirene fra kveld til All Night.

Dette prøver handlingene og beviskontinuiteten, mens trinn 4 må godkjenne hele kapittel-, transport- og tidsforløpet uten testhopp. I full natt ligger andre besøk etter de mellomliggende kapitlene; faktisk total varighet må måles. 20–30 minutter er en målsetting for prøven, ikke et løfte om hele spillets lengde eller en ferdig måling.

Første gameplaypilot bruker I01 og D01 sammen med N01, N02, A02 og P01, med faktiske referanser og riktig datogrunnlag. Direkte innkjøring og matstopp skal begge prøves. Telefon 1 er valgfri. D02, T01/T02 og resten av bygningsløftet er påkrevde senere leveranser i trinn 2–3, ikke valgfri polering. A04–A09, en ny lydopptaker og nye UFO-/alienanimasjoner inngår ikke i trinn 1. Dagens lysbølgegrense 04:55 beholdes inntil ny rute er prøvd; All Night fra 04:10 kan nå gi plass før grensen.

Etter denne prøven vurderes om ett sted må kuttes, om sammenligningen er for tung, eller om et mellomliggende spor bør flyttes. Først deretter utvides katalogen. Ingen ny økonomi, sult-/søvnsimulering, stor inventory, kamp eller åpen verdensstruktur inngår i grunnplanen.

## 11. Gauntlet og akseptkriterier

Fem mål brukes med fem nye spillere. Tallene er foreslåtte akseptgrenser for første prøve.

1. Minst 4 av 5 kan forklare handlingen «oppdag, dokumenter, sammenlign, følg opp» uten muntlig instruksjon fra testleder.
2. Minst 4 av 5 oppdager ett avvik og kan etterpå beskrive hva som var annerledes enn normalreferansen.
3. Minst 3 av 5 bruker minst to steder i byen av en forstått praktisk grunn. Ingen trenger tilfeldig spawn eller et utilgjengelig vitne for å fortsette.
4. Begge bydelstrekninger, SARO-kontrollen etter residualen og bevisoverføringen kan fullføres med ekte input og lagring/lasting uten feil kapittel, tapte bevis eller endrede fotografier. Ingen obligatorisk ren venting overstiger to minutter. Full natt uten testkutt er et eget krav i trinn 4.
5. Testen viser økende forventningsuro før toppen og en merkbar avlastning i dineren. Registrer ubehag 0–10 ved fire definerte punkter; sikt mot minst 3 av 5 som beskriver denne utviklingen. Bruk forklaringene deres sammen med tallene.

Logg start på normalreferanse, første mistanke, dokumentasjon, kontroll, konklusjon, neste selvvalgte handling og tid uten forstått mål. Frivillig utforskning skal skilles fra å være fast. Spør om ubehag ved naturlige pauser eller etter gjennomspillingen, slik at målingen ikke stadig avbryter opplevelsen.

Teknisk kontroll ved implementering:

- Innkjøring: kjør direkte til SARO og via diner, kjøp mat én gang, spis/ta med og last saken på nytt før og etter ankomst. Begge ruter gir en tydelig overlevering 18:00 uten å aktivere ch5 eller flytte servicepickupen.
- Dale: overlevering og telefon bruker samme karakteridentitet; prøv med generell dokumentopplesning av og karakterstemmer på. Nye klipp, talernavn og undertekster må stemme med handlingene.
- Privat kontor: finn riktig nøkkel, åpne den eksisterende døren, ta dokumentet og spill VHS med faktisk bevegelse/lyd. Prøv pause, strømbrudd og lasting; bevar Noras nøkkel, avisen i dineren og P13s senere funksjon.
- Kvelden og midnatt: faktisk 18:00-start, vanlig kamerabruk og kjøring, riktige datoer, radio/ON AIR, NPC-er og lampetilstander. Ingen tidlig kapittellås eller full horrorintensitet klokken 18.
- Tidsreferansene: ny 00:13:47 i alle relevante 1986-tekster/opptak/dokumenter, bevart historisk 1947-tid og korrekt videreføring av eldre saker. Ingen blind søk-og-erstatt av klokkeslett.
- Trafostasjonen: kjør begge turer med parkering, ut/inn av truck, T01/T02, synlig kontrollresultat og lagring. Vanlig feil og senere avvik må kunne skilles av spilleren.
- Bygningene: før/etter fra identiske steder, hele den valgte gaten og nærbilder av innganger/vinduer, på kveld/natt/gry. Prøv alle gamle interaksjoner og anomalireferanser mot de nye bygningsskallene. Hele byens utvendige oppgradering må kunne spores i leveransen.
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
- `Signal47/Source/Signal47/Public/S47Story.h` og `Private/S47Story.cpp`: dagens 23:41-start, tidshopp, 02:13-tekster og nattomregning. `Signal47/Config/Props/town.json` og `LIGHTING.md` viser fungerende native props-/materialmønster som nye bygg må integreres med.

Eksisterende visuelle referanser er `/home/tom/Desktop/Signal47 konseptbilder/Caliche-byen.png` og `/home/tom/Desktop/Signal47 konseptbilder/Mesa-Diner.png`. Retningen er brukt til bygningsplanen, ikke som påstand om ferdig gameplay. USNO-data for 13. april 1986 ved Roswell er kontrollert for kveldens lysreferanse; [API-beskrivelsen](https://aa.usno.navy.mil/data/api.html) forklarer dato, posisjon og MST-parameter. Den nøyaktige SARO-posisjonen og horisonten må styre endelig lyskalibrering.

Eksterne referanser undersøkt i forrige vurdering:

| Kilde | Hva beskrivelsen støtter | Designlesning brukt her |
|---|---|---|
| [Voices of the Void, utviklerside](https://mrdrnose.itch.io/votv) og [utviklerlogg](https://mrdrnose.itch.io/votv/devlog/1474386/voices-of-the-void-alpha-090) | Signalinnsamling og behandling, vanlige og uvanlige kilder; vedlikehold og fysiske hendelser omtales i loggen. | Vanlig arbeid gir et sted og en rytme som fremmed tilstedeværelse kan bryte inn i. |
| [I'm on Observation Duty, offisiell beskrivelse](https://store.steampowered.com/app/1046820/Im_on_Observation_Duty/) | Observere kameraer, huske normalen og rapportere type/sted for avvik. | En oppdagelse må ha en forståelig handling og respons. |
| [The Exit 8, offisiell beskrivelse](https://store.steampowered.com/app/2653790/The_Exit_8/) | Snu ved avvik og fortsette når det ikke er avvik. | Tydelig undersøkelsesregel kan sameksistere med usikker årsak. |
| [Signal Simulator, offisiell beskrivelse](https://store.steampowered.com/app/839310/Signal_Simulator/) | Signalbehandling, antenner, kalibrering og vedlikehold. | La tekniske kontroller gi spilleren ansvar for konklusjonen. |

Prosjektbiblioteket ble søkt med behov og teknologi, deretter ett bredere søk. Ingen relevant løsning ble gjenbrukt. Brainstorming og gauntlet-loop er brukt fra `../Automation/Skills/`. `game-design` var ikke tilgjengelig som egen skill.

Neste produksjonsarbeid starter med trinn 0 og hele trinn 1: mål baseline og lag tidsreferanseoversikten, bygg kvelden og den første sammenhengende gatebiten med aktive sammenligninger og gjenbesøk. Deretter følger trafostasjonen, de øvrige bybygningene og den fremmede tilstedeværelsen etter trinnene over. Planen endrer ingen kildekode, assets, lagringer, pakket utgave eller standardstarter.

## 13. Skyggemenn og oppbygging av ubehag, 25. oktober 2026

Toms idé: fullstendig svarte menneskeaktige skikkelser som står helt stille når de blir sett, og bare beveger seg usett. Anbefalingen er én pålitelig observasjonsregel, gradvis nærhet og usikker hensikt. Vanlig SARO-arbeid gir spilleren en grunn til selv å se bort. Etterforskningen lar spilleren kontrollere det som skjedde.

Dette er et nytt designforslag etter parallell gjennomgang av psykologi, eksisterende kode og møter. Det er ikke implementert, modellert eller spilltestet. Delene under utvider den tidligere planen; de omskriver ikke allerede leverte N01/N02/A02/P01. Bibliotekssøk ga ingen relevant, verifisert løsning. Den generelle UNVERIFIED-oversikten over Unreal-verktøy ga ingen gjenbrukbar mekanikk eller psykologisk dokumentasjon.

### 13.1 Forskningsgrunnlag og designvalg

| Forskningsfunn | Anvendelse som skal prøves i SIGNAL / 47 |
|---|---|
| I et forsøk med usikker sammenheng mellom visuelle signaler og elektriske støt fulgte rapportert stress, pupillrespons og hudledning usikkerheten. [de Berker mfl., 2016](https://www.nature.com/articles/ncomms10996). | Spilleren lærer en stabil blikkregel, men vet ikke når en figur kommer, hvor den vil eller hva den betyr. Vilkårlige regelbrudd gir ikke en bedre lærbar trussel. |
| Et forsøk med en virtuell forfølger og støt fant endret trusselrespons ved kortere avstand, sammen med mer gru og lavere tro på å unnslippe. [Mobbs mfl., 2007](https://pmc.ncbi.nlm.nih.gov/articles/PMC2648508/). | Avstand brukes som dramaturgi: først en person ved gjerdet, senere nærmere et kjent arbeidssted. Et sjeldent nært møte får en lesbar vei ut. |
| Mennesker som fulgte en visuell oppgave kunne overse et uventet, fullt synlig objekt. [Simons og Chabris, 1999](https://www.chabris.com/Simons1999.pdf). | Arbeidet skaper naturlige blikkvekslinger. Faste stolper og fotografier støtter hukommelsen. Spilleren straffes ikke for å overse første figur. Kameraretning måler ikke faktisk oppmerksomhet. |
| En feltstudie av 110 besøkende i en skrekkattraksjon fant en omvendt U-sammenheng mellom frykt og glede ved bestemte steder, men ikke i helhetsvurderingen. [Andersen mfl., 2020](https://journals.sagepub.com/doi/10.1177/0956797620972116). | Veksle mellom rutine, uro, en kort topp og reell lettelse. Konstant maksimal intensitet er ikke et dokumentert kvalitetsmål. |

Studiene tester ikke skyggemenn i dette spillet. Overføring til mekanikk, avstander, antall møter og pauser er designhypoteser. Ingen bestemt frekvens, infralydtone, synsvinkel eller avstand kan loves å skape frykt. Målet er engasjerende uro med korte panikktopper, ikke vedvarende press gjennom hele skiftet.

### 13.2 Regelen spilleren skal kunne stole på

1. Figuren er en original tredimensjonal menneskesilhuett, helt svart uten ansikt, øyne, glans, randlys, pust eller tomgangsanimasjon. Ingen automatisk kameravending mens den observeres. En vanlig, avslappet ståpositur prøves før monsterpositurer.
2. Direkte observasjon stanser alltid figuren. Første utgave bruker den romsligere regelen at også en synlig del i periferien fryser den. Dette unngår at en figur flytter seg i spillerens syn fordi siktekornet peker litt ved siden av.
3. Flytting skjer bare mellom forhåndskontrollerte ståsteder når både avgang og ankomst er helt usett. Figuren krysser ikke solide vegger eller gjerder. En kort sammenhengende periode ute av syn og margin ved bildekanten hindrer flimring; verdiene stemmes i prøven.
4. Lykt, zoom og foto får den ikke til å forsvinne mens den sees. Svartheten leses mot en lysere himmel, grus eller vegg. Bakgrunn og plassering løser lesbarheten; kroppen får ikke glød for å bli synlig.
5. Pause, meny, dokumenter, dialog, lasting og andre beskyttede sekvenser fryser møtet. Ingen oppspart tilnærming utløses når panelet lukkes. Den aktive fotosøkeren teller som observasjon; lukker/opptak beholder samme figurtilstand gjennom eksponeringen. Et stillbilde i albumet fryser ikke en fjern figur etter at vanlig spilling har begynt igjen.
6. Hvert møte har et lite, endelig antall mulige flyttinger. Figuren kommer ikke nærmere ved hvert hodekast resten av natten. Spilleren kan undersøke, holde den under oppsyn, trekke seg unna eller fortsette oppgaven. Ingen evig baklengs marsj eller blokkering av eneste utgang.
7. Møtestadium, ståsted, opplevde flyttinger og eventuelt tilfeldig valg lagres. Continue gjentar ikke en topp eller setter figuren nærmere. Hovedhistorien krever ikke at alle ser alle valgfrie skikkelser.

Behold pappfiguren og de særlige fotoomrissene som andre, gjenkjennelige fenomener. Ekte skyggemenn skal ha samme regel overalt. Hvis de eksisterende mørke skikkelsene beholder samme utseende, må deres gamle forsvinningsregel erstattes ved innføringen.

### 13.3 Første sammenhengende pilot

Bygg SARO-vindu, trafostasjon og retur som én spillerreise med én skikkelse. Ingen skade eller død i introduksjonen. Meter og varigheter nedenfor er utgangspunkt for utprøving, ikke forskningsfastsatte terskler.

| Del | Handling og endring | Hensikt |
|---|---|---|
| Normalen | Spilleren lærer utsikten fra kontrollrommet og gjør en ordinær kontroll. Gjerdet og en navngitt stolpe gir faste referanser. | Et sted spilleren kjenner, før avviket. |
| Første observasjon | En figur står omtrent 30 til 50 meter unna, mot lesbar bakgrunn. Den blir stående under direkte blikk, zoom, lykt og et valgfritt foto. | Lære svartheten og stillheten uten lydstikk eller kamp. |
| Første flytting | Spilleren velger å se mot et fysisk instrument og tilbake. Figuren har flyttet til neste gjenkjennelige sted. En modal instrumentskjerm gir ingen tilnærming; flytting kan først skje under vanlig spilling mens man ser bort. | Spilleren oppdager selv konflikten mellom arbeidet og observasjonen. |
| Trafostasjonen | Et faktisk tilgjengelig serviceoppdrag leder ut. Først vanlig ankomst, senere figuren utenfor gjerdet. Ved neste usette flytting forblir den på samme side. | Etterprøve regelen på et nytt sted uten å bryte rommets logikk. |
| Bevis | Kameraet tar ekte bilder med samme gjerdehjørne eller stolpe. Sammenligning kan fastslå ulik plassering, men ikke hensikt eller identitet. Manglende eller uleselig bilde gir utilstrekkelig bevis. | Undersøkelse gir kunnskap og et spor videre. |
| Retur | Ingen figur i kjørefeltet eller inne i bilen. Kontrollmaterialet arkiveres på SARO. Enten én siste fjern, stillestående observasjon eller bare fravær før en roperiode. | Avslutte møtet og la ettervirkningen vare uten enda et fremstøt. |

Trafoens senere T01/T02-fortelling er delvis forfattergrunnlag. Produksjonen må velge en eksisterende fungerende servicefase eller fullføre nødvendig oppgaveflyt først. Ingen påstand om at den nye spillerreisen allerede finnes i runtime. Ett av disse møtestedene kan utgå dersom begge føles som samme gjentakelse.

### 13.4 Videre eskalering og lyd

Etter piloten velges få møter fra denne rekken. De skal ikke automatisk komme ved hvert besøk:

| Sted | Ny erkjennelse eller handling |
|---|---|
| Arrayet | En menneskestor figur mellom parabolene gir målestokk. Senere posisjon ved en annen støtte knytter tilstedeværelsen til spillerens arbeidsrute. |
| Motor Court etter Nora | Figuren står på motsatt side av parkeringen etter at samtalen er ferdig. Rommet og samtalen får være i fred. Bil eller kjent dør gir et tydelig neste valg. |
| Dineren etter vanlig arbeid | En figur ute mot en lysere vegg kan sees etter lesing eller kartarbeid. Menneskestemmer og det opplyste rommet gir kontrast. Ingen tilnærming mens en bok eller meny er åpen. |
| Sen topp ved servicebil eller array | Først når regelen er lært: én nær figur, eventuelt en andre langt unna, og kort vei til en kjent dør eller bilen. Ett eller to usette fremstøt, lesbar retrett og rask avslutning. Nye skade-/dødsregler inngår ikke i første pilot. |

Noen tilbakeblikk skal vise nøyaktig samme plassering. Ikke alle tomme steder får et senere angrep. Bilen skal fortsatt kunne brukes, og ingen skikkelse tvinger frem påkjørsel. Speilmøter utsettes til refleksjoner kan omfattes av synsregelen pålitelig.

Lyd begynner med en kjent normal: vind, insekter, ventilasjon, trafosumming og radio. Et lokalt bortfall eller én tørr gruslyd fra figurens forrige ståsted prøves før musikkstikk. Ingen lydkvittering ved hver flytting. Bevar kritisk tale og gjenopprett normal ambience etter møtet. Romlig lyd kan støttes av valgfrie, nøkterne lydtekster uten å avsløre figurens posisjon med et HUD-merke.

Dale kan via eksisterende telefon/radio be om en konkret kontroll av adgangslogg eller hvor personen står. Han kjenner ikke monsterets regler eller årsaken. Forslag til nye engelske replikktekster er ikke ferdig manus eller innspilt dialog. Skyggemenn forklarer ikke Nora, Dale, aliens eller den tredje personen i 1947-bildet. THE EVENT, 47 sekunder, 1420.405 MHz, az 026 og 27 antenner beholdes.

### 13.5 Eksisterende kode og gjennomføringsrekkefølge

| Prioritet | Arbeid | Gjenbruk og avklaring |
|---|---|---|
| P0 | Samle regel og visuell kategori. | `Private/S47Glimpses.cpp`, `FS47Glimpses::Update`, skjuler i dag ved direkte blikk innen 25 grader, zoom, lykt eller nærhet. `Private/S47SaroDark.cpp`, `FS47SaroDark::Watch`, skjuler ved zoom, lykt eller nærhet. Ingen av dem har den nye bevegelsesregelen. |
| P1 | Lag original Blender-silhuett og tre til fire mulige ståsteder per pilotsted. | `Private/S47Figures.cpp`, `FS47CardSilhouette`, er et malt kort med rand og kameravending. Det er ikke en svart menneskemodell. Kontroller volum og underlag fra spillerhøyde i dagslys, skumring og natt. |
| P2 | Lag konservativ synlighetsport og liten lagret møtetilstand. | `Private/S47Seen.cpp` gir kamera og observasjonslogg; `S47Facing` tester ikke fri sikt. Hode, kropp og føtter eller silhuettvolum trenger både projeksjon og bevisst okklusjon gjennom glass og rundt native modeller. Ikke bruk «nylig rendret» som bevis for faktisk observasjon. |
| P3 | Koble hele SARO/trafo/retur-reisen til eksisterende rytme, foto og lagring. | `Public/S47Curve.h`, `Private/S47Night.cpp` (`CurveStep`, `OneAtATime`) og `Private/S47AnomalyDread.cpp` (`Protected`) gir felles trykkurve og beskyttelse. `Private/S47FieldCamera.cpp` (`Sync`, `ClearLine`, `Render`) gir ekte eksponering. Ingen konkurrerende tilfeldig skrekktimer. |
| P4 | Kort menneskelig prøve, deretter justering og noen få senere møter. | Figuren registreres ikke som opplevd bare fordi den er plassert. «Sett»-loggen er en teknisk tilnærming, ikke bevis for spillerens bevissthet eller frykt. Behold etablert restfase før neste topp. |

Filene i tabellen ligger under `Signal47/Source/Signal47/`. Kameraretning, utsyn i bil, FOV og glass må kontrolleres mot faktisk hovedbilde. Refleksjoner og andre levende kamerabilder krever uttrykkelig håndtering; første ståsteder velges slik at de ikke kan vise en ukontrollert flytting der. Generiske kollisjonsstråler kan overse dekor. Ikke anta at gamle hardkodede bygningsavtrykk beskriver alle de nye modellene.

### 13.6 Avgrenset godkjenning av en senere pilot

- Ingen posisjons- eller rotasjonsendring mens en relevant del kan sees, heller ikke med raskt hodekast, bilblikk, zoom, lykt eller aktiv fotosøker. Pause, panel og lasting gir ingen oppspart flytting.
- Samme figur og møtestadium gjennom lagring og Continue. Ekte fotopiksler viser den fotograferte plasseringen. Ingen oppdiktet filmbekreftelse eller ødelagte gamle foto.
- En spillbar SARO/trafo/retur-rute, med mulighet for å overse eller forlate møtet uten å låse historien. Kritisk dialog, den eksisterende A02-piloten og hovedhendelsen beholdes.
- Kort visuell kontroll i relevante lysforhold og VHS/Clean/lav kvalitet, samt menneskelig prøve med vanlig mus/tastatur og Xbox. Figuren må være lesbar uten å endre hele nattens eksponering.
- Spør etter prøven: «Hva trodde du regelen var?», «Når ble du urolig?» og «Når ble dette irriterende?». Sammenhold svar med faktiske oppdagelser og oppgaveavbrudd. Flere kameravendinger eller høyere puls alene beviser ikke bedre skrekk.

Bygg og et kort relevant pakket utvalg kommer når runtime faktisk endres. Ingen full verdensrunde, PSO-kø eller GPU-jobb er nødvendig for dette designarbeidet. Frykt, spilletid og ytelse for den nye mekanikken er foreløpig UNVERIFIED.
