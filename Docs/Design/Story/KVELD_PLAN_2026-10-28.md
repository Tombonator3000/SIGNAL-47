> **Status: Gjeldende** · Klassifisert 28.10.2026. Gjelder det dokumenterte omfanget; [gjeldende leveringsstatus](../../Active/STATUS.md).

# Ny rekkefølge for kvelden, etter Toms spilltest 28.10.2026

Tom spilte SC01 og trafooppdraget og ga fem tilbakemeldinger. Tom har valgt at signalet slår ut strømmen. Denne planen samler dem til én rekkefølge.

## Toms tilbakemeldinger

1. Trafooppdraget bør skje senere, når det er mørkt, og lyset og lysbuen på trafoen bør stå på hele tiden.
2. Alien kommer for tidlig og ødelegger for mye. Den første ekte møtet bør vente til omtrent kl. 02.
3. Etter trafoen hoppet klokka til 00:13 og signaloppdraget. Det er mer logisk at noe skjer med strømmen, så man må til trafoen for at antennen skal virke igjen.
4. Hånden på ruta er for direkte. Spilleren skal bare høre et klask mot ruta, åpne skjermen og se avtrykket.
5. Lampetesten krever for mange turer inn og ut og bør gjøres enklere.

## Slik er det i dag

1. 18:00 overlevering, deretter SC01 i sanntid fra 18:00.
2. Dale ringer om en svak inspeksjonslampe. Spilleren kjører til Caliche og gjør T01 servicekontrollen.
3. SC02 starter rett etter normalt resultat, altså tidlig på kvelden (S47Horror.cpp:515).
4. Relékoblingen er låst til SC01 og T01 er ferdig (S47Evening.cpp:81, S47Story.cpp:546).
5. Etter koblingen: K3 løser ut, lyset dipper, tidshopp til 00:13 og restsignalalarmen (S47Story.cpp:575 til 668).
6. Lysbuen spilles én gang når kroppen dukker opp (S47SubstationArc.cpp:43).

## Ny rekkefølge

1. 18:00 overlevering og SC01 kveldsrunden ved SARO, forenklet (se under). Ingen tur til Caliche ennå.
2. Relékoblingen krever bare SC01. Tidshopp til 00:13 som i dag, og restsignalet kommer.
3. Samtidig løser K3 ut og blir stående ute. Antennen er død og kan ikke følge signalet. Lysene i bygget går på nødlys.
4. Dale eller fylkets vakt ringer: matingen fra Caliche junction er ute, sjekk stasjonen. Dette erstatter dagens «svak inspeksjonslampe» som grunn.
5. Kjøreturen hopper klokka til omtrent 01:35. Det er mørkt. Trafoen har feillys som blinker og en svak, rolig lysbue som står på hele tiden, slik at den synes på avstand.
6. T01 servicekontrollen gjenoppretter strømmen. Rundt 02:00 kommer SC02 alienmøtet. Den kraftige lysbuen spilles når kroppen dukker opp, som i dag.
7. Etter flukten er strømmen tilbake og antennen virker. SC03 rapporten fra telefonkiosken i Caliche passer her, nå rundt 02:15, og «følg undersøkelsen til 06:00» blir naturlig.
8. Tilbake ved SARO fortsetter signaloppdraget.

Tidsstemplene 00:13 i historien kan stå. SC03 sine tekster om kveldstid må justeres til natt.

## SC01 endringer

1. Hånden: den synlige hånden fjernes. Når skjermen er nede, kommer et klask mot ruta (lyd fra utsiden av glasset). Spilleren hever skjermen og ser avtrykket, fem striper og et håndflateavtrykk. Avtrykket finnes fra før (sc01:exteriorTraces). Faktum «hånd sett» erstattes av «klask hørt» og «avtrykk sett».
2. Lampetesten: én kontroll innenfra. Slå lampa av og på mens den er i bildet gjennom glasset, ta ett referansebilde med lampa på. Kontrollen direkte utenfor blir valgfri. Skjermkontrollen beholdes fordi den nå trengs for klasket.

## Fordeling Claude og Codex

1. Claude: SC01 (klask, avtrykk, enklere lampetest), dialog og tekster for ny grunn til trafoturen, SC03 tid og tekster.
2. Codex: rekkefølgen i Story og Evening (koblingen uten T01, K3 som blir stående ute, antennen død til strømmen er tilbake, tidshopp ved kjøreturen), trafoens faste feillys og rolige lysbue, SC02 etter 00:13.
3. Felles: SC02 og SC03 sine tester oppdateres etter testregelen øverst i OPPGAVER.md. Hele matrisen kjøres én gang før installasjon.

## Fra todo.md, senere

1. TB2 sheriffkontor, kino og fôrbygg finnes bare som Blender kilder. Import, materialer, montering og pakking mangler.
2. Vanlige hus i Caliche, sammenligning av installerte fasader med godkjente Blender modeller.
3. Diner Vulkan krasj, svevende deler ved trafoen, Motor Court, SARO panel og dør brukervennlighet.

## Mer å gjøre ved SARO fra 18:00 til 24:00

Tom: «nesten ingenting frem til 24». Når trafoen og alien flyttes etter midnatt, blir kvelden enda tommere. Kvelden skal fylles med arbeid, sted og uro, ikke alien. Mye er allerede skrevet i MISSION_DETAILS_REFERENCE.md (NATT kortene), DALE_OFFICE_DOCUMENTS.md og S.3 himmelbanene, men er ikke laget i spillet.

Klokka går videre når en oppgave er ferdig, med korte hopp. Tidene er vinduer, ikke ventetid.

1. 18:00 Overlevering, lær stedet og veien ut (NATT-01, NATT-02, finnes).
2. 18:30 Mottakerreferanse (NATT-03, finnes som baseline).
3. 19:00 Ny: ukentlig test av dieselaggregatet i annekset. Start, les av, slå over til nødlys og tilbake. Lærer spilleren nødstrømmen som trengs kl. 00:13.
4. 19:30 Valgfritt kveldsmat i Caliche: diner, Nora og en vanlig by (NATT-05). Byen skal huskes som normal før den blir feil.
5. 20:30 Arkivet i annekset: den bortredigerte linjen, sju toner på et eldre bånd, det Dale beholdt (NATT-13 til NATT-15), og Dales seks valgfrie dokumenter. Gir historien bak det gamle forsøket.
6. 21:30 Et lys krysser himmelen (SKY01). Kan sees og fotograferes fra porten. Bare et lys, ingen skikkelse.
7. 22:00 Runde ute med lykt: kabelkanaler, porten, gjerdet. Små tegn uten alien: lydene fra nattdyrene stopper, porten står åpen selv om den ble lukket.
8. 23:00 SC01: lampekontroll, så klask mot ruta og avtrykket. Kveldens første ordentlige skrekk.
9. 23:40 Arkiver kvelden og slipp nattsurveyen (NATT-06). Tidshopp til 00:13.

Forslag til fordeling: Claude tar aggregattesten, arkivoppgavene og runden ute. Codex tar himmelbanen SKY01 og klokkekoordinatoren (R2.2) som styrer hoppene. Dineren finnes, men har en kjent Vulkan krasj som må rettes først.

## Arbeid ved arrayet 19:00 til 22:50, Tom 28.10.2026

Tom godkjente planen for 18 til 24, med ett tillegg: mer array- og dataarbeid, slik man faktisk undersøker himmelen ved et radioobservatorium. Kvelden skal føles som en vanlig vakt før det blir skrekk, men gi hint om unormale ting som lyder og lys på himmelen.

Kildene er ekte og står der de sto over stedet 13. april 1986 (MST), øst for meridianen fordi konsollen bare dreier arrayet fra 0 til 180 grader.

| Tid | Kjøring | Konsoll | Data | Hint |
|---|---|---|---|---|
| 19:00 | Pekekontroll 3C 286, fempunktskors | 1419.900, AZ 070, gain 50 til 60, BW 50 til 70 | Strimmel fra skriveren | Ingen. Helt vanlig |
| 19:50 | Pulsar PSR B1133+16, 1.18791 s | 1419.600, AZ 112, gain 65 til 80, BW 90 til 100 | Strimmel fra skriveren | Pulsaren tikker i høyttaleren. Ekstra, svakere tikk utenfor takt: fire, pause, sju |
| 20:45 | Hydrogenlinjen, drift scan stripe 12 | 1420.380, AZ 090, gain 50 til 65, BW 10 til 24 | Reduseres på driftsterminalen | 38 sekunder bredbåndsstøy i alle kanaler, som om noe lyst krysset strålen. Ikke i flyloggen, ingen motorlyd |
| 22:05 | Fluksmåling Virgo A | 1419.900, AZ 146, gain 45 til 60, BW 40 til 60 | Strimmel fra skriveren | Cricketene stopper på en gang. Drivmotor 14 går i tre sekunder uten kommando |

Hver kjøring: still inn konsollen etter kortet, trykk START RUN, hør og se integrasjonen, og hent papiret ved skriveren eller terminalen. Klokka hopper til kildens tid når kjøringen starter, og til kjøringens slutt når papiret er arkivert. Siste papir er arkivert kl. 22:50. Lysene på himmelen kommer fra de eksisterende himmelbanene (S47SkyVisitors), som allerede trappes opp kl. 20 og 22.

SC01-vinduet (hånd i dag, klask etter neste pass) blir ikke klart før timeplanen er ferdig. Lampekontrollen kan gjøres når som helst. Gamle lagringer får ingen timeplan i ettertid.

Status: kode i S47Observing (ny), med små koblinger i Story (konsollkortet), HUD, Flow, Main, widgets og Horror. Aggregattest, arkivrunde, runde ute, stemmen ved døra og SC01-klasket gjenstår.
