# Uro: små ting som bygger psykologisk skrekk

**Status:** Forslag fra Tom og Claude 8. november 2026, til implementering i M1. **Gjelder:** Freeform. **Sist kontrollert:** 14. november 2026 (mot kilden, se «Status i koden»).

Toms ord: «uro-liste over ting som kan skje bare for å bygge litt psykologisk skrekk, som bilalarm som går av, banking på døren men ingen der». Det er ingen fare og ingen belønning. Målet er at spilleren aldri er helt sikker på om noe skjedde. Det finnes fra før: telefonen som ringer én gang, skritt og tikk i taket, printeren, klikk i racket, radiosus, servicedøra, antennene som knirker, bank i gjerdet og stillheten. Listen under kommer i tillegg.

## Lyd
1. Bilalarmen på bilen går av ute. Når spilleren kommer, er ingen der.
2. Tre bank på døra. Når spilleren åpner, er det ingen der og ingen spor.
3. Noen sier navnet på spilleren lavt, bak ryggen, bare én gang.
4. Skritt i grusen utenfor stopper samtidig med spilleren.
5. En hund bjeffer langt borte og stopper brått midt i et bjeff.
6. Vannet i springen renner, og ingen har skrudd det på.
7. Radioen hopper selv til en stasjon med en gammel sang, og så tilbake.
8. Telefonen ringer, og når spilleren tar den, er det pust. Ingen sier noe.
9. Et barn ler langt ute i ørkenen. Ingen gård er nær.
10. Kjøleskapet eller kaffetrakteren slår seg av, og stillheten blir høyere.

## Syn
11. Et lys i et vindu langt borte slukkes akkurat når spilleren ser på det.
12. En stol står flyttet når spilleren kommer tilbake til et rom.
13. Bildøra står åpen, og spilleren lukket den.
14. Skygge forbi under døra.
15. Avtrykk av en hånd på innsiden av et dugget vindu.
16. Lommelykta flimrer bare når den peker mot ett bestemt sted.
17. Klokka på veggen står stille i ett minutt og går så videre.
18. På et foto er det en figur i bakgrunnen som ikke var der.
19. Speilet i bilen viser baksetet et øyeblikk for tidlig.
20. En lapp på pulten med spillerens egen håndskrift, som spilleren ikke husker å ha skrevet.

## Verden
21. Kveg eller hester står helt stille og ser mot SARO.
22. Et dyr ligger dødt i veien, og det er borte neste gang spilleren kjører forbi.
23. Gatelyktene i Caliche slukkes én etter én bortover gata.
24. En bil med lyset av følger etter spilleren og svinger så av.
25. Alle fuglene letter samtidig.

## Regler
- Høyst to til tre uroting per natt, aldri to like samme natt, og ikke rett før eller etter en fare, så de ikke drukner.
- Ingen belønning og ingen forklaring. Spilleren skal tvile.
- Noen få kan senere bli forvarsel om en ekte hendelse samme natt, men aldri alle, så spilleren ikke lærer mønsteret.
- Stedet trekkes fra stedskatalogen når hendelsen krever et sted.
- Nr. 20 (lapp med egen håndskrift) og nr. 3 (navnet) er sterke. De bør være sjeldne, høyst én gang hver per spill.

## Status i koden (kontrollert mot kilden 14. november 2026)
Alle 25 finnes i `FireUnease` i `S47Happenings.cpp` (kodet før, ikke spilt av Tom), med F10-linje hver. Reglene håndheves i koden:
- Aldri to like samme natt, ikke rett etter en fare (20 min) eller rett før en trukket fare (30 min), 3 og 20 én gang per spill og ikke før uke tre: kodet før.
- Høyst tre per natt, to på en rolig natt, aldri samme uroting to netter på rad, og minst 45 minutter mellom to: kodet 14. nov, ikke bygget (`UneaseWantFor`, `UneaseMayCome`, gjelder også reserven ved midnatt og gulvet; før kunne uke fire gi fire).
- Forvarsel om en ekte hendelse samme natt: ikke kodet for uro; nattens egne forvarsler (`PlanCues`) gjør den jobben.
