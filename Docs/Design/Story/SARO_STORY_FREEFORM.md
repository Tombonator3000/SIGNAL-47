# SARO-løkken, Story og Freeform

**Status:** Gjeldende. **Gjelder:** Toms modusretning og første SARO-kontrollsløyfe i Unreal. **Sist kontrollert:** 31. oktober 2026.

Tom har valgt at dagens spill videreføres som Story, med en senere separat Freeform-modus. Første bestilling er å føre planen i [TODO](../../Active/TODO.md#story-og-freeform) og legge en aktiv SARO-løkke inn i Story-kvelden. Dette dokumentet beskriver design og avgrensning; faktisk levering føres i [STATUS](../../Active/STATUS.md).

## To moduser med felles arbeidssted

| | Story | Senere Freeform |
| --- | --- | --- |
| Ramme | Den eksisterende planlagte natten, med personer, oppdrag og fast slutt. | Vedvarende dager og netter uten Story-finalen. |
| Hovedarbeid | SARO-arbeid lærer normal drift og avdekker spor før nattens større hendelser. | Gjentakbart og variert SARO-arbeid gir fremgang mellom uvanlige hendelser. |
| Hendelser | Avtalte scener, kontrollerte forvarsler og senere eskalering. | Tilfeldige hendelser med sted, tid, tilstand og tidligere forløp som vilkår; rolige perioder inngår. |
| Steder | SARO er sentrum for kontrollløkken. Byen og motellet beholder sine roller i historien. | SARO, antennefeltet og teknisk område er hovedsløyfen. Andre steder kan få valgfrie roller senere. |
| Lagring | Eksisterende saker og historieprogresjon bevares. | Egen modusidentitet, klokke, progresjon og lagring, med delte tekniske grunnsystemer. |

Freeform er planlagt. Dette arbeidet hevder ikke at et Freeform-valg eller en endeløs modus allerede finnes i spillet.

## Første SARO-løkke i Story

**Normalreferanse → avvik → fysisk kontroll → sikret bevis → sammenligning → forståelig neste handling.**

Reisen skal hovedsakelig ligge innenfor SARO-basen, antennefeltet og det tekniske området. Spilleren kan oppdage noe ved mottakeren, kontrollere det på stedet og komme tilbake til instrument, arkiv eller papir for å sammenligne. By- og motellturer skal ikke være en forutsetning for denne grunnløkken.

Bygg videre på eksisterende Observing-kveld, dens kontrollsteder, instrumenter og lagrede resultater. Første implementering gjør ett eksisterende avvik til en aktiv kontroll med tydelig respons og eksplisitt sammenligning. T01 og SC08/SC09 er eksisterende innhold; de opprettes ikke på nytt som følge av denne planen.

Løkken skal gi spilleren et forståelig normalt resultat, et etterprøvbart avvik og en avgrenset konklusjon som bygger på faktisk utført arbeid. Tekstlesing eller automatisk journalføring alene skal ikke telle som en fysisk kontroll. Et avbrudd skal bevare gjennomførte ledd, og en feilhandling skal gi en forståelig vei til nytt forsøk.

## Kvelden og overgangen til natt

Arbeidsløkken brukes hovedsakelig frem til 00:00. Historien er til stede allerede i kveldsvakten gjennom overlevering, observasjoner og små uoverensstemmelser. Rundt midnatt eskalerer den avtalte historien; rutinearbeidet skal gi plass til hovedhendelsene.

[Kveldsrekkefølgen](KVELD_PLAN_2026-10-28.md), [SC02 til SC13-revisjonen](HISTORIE_SC02_SC13_REVISJON.md) og nyere [beslutninger](../../Active/DECISIONS.md) styrer hendelsene. Denne planen flytter ikke det avtalte signalet rundt 00:13 eller den senere trafoturen. Den innfører ikke et nytt krav som kan sperre en eldre sak eller hindre et allerede nådd historietrinn.

## Videre Freeform-arbeid

Etter at SARO-løkken fungerer i Story, skilles delt utstyr og interaksjon fra modusenes fremdriftsregler. Den første Freeform-prøven bør omfatte én arbeidsdag og natt, lagring og overgang til neste døgn. Deretter utvides hendelsesvariasjon og langsiktig fremgang.

Hendelser skal knyttes til situasjonen og gi plass til respons. Eksempler på hendelsesfamilier er måleavvik, uventede lyder eller spor, besøk og farlige møter. Hvilke som bygges først, og hvor ofte de skjer, avklares ved spilling. Lagring må bevare relevante tilfeldige valg og aktive hendelser, slik at lasting ikke blir en ny trekking eller gjentar et avsluttet møte.

Førsteversjonen krever ikke omfattende butikkøkonomi, behovsmålere eller global fysikksimulering. Viktige medier og instrumenter skal ha pålitelige tilstander; løs fysikk kan utvides avgrenset etter [VotV-notatet](../../Research/VoicesOfTheVoid/GAMEPLAY_LESSONS.md).

Kanon gjelder fortsatt: New Mexico i 1986, 1420.405 MHz, 27 antenner, azimut 026 og Story-hendelsens faste 47 sekunder. Spilltekst skrives på engelsk, dokumentasjon på norsk.
