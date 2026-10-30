> **Status: Gjeldende** · Godkjent av Tom 29.10.2026: behold 05:29:47, SC05 som eget scenario, egen bil fritt fra SC03. Erstatter tidene i [HISTORIE_SC02_SC13_REVISJON.md](HISTORIE_SC02_SC13_REVISJON.md) når den er godkjent. Innholdet i scenarioene står der; dette dokumentet flytter dem i tid og beskriver rammen i koden.

# Ny tidslinje SC01 til SC13 og ramme for hele natta

## Hvorfor

Revisjonen av SC02 til SC13 ble skrevet før kveldsplanen 28. oktober. Der står SC02 klokka 19:00 og residualen i SC05 klokka 00:13:47. Nå er kvelden vanlig arbeid, restsignalet og strømbruddet kommer 00:13, trafoturen 01:35 og SC02 rundt 02:00. Alt etter SC02 må derfor inn mellom omtrent 02:15 og finalen 05:29:47, som står fast.

## Faste punkter som ikke flyttes

1. 18:00 til 23:40: kvelden og SC01 (ferdig).
2. 00:13: restsignal, K3 ute, strømbrudd, telefon om Caliche Junction (ferdig).
3. 01:35: trafoturen, T01 gir strømmen tilbake (ferdig).
4. Rundt 02:00: SC02 ved trafoen (ferdig).
5. 05:29:00 til 05:29:47: THE EVENT, uttrekk og 1947-bildet (ferdig).
6. Ingen alien før rundt 02:00.

## Forslag til ny tidslinje

Klokka hopper ved kjøreturer og når en oppgave er ferdig, som i kvelden. Tidene er vinduer.

| SC | Tid | Sted | Hva skjer | Jakt | Endring mot revisjonen |
|---|---|---|---|---|---|
| SC02 Returen til bilen | 02:00 | Caliche Junction | Den bleke kroppen, flukt til servicebilen | Ja | Ferdig |
| SC03 Hjelpen som kom | 02:15 til 02:45 | Caliche, bensinstasjonen | Melding, Ortiz kommer og blir tatt, sheriffen | Nei | Fra 19:30. Tekster om kveld blir natt |
| SC04 Sikre originalene | 02:55 til 03:15 | SARO, annekset | Hent mottakerlogg og T01-journal til savnetsaken, sperr planen, annekset | Ja | Fra 22:00. Slutter ikke med tidshopp til 00:13 |
| SC05 Samtalen som ligger foran | 03:15 til 03:25 | SARO kontrollrom | Strømmen er tilbake: løs retningen, skriv ut, telefonen, rommet 47 sekunder senere, S-03 | Nei | Residualen er alt funnet 00:13. SC05 er bare samtalen og rommet |
| SC06 Det andre motivet | 03:25 til 03:40 | SARO og B-12 | B-12, FRAME01/02, fremkalling | Nei | Uendret innhold |
| SC07 Det Dale ikke leverte inn | 03:40 til 03:55 | Arkivrommet, Dales kontor | Den rettede linjen (frøet fra 20:30-permen), feltmappen | Nei | Uendret innhold |
| SC08 De peker på deg | 04:00 til 04:20 | Earl 1420 AM, arrayet | Spolen, antennene følger Reyes, går til 026 til slutt | Ja | Fra 01:15. «Etter 04:00»-kopien hos Earl utgår |
| SC09 Den avbrutte oppstillingen | 04:25 til 04:45 | STATION 01 | Flyttet referanse, kuttet kabel, Tomás | Nei | Uendret innhold |
| SC10 Rom 6 | 04:50 til 05:00 | Sierra Motor Court | Nora | Nei | Fra 03:20 |
| SC11 SARO blir avsøkt | 05:00 til 05:10 | SARO | Fjernarkene ferdige 05:04, beleiringen | Ja | Fra 03:56. Fjernarkene 05:04 i stedet for 04:04 |
| SC12 Det dere rekker å sende | 05:12 til 05:20 | Mesa Diner | Kildene, varselet til Ward, patruljearket | Nei | Sheriffen kommer 05:15 i stedet for 04:30 |
| SC13 Siste vei mot Roswell | 05:20 til 05:29:47 | Veien nord | Kjøreturen, motorstopp ved C 8.15, finalen | Nei | Uendret |

Fire jakter blir igjen: SC02, SC04, SC08 og SC11, som i revisjonen.

## Valg Tom bør ta

1. **Tett natt eller senere finale.** Ni scenarioer på drøyt tre timer spilltid krever korte tidshopp. Alternativet er å flytte finalen mot daggry, men 05:29:47 er knyttet til 1947-bildet og THE EVENT. Anbefaling: behold 05:29:47 og godta tette hopp.
2. **SC05 blir kort.** Residualen og strømbruddet er flyttet til 00:13, så SC05 er bare signalet som løses når strømmen er tilbake, telefonen og rommet. Anbefaling: behold SC05 som eget scenario, fordi samtalen og de 47 sekundene er et av de sterkeste øyeblikkene.
3. **Egen bil.** Fra SC03 kan spilleren bruke egen bil eller servicebilen fritt mellom stedene. Anbefaling: ja, uten at spillet sier det. Servicebilen er fortsatt den trygge bilen i finalen.

## Rammen i koden

Status 29.10.2026: laget som `FS47Spine` (Public/S47Spine.h). Plassholderne er gule stolper med kort: SC04 og SC11 på gulvet øst for konsollene, SC08 innenfor arrayporten. Kontrakt: `-S47Case=0 -S47EveningTimeline -S47Jump=chapter2 -S47SpineContract=sc08` og `-S47Jump=chapter4 -S47SpineContract=sc11`.

En ny styringsdel, `FS47Spine`, eier rekkefølgen SC01 til SC13 for nye saker. Den erstatter ikke det som er laget, men kobler det sammen.

1. Hvert scenario er en rad med id, tidsvindu, sted, utløser, avslutning, oppdragstekst og oppgavelinje i notatboka.
2. Ferdige scenarioer (SC01, SC02, natten 00:13 til 02:00) meldes ferdige av sine egne systemer. Rammen leser bare resultatet.
3. Scenarioer som ikke er laget får en plassholder på riktig sted i verden: et merket kort som sier hva som skal skje, én handling og tidshoppet til neste. Plassholderen er tydelig merket i spillet, slik at ingen tar den for ferdig innhold.
4. Lagring: ett felt per scenario (ikke startet, aktivt, ferdig) og klokka ved overgangen. Gamle saker får ikke rammen i etterkant.
5. Testing: hvert scenario kan startes direkte med `-S47Spine=SC07`. Én kontrakt går hele rammen SC01 til SC13 med lagring og ny lasting ved hver overgang.
6. Når en akt fylles inn, byttes plassholderne ut ett for ett. Rammen, lagringen og testen står.

## Rekkefølge for arbeidet

1. Tom godkjenner eller retter tidslinjen over.
2. Claude lager `FS47Spine`, plassholderne og helkontrakten. Én PR.
3. Installasjon av rammen, så Tom kan spille fra 18:00 til finalen med plassholdere og vurdere tempo og logikk.
4. Akt 1 (SC03 til SC05), akt 2 (SC06 til SC08), akt 3 (SC09 og SC10), akt 4 (SC11 til SC13). Full matrise og installasjon én gang per akt.
5. Logikkpasset STORY-05 når alle plassholdere er byttet ut.

Detaljert produksjonsmanus per scenario: [PRODUKSJON_SC01_SC13.md](PRODUKSJON_SC01_SC13.md).
