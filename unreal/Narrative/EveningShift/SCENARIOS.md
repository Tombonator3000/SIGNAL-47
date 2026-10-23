# Kontrollscenarier for kveldspakken

JSON-validering og eksport kan vise sammenheng i manusdata. Scenariene nedenfor er også konkrete krav for senere runtime. Programmatisk avspilling av flagg er ikke ekte kjøring, hørbar dialog eller en menneskelig opplevelsestest.

## Ruter og avbrudd

| Scenario | Handling | Forventning |
|---|---|---|
| Direkte til SARO | I01/I11, deretter D01_HANDOVER og grenen uten mat. | intro_arrived før handover_done; intro_completed etter avsluttet telefon. Ingen mat eller ch5-effekt. |
| Burger med hjem | Diner, meny, burger, takeaway, SARO og matgrenen i telefonen. | Ett kjøp, food_taken og ikke food_eaten. Synlig matrespons; Dale får vite om maten gjennom spillerens svar. |
| Sandwich i dineren | Meny, sandwich og spis før SARO. | Ett kjøp, food_eaten og ikke food_taken. Ingen tap av senere spor. |
| Bare vann | Vann, avslutt uten kjøp, videre til SARO. | Gyldig rute. Dales «ikke spist»-valg må også være sant etter et stopp uten mat. |
| Avbrudd etter betaling | Lagre etter on_select, før serveringsreplikken. | Pending svar og kjøpet gjenopptas atomisk. Ikke tom ordre eller nytt gratis kjøp. |
| Ankomst/telefon om igjen | Last ved SARO og forsøk samme trigger. | Ingen ny 18:00-reset, doble dokumenter eller gjenkjøp. |
| N01 før og etter residual | Kontroller mappen, forsøk sammenligning før signalet, gjennomfør etter faktisk residual. | Først normalrutine. Senere gyldig sammenligning; datert kopibok beviser ikke hele byens strømtilstand. |
| Feil årsak | Velg at gammel feil beviser årsaken, og korriger etter hint. | Behold dokumenter og avslutt som uavklart. Ingen straff eller fabrikkert bevis. |
| Kontoret uten VHS | Serviceskuff, nøkkel, dør og ta feltmappen. | Hovedsporet er tilgjengelig uten hele filmen eller alle dokumentene. Noras nøkkel er uberørt. |
| Full VHS | Start med strøm, se frem til faktisk komplett-merke, behold referansen. | vhs_seen ved riktig hendelse; syv filmbeats, miljølyd, ingen Kinski-forteller. |
| Avbrutt VHS | Stopp/strømbrudd før relevant forløp, lagre og last. | Ingen vhs_seen av å bare trykke play; korrekt posisjon og lyd ved retur. |
| Normal strømfeil | Les T01-kort, start kontroll, prøv å avslutte før resultat, fullfør faktisk kontroll. | Ingen suksess før ctx_trafo_service_ok. Kortets dato/tid kommer fra virkelig test. |
| Uavklart T02 | Match referanse, gjennomfør bare deler av kontrollene, velg uavklart. | Ikke trafo_anomaly_checked. Videre hovedhistorie er fortsatt mulig. |
| Etterprøvd T02 | Gjennomfør alle fire eksterne kontroller og observer lampen. | Bekreftet avvik er tilgjengelig først nå. Dale vet resultatet først når spilleren melder det. |
| Fast Dale-fragment | Spill første overlevering og senere D61 via radio. | Identisk grunnopptak, kanalfilter på mottakeren, ingen svarvalg. |
| Sen diner | Test food_bought sant og usant med ctx_all_night_active. | Riktig én av to hilsener; tidlig diner kan aldri starte disse eller P13. |

## Uavhengige tekniske gater

- Alle valg peker til eksisterende scener; choice-scener er ikke globale triggerpunkter.
- Ingen Dale-replikk kan ha physical/ui-kanal, og ingen media-beat bruker Kinski som forteller.
- Manifestets gjenbruk peker til samme aktør, stemmeprofil og eksakte tekst. Ingen aliasløkker.
- Eksterne kontekst-/verdenflagg kan ikke skrives av manusets effekter.
- Dokumenter gis etter stabil ID. Lagring/lasting gjenskaper ingen «nye» kopier eller endrede bilder.
- Ny og gammel tidslinje prøves separat. Ingen masseflytting av 1947-tid eller finalens 05:29.
- Genererte filer må være byteidentiske etter `build_pack.py --check`.

Forfatterkontrollene ovenfor beviser ikke runtime. Faktiske reise-, bilde- og lagringsresultater for trinn 1 og 2 føres i [runtime-rapporten](../../EVENING_RUNTIME.md). Hørbar ny stemme, senere filmbevegelse og målt total spilletid må prøves separat. Ingen Unreal-bygg er nødvendig for å lese eller kontrollere manuspakken.


## Runtimekontroller for trinn 1 og 2

`-S47SelfTest` kjører `FS47EveningPack::SelfTest` med 60 interne sjekker for alle de seksten manus-/tilstandsgrenene over, også avvisning av for tidlige kontor-/VHS-/traforesultater. Det beviser ikke at senere verdensinnhold eller medier er produsert.

`-S47Evening=commute|diner|order|paid|burger|sandwich|water|parking|phone|dusk` er målrettede oppsett med de samme transaksjonene. Diner-/parkeringsoppsettene flytter kamera/tilstand og må merkes som scenariooppsett.

`-S47EveningRoute=direct|burger|sandwich|water -S47EveningShots=/absolutt/testmappe` kjører faktiske kjøre- og gåkontroller gjennom åpningen, med API-valg i dialogen. Ingen menneskelig input påstås. Ruten kontrollerer at tidlig diner ikke starter ch5, at betaling og ventende svar tåler disk-lagring og full verdenslasting, at faktisk SARO-parkering starter 18:00 én gang, og at D01 gir tre dokumenter én gang. Alle rutene laster også en pågående D01 fra disk uten å gjenta ankomsten. Skumringsbildet bruker et uttrykkelig fremoverhopp til 18:35 etter overleveringen. Testen avslutter selv; bruk isolert `-UserDir` og ikke brukerens aktive sak. Godkjenning krever PASS COMPLETE, normal exit og ingen fatal-/assert-feil i loggen; prosessens returkode alene er ikke tilstrekkelig.

`-S47EveningTimeline -S47Jump=<eksisterende hopp>` prøver den nye tidsplanen ved de gamle regresjonspunktene. Uten `-S47EveningTimeline` beholder de eksisterende hoppene sin legacy-tidslinje.

Aktuelle runtimebevis og gjenstående menneskelige tester står i [runtime-rapporten](../../EVENING_RUNTIME.md).
