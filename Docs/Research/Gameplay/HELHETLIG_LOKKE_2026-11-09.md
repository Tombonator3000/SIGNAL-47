# Helhetlig spilløkke: SARO-jobben og undersøkelser i verden

**Status:** Forslag, ikke vedtatt. **Gjelder:** Freeform-løkka (jobbkort, avvik, saker, steder, natthendelser, utstyr, byen). **Sist kontrollert:** 9. november 2026.

Tom ba 9. november om design- og kritikeragenter (ultracode) for å knytte jobben på SARO tettere til undersøkelser i verden. Arbeidet gikk i fire ledd: én kartlegger, tre designretninger (signalet peker ut, arbeidsdagen går ut, tråder og folk), tre kritikere (spiller, system, gjennomførbarhet) og én samler. Kritikerne ga «signalet peker ut» 7/10 alle tre, «arbeidsdagen» 5–6 og «tråder» 5–6. Bare koden er lest. Arbeidet føres i [TODO](../../Active/TODO.md) (LOOP-WHOLE-20261109).


Jeg er enig i diagnosen din, men vil justere ønsket på ett punkt. Ikke *alt* arbeid på SARO bør peke ut. Rutinekortene bør fortsatt være ankeret. Får hvert kort en tur ut, blir SARO en oppdragstavle, og tid slutter å være valuta. Det viktigste vi mangler er ikke flere ærend. Det vi mangler er at feltet stiller et spørsmål som bare stasjonen kan svare på, og omvendt. Mitt forslag er at høyst én ting per natt peker ut, og at turen hjem skal være nysgjerrighet, ikke papirarbeid.

Dette bygger på kartet, tre designforslag og tre kritikere. Jeg har sjekket de viktigste påstandene i koden selv. Ingenting er redigert eller bygd. Kodefilene ligger i `/home/tom/SIGNAL-47/unreal/Signal47/Source/Signal47/Private/` når ikke annet står.

## 1. Dagens løkke og de største hullene

I dag følger kveldsjobben dette mønsteret: tre kort, deretter registrering på OPS, så lønn. Turene ut gir mest bilder, steder og spor. De to delene møtes for alvor bare i sak 3–7 og i den store natta, og de er sjeldne eller kommer sent.

1. **Kveldsjobben er lukket.** Kortene har faste kilder og klokkeslett, og aldri et sted (`S47FreeformJobs.cpp:16-21`, `:66`). Den eneste peilingen ut krever reparerte grener, så den gir ingenting i uke 1 (`S47Places.cpp:446`). Kartet tok feil om at kallet bare skjer ved rutinejobber: `AfterRegistered` kalles allerede ved avvik, og den sikter mot gården når gården er aktiv.
2. **Sak 0 og 1, som alle spillere ser, testes bare på SARO.** Ut kommer man bare for å handle drivstoff eller relé i Caliche. Svaret i sak 0 lover en feltlogger «for an independent second site», men den prøven står ved østre kant av arrayet (`S47FreeformCases.cpp:156`, `:185`). Det er et løfte spillet bryter.
3. **Sak 2 er et sluk.** Alle de 31 natthendelsene, flyttede gjenstander og viltkameraet havner i VISITORS AROUND THE BASE (`S47FreeformCases.cpp:284-286`). Der teller de ikke, og prøvene er takplaten og porten. Taket `MaxLeads=32` (`S47FreeformCases.h:20`) gjør at spor forsvinner stille.
4. **Utstyr uten konsekvens.**
   - Magnetometeret har en fast tekst per sted (`S47Places.cpp:386-389`).
   - Radiopeileren gir bare en toast (`S47Outfit.cpp:118`).
   - Svaret når arrayet holdes på et punkt teller ingenting (`S47Answer.cpp:180`).
5. **Belønninger som ingen kode leser.** `access.nightwatch`, `visitors`, `sameminute`, `emptyPost`, `printedMinute` og `rakedGround` gis, men leses aldri. En lukket sak gir bare SheriffTrust +1.
6. **Ingen pris for å la være.** Bare sak 3 har en frist («neither lasts past noon»). Alle andre spor kan ligge for alltid, så valget mellom kort 3 og en tur ut er aldri ekte.
7. **Tilbakemeldingen kommer for sent.** Det meste kommer først i Wards vurdering etter 30 dager. Bare regissørens hendelser kan registreres på OPS, ikke de 31 natthendelsene (`S47FreeformDirector.cpp:1715-1731`). Ward teller likevel alle bilder i albumet.
8. **Uke 1 har ingen kobling som virker med bare bil og lommelykt.** Radiopeileren kommer først dag 15 og koster $120 (`S47Openings.cpp:45`). Det spilleren lærer rundt den store natta 2–3, er at SARO og landet er to adskilte spill.

## 2. Anbefalt løkke

**Ryggraden** er «Signalet peker ut» fra designforslag 1, fordi det er billigst, kan testes og bruker det som finnes (`FBearing`, `Fixes`, `FillMap`). Fra forslag 2 tar jeg delingen av sak 2 og stedsvalget etter gårsdagens spor. Fra forslag 3 tar jeg Earl på lufta og en ekte mottaker for `access.nightwatch`. Kritikerne la til de fire grepene som gjør løkka helhetlig:

- **Spørsmål begge veier:** feltet gir et tidsstempel, og stasjonen svarer på om noe skjedde samme minutt.
- **Spor kjølner synlig.**
- **Frykt som pris:** bedre informasjon jo seinere spilleren drar.
- **Tomme turer har en hensikt:** de utelukker et sted eller en forklaring.

**Dette tar jeg ikke med:**
- kryss fra samme stasjon (to peilinger fra SARO krysser aldri hverandre),
- fire tillitsmålere,
- ASK-poster formet som en oppdragslogg,
- +$3 på toppen av lønna,
- en ny kilde 4 på jobbkortene. Kilde 4 bryter `Valid`, klemmingen av kilde til 0–3 og `Runs()`, som deles med Story.

Oppfølgingen bygges i stedet på den veien som allerede finnes, der en sak lager et jobbkort (`PlanCaseTest`, `S47Freeform.cpp:2894`).

**Eksempel, uke 1, natt 4** (ingen reparerte grener, ingen radiopeiler):
1. **19:52.** Pulsarkortet viser ekstra tikk. Spilleren registrerer det som «unresolved» på OPS. Sak 0 åpnes, og skriveren gir en peilelapp: «about 110°, ±40°, 19:52. Cold by noon.»
2. **Kartet** viser en bred sektor. Vindmølla og varden er uthevet som kjente steder inne i den.
3. **Kort 3 kl. 22:05 eller en tur ut?** Spilleren drar kl. 22:30. Klokka er før 24, så bare hint: ferske dekkspor og en grind som står åpen. Begge kan bortforklares. Hun tar bilde.
4. **Tilbake på SARO kl. 23:40.** Bildet havner i den nye saken OUT ON THE LAND. Nattloggen viser at arrayet ikke har noe i det minuttet bildet ble tatt. Dermed er «samme kilde» ennå ikke vist, men grinden er nå et spor med tid.
5. **Neste kveld** er saksprøven «Repeat in the same window» spilleren selv velger. Telexen har allerede gitt hypotesen: en kilde på himmelen kommer 3 min 56 s tidligere hver natt, mens noe på bakken holder klokketid. Tikket kommer på klokketid, og peilelappen peker ut igjen. Spilleren har selv funnet ut at det er noe på bakken.

**Eksempel, uke 3, natt 17** (grenene er reparert, sektoren er ±12°):
1. **Morgen.** Svaret i sak 0 har gitt feltloggeren. `access.nightwatch` lar spilleren slå opp i sheriffens nattlogg for én natt og ett klokkeslett: «lights east, 01:10, the 14th.»
2. **21:10.** Avviket gir en smal peiling som går gjennom området ved Saucer Rock. Spilleren dropper kort 3, setter feltloggeren der og drar hjem før midnatt.
3. **Å dra ut igjen etter 02** er farlig, men nålen gir klarere utslag da. Spilleren velger selv avveiingen mellom å vite mer og å være redd.
4. **Morgen.** Loggeren har et utslag kl. 01:22. OPS viser at arrayet hadde det samme minuttet. Prøven «Two simultaneous sites» blir gyldig, og vinden strykes. Ward teller det, og Ledger trykker stedet på torsdag. Earl sier på lufta at stasjonen hadde rett.
5. **Hopper spilleren over turen,** blekner streken ved middag, og saken mister sporet. Det er synlig og har en pris.

## 3. Koblingene, i prioritert rekkefølge

1. **Peilelapp fra hvert avvik, uten krav om grener.** Spredningen er ±40° og smalner med reparerte grener. Det er én utpeking per natt totalt. Regissøren holder grensen og filtrerer på tidsnivå (`"tier"`).
   - *Spilleren merker det slik:* en papirlapp fra skriveren, en strek på kartet og kjente steder uthevet i sektoren.
   - *Konsekvens:* et sted å dra i kveld. Uke 1 får en kobling som virker uten utstyr.
2. **Sak 2 deles, og natthendelsene går til en ny sak, OUT ON THE LAND.** Den legges bakerst som indeks 8, slik at lagrede spill ikke må nummereres om. Den første versjonen bruker prøver med utstyr som finnes: viltkamera, magnetometer og foto.
   - *Spilleren merker det slik:* en kuring ute åpner en sak om landet, ikke om takplaten.
   - *Konsekvens:* spor teller for en konklusjon, og ingen spor kastes ved taket på 32.
3. **Feltfunn kommer hjem som spørsmål.** Tidsstempel fra bilde, logger eller viltkamera sammenlignes med nattloggen: «SAME MINUTE ON ARRAY: YES/NO». Bilder fra natthendelser kan registreres på OPS, under taket på $35 i uka.
   - *Spilleren merker det slik:* svaret skrives ut på telexen, og en forklaring strykes på saksbrettet.
   - *Konsekvens:* saken kommer videre samme natt. Spilleren drar hjem fordi hun vil vite svaret.
4. **Spor kjølner.** Peilinger og spor får en levetid, til neste middag.
   - *Spilleren merker det slik:* streken på kartet blekner, og nålen går fra rød til grå.
   - *Konsekvens:* et kaldt spor kan ikke brukes i saken. Valget mellom kort 3 og en tur blir ekte.
5. **Magnetometer og radiopeiler får konsekvens.** Nålen får en historie: «will not settle» når noe har skjedd i nærheten de to siste nettene. Nålen slår ikke bare ut inne i sektoren, ellers blir den en metalldetektor som tar bort tvetydigheten. Radiopeileren legger en peiling fra bilen inn i `Fixes`, slik at to peilinger kan krysse hverandre fra dag 15.
   - *Spilleren merker det slik:* ny nåletekst og en strek fra bilen på kartet.
   - *Konsekvens:* spor i sak 8, og ekte krysspeiling.
6. **Kveldsjobben peker ut via saken, ikke via et nytt kort.** Saksprøven bygges som en ny `ECondition` på veien fra `PlanCaseTest`. Stedet velges i denne rekkefølgen: et sted der noe ble flyttet i går, så gårsdagens peiling, så et sted spilleren har hørt om, ellers `NextUnknown`. Klokkeslett som faller på 8.15 eller 20.15, eller som inneholder 47, hoppes over.
   - *Spilleren merker det slik:* saksprøven på kortet viser gårsdagens peiling.
   - *Konsekvens:* jobben *er* undersøkelsen, og den erstatter kort 3 uten ekstra lønn.
7. **Adgangene får noe som leser dem.**
   - `nightwatch` gir et oppslag på sheriffkontoret.
   - `sameminute` gjør at mast 31-B kan besøkes.
   - `rakedGround` holder stedet varmt en dag til.
   - `visitors` gir et vitne hos Baca.
   - *Spilleren merker det slik:* telexsvaret nevner stedet, og det kommer en ny nål på kartet.
   - *Konsekvens:* en ny kilde eller en lengre frist.
8. **Earl på lufta.** Er en registrert bekreftelse knyttet til et tips han har lest, sier han i sendingen at stasjonen hadde rett. Det bruker én eksisterende tillitsverdi, ikke fire nye.
   - *Spilleren merker det slik:* hun hører det på 1420.
   - *Konsekvens:* neste tips fra Earl kommer tidligere og er mer presist.

## 4. Arbeidsplan (hvert trinn er en egen patch)

0. **Grunnlag, uten motorjobb.**
   - Sjekk inn `econ.py` fra scratchpaden under `/home/tom/SIGNAL-47/Tools/`. I dag finnes den bare i `/tmp/claude-1000/-home-tom-SIGNAL-47/2673bb7b-c3bc-4733-be24-fcf10a15ae54/scratchpad/m7/C/econ.py` og forsvinner med økta.
   - Lag et tidsark for kortene og kjøreturene mot vinduet 19:00–23:30.
   - Før beslutningene inn i `Docs/Active/DECISIONS.md` når du har svart på spørsmålene.
1. **Peilelapp, kart og radiopeiler.**
   - Filer: `S47Places.cpp` (`AfterRegistered`, `SpreadFor`, `FillMap`; `SelfTest` ved `:486` krever i dag `SpreadFor(0)==0`), `S47Freeform.cpp:1143`, `S47Outfit.cpp:108-119`, `S47FreeformTelex`, `S47FreeformDirector.cpp` og `director.json` (grensen på én utpeking per natt og filteret på tidsnivå).
   - Nye felt i `FBearing` har standardverdi i JSON, så lagringen trenger ingen ny versjon.
2. **Sak 8, OUT ON THE LAND.**
   - Filer: `S47FreeformCases.h/.cpp` (`CaseCount` 8→9, `LineForFinding` `:284-286`, sannhetstabeller og lagringsprøver i `Cases.SelfTest`), `S47Predecessor.cpp` (sjekk `CasesToStay`).
   - `CaseCount` brukes 38 steder, blant annet i Flow, LateCasesDev og ScriptEditor, og alle må gjennomgås.
   - Prøvene gjenbruker eksisterende utstyr. Bygg ikke nye feltprøver: de tre i sak 2 tok 470 linjer.
3. **Tilbake til SARO.**
   - Filer: `S47FreeformDirector.cpp:1682-1731` (`RegisterPhotoClaim` blir `RegisterFieldRecord` for natthendelser), `S47Freeform.cpp` (kontrollen av samme minutt mot nattloggen), `S47PinBoard.cpp` (`PinWhatCameBack` i stedet for en ny knapp), `S47Answer.cpp:180` (gjøres til et nattfunn).
   - Wards terskler endres ikke, og registrering kreves ikke for at Ward skal telle.
4. **Kjølning og nålens historie.**
   - Filer: `S47Places.cpp:386-389`, utløp i `FBearing` og blekning i `FillMap`, og en spørring etter nylige hendelser i nærheten i `S47Happenings.cpp`.
5. **Saksprøve som peker ut, adganger og Earl.**
   - Filer: `S47FreeformCases.cpp` (ny `ECondition`), `S47Freeform.cpp:2894-2941`, `S47Places` (`ErrandPlace`), `S47RadioTips.cpp`, `S47TownDay.cpp` og `S47Townsfolk.cpp` (oppslaget på sheriffkontoret), eventuelt `S47CaseFieldTests.cpp`, som i dag er låst til `PostPoint`, hvis loggeren skal kunne stå på et vilkårlig sted.
   - `FCard` og `Jobs.SelfTest` røres ikke.
6. **Reise, kritikere og installering.**
   - En headless test av selve løkka: avvik, peiling, sted, feltfunn, registrering og neste kort.
   - Kjør `econ.py` mot målet $0–150 i uka og taket på $35.
   - Kritikerrunde mot «felt, not fired», deretter rask installering.
   - Mål for kritikerne: i uke 1 drar spilleren ut av seg selv én gang per natt, og minst én tur endrer noe på SARO samme natt eller neste kveld.

## 5. Spørsmål du må avgjøre

1. **Skal tråder spilleren knytter selv på pinboardet kunne telle som bevis?** Det bryter regelen fra CASEBOARD-20261108: «the board never says whether one holds». Jeg anbefaler nei. La tidsstempel og samme minutt bære bevisene.
2. **Er «høyst én utpeking per natt, og kort 3 som et valg» riktig tolkning av «alt jobb»?** Eller vil du at også vanlige kort skal få en del ute? Jeg anbefaler det første.
3. **Skal sak 0 og 1 få en prøve som bare kan gjøres ute?** Det krever enten at `TestCount=3` økes eller at en prøve byttes. Begge deler rører lagrede spill og sannhetstabellene. Alternativet er å la peilelappen og sak 8 stå for koblingen ut, og holde sak 0 og 1 som de er. Jeg anbefaler alternativet først, og å vurdere saken på nytt etter kritikerrunden.
