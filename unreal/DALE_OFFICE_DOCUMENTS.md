# Dales kontor: feltmappe, UFO-materiale og lesbare dokumenter

Status 26. oktober 2026: De seks private dokumentene er registrert i EveningShift og integrert med individuell tekstvisning, tekstskalering og lagret lesestatus gjennom NP1. Den installerte R5-/terreng-R2-linjen bevarer dette, med fysisk kontoradgang allerede ved ankomst og lagring av faktisk lest innhold før overleveringen. Mappeåpning gir ingen uleste dokumenter eller gratis lesestatus. Se [ankomst-/kontorleveransen](ARRIVAL_OFFICE_2026-10-26.md).

Bilderekvisittene er produsert separat: [Dokumentpakken](ArtSource/Documents/README.md) har alle sju eksisterende ark, de seks private tekstmotivene med skisser/foto, et mappeomslag og eldre arkivmateriale som JPG. Import, UV-/materialbinding, bildevisning og eventuell kontrollert bruk som fotobevis gjenstår. Tekstintegrasjonen beviser ikke at bildene er synlige i spillet. Ingen VHS-film er produsert.

25. oktober 2026. Vedlegg til [hovedscenarioets SC07](MISSION_FLOW_1800_ROSWELL.md#sc07-det-dale-ikke-leverte-inn). Dette viderefører tidligere kontorideer i [ANOMALY_PLAN](ANOMALY_PLAN.md) og [EveningShift office.json](Narrative/EveningShift/modules/office.json), uten å erstatte deres eksisterende ID-er. De seks engelske tekstene nedenfor var opprinnelig manusutkast og er senere integrert som spilldokumenter; de er fortsatt fiktive, ikke historiske kilder. Rom-/historieregien nedenfor beskriver SC07-målet, ikke en levert eller prøvd full SC01–13-rute.

## Et menneskes arbeidsrom

Spilleren har allerede sett én overflygning, flere senere lys, en kropp og en bortføring når kontoret undersøkes ordentlig. Derfor virker samlingen som gjenkjennelse: Dale har hørt om lignende ting og forsøkt å forstå dem alene. Han er verken allvitende konspirasjonssjef eller en hemmelig fiende. En nøktern overlevering ligger over en privat mappe han ikke ville sende videre uten bedre grunnlag.

Tre lag i rommet:

1. Vanlig jobb: kaffering, påminnelse om å kjøpe melk, skiftark, serviceskjema og radioskjema. Lesbare praktiske dokumenter skaper kontrast til det private materialet.
2. Feltmappe: én tydelig, nødvendig servicehenvisning med navngitt kameraståsted. Kort hovedrute, uten dokumentjakt i alle skuffer.
3. Privat samling: avisutklipp, daterte vitneark om lys og humanoider, fotokonvolutt, håndtegnet lysbane, et usendt brev og en lånt offentlig kontaktforelesning. Hvert objekt har opphav og begrensninger. Skisser/fotografier finnes nå i den separate JPG-kildepakken; deres fysiske binding og bruk i spillet gjenstår.

En leser kan bruke noen minutter her, en annen ta feltmappen og gå. Første lesing er beskyttet mot aktivt angrep. Etter at spilleren lukker materialet, kan et kjent vindusutsyn ha endret seg eller en lyd utenfor kreve oppmerksomhet. Ikke straff hver skuff med jumpscare. Læring og ubehag skal ha plass samtidig.

## Eksisterende materiale som beholdes

| ID | Eksakt tittel | Funksjon |
|---|---|---|
| `EV_HANDOVER` | Evening handover / Sunday, April 13 | Ward/Dale, 27 antenner, referanse 026, merket serviceskuff og praktisk overlevering. |
| `EV_SERVICE_LOG` | Caliche service log / released copy | Daterte lampemeldinger og åpen inspeksjon. En tidligere rapport beviser ikke senere strømtilstand. |
| `EV_RADIO_SCHEDULE` | 1420 AM / transmission schedule | Ordinær plan og Earls Night Line 00 til 04. Plan er ikke faktisk sendt lyd. |
| `EV_DALE_INDEX` | Field checks / working index | Hovedspor til serviceveien: kulvert, stolpe med to hvite bånd, nærmeste stolpe/ledere og kameravinkel. |
| `EV_DALE_CLIPPINGS` | Local cuttings / dates to check | Fiktive lokale utklipp om bilstans og lampe etter linjefeil, med Dales kritiske merknader. |
| `EV_DALE_CONTACT` | Field contact / keep with the radio | Oppfølging via eksisterende telefon/tjenesteradio, ingen oppdiktet frekvens. |
| `EV_DALE_NOTE` | A note beneath the inspection sheets | Personlig forklaring på hvorfor han samlet stoffet. Et spor, ingen fasit. |

Operations-hovednøkkelen er hjemme hos Dale. Tydelig merket reserve ligger fysisk i serviceskuff ved arbeidsbenken. Den skal ikke blandes med brevets historiske opplysning om en nøkkel i et krus, som brukes i Nora/Tomás-sammenligningen og ikke er en ny nåværende rekvisitt. Ta feltmappen gir den nødvendige indeks-/notatkopien; øvrig samling er valgfri. Dale kan reagere på at spilleren fant reserven og innrømme samlingen gjennom telefon/radio/walkie. Han kommer ikke fysisk inn i rommet.

Det separate ordinære `EV_DALE_ARRAY_SERVICE` er registrert og lesbart gjennom NP1, med A / ROW 1 WEST, B / ROW 1 EAST og REF / R1. Selve ARRAY-01-mekanikken og den nye sammenhengende scenarioruten gjenstår. Vedlegget er adskilt fra de seks frivillige loretekstene; eksisterende EV_DALE_INDEX peker fortsatt til T02s kulvert-/kameraståsted.

## VHS og opphav

`VHS_DALE_FIELD_01`, «Service Road / Field 01», har et eksisterende 72-sekunders produksjonsmanus med miljølyd, navngitt ståsted, stabil stolpe/ledere og en uklar bevegelig forskyvning. **Den faktiske filmen er ikke produsert eller spillbar i gjeldende kilde.** `S47Office.cpp` sier dette og setter ikke `vhs_seen` som om spilleren hadde sett film. Senere produksjon krever bevegelse, play/stopp/spoling, strøm-/avbruddsforløp og lagret avspillingssted. Feltmappen/indeksen bærer hovedsporet uansett; et eget fotografisk kontaktark er ikke et eksisterende dokument i office.json.

Kameradatoen er feil; ikke avled presist hendelsestidspunkt fra den. Ingen Dale-/Kinski-forteller i videoen. Hans eget opptak er ikke samme kilde som en offentlig Unarius-/kontaktforelesning. Dersom sistnevnte skal ha ekte lyd/bilder, må konkret materiale, datering og bruksrett avklares senere; det nye fiktive omslaget nedenfor krever ikke et faktisk tredjepartsklipp.

## Nye frivillige lesedokumenter

`EV_DALE_*`-ID-ene nedenfor er innført i EveningShift-JSON og runtime. Tekstene åpnes enkeltvis i vanlig dokumentvisning med tekstskalering og faktisk lesestatus. Det er ingen nye stemmeopptak knyttet til dem. De inneholder lokal UFO-/humanoiduro, ingen avsløring av Nora, Tomás, Reyes i 1947 eller tidspunktet for en invasjon. Ingen tekster trenger fortellerstemme.

### EV_DALE_SKYLOG: Lights above the ridge / 6 April

Plassering: håndskrevet ark med en enkel horisontskisse i privatmappen. Den planlagte skissen er et minne fra et vitne, ikke målt flygeometri. Gir gjenkjennelse til kveldens tidlige lys.

> 6 APR. Kitchen clock about 20:40. One white light crossed above the ridge and stopped where there is no tower. I went outside. It moved again before I found the binoculars. Aircraft remain possible; I lost the horizon behind the roof. What bothers me is the dog. He kept watching the empty place for another ten minutes.

### EV_DALE_WITNESS: Statement copied at the service desk / 29 March

Plassering: avskrift med dato og Dales initialer, ikke et offisielt politibevis. Gir direkte, ubehagelig humanoidstoff uten å erklære art eller identitet.

> Mrs. Bell says she heard someone crossing the yard after the porch lamp failed. The shape stood beside the washing line, taller than her husband. She called his name. It bent toward the window without taking a step. Her husband was asleep upstairs. She asked me to write “person,” not “creature.” I have kept her wording. I did not see it.

### EV_DALE_EXPLANATIONS: Explanations retained / working sheet

Plassering: flere kryssede saker foran de åpne. Gjør Dale troverdig og lar normale forklaringer fortsatt være sanne uten å bortforklare spillerens faktiske møter.

> Three reports were aircraft. One bright object was visible from the same chair on four clear evenings: a star, made strange by the window glass. Keep those pages. Crossing out an explanation does not make the next report true. The service-road account remains open because neither witness describes the same direction. Ask separately before showing either one the other statement.

### EV_DALE_FRAME18: Photograph envelope / frame 18

Plassering: konvolutt med dårlig eksponert fotografi/negativ som motiv i JPG-kildepakken. Bildet må fortsatt bindes og granskes i spillet før det eventuelt brukes som bildebevis. Vanlig sammenligningspunkt skal være synlig hvis fotoet brukes.

> Do not crop the left edge. The porch post is the only useful measure of height. The pale mark might be glare from the kitchen window; the negative needs checking. What I remember is a shape standing beyond the fence. What the photograph contains may be something else. Keep both sentences. I am starting to replace one with the other.

### EV_DALE_TAPELOAN: Borrowed cassette / return sleeve

Plassering: lånt offentlig kontaktforelesning, tydelig atskilt fra hans felt-VHS. Gir 1986-miljø og viser hvordan ord kan spre seg mellom vitner uten å skape uavhengige bevis.

> Public lecture, borrowed from a neighbor. Speakers describe visitors, contact and messages from elsewhere. No local recording. No connection established with the service-road tape. I kept it because people use these words when they call, and I need to know what they have heard already. Return it on Tuesday. Do not record over the label. There are family birthdays underneath.

### EV_DALE_UNSENT: Unsent letter / no address

Plassering: under vanlige arbeidsark, ikke låst som en nødvendig hemmelighet. Dale blir en person med søvn, handleliste og skam over hvordan opplevelsene høres ut.

> I was going to tell you about the lights, then I read this back and heard how it sounded. You would ask whether I am sleeping. Fair question. I still do the shopping. I still forget the milk. I would like one evening when the telephone rings and I expect an ordinary problem. Do not mail this. Get some sleep first.

## Integrasjon og kontroll

De seks tekstene og servicevedlegget har sine autoritative kilder i EveningShift, som allerede eier kontoret. Legg eventuelle senere tekstendringer der først. HorrorStory skal referere til funnene og skrive hovedmotivasjon, ikke duplisere alle dokumentene eller overleveringen. Behold eksisterende ID-er, kjør tilhørende generator og registrer nye ID-er før runtime kobles på. Ikke rediger genererte manus som kilde.

Avgrenset R5-kontroll har prøvd tidlig fysisk kontoradgang, frivillig lesing/skalering og diskrestore av faktisk leste dokumenter, også på Low. Kort feltmapperute er uavhengig av VHS. Ingen ulest tekst markeres som lest; ingen film flagges sett fordi et panel ble åpnet. Nye JPG-bindinger, faktisk avbrutt film, videoens transport-/strøm-/saveforløp og hele den nye SC-ruten er ikke levert av disse kontrollene. Nøyaktige bevis og begrensninger står i ankomst-/kontorrapporten.
