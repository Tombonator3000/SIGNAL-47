# 31 ting som kan skje om natta i Freeform

**Status:** Forslag til Tom, 8. november 2026. **Gjelder:** Freeform, med stedstrekning fra TODO ANYWHERE-20261108. **Sist kontrollert:** 14. november 2026 (mot kilden, se «Status i koden»).

Inspirert av kjente UFO-mønstre i rapporter, bøker og film (Roswell, Socorro 1964, Levelland 1957, Hill-paret, Travis Walton, Cash–Landrum, kvegmutilasjonene i New Mexico på 70- og 80-tallet, *Close Encounters*, *Fire in the Sky*, *Signs*, *Nope*), uten sitater eller kopierte scener. Toms Google Drive er ikke lest.

Merking: **S** = liten, kan skje hver natt; **O** = observasjon; **E** = etterforskning med belønning; **F** = fare (høyst én per natt); **H** = humor, sjelden.

| # | Hendelse | Type | Hvor (trekkes) |
|---|---|---|---|
| 1 | Bilradio og lykt flimrer og dør i noen sekunder når noe passerer over (Levelland) | S | på vei, ute |
| 2 | Alle hunder på en gård bjeffer samtidig, så blir stille | S | gård, Caliche |
| 3 | Kveg står i ring og stirrer samme vei | O | gård, åpent land |
| 4 | Lysende kule følger bilen på avstand og stopper når du stopper | O | vei |
| 5 | Formasjon av lys over fjellene (V, rekke, trekant) | O | himmel |
| 6 | Telefonen ringer på en tom bensinstasjon eller i kirka; bare sus | S | Caliche |
| 7 | CB-rop: lastebilsjåfør sitter fast langt ute og ber om hjelp | E | åpent land |
| 8 | Samme rop neste natt, men stemmen gjentar det du selv sa | F | åpent land |
| 9 | Landing: lys går ned i ørkenen, peiling viser retning; svidd ring og fire avtrykk (Socorro) | E | åpent land |
| 10 | Kvegkadaver med rene kutt og ingen spor rundt | E | gård, åpent land |
| 11 | Tid mangler: klokka i bilen er 40 min foran spillets klokke | S | vei |
| 12 | Opplyst rom: lys tennes i et hus som skal være tomt | O/F | alle bygg |
| 13 | Dørbesøk: noe står og venter utenfor døra di | F | alle bygg med dør |
| 14 | Skikkelse står ved horisonten og er borte når lykta treffer | O | åpent land |
| 15 | Kinoen spiller film for ingen; lyset fra salen blinker | O | kino |
| 16 | Motellrom 6 har tv-sus og døra står på gløtt | E | motell |
| 17 | Strømbrudd i hele Caliche i ett minutt; trafoen surrer | S | Caliche, trafo |
| 18 | Arrayen fanger et kort signal med peiling mot et sted i verden | E | SARO, peker ut |
| 19 | 1420 AM: innringer beskriver det du så i går, med feil detaljer | S | radio |
| 20 | Varme og stråling: bilen er varm på panseret når du kommer tilbake (Cash–Landrum) | S | vei |
| 21 | Spor i sanden går rundt bilen og slutter brått | O | bil, åpent land |
| 22 | Dyr flykter forbi deg i samme retning | S | åpent land |
| 23 | Kvinne ved veien ber om skyss, svarer ikke, er borte i speilet | F | vei |
| 24 | Sterkt lys over bilen; motoren dør (bortføring ved bil) | F | vei, parkert bil |
| 25 | Støvvirvel som står stille i vindstille | O | åpent land |
| 26 | Sheriffen melder en forsvunnet person; du finner bilen tom | E | åpent land, vei |
| 27 | Den strandede bobilen: lys inne, men ingen; en bok og kjemikaliestank (hommage, ingen navn) | E | langt ute |
| 28 | Rosa cabriolet parkert ved dineren med motoren i gang og ingen i den | H | diner |
| 29 | Elvis-imitator synger alene ved motellbassenget og vinker | H | motell |
| 30 | Bigfoot krysser veien i lyskasteren og ser forlegen ut | H | vei |
| 31 | Zet fra Toms bok sitter i skjorte og hatt ved bensinpumpa og vil kjøpe vann | H | bensinstasjon |

## Regler
- Én fare per natt og ro etter fare. Første natt fredet.
- Humor (28–31) høyst én gang i uka, aldri samme natt som fare, og aldri i Story.
- Stedet trekkes med frø fra stedskatalogen; gården er bare ett sted.
- Hver hendelse gir noe: bevis, penger, film, peiling eller et spor i Reyes-sporet.

## Status i koden (kontrollert mot kilden 14. november 2026)
Alle 31 finnes i spillet, ingen er bygget og spilt av Tom etter denne kontrollen. 1 til 3, 6 til 12 og 14 til 31 er egne hendelser i `S47Happenings.cpp` (`Ids()`, `BeginMore`, `StepMore`) med rad i `Content/Director/director.json` (system `night`). 4 er `highway_ufo` (systemet `highway`), 5 er `sky_formation` (systemet `sky`) og 13 er `door_visit` (systemet `indoor`), alle i `director.json`. Delvis: 17 (strømbrudd) bare i Caliche og fra dag 18, 10 (kvegkadaver) fra dag 22, 16 (rom 6) fra dag 16.

## Steder utover gården (kodet 14. nov, ikke bygget)
Ni nye av samme slag, hver ved ett sted i stedskatalogen, av det verden har fra før (`S47NightPlaces.cpp`, rader i `director.json`). Hver som blir lagt merke til gir en nattrapport (OUT ON THE LAND og nattloggen), noe stemmen i 8 kan si tilbake, og stedet i nålens historie i to netter.

| Hendelse | Tier, fra natt | Sted | Spilleren merker | Følge |
|---|---|---|---|---|
| `windmill_knock` | hint, 1 | vindmølla på Baca | pumpestangen banker i vindstille | rapport, stopper når han går bort |
| `school_swing` | hint, 2 | skolen | en huske knirker i vindstille | rapport, stopper ved lykt eller når han kommer |
| `bay_door` | hint, 3 | bensinstasjonen | verkstedporten går opp | porten blir stående oppe (vei inn og ut), rapport |
| `cemetery_lamp` | hint, 4 | kirkegården | en lykt går mellom gravene | rapport, slukket ved porten |
| `shack_lamp` | hint, 2 | line shack | lampe i vinduet langt ute | rapport, lampeglasset varmt |
| `road_flare` | hint, 2 | lommer langs riksveien | nødbluss på skulderen, ingen bil | rapport, brenner ned når han stopper |
| `mine_knock` | glimt, 6 | den gamle gruva | banking nede i sjakta | rapport, stopper ved sjakta |
| `cairn_stones` | glimt, 5 | varden | steiner lagt i ring, funnet ved dag eller lykt | rapport, hørt om natta når han var nær nok |
| `trough_birds` | full, 9 | saltsteinen og trauet | døde fugler rundt trauet | rapport, rancheren sier det neste kveld når ingen fant dem |
