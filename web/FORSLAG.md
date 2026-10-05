# Forslag til forbedringer

Skrevet 3. oktober 2026, etter at kapittel 1 ("The Second Exposure") kom inn i nettversjonen. Punktene står i den rekkefølgen jeg ville tatt dem. Det som krever en beslutning fra Tom, står først, fordi det styrer det meste av resten.

En kortere versjon som kan deles, med en statuskolonne for beslutningene, en testliste og en grafikktabell for ChatGPT, ligger i Claude Docs: https://claude.ai/artifact/XQhxbdHbroo7gx7GuytZmK (privat til Tom deler den).

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
