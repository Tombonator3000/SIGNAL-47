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

`-S47SelfTest` kjører `FS47EveningPack::SelfTest` med sjekker for alle de seksten manus-/tilstandsgrenene over og målte taletider for alle 110 cues, også avvisning av for tidlige kontor-/VHS-/traforesultater. Det beviser ikke at senere verdensinnhold eller medier er produsert.

`-S47Evening=commute|diner|order|paid|burger|sandwich|water|parking|phone|dusk` er målrettede oppsett med de samme transaksjonene. Diner-/parkeringsoppsettene flytter kamera/tilstand og må merkes som scenariooppsett.

`-S47EveningRoute=direct|burger|sandwich|water -S47EveningShots=/absolutt/testmappe` kjører faktiske kjøre- og gåkontroller gjennom åpningen, med API-valg i dialogen. Ingen menneskelig input påstås. Ruten kontrollerer at tidlig diner ikke starter ch5, at betaling og ventende svar tåler disk-lagring og full verdenslasting, at faktisk SARO-parkering starter 18:00 én gang, og at D01 gir tre dokumenter én gang. Alle rutene laster også en pågående D01 fra disk uten å gjenta ankomsten. Skumringsbildet bruker et uttrykkelig fremoverhopp til 18:35 etter overleveringen. Testen avslutter selv; bruk isolert `-UserDir` og ikke brukerens aktive sak. Godkjenning krever PASS COMPLETE, normal exit og ingen fatal-/assert-feil i loggen; prosessens returkode alene er ikke tilstrekkelig.

`-S47EveningTimeline -S47Jump=<eksisterende hopp>` prøver den nye tidsplanen ved de gamle regresjonspunktene. Uten `-S47EveningTimeline` beholder de eksisterende hoppene sin legacy-tidslinje.

Aktuelle runtimebevis og gjenstående menneskelige tester står i [runtime-rapporten](../../EVENING_RUNTIME.md).


`-S47EveningAudio -S47EveningRoute=burger` utvider ruten med hele faktiske takes, målte PCM-varigheter, kanal-/filter- og nivåkontroll, radio av/på, 0,5 s frosset lydkontekst og disk-/verdenslasting under aktiv D01. Bruk for eksempel `-S47Seconds=920`; kontrollen avslutter selv. Vanlige rutekontroller beholder rask dialog. `-S47VoiceLevels` logger faktisk trukne source-/kanalsamples, RMS, peak og bussgain. Dette beviser avspilling i grafen, ikke menneskelig lytting. Se [lydrapporten](../../EVENING_AUDIO.md).


## Takeaway etter åpningen

`-S47Evening=burger -S47EveningTakeaway -S47SelfTest -S47EveningShots=/absolutt/testmappe`
prøver kjøpt/ventende svar, båret og avsatt mat på alle seks stedene, opptak igjen,
telefon/dokumenter og kamera/lykt/notatbok. Plassering og opptak bruker faktisk
blikkstråle/fokus og vanlig OnUse/HUD-kjede. Oppsett og dialogvalg bruker API.
Ti ferske disk-/verdenslastinger sammenligner også faktisk overflateposisjon,
klokke, flagg, dialogpeker og dokumenter. Måltid fra hånden ved annexbordet
fullføres én gang med 720 sekunders fremovertid og fire sekunders overgang.

Legg til `-S47TakeawayEatPlaced` for måltid fra bordet og en ekstra lasting
under overgangen: maten forblir spist, innpakningen vises og tiden økes ikke
på nytt. Begge variantene kontrollerer til slutt med et uttrykkelig records-oppsett
at original arkivhandling på arbeidsbordet fortsatt åpner panelet.
Krev `takeaway PASS COMPLETE`, normal exit og ingen FAIL/fatal/assert, ikke bare
selvtestlinjen. Bruk isolert UserDir og maskinvakten som for rutene ovenfor.

Sluttpakke, biler/inngang, bilder og avgrensninger føres i
[takeaway-rapporten](../../EVENING_TAKEAWAY.md). Ingen menneskelig input påstås.


## Dales frivillige lesestoff og servicevedlegg

1. Ta bare feltmappen i det åpnete kontoret. Fortsett uten privatmappe, seks tekster eller VHS. T02-indeksen skal fortsatt vise kulverten, og ingen ny lesekvote skal sperre avreise.
2. Åpne privatmappens liste og lukk den uten valg. Ingen av de seks tekstene er lest eller lagt til som nye dokumenter. Fortsett fra faktisk disk-save og kontroller samme tilstand.
3. Åpne ett konkret dokument i vanlig tekstvisning. Bare dette blir lest/retained. Last fra disk og kontroller lest/ulest uavhengig; prøv vanlig og stor tekst med fullt synlig eller rullbar side.
4. Bruk gamle kontorsaves uten `readDocuments`, og en isolert feiltypet liste. Ingen gratis ny leststatus. Behold kontornøkkel/feltmappe og eksisterende hovedrute.
5. Åpne fotokonvolutten og låneomslaget. Ingen nye fotopiksler, VHS-transport eller `vhs_seen`. Kontaktforelesningen er ikke Dales feltopptak.
6. Les det separate servicearket. Kontroller fysisk A/B/REF på Row One med fri gang fra port/spine og begge rekkefølger; ingen tur over antennefundament. Gjennomført lesing er ikke gjennomført ARRAY-01.

`FS47DaleDocumentsTest` tilbyr avgrenset API/Walker/disk-kontroll. Root eier sekvensiell UE-/pakke-/bildeprøving; kildetestene her er ikke bevis på en ferdig bilde- eller filmrekvisitt.
