# Mottaker-PC-en: utseende og effekter (RX-LOOK-20261113)

**Status:** Under arbeid. Kodet 13. november 2026, ikke bygget, ikke sett i spill. **Gjelder:** mottaker-PC-en (midtre CRT) i Freeform, alle fem faner. **Sist kontrollert:** 13. november 2026 (kildekode, ikke motor).

Tom 13. november: «Få receiver til grafisk å se mer ut som bilde nr. to. Masse FX.» Bilde nr. to er Toms egen referanse: grønn fosfor med rav for det som er levende eller låst, tynne doble rammer med tittel felt inn i ramma, en himmelkule som hovedpanel, fossefall, spektrum og sporingsavlesninger til høyre, I- og Q-striper under, tastrad nederst.

Alt er tegnet i kode (strøk, bokser, tekst). Ingen nye assets. Hvert element viser en ekte verdi fra spillet; ingenting er pynt som kan villede. Oppgaven står i [TODO](../../Active/TODO.md) som RX-LOOK-20261113 og bygger på [RX-ACQUIRE-SKY, RX-TABS-DUE og RX-ANOMALY-FLOW](../../Active/TODO.md).

## Rammen (alle faner)

| Element | Viser | Ekte verdi |
|---|---|---|
| Hode | Parabol tegnet i strøk, «SARO OPS», «SOUTHWEST ASTRONOMICAL RESEARCH OBSERVATORY / NEW MEXICO» (som portskiltet), terminalnavn, dato og klokke | `TerminalStation`, `TerminalClock` (uendret tekst) |
| STN / EL / AZ | Hvor arrayen faktisk peker | Middel av antennene på fjernstyring (`RxLive`, `DishAz`/`DishEl`) |
| Fanerad | Sidetittel i rav boks, så ACQUIRE, TUNE, ARRAY, RECORD, DATA | Samme navn, valgt stil og pulsmerke som før (`RxDueTab`) |
| Tilstandsbanner | NO JOB, NO POWER, SOURCE NOT UP, POINT THE ARRAY, SLEWING n° TO GO, ARRAY SETTLING, SEARCHING, TAKING THE FIX, FIX HELD, SOURCE LOCK, ANOMALY DETECTED, RECORDING n/N s, REFERENCE n/8 s, TAKE ON TAPE | `S47RxLook::StateOf`, samme rekkefølge og vilkår som `ReceiverNextAction`. ANOMALY DETECTED bare der skjermen på rommet allerede sier det (`TerminalLines`) |
| ARRAY n/27, RATE | Antenner på fjernstyring og målehastighet | `AvailableDishCount`, `ArrayMeasurementRate` |
| Frekvens, linje, bølgelengde | Kortets frekvens i store sifre, «H I» eller «CONT», bølgelengde i cm | Kortet (`EffectiveProfile`), regnet ut (`WavelengthCm`) |
| Neste steg | Én eller to linjer i rav ramme | `ReceiverNextAction`, på ACQUIRE `SkyPrompt` (uendret tekst) |
| Tastrad | F1 til F5 for fanene, piltaster på ACQUIRE, klikk, ESC | Bare taster som virker. F-tastene vises og virker bare når spilleren ikke har gitt dem til noe annet |

Skjermens egen hjelpelinje («ESC / RETURN TO ROOM …») er tatt bort for mottakeren, siden ramma har sin egen tastrad. De andre terminalene er uendret.

## ACQUIRE: ekvatorial himmelkule

Regelen (`S47SkyAcquire`) er ikke rørt og regner fortsatt i asimut og elevasjon. Kula er bare hvor ting står på glasset (`S47SkySphere`, `SS47SkyChart`).

- **Projeksjon:** mild perspektiv av kula sett innenfra, nord opp og øst til venstre, vist gjennom en elliptisk kant. Bildet står fast i forhold til bakken, så sikte, katalog og peilinger står der regelen har dem. Stjernene og timene i rektascensjon driver vestover med spillets klokke (stedets sideriske tid: breddegrad 34,08, lengdegrad 106,6 V, Mountain-tid, samme himmel som spillets måne).
- **Nett:** deklinasjonssirkler med grader ved høyre kant, rektascensjonstimer under kanten.
- **Stjerner:** 242 ekte, lyse stjerner med størrelse etter lysstyrke og 154 streker i kjente figurer (Orion, Karlsvogna, Løven, Svanen med flere). Skjematisk; spillets stjernehimmel er verken lest eller rørt.
- **Melkeveien:** stiplet bånd langs galaktisk ekvator, tettere og bredere mot sentrum.
- **Arrayens vindu:** stiplet omriss av området siktet kan stå i (regelens asimut 50 til 166, elevasjon 18 til 70).
- **Katalog:** de fire kortkildene med boks og navn, de mindre «SC»-oppføringene som kryss. Navn flyttes så de ikke dekker hverandre.
- **Peilinger:** én linje per antenne, hver arm som en vifte inn fra kanten (øvre venstre, høyre, bunn). Hel og lys når armen har låst, svak med stiplet midtlinje mens den sveiper. Den gule trekanten er det som skal lukke seg.
- **Sikte:** hjørner som lukker seg, to ringer med merker, fylt midte på krysset.
- **Hjørner:** epoke B1950.0, projeksjon, modus, kilde; peilinger og armer i ord; siktets asimut og elevasjon, rektascensjon og deklinasjon (B1950) og galaktisk lengde og bredde, alt regnet fra siktets virkelige plass; rose med NCP, E og W.
- **Uvanlig kilde:** som før uten ord. Linjene krysser utenfor katalogboksen, den tredje vifta blir aldri hel, siste ring står åpen og stiplet, merket er en naken rombe.

Kortene er skrevet etter den virkelige himmelen 13. april 1986. Selvtesten sjekker at 3C 286, pulsaren og Virgo A står der kortene peker til sine klokkeslett (største avvik under fire grader).

### Det som er annerledes enn på det flate kartet

På en kule er en grad asimut bare cos(elevasjon) grader bue. Regelens låsesirkel (radius 1,6 grader i asimut og elevasjon) blir derfor smalere på tvers jo høyere kilden står: radien er 2,0 til 9,7 enheter på det åpne bildet, mot 9,9 hele veien rundt på det flate kartet. Derfor **lukker bildet seg inn**: når pekeren (eller siktet, med tastene) nærmer seg krysset, vokser kula rundt krysset, opptil 2,4 ganger. Lukket inn er radien minst 4,9 enheter, altså minst 9,7 tvers over; det flate kartets selvtest krevde minst ni. Bildet går helt inn mens fiksen tas og åpner seg igjen når den er holdt. Regelens egne tall (låseradius, fangstradius, hint, assistent) er de samme.

Piltaster, gåtaster og styrekors flytter siktet den veien de peker på glasset, med regelens egen fart. Trinnknappene heter LEFT, UP, DOWN og RIGHT 5° og gjør regelens fem graders steg den veien.

## Høyre kolonne og stripene

| Panel | Viser | Ekte verdi |
|---|---|---|
| SIGNAL WATERFALL | Rullende tid mot frekvens, vinduet sentrert på skiva. Kortets linje står lys i midten når skiva er på kortet, ute til siden når den er av, og er borte uten lås | Skiva og kortet (`FS47Rx`), styrke = bærerkvalitet × arrayen på målet × fiks på kartet |
| Tikk i fossefallet | Kort, bred strek i raden som skrives idet båndet passerer tikket | Opptakets egne tikk, bare mens opptaket skrives (som båndstripa). Aldri før |
| FFT SPECTRUM | Liten utgave av TUNE-sidas zoom: støygulv, topp på kortets linje, skivas merke, dB og MHz | Samme formel som `TuneZoom` |
| ACQUISITION / TRACKING | S/N, residual, forsinkelse per arm, rate, nærmeste katalogkilde og avstand | `S47SkyAcquire` (teller opp gjennom låsesekvensen som før). Rate er ny: hvor fort lengste forsinkelse endrer seg når himmelen dreier, som frynserate ved kortets frekvens |
| Blink | Siktets avstand fra krysset, asimut bortover og elevasjon opp, med ring for låseradius og fangstradius | `ReticleAt - Cross` i regelens grader |
| I og Q | To spor: støy uten lås, to rene sinuser en kvart omdreining fra hverandre med lås | Samme styrke som fossefallet |

**TUNE:** zoomspekteret er fortsatt hovedsaken, med fossefallet ved siden av. I og Q står der båndstripa står, og viker for den mens noe skrives. **ARRAY, RECORD og DATA** har sine kontroller som før inne i ramma (RETAIN TAKE og resten uendret).

## Effekter

Alle effekter følger røret, ikke et filter oppå. Med «Calm flashes» står alt i tabellen stille.

| Effekt | Hva | Med Calm flashes |
|---|---|---|
| Glød | Lyse strøk tegnes tre ganger: bred og svak, middels, tynn og lys. Strålen er litt bredere der den er lysere | Uendret (ingen bevegelse) |
| Etterglød | Siktets korte spor, sveipets hale, fossefallets nyeste rad litt varmere | Fossefallet uten varm rad; sporet følger bare bevegelse |
| Sveip | Langsom viser rundt kula med hale mens søket pågår, sju sekunder per runde | Ingen sveip |
| Ringer ved lås | Tre ringer går utover fra fiksen og dør ut | Ingen |
| Oppblomstring | Hele bildet sveller én gang, mykt, over 0,9 s ved låsen (opp én gang, ned én gang) | Ingen |
| Banneret | Bokstavene skrives inn når tilstanden skifter, siste bokstav varm | Står ferdig |
| Stjernene | Svak skimring, 0,35 til 0,85 Hz | Stille |
| Linjeskjelv | Et bånd av bildet flytter seg én enhet i et tidels sekund, sjelden | Ingen |
| Brumbånd | Rørets sjeldne, svake brumbånd, som før | Som før |

Hele glassets linjer og vignett er skjermens egne fra før (`SS47CrtGlass`) og er ikke gjort sterkere. Farge er aldri eneste tegn: hver tilstand står også i ord eller form.

## Kilde og prøver

`S47SkySphere.h` (tabell, projeksjon), `S47RxLook.h` (banner, effekter, skop), `SS47RxLook` (malte deler), `SS47SkyChart` (kula), `S47FreeformTerminal::RxFrame` (ramma), `FS47FreeformSession::RxLive` (alle tall). Selvtestene «sky sphere», «receiver look», «sky chart» og «sky acquire» kjøres fra `SkySelfTest`. Mottakerprøven (`-S47InternalControlTest`) finner fortsatt hver knapp på navnet og måler at sidene får plass uten rulling. Den tar nå også fire bilder av himmelkula før opptaket: `acquire-searching`, `acquire-searching-calm`, `acquire-closing-in`, `acquire-lock-moment`, og tar da fiksen for hånd slik et ekte spill gjør.

## Gjenstår

Bygg og selvtest. Se i spill: høyder på glasset, lesbarhet i små tekster, om innlukkingen kjennes rolig. Tom vurderer uttrykket mot referansen. Story-nattas mottaker er ikke rørt.
