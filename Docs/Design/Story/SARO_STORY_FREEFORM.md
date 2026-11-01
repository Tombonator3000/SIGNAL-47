# SARO-løkken, Story og Freeform

**Status:** Gjeldende. **Gjelder:** Toms modusretning og første SARO-kontrollsløyfe i Unreal. **Sist kontrollert:** 31. oktober 2026.

Tom har valgt at dagens spill videreføres som Story, med en senere separat Freeform-modus. Første bestilling er å føre planen i [TODO](../../Active/TODO.md#story-og-freeform) og legge en aktiv SARO-løkke inn i Story-kvelden. Dette dokumentet beskriver design og avgrensning; faktisk levering føres i [STATUS](../../Active/STATUS.md).

## To moduser med felles arbeidssted

| | Story | Senere Freeform |
| --- | --- | --- |
| Ramme | Den eksisterende planlagte natten, med personer, oppdrag og fast slutt. | Vedvarende dager og netter uten Story-finalen. |
| Hovedarbeid | SARO-arbeid lærer normal drift og avdekker spor før nattens større hendelser. | Gjentakbart og variert SARO-arbeid gir fremgang mellom uvanlige hendelser. |
| Hendelser | Avtalte scener, kontrollerte forvarsler og senere eskalering. | Tilfeldige hendelser med sted, tid, tilstand og tidligere forløp som vilkår; rolige perioder inngår. |
| Steder | SARO er sentrum for kontrollløkken. Byen og motellet beholder sine roller i historien. | SARO, antennefeltet og teknisk område er hovedsløyfen. Byen støtter drift gjennom forsyninger, bruktutstyr og reparasjonsdeler. |
| Lagring | Eksisterende saker og historieprogresjon bevares. | Egen modusidentitet, klokke, progresjon og lagring, med delte tekniske grunnsystemer. |

Første Freeform-leveranse er implementert og lokalt installert via PR297 fra `0f8f946e`. Modusvalg, vedvarende døgn, arbeidsløkken og hendelsesregissøren er motorprøvd; naturlig balansering og utvidet nabobygginteriør gjenstår. Faktisk status føres i STATUS, og planlagt varighet er ikke målt spilletid.

## Første SARO-løkke i Story

**Normalreferanse → avvik → fysisk kontroll → sikret bevis → sammenligning → forståelig neste handling.**

Reisen skal hovedsakelig ligge innenfor SARO-basen, antennefeltet og det tekniske området. Spilleren kan oppdage noe ved mottakeren, kontrollere det på stedet og komme tilbake til instrument, arkiv eller papir for å sammenligne. By- og motellturer skal ikke være en forutsetning for denne grunnløkken.

Bygg videre på eksisterende Observing-kveld, dens kontrollsteder, instrumenter og lagrede resultater. Første implementering gjør ett eksisterende avvik til en aktiv kontroll med tydelig respons og eksplisitt sammenligning. T01 og SC08/SC09 er eksisterende innhold; de opprettes ikke på nytt som følge av denne planen.

Løkken skal gi spilleren et forståelig normalt resultat, et etterprøvbart avvik og en avgrenset konklusjon som bygger på faktisk utført arbeid. Tekstlesing eller automatisk journalføring alene skal ikke telle som en fysisk kontroll. Et avbrudd skal bevare gjennomførte ledd, og en feilhandling skal gi en forståelig vei til nytt forsøk.

## Aktive arbeidsstasjoner og antenneservice

Tom har godkjent denne utvidelsen 31. oktober. Første leveranse gjelder Story i eksisterende rom og ved antennene. Nabobygget blir senere Freeform-arbeid. Faktisk leveringsstatus står i STATUS og VOTV-02 i TODO.

| Stasjon | Handling og resultat |
| --- | --- |
| Midt-PC | Still mottakeren og ta opp observasjonen etter at antennene er på mål. |
| Venstre PC | Velg antenne, les bestilt/faktisk retning, driftstilstand og kommandologg. Finn S-03-feilen. |
| Høyre PC | Velg kveldens observasjon, send pointing og følg bevegelse/stabilisering til på mål. |
| Lydprosessor | Spill pulsar og kontroll separat, sammenlign tidsmerkede utslag og marker ekstratikkene. |
| Hjørne-PC og telex | Arbeidsordre/referanser, sammenligning og registrering av en begrunnet konklusjon. |
| Arkivmappe | Legg registrerte original-/kontrollark med samme referanse i kveldens mappe. |

Etter normalreferansen og aggregatprøven oppdages en S-03-feil før pulsaren. Spilleren går til det eksisterende styreskapet i serviceområdet, setter lokal service, åpner panelet, fester en synlig løs signalkontakt og lukker. Lokal kontroll dreier fem grader og tilbake; deretter settes fjernstyring og ny pointing prøves fra kontrollrommet. Den mekaniske feilen og ekstratikkene er separate funn. Ingen generell verktøy-/reservedelsøkonomi inngår i førsteversjonen.

Antennemodeller, avlesning og målekrav bruker samme faktiske azimut/elevasjon. Måling venter på begge akser innen 0,1 grad i ett sammenhengende sekund. 3C 286 utfører et fysisk fempunktskryss med 0,5 grads forskyvning. Pulsar/Virgo A krever omstilling til planlagte mål; hydrogenets driftskann holder valgt retning. Senere historiestyrte antennehendelser overtar etter kveldsløkken.

Måledata fyller tidspunkt og instrumentinnstillinger automatisk. Spilleren vurderer konklusjonen og gjør arkivhandlingen. Én stabil undersøkelsesreferanse følger original, kontroll, maskinregistrering og mappe. Sikrede dokumenter forblir lesbare i notatboken.

Pointing, service, opptak, merket intervall og registrering/arkivering lagres. Pause og lasting bevarer gjennomført arbeid. Eldre saker får ingen nye tilbakevirkende krav. Den samlede prøven skal dekke faktisk gange og interaksjon, synlige modellbevegelser, feilhandlinger og lagring under uferdig arbeid, før pakking og lokal installasjon.

## Kvelden og overgangen til natt

Arbeidsløkken brukes hovedsakelig frem til 00:00. Historien er til stede allerede i kveldsvakten gjennom overlevering, observasjoner og små uoverensstemmelser. Rundt midnatt eskalerer den avtalte historien; rutinearbeidet skal gi plass til hovedhendelsene.

[Kveldsrekkefølgen](KVELD_PLAN_2026-10-28.md), [SC02 til SC13-revisjonen](HISTORIE_SC02_SC13_REVISJON.md) og nyere [beslutninger](../../Active/DECISIONS.md) styrer hendelsene. Denne planen flytter ikke det avtalte signalet rundt 00:13 eller den senere trafoturen. Den innfører ikke et nytt krav som kan sperre en eldre sak eller hindre et allerede nådd historietrinn.

## Byen som forsynings- og reparasjonssted i Freeform

Tom har 31. oktober bedt om å legge denne retningen inn i planen. SARO-arbeidet skaper konkrete behov som gir byen en praktisk rolle: diagnostiser en feil, velg løsning, skaff det som mangler og ta det tilbake til montering og kontroll.

| Sted | Planlagt rolle |
| --- | --- |
| Bensinstasjonen | Drivstoff til aggregat og bil, sikringer, batterier, lyspærer og enkle reservedeler. |
| Verksted eller bruktlager | Brukt måleutstyr og ødelagte apparater som kan repareres eller gi kompatible deler. Kartlegg eksisterende lokaler før plasseringen velges. |
| Feeder-anlegget | Feilsøking og vedlikehold av strømforsyningen, med måling og reparasjon av årsaken til strømproblemet. |
| SARO-nabobygget | Testbenk, delelager og reparasjon av hentet utstyr, knyttet til MODE-03B. |

Eksempel: S-03 mister styring → spilleren måler feilen og finner et defekt relé → velger et nytt relé eller et billig brukt apparat med passende del → henter det i byen → monterer og tester → observasjonen fortsetter. Nytt, brukt og reparasjon skal ha forståelige fordeler og ulemper i pris, arbeid og tilstand. Kompatibilitet skal kunne undersøkes før kjøp eller demontering.

Første forsyningsprøve avgrenses til sikringer, ett utskiftbart relé og aggregatdrivstoff. SARO starter med et lite reservelager. Spilleren kan planlegge og samle innkjøp; hver feil skal ikke kreve en kjøretur. Reserveutstyr eller midlertidig drift med redusert kapasitet skal gi mulige veier videre når det passer feilen. Bevar en vei ut av tomt lager eller budsjett slik at vanlig drift kan gjenopprettes.

Økonomien begynner som et enkelt driftsbudsjett per døgn, med fremgang gjennom utførte observasjoner og levert målearbeid. Priser, forbruk, belønning og tilgang til bruktutstyr balanseres først gjennom den spillbare døgnprøven. Lager, drivstoff, penger og utstyrets tilstand må lagres sammen med Freeform-saken.

Døgnrytmen kan endre turen: et vanlig ærend på dagtid kan få andre vilkår når bensinstasjonen er stengt, strømmen faller ut eller noe beveger seg ved feeder-anlegget. Åpningstider og hendelser skal støtte planlegging og uro, med en forståelig reservevei ved kritiske feil. Hovedarbeidet forblir på SARO.

## Videre Freeform-arbeid

Etter at SARO-løkken fungerer i Story, skilles delt utstyr og interaksjon fra modusenes fremdriftsregler. Den første Freeform-prøven bør omfatte én arbeidsdag og natt, lagring og overgang til neste døgn. Deretter utvides hendelsesvariasjon og langsiktig fremgang.

Hendelser skal knyttes til situasjonen og gi plass til respons. Eksempler på hendelsesfamilier er måleavvik, uventede lyder eller spor, besøk og farlige møter. Hvilke som bygges først, og hvor ofte de skjer, avklares ved spilling. Lagring må bevare relevante tilfeldige valg og aktive hendelser, slik at lasting ikke blir en ny trekking eller gjentar et avsluttet møte.

Førsteversjonen krever ikke omfattende butikkøkonomi, behovsmålere eller global fysikksimulering. Viktige medier og instrumenter skal ha pålitelige tilstander; løs fysikk kan utvides avgrenset etter [VotV-notatet](../../Research/VoicesOfTheVoid/GAMEPLAY_LESSONS.md).

Kanon gjelder fortsatt: New Mexico i 1986, 1420.405 MHz, 27 antenner, azimut 026 og Story-hendelsens faste 47 sekunder. Spilltekst skrives på engelsk, dokumentasjon på norsk.

## Godkjent gjennomføring: vedvarende døgn og hendelsesregissør

Tom godkjente gjennomføringsplanen 31. oktober etter valg av 60–90 minutter per døgn, lokal recovery med valgfri død/lasting, utstyr/skjul/flukt og fremgang gjennom mysterier og bedre base. Første balanse bruker 75 minutter aktiv tid per døgn fra kl.08:00. Tre daglige oppdrag gir 20 dollar hver; tredje oppdrag velges. Startbudsjett er120 dollar. Sikring5, relé25, donorapparat15 og aggregatkanne10 dollar er spillverdier som skal balanseres gjennom spilling.

Freeform får egen modusidentitet og lagring. En ny hendelsesregissør eier tempo, tilfeldige valg, aktive hendelser og hvile, mens eksisterende apparater, sanser og bevegelse gjenbrukes. Hendelser er situasjonsstyrt, én større sekvens om gangen, og ingen backlog etter pause. Første døgn har ingen farlig jakt. Startprofilen er2–4 små muligheter og0–2 undersøkelser per døgn, maksimalt ett farlig møte med35 prosent nattmulighet fra døgn2 og stor hendelse med10 prosent fra døgn4/minst3 døgn mellom. Etter fare følger minst8 aktive minutters hvile.

Katalogen omfatter ekstra puls, taklyder, antennefeil, observatør, feederavvik, farlig serviceområde, signal på feil sted, avsenderløst telex, tom bil, lysmønster, spor/gjenstand, uenige tidsstempler, ekstra utskrift, passasje over basen, stille sone og sjelden stor nærhendelse. Hver hendelse skal ha fysiske/lydlige/instrumentelle følger, respons og avslutning. Tre mysterielinjer følger gjentatte intervaller, besøk rundt basen og uenige registreringer over flere døgn. Ingen full forklaring eller slutt kreves for å fortsette.

Gjennomføringen går fra modus/lagring og drift/forsyninger til de første seks hendelsene, tredøgnsprøve, utvidet katalog/mysterier og lokal levering. Før farlige møter godkjennes må fysisk sikt, dører, rekkevidde, tilflukt og tidsavgrenset tilbaketrekning fungere. Åpne Story-krasj og SC04-feil føres fortsatt etter faktisk kontroll. Referanser: The Vast of Night, Close Encounters, Nope, Roadside Picnic og Solaris brukes som designinspirasjon med egne assets og scener.
