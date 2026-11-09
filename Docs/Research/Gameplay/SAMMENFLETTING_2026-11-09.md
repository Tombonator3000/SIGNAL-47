# Sammenfletting: dagens systemer som én kjede

**Status:** Forslag, ikke vedtatt. **Gjelder:** Freeform. **Sist kontrollert:** 9. november 2026.

Tom avviste 9. november [syntesen med nye 1986-mekanikker](HELHETLIG_DESIGN_1986_2026-11-09.md): forslagene ødela mer enn de hjalp. Bestillingen er å ta systemene som finnes i dag og flette dem til ett logisk system som belønner, som gir lyst til å spille mer, og som er lett å forklare. Dette dokumentet legger ikke til noen ny mekanikk. Det endrer bare hva systemene sender videre til hverandre. Problemene er hentet fra [gjennomgangen av d38](GJENNOMGANG_D38_2026-11-09.md). Koden er lest, ikke kjørt.

## Spillet forklart for spilleren

> **Gjør jobben. Når et kort kommer rart tilbake, skriver OPS ut hvor. Dra dit og ta med noe hjem. Det du leverer, betaler stasjonen og avgjør om den får leve.**

Alt under er den samme setningen, ledd for ledd.

## Kjeden

| Ledd | System som finnes | I dag | Kobling som mangler |
|---|---|---|---|
| 1. Jobben | Jobbkortene | Samme tre kort, uten sammenheng med natta | Nattens hendelse er det som gir avviket på kortet. Nåla som ikke faller til ro, betyr at noe skjer der ute i natt. |
| 2. Hvor | Peilelappen | Peker på neste ubesøkte sted | Peker på stedet der nattens hendelse faktisk er. |
| 3. Ut | Stedene, kamera, opptaker, feltnotat | Ti faste steder, brukt opp i uke 3 | Én liste (de 44 hendelsesstedene). Stedet har det hendelsen etterlot. |
| 4. Hjem | Fotolab, båndpult, samme minutt på telex | Virker | Ingen endring. |
| 5. Sak | Sakene og brettet | Brettet viser svaret, eller sporene er alltid uenige | Velg forklaring først, så viser brettet prøven. I mysterier stemmer det ene sporet med sannheten. |
| 6. Lønn | Feltpenger, butikken, Ward | Rutine alene gir full score og $624 | Se under. |

## Belønningen: tre ting, alltid de samme

Hvert funn som leveres, gir det samme, og nattarket kl. 06 viser det på tre linjer:

1. **Penger.** Feltpengene som finnes i dag. Et funn levert samme natt som kortet varslet, betaler best.
2. **Noe åpner seg.** Butikken har allerede varer med krav (magnetometer etter første sted, bedre film etter første kropp på bilde). Gjør det til regelen for alt: funn åpner varen, penger kjøper den. Bilens oppgraderinger åpner stedene lenger ute.
3. **Ward.** Poengtabellen finnes. Endringen er at stasjonen ikke kan beholdes uten minst én sak med noe hentet ute. Mandagsnotatet viser de samme linjene hver uke, så spilleren ser hvor det står.

Dermed blir pengene verdt noe (de kjøper det funnene åpnet), og natta ute blir veien til alt det andre.

## De seks problemene, løst med det som finnes

| Problem | Løsning uten nytt system |
|---|---|
| Jobb og skrekk møtes aldri | Ledd 1: kortet varsler om natta. Aggregatet og Dales ærender finnes og virker; la regissøren også legge dem etter kl. 02. |
| Etterforskning er avkrysning | Ledd 5. To små endringer i `S47FreeformCases.cpp`. |
| Peilingen peker feil | Ledd 2 og 3. `S47Places.cpp` henter målet fra hendelsen. |
| Økonomien presser ikke | Funn åpner, penger kjøper. Kvalitet betaler mer enn kort nr. 3. Radiopeileren fra dag 8. |
| Uke 2–4 mangler store øyeblikk | Den store natta har tre varianter (søylen, fallet, stigningen), og spilleren ser bare én. Bruk de to andre i uke 2 og 3 på nye steder. Ukesaken gjøres ferdig for uke 1 og flyttes til nytt sted i uke 4. |
| Dødtid på SARO | Glimtene som finnes, kan også ses fra vinduene. Earl, CB-en og skriveren ved racket finnes; la dem si noe om nattens hendelse. |

## Regler som rettes samtidig

- Reparerte antenner gjør ukearket billigere, ikke dyrere.
- Pengene får ett navn, og brevet og koden sier det samme om overført beløp.
- Taket på feltpenger står som en linje på ukearket.
- Kort som ikke tas, faller ut ved døgnskiftet.
- Telexen om himmelens drift tas ut eller skrives om, så den ikke lover noe kortene ikke gjør.

## Rekkefølge

1. Kortet varsler om natta, og peilelappen peker dit (ledd 1–3). Dette er selve sammenflettingen.
2. Belønningen: nattarket med tre linjer, funn åpner varer, Ward krever ett funn ute.
3. Sakene: velg først, og ett spor som stemmer.
4. Store netter i uke 2 og 3, ukesak 1 ferdig.
5. Regelrettingene og glimt fra vinduene.

1986-preget kommer av det som alt er der: film, bånd, telex, CB, Earl, avisa og papiret. Det trenger ikke flere systemer.
