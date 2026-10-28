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
