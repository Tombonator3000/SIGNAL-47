# Forslag til forbedringer

Skrevet 3. oktober 2026, etter at kapittel 1 ("The Second Exposure") kom inn i nettversjonen. Punktene står i den rekkefølgen jeg ville tatt dem. Det som krever en beslutning fra Tom, står først, fordi det styrer det meste av resten.

En kortere versjon som kan deles, med en statuskolonne for beslutningene, en testliste og en grafikktabell for ChatGPT, ligger i Claude Docs: https://claude.ai/artifact/XQhxbdHbroo7gx7GuytZmK (privat til Tom deler den).

## SARO som knutepunkt, lastebilen når som helst, lommelykt og undersøkelse (15. oktober)

Tom 15. oktober: lastebilen skal kunne brukes når som helst i vanlig spill (avgjort). På sikt: SARO og observatoriet som et knutepunkt der flere rare ting skjer, motellet lenger unna så man kjører dit (eller går et stykke), gå ut og undersøke antennene, lommelykt, og høyreklikk for zoom og undersøkelse. Og: kan noe av det som står i rulleteksten til They Are Here brukes?

Dette flytter spillet litt bort fra beslutningen 4. oktober om korte, håndlagde etapper og ikke en åpen verden. Forslaget under holder det likevel lite: noen få steder på samme vei, ikke et åpent landskap.

### 1. Lastebilen når som helst (avgjort)

- Bilen står på SARO-plassen fra start. Man går til førerdøra og setter seg inn når man vil, og kjører på veiene som finnes: gården, riksveien sør til dineren og sporet til STATION 01. Man går ut på plassene: SARO, porten ved stasjonen, dineren, og senere motellet og antennefeltet.
- Stedene har en tilstand før kapitlet sitt. Porten ved STATION 01 er lukket før kapittel 3. Dineren er åpen, men servitrisen sier bare noe vanlig; utklippet, telefonen til Ward og kartet kommer først i kapittel 5. Rom 6 er låst til kapittel 4. Den gamle veien er sperret ved milestolpe 0 (`ROAD CLOSED`, sperrebukker) til kapittel 6, så slutten bare kan skje der den skal.
- Målet står i notatboka som før. Kjører man bort, sier en kort linje hvor man skal når man kommer tilbake. Klokka går som vanlig, og det lagres ikke under kjøring.
- Teknisk er mye på plass: fri kjøring finnes i utviklermenyen (`World.freeDrive`), og turene går allerede gjennom alle områdene uten kutt. Det som mangler, er at bilen alltid har en interaksjon (ikke bare når et kapittel overstyrer den), fri kjøring mellom flere områder i vanlig spill, og tilstandene over. Tester: `drives.py`, `devmenu.py`, kapittel 3 til 6 og lagringen.

### 2. SARO som knutepunkt der rare ting skjer

Hovedhistorien og den ene slutten står. Rundt den kommer små hendelser man kan oppleve når man er ute, og som legger et funn i notatboka (Findings). De er aldri nødvendige for å komme videre, skjer én gang hver, og følger regel R1: fenomenet følger referansene (1947-linjene, az 026, 47 sekunder, 1420 MHz). Ingen monstre, og ingen lys over motellet (Tom 4. oktober).

Ti eksempler å velge fra:

1. Alle antennene snur seg samlet mot az 026 i 47 sekunder, uten at noen sitter ved pulten. Driftsloggen viser ingen kommando.
2. De røde varsellysene på antennene blinker fire og sju ganger, én gang, og så er de som før.
3. Rimfrost på én antenne (S-11) en varm natt. Den smelter mens man ser på.
4. En ring av vissent gress rundt foten av S-19. Jorda er varm.
5. Fotspor i grusen ut mot S-03 som stopper midt på stien.
6. En bil står ved gjerdet med lysene på og motoren varm, ingen i den. Når man kommer tilbake, er den borte.
7. Bilradioen: AM-stasjonen blir stille i nøyaktig 47 sekunder, samme minutt hver time.
8. Telefonen på motellkontoret ringer når man står der. Ingen i den, bare suset fra hydrogenlinja.
9. Lys over mesaen i nordvest som bare synes i feltkameraet, ikke med øynene.
10. Lommelykta og bilen flimrer når man er nær et av stedene over, svakt, som et forvarsel om THE EVENT.

### 3. Motellet lenger unna

I dag ligger Sierra Motor Court rett over riksveien fra SARO, rundt 45 m unna. Forslag: flytt det rundt 700 m sør langs riksveien, på vestsiden, i veiens område slik dineren nå står der. Det blir rundt 40 sekunder i bil eller 6 minutter til fots (1,9 m/s), og neonskiltet blir et landemerke på turen sør til dineren. Rom 6 er fortsatt et eget område innenfor døra.

For å kunne gå dit må veiskulderen mellom SARO og motellet bli gangbar, noe veiens område ikke har i dag. Kapittel 4 må skrives om fra «gå over veien» til «kjør eller gå», og `chapter4.py`, `reachcheck.py`, `drives.py` og gjennomspillingen må følge med.

### 4. Ut til antennene

Antennefeltet ligger nord for nettinggjerdet ved z -20: 27 antenner fra 42 til 420 m ut, og i dag kan man bare gå til S-03. Forslag: en port i gjerdet, grusspor mellom antennene, og ved hver antenne et apparatskap med skilt (navnet, az og el), en stige og en kabelgrøft. Ting å undersøke: skiltene, en driftslogg i skapet, boltene, kabler som er kuttet eller skjøtt. Mellom natriumlampene er det mørkt, og det er der lommelykta trengs. Detaljene ved foten bygges bare nær spilleren, så mobilen holder.

### 5. Lommelykt

- Den ligger allerede på passasjersetet i lastebilen (tegnet i `Truck.ts`). Man tar den derfra, eller fra en skuff i kontrollrommet.
- F på PC, en knapp på mobilen.
- Lyset bygges som frontlysene: tre falske lyspunkter langs strålen i stedets eget lampesett, som er billig på mobil. På Ultra kommer ett ekte spotlys med skygge.
- En mørk metallykt fra 1980-tallet nederst i høyre hjørne. Senere kan en hånd holde den (forslaget om kroppen fra 5. oktober).
- Batteri: jeg anbefaler uten, men at den flimrer nær hendelsene (punkt 10 over).

### 6. Høyreklikk: zoom og undersøk

- Hold høyre musknapp for å se nærmere: synsfeltet går mykt fra 70 til rundt 30 grader. På mobilen en lupeknapp man holder. Ikke mens feltkameraet er løftet eller et panel er åpent.
- Undersøk: står siktet på noe som kan undersøkes, viser zoomen en kort tekst nederst (engelsk, i spillet), og funnet går i notatboka hvis det betyr noe. Først tekst og zoom. Senere, for noen få ting (skiltene, bolten, driftsloggen, lommelykta), et nærbilde man kan snu.
- Kapittel 1 har allerede «Examine contact print» i fotolaben. Den nye undersøkelsen gjør det samme ute i verden.

### 7. Lenkene i rulleteksten til They Are Here

Vi tar aldri noe fra They Are Here selv, bare fra originalkildene og under deres egne lisenser. Sjekket 15. oktober:

| Hva | Lisens | Kan vi bruke det? |
| --- | --- | --- |
| Horror Engine (Unreal Marketplace) | Bare for Unreal Engine | Nei, ikke i three.js |
| Flashlight (Sketchfab, Hepatitis_Bee) | CC BY, kan lastes ned | Ja, med kreditering. Kan bli lommelykta i hånda hvis den er lett nok |
| Bob Lazar Ufo (Sketchfab, Batuhan13) | CC BY, kan lastes ned | Kanskje, som en fjern silhuett i et glimt. Lazar-historien er fra 1989 og spillet fra 1986, så den må ikke kalles det |
| Phone, Desk_Table_Chair, Book, Old monitor (Sketchfab) | CC BY | Mulig, men vi har modeller i kode alt. Bare verdt det for PC-nivået, hvis de er lette og passer stilen |
| Computer Setup (Sketchfab, Brandon Westlake) | UNVERIFIED, siden svarte ikke | Sjekkes på nytt |
| Walkie Talkie, Textured (Sketchfab) | CC BY | Vi har en walkie; bare som referanse |
| Freesound-lydene (romlyder, fremmedspråk, slag, «Paranormal Activity Sound» og flere) | UNVERIFIED: freesound.org svarte 403 herfra. Rulleteksten sier selv at «Dark ambient scream» og «OhDearScare» er CC BY, og at Timbres to lyder er CC BY-NC | Hver lyd sjekkes på siden sin før bruk. CC BY-NC binder hvis spillet noen gang skal selges |
| Flickr-bildene | Ukjent per bilde. Flere er nyhetsklipp og filmbilder (Independence Day er beskyttet) | Nei, høyst som referanse. De passer heller ikke i 1986 |
| Video fra Pexels (Luis Quintero) | Pexels-lisensen: fri bruk | Innholdet er ukjent, og video passer dårlig i stilen |

Det som brukes, føres i `THIRD_PARTY_NOTICES.md` og i rulleteksten. 3D-modeller er noe nytt i spillet (`GLTFLoader`, større enkeltfil), så det bør være noen få utvalgte. Repoet har ingen lisensfil for selve spillet.

### Rekkefølge jeg foreslår

1. Lastebilen når som helst, med tilstandene før hvert kapittel. Avgjort, middels stort.
2. Lommelykt og høyreklikk med zoom og undersøk. Grunnmuren for å gå ut og se.
3. Antennefeltet gangbart med port, spor og skap ved antennene.
4. Motellet 700 m sør, med kapittel 4 skrevet om.
5. Hendelsene, tre eller fire først, så flere.

Codex kan ta en avgrenset del underveis, for eksempel skapet og skiltet ved antennene som egen modul med kontrakt (som Kessler-porten), eller å gjøre Flashlight-modellen klar (sjekke polygoner, lage en liten glTF, lisensnotat).

**Det Tom må avgjøre:** (1) Motellet sør mot dineren (min anbefaling) eller nord gjennom antennefeltet? (2) Lommelykt med batteri eller uten? (3) Den gamle veien før kapittel 6: sperret ved milestolpe 0 (min anbefaling) eller åpen uten at noe skjer? (4) Skal hendelsene bare være stemning og funn, eller kan de også gi hint i hovedsaken?

**Tom svarte 15. oktober:** (1) motellet sør, (2) lommelykt uten batteri, (3) den gamle veien sperret ved milestolpe 0 til kapittel 6, (4) hendelsene kan også gi hint i hovedsaken. Punkt 1 og 2 i rekkefølgen bygges først.

## Innlevelse, lyd, grafikk og Unreal (5. oktober)

Tom spurte hva spillet trenger for mer innlevelse, om prosjektbiblioteket har noe vi trenger, om vi bør dekompilere Voices of the Void, og om spillet kan flyttes til Unreal. Hele natten er nå bygd, så dette handler om å løfte det som finnes.

**Det som gir mest, i rekkefølge:**

1. **Stemmer.** Tomás, Nora, Ward, radioen og telefonen er i dag sus i et stemmebånd med tekst under. Ekte stemmer på de 15 til 20 replikkene som høres (ikke alt som står i panelene) gir mer enn noe annet. Enklest: Tom og to eller tre andre leser dem inn med telefonen, og jeg legger på telefon-, radio- og båndfilter i koden. Alternativt designede stemmer (ikke kloning av ekte personer) laget lokalt på Toms PC.
2. **Lydbilde per sted.** Fugler som begynner ved gryet, vind og grus på den gamle veien, lysrørsus og kjøleskap i dineren, aircondition og tv-sus på motellet, metallknepp i antennene når de snur. Lydene finnes som CC0 (Freesound, Kenney) eller kan lages i koden.
3. **Musikk, sparsomt.** Spillet har bare tittelmusikken. Lave flater og stikk i noen få øyeblikk (utskriften kl. 02:13, kabelbruddet, Nora), ikke under kjøringen ut på veien. Kan lages i koden med Web Audio, eller hentes fra Scott Buckley (samme lisens som tittelmusikken).
4. **Grafikk i three.js.** Gryet over mesaen med skyer og lysstråler, tettere vegetasjon og steiner nær veiene, bedre folk i dineren og på motellet (enkle animasjoner), og på PC fargegradering og dybdeskarphet i overgangene. På sikt WebGPU-rendereren i three.js for PC.
5. **Kroppen.** Hender som holder kameraet og lommelykta, pust når det er kaldt, at hodet ser ned på papirene.

**Prosjektbiblioteket:** relevant er `scottstts/Threejs-Awesome-Graphics-Agent-Skills` (MIT: lys, atmosfære, vær, materialer i three.js), `ektogamat/threejs-conference` (MIT: WebGPU og TSL, våt asfalt, regn, filmisk etterbehandling i førsteperson) og `debpalash/VoiceStudio` (AGPL, lokale designede stemmer; programmet må kjøres på Toms PC, og lisensen gjelder programmet, ikke det som lages). `YuE` (musikkgenerering) krever et kraftig skjermkort og er ikke nødvendig. Lydene vi allerede bruker fra Morbidium og Loincloth Legends står i `THIRD_PARTY_NOTICES.md`.

**Voices of the Void:** ingen dekompilering for SIGNAL / 47. Spillets kode og assets har ingen lisens for gjenbruk, og kopien ligger bare på Toms PC. Det nyttige er å spille det og skrive ned hva som gjør stemningen (lyden i basen, signalbehandlingen, radioprat, døgnrytmen), så lager jeg vår egen versjon fra beskrivelsen.

**Unreal:** mulig, men ikke nå. Det gir mye bedre lys og folk (Lumen, MetaHuman, volumetrisk tåke, Sequencer for slutten), men Unreal kan ikke lenger lage nettversjoner, så Pages og spilling på telefon i nettleseren forsvinner. Alt spillet gjør (kapitlene, gåtene, panelene, lagringen) må skrives på nytt i C++ og Blueprints, og Unreal-editoren kan ikke kjøres i skymiljøet jeg jobber i, så all bygging og testing må skje på Toms PC. Forslag: gjør nettversjonen ferdig og spill den igjennom. Vil Tom prøve Unreal, start med en liten bit (den gamle veien i gry og THE EVENT) som egen prøve på hans PC, og bestem etter den.

## Status 4. oktober

Tom ba meg gjennomføre forslagene. Gjort og i spillet: arbeidsordren (3.1), kapittel 2 (3.2, heter «The Amended Record» i spillet), saksmappen (3.3), fotografiene i IndexedDB (3.4), de nye innstillingene og fotsteg ute (4). Teksturene (4) kom fra ChatGPT/Codex i PR #30 og #31 og er koblet inn med tekst i kode oppå. Ikke gjort: skjuling av smådetaljer på avstand (4), fordi målingene ikke viser behov ennå, bytte av GitHub-handlinger og npm audit (ingen fiks finnes). Del 2 og 5 gjelder fortsatt. Neste store steg er kapittel 3, feltreisen til STATION 01 (se todo.md).

## 1. Beslutninger Tom må ta

Avgjort 4. oktober: Tom ba meg gjennomføre forslagene mine, så alle fire er avgjort slik jeg foreslo under. Se memory.md.

1. **Rom 6 eller rom 47.** Designbibelen (`Docs/DesignBible13/design-bible.md`, K4) legger Nora Vega i rom 6 på Sierra Motor Court, og sier at kontoret og rom 6 er de eneste interiørene. `ART_BRIEF.md` og `todo.md` snakker om rom 47. Mitt forslag er rom 6. Tallet 47 bærer allerede mye (47 sekunder, SIGNAL / 47), og et rom med samme nummer kan fort virke som en vits.
2. **To eller tre slutter.** Designbibelen har to: A "Bryt referansen" og B "Fullfør én registrering", og skriver rett ut at det ikke finnes noen tredje, hemmelig slutt. De tre sluttene Silence, Answer og Listen kommer fra ChatGPT-samtalen. memory.md påsto at designbibelen også hadde tre. Det stemte ikke, og er rettet.
3. **Dale, R. og Reyes.** Webversjonen lar Dale skrive skiftloggen. Unity signerte "R.". Designbibelen kaller spilleren Reyes og vaktsjefen Dr. Evelyn Ward. Forslag: behold Dale som kollegaen som gikk hjem, og la Reyes og Ward komme inn gjennom arbeidsordren (punkt 3.1).
4. **Bilen bak gjerdet og telefonnummeret til motellet.** Står i todo.md under kapittel 1, men finnes ikke i Unity-kapitlet. I designbibelen er det arkivet i K2 som viser at Nora driver Sierra Motor Court. Et telefonnummer på et bilde i kapittel 1 hopper over den oppdagelsen. Forslag: la det være. Vil Tom ha et hint, kan motellskiltet lyse svakt i horisonten på FRAME 01, uten nummer.

## 2. Det som bør testes på ekte maskiner

Alt under er sjekket headless, men ingenting er prøvd på en ekte telefon ennå.

- Spill prologen og hele kapittel 1 fra https://tombonator3000.github.io/SIGNAL-47/ på telefonen. Se særlig etter: om kameraknappen og utløseren er lette å treffe med tommelen, om lupe og X/Y-glidere i fotopanelene er brukbare, og om fotolaben er for mørk.
- Legg `?debug` bak adressen og skriv ned fps-linja tre steder: ved skrivebordet, ute på gangveien og inne i fotolaben. Målt antall draw calls i skyen er 34 til 202, med 202 som verste når man ser sørover mot kontrollrommet fra B-12. Budsjettet er 300.
- Lyd: vinden blir sterkere ute, S-03-skapet summer når man står ved det, og sikkerhetslampen i laben surrer svakt. Si fra om noe av det blir for mye.
- Prøv Continue etter å ha lukket fanen midt i kapittel 1. Saken og begge fotografiene skal komme tilbake.

## 3. Neste store steg

### 3.1 Arbeidsordren og Ward (lite, gir mye)

Designbibelen sier at K1 skal åpne med en kort arbeidsordre om morgenserien, med Dr. Evelyn Ward som vaktansvarlig på telefon. Nettprologen har ikke dette. Et ark på pulten ved skiftloggen holder: hvem Reyes er, at morgenserien starter 06:00, og at avvik skal dokumenteres før den kan kjøres. Det gir spilleren en grunn til å bry seg om rapporten i kapittel 1, og det knytter overgangen til K2. Telefonen bør ikke brukes til Ward før framtidssamtalen, den skal fortsatt være en overraskelse.

### 3.2 Kapittel 2: "Den strøkne protokollen"

Etter designbibelen: Ward gir tilgang til et lite arkiv på SARO fordi B-12-rapporten viser et referanseproblem. Tre samlinger kan leses i valgfri rekkefølge (gammel feltjournal, korrigerte protokoller, moderne vedlikeholdskart). To sammenkoblinger er nødvendige. Feil valg viser hvilket dokument som ikke støtter koblingen, uten at spilleren må gå en runde til. Til slutt en kort telefon til Nora og et reisepunkt som låses opp.

Det meste av verktøyet finnes allerede. Panelene for kontaktkopi og referansefil (`src/ui/Panels.ts`) kan brukes til å legge to dokumenter ved siden av hverandre og peke på det som er fjernet. Arkivrommet kan bygges som fotolaben: én liten blokk med egne lamper, inngang fra servicegården eller fra gangen sør i kontrollrommet.

Jeg anslår omtrent like mye arbeid som kapittel 1.

### 3.3 Saksmappe i notatboka

Spilleren har nå et motorlogg-ark, to fotografier, en observasjon, et funn og en rapport. De ligger som enkeltdokumenter. Før kapittel 2 bør notatboka få en fane "Case file" med små utgaver av fotografiene og dokumentene, så alt kan leses igjen samlet. Det er samme idé som evidence board i todo.md, bare enklere, og det trengs uansett når arkivet kommer.

### 3.4 Lagring som tåler flere kapitler

Saken lagres i `localStorage` (`s47.case`), med fotografiene som JPEG-tekst. Målt etter et helt kapittel: 414 kB, altså rundt 200 kB per bilde. Nettleserne gir omtrent 5 MB per opprinnelse, og alle Toms Pages-sider deler opprinnelsen `tombonator3000.github.io`, så plassen deles med de andre prosjektene. Designbibelen har seks eksponeringer. Flytt bildene til IndexedDB før kapittel 2, og la `localStorage` bare holde saken og hvor langt man har kommet.

## 4. Mindre forbedringer

- **Innstillinger:** i dag finnes lydstyrke, musfølsomhet og grafikknivå. Invertert Y-akse, synsfelt (FOV) og tekststørrelse er billige å legge til og gjør mye for noen spillere.
- **Fotsteg ute:** samme trinnlyd brukes inne og ute. Litt lavere avspillingshastighet og et svakt grusaktig lag på betongen ute vil gjøre overgangen tydeligere.
- **Teksturer fra ChatGPT:** kapittel 1 har fått mange flater som tegnes av koden og kan byttes mot ekte grafikk uten å endre spillet: S-03- og B-12-skapene, B-12-skiltet med stripen, feltkartet i laben, gulvmerkingen på gangveien, skiltene og prosesskortene i fotolaben. Jeg kan legge dem inn i `ART_BRIEF.md` med mål og format hvis Tom vil.
- **Ytelse hvis telefonen sliter:** kontrollrommet tegnes fullt når man ser det gjennom vinduene fra B-12. Det er det dyreste synsfeltet (202 draw calls). Smådetaljer inne i rommet kan skjules når spilleren står mer enn 15 meter unna.
- **GitHub-handlinger:** configure-pages v5, setup-node v4 og upload-pages-artifact v3 er laget for Node 20, og GitHub tvinger dem over på Node 24. Publiseringen virker. Bytt til versjoner laget for Node 24 når de er bekreftet å finnes.
- **npm audit:** 3 high i byggverktøyet (braces via vite-plugin-singlefile). Gjelder bare bygging, ikke spillet. Oppgrader pluginen når den får en fiks.

## 5. Det jeg ikke har kunnet sjekke

- fps, berøring og lydmiks på ekte telefon og PC.
- Safari på iPhone og iPad. Headless-testene kjører Chromium.
- Hvordan kapittel 1 føles i tid. Designbibelen budsjetterer 50 minutter for hele K1, prologen inkludert. Testene går gjennom på noen sekunder fordi de hopper mellom punkter, så det sier ingenting om spilletiden.
