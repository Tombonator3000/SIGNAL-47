# Gjennomgang av Freeform-opplevelsen i d38

**Status:** Gjeldende vurdering. **Gjelder:** Freeform i d38 (`53cfda05`). **Sist kontrollert:** 9. november 2026.

Én agent leste koden og dokumentene og spolte gjennom månedsloggene. Unreal ble ikke startet. [K] betyr sjekket i kode eller kjørt, [T] betyr tolkning (minutter i sanntid og hvordan noe føles). Les sammen med [SAMMENLIGNBARE_SPILL](SAMMENLIGNBARE_SPILL_2026-11-09.md).

**Tidsskala [K]:** natta 19–06 tar 45 minutter i sanntid (omtrent 4,1 minutter per spilltime), dagen tar 30 minutter, og 30 døgn er omtrent 37 timer spill.

## Kort konklusjon
1. **Wards vurdering kan vinnes uten natta ute og allerede i uke 1–2.** Sak 0 testet på SARO, et reparert array og rene opptak gir 6 av 6 poeng. Etter det har uke 3–4 ikke noe som presser.
2. **Jobb og skrekk er skilt i tid med vilje.** Jobben ligger 19–23:30, skrekken kommer etter 02, og de møtes aldri. Kveldsjobben er de samme tre kortene med de samme fire kildene på samme klokkeslett i 30 netter.
3. **Etterforskningen er stort sett avkrysning.** I prøvesakene viser brettet hva testen forventer. I mysteriene er de to sporene alltid uenige, så det blir et myntkast.
4. **Peilelappen peker på neste ubesøkte sted (`NextUnknown`), ikke på årsaken til avviket.** De ti stedene har fast tekst og er brukt opp i uke 2–3. Dessuten finnes det to lister med steder: `Places` (10) og `EventPlaces` (44).
5. **Økonomimålet $0–150 holdes bare fordi modellspilleren kjøper.** Uten kjøp gir to kort per kveld omtrent +$120 i uka ($624 på dag 30). Mengde slår kvalitet. Ett kort per kveld fører til at stasjonen stenger. Lite av det som kan kjøpes, endrer natta. RDF kommer på dag 15 og koster $120.

**Dette virker:** den store natta, aggregatet som tvinger deg ut, Dale som reagerer på det du har gjort, banking som får svar fra skjulet, samme minutt på telexen og nålen som ikke vil falle til ro.

## Regler som ikke gir mening for spilleren [K]
- Himmelkildene står på samme klokkeslett hver natt, men telexen i sak 0 lærer bort at himmelen driver 3 min 56 s per natt.
- Peilingen er tilfeldig i forhold til hva som faktisk forårsaket avviket.
- Mysteriesporene er alltid uenige.
- Taket på $35 for feltpenger har ingen forklaring i fiksjonen.
- Hver reparert antenne gjør ukearket dyrere, så det å gjøre jobben straffes.
- Pengene heter BUDGET, men brevet sier «you need the money».
- Brevet sier «not in arrears», mens koden godtar opptil $79 overført.
- Kort som ikke tas, blir liggende i køen for alltid.
- Ukesakens utfall C gis når letingen gir opp, uansett hvor spilleren er.

## Eskaleringskurven
| Uke | Nytt | Hull |
|---|---|---|
| 1 | Den store natta natt 2–3, skjulsaken natt 4–5, peilelapp | SARO er tomt etter kortene, glimtene krever at du er ute, og ukesaken kan ikke koste noe |
| 2 | Fare 0,4, humor, militæret | Ingen forfattet høydepunkt og ingen ukesak |
| 3 | Fare 0,6, de sene sakene, Reyes fase 2, RDF | Stedene er brukt opp og peilingen dør. Ingen ukesak |
| 4 | Fare 0,85, tidsnivåene kryper, inspektøren og finalen | Ingen ukesak, og målet er ofte allerede nådd |

## Topp 10 mangler
| # | Mangel | Alvor | Str. | Forslag |
|---|---|---|---|---|
| 1 | Ward-målet belønner ikke natta | Høy | M | Stays krever minst én sak med tidsstempel fra feltet. Ett konkret mandagskrav per uke. |
| 2 | Samme tre kort i 30 netter | Høy | M–L | Siderisk drift. Nye kilder fra reparerte grener. Ett Ward-ønske eller én telexbestilling per uke. En kjøpbar automatlogger gjør at penger kan kjøpe tid. |
| 3 | Avkrysning og myntkast i sakene | Høy | M | «Spå, så test»: skjul prediksjonen til spilleren har valgt. Tre spor der to bekrefter hverandre. Spilleren setter selv minuttet mot strimmelen. Riktige svar teller høyere. |
| 4 | Peilingen peker ikke på årsaken, og stedene er faste | Høy | M | Målet følger sakens sannhet. Steder med tilstand per natt. Slå sammen Places og EventPlaces. |
| 5 | Skrekken forstyrrer aldri jobben | Høy | M | Arbeidsoppdrag etter 02, for eksempel Dale som ringer om en drive som må resettes eller aggregatet ute. Saksprøver med vindu etter 02. |
| 6 | Ukesaken er halvbygd, og uke 2–4 mangler | Høy | L | Patch 4 (leteren og gjemmestedene) og patch 5 (følgene) for uke 1, deretter kirka i uke 2. |
| 7 | Pengene presser ikke og åpner lite | Middels | M | Oppgraderinger som gir tilgang til fjerne steder. RDF fra dag 8. Kostnader etter møter. Kvalitet verdt mer enn kort nr. 3. |
| 8 | Dødtid på SARO i uke 1 | Middels | S–M | Glimt som kan ses fra vinduene. CB-rop med frist. Kl. 06 som et øyeblikk. |
| 9 | Fiksjonen sprekker i reglene | Middels | S | Rett punktene over. |
| 10 | Ingen målt spillerreise, og regnemodellen i repoet er utdatert | Middels | S–M | Sjekk inn regnemodellen fra pakke E. Lag en headless «typisk spiller»-uke som logger dødtid. |

Agentens anbefalte rekkefølge: 1 og 4 (hvorfor spilleren drar ut), så 3 og 5, så 6. 2 og 7 tas sammen, etter at regnemodellen (10) er på plass. Hovedvalget er punkt 1: **skal det å vinne kreve natta ute?** Det avgjør Tom.

Kopien av regnemodellen og resultatene ligger i scratchpad (`review_econ/`), ikke i repoet.
