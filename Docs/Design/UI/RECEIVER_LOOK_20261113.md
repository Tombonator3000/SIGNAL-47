# Mottaker-PC-en: utseende og effekter (RX-LOOK-20261113)

**Status:** Under arbeid. Første runde er bygget, selvtestet og sett i spill 13. november 2026. Runde 2 (rettelser etter første blikk i motoren mot Toms målbilde) er bygget og installert. Runde 3 (16 funn fra en uavhengig leser 14. november, se nederst) er kodet, ikke bygget. **Gjelder:** mottaker-PC-en (midtre CRT) i Freeform, alle fem faner. **Sist kontrollert:** 14. november 2026 (kildekode, ikke motor).

Tom 13. november: «Få receiver til grafisk å se mer ut som bilde nr. to. Masse FX.» Bilde nr. to er Toms egen referanse: grønn fosfor med rav for det som er levende eller låst, tynne doble rammer med tittel felt inn i ramma, en himmelkule som hovedpanel, fossefall, spektrum og sporingsavlesninger til høyre, I- og Q-striper under, tastrad nederst.

Alt er tegnet i kode (strøk, bokser, tekst). Ingen nye assets. Hvert element viser en ekte verdi fra spillet; ingenting er pynt som kan villede. Oppgaven står i [TODO](../../Active/TODO.md) som RX-LOOK-20261113 og bygger på [RX-ACQUIRE-SKY, RX-TABS-DUE og RX-ANOMALY-FLOW](../../Active/TODO.md).

## Rammen (alle faner)

| Element | Viser | Ekte verdi |
|---|---|---|
| Hode | Parabol tegnet i strøk, «SARO OPS», «SOUTHWEST ASTRONOMICAL / RESEARCH OBSERVATORY / NEW MEXICO» (som portskiltet) på tre linjer, terminalnavn på tre linjer, dato og klokke | `TerminalStation`, `TerminalClock`; klokka etter regelen for klokker i spillertekst (`S47RxLook::ClockLine`: aldri xx:47, 08:15 eller 20:15) |
| STN / EL / AZ | Hvor arrayen faktisk peker. Blokken står 26 enheter inn fra glassets avrundede hjørne | Middel av antennene på fjernstyring (`RxLive`, `DishAz`/`DishEl`) |
| Fanerad | Sidetittel i rav boks, så ACQUIRE, TUNE, ARRAY, RECORD, DATA | Samme navn, valgt stil og pulsmerke som før (`RxDueTab`) |
| Tilstandsbanner | NO JOB, NO POWER, SOURCE NOT UP, TEST NOT READY, ARRAY BLOCKED, POINT THE ARRAY, SLEWING n° TO GO, ARRAY SETTLING, SEARCHING, TAKING THE FIX, FIX HELD, SOURCE LOCK, ANOMALY DETECTED, RECORDING n/N s, RECORDING HELD, REFERENCE SCAN, REFERENCE n/8 s, TAKE ON TAPE, TAKE REGISTERED | `S47RxLook::StateOf`, nøyaktig samme rekkefølge og vilkår som `ReceiverNextAction` (runde 3). ANOMALY DETECTED bare der skjermen på rommet allerede sier det (`TerminalLines`) |
| ARRAY n/27, RATE | Antenner på fjernstyring og målehastighet, to linjer | `AvailableDishCount`, `ArrayMeasurementRate` |
| Frekvens, linje, bølgelengde, kilde | Kortets frekvens i store sifre, «H I» eller «CONT», bølgelengde i cm, kortets kilde | Kortet (`EffectiveProfile`), regnet ut (`WavelengthCm`) |
| Neste steg | Én eller to linjer i rav ramme | `ReceiverNextAction`, på ACQUIRE `SkyPrompt` (uendret tekst) |
| Tastrad | F1 til F5 for fanene, piltaster på ACQUIRE, klikk, ESC | Bare taster som virker. F-tastene vises og virker bare når spilleren ikke har gitt dem til noe annet |

Skjermens egen hjelpelinje («ESC / RETURN TO ROOM …») er tatt bort for mottakeren, siden ramma har sin egen tastrad. De andre terminalene er uendret. Rammens rader tar 262 enheter av glassets 824; siden har 562.

## Tekst og tall

- **Minste tekst er neste-steg-linja (Mono 22).** Målt på spillbildet i 1600 × 900: glasset er 895 px bredt og 702 px høyt for 1099 × 824 enheter, altså 0,81 px per enhet bortover og 0,85 oppover. Mono 22 står 19 px høyt og leses; Mono 14 til 18 gjorde det ikke. Regelen er færre ord, ikke mindre ord (`S47RxLook::TextFloor`). Den gjelder hodet, banneret, paneltitler og notater, akser, hjørneblokker, sporingspanelet, tastraden og TUNE-zoomens ord.
- **Bokstavbredde regnes som glasset tegner den:** en bokstav er et helt antall piksler i 1280-bildet (`S47RxLook::GlyphWide`), så høyrestilt og midtstilt tekst står der den skal.
- **Ingen tall viser 47.** Hvert tall ramma tegner skrives av `S47RxLook::Num`, som velger det nærmeste tallet uten 4 og 7 ved siden av hverandre i sifrene (4.7, 14.7, 147, 0.47). Rektascensjon og deklinasjon gjør det samme i sekunder og bueminutter (`Clean47`). Runde 3: også TUNE-sidas skiver og avlesninger, RECORD-nivåene, TuneZoom-aksen, ARRAY- og DATA-tallene og neste-steg-linjas tall går gjennom `Num`; en skive som feies går forbi 47 likt hver gang (47 % er 48 %). Arrayens tre armer heter 1, 2 og 3 (ingen enslig stor C).

## ACQUIRE: ekvatorial himmelkule

Regelen (`S47SkyAcquire`) er ikke rørt og regner fortsatt i asimut og elevasjon. Kula er bare hvor ting står på glasset (`S47SkySphere`, `SS47SkyChart`).

- **Projeksjon:** mild perspektiv av kula sett innenfra, vist gjennom en oval kant. Bildet er oppreist: hele halvkula om et punkt på ekvator midt mellom vinduets østende og vestende, nord rett opp og øst til venstre, polen øverst på kanten. Ovalen står midt i panelet med luft rundt. Bildet står fast i forhold til bakken, så sikte, katalog og peilinger står der regelen har dem. Runde 3: stjernene og timene i rektascensjon står på kortets egen sideriske tid (`S47SkySphere::CardLst`: øyeblikket kortets kilde står der katalogen har den), så merket «<kilde> / CARD», RA/DEC-avlesningen der og stjernene rundt stemmer overens når spilleren enn ser; uten kort stedets egen tid (breddegrad 34,08, lengdegrad 106,6 V, Mountain-tid). Den virkelige himmelen ute er ikke rørt.
- **Nett:** deklinasjonssirkler med grader rundt kanten der hver sirkel forlater bildet (+90° til -75°), rektascensjonstimer i en rad under ovalen (hver under stedet der meridianen krysser bildets nedre del).
- **Stjerner:** 242 ekte, lyse stjerner i tre klasser (hvitglødende punkt med halo og glint, lyst punkt med halo, enkelt punkt) og 154 streker i kjente figurer, nær hvite og tydelig over gradnettet. Under dem et svakt felt på 2400 faste stjerner: 900 på det åpne bildet, resten kommer når bildet lukker seg. Skjematisk; spillets stjernehimmel er verken lest eller rørt.
- **Melkeveien:** 900 korn langs galaktisk ekvator, tettere og bredere mot sentrum, men et bånd hele veien rundt som leses ved alle klokkeslett.
- **Arrayens vindu:** stiplet omriss av området siktet kan stå i (regelens asimut 50 til 166, elevasjon 18 til 70).
- **Katalog:** de fire kortkildene med boks og navn, de mindre «SC»-oppføringene som kryss. Navn flyttes så de ikke dekker hverandre, og holder seg unna siktet når fiksen tas. Småkatalogens navn står bare nær siktet eller når bildet er lukket.
- **Peilinger:** én tynn linje per antenne, hver arm som en vifte inn fra kanten (øvre venstre, øvre høyre, bunn) med armens tall 1, 2 eller 3 ved kanten. Aldri en kile: ingen glød, annenhver linje starter lenger ute, ved lås stopper alle før siktets ringer, og tverrstreker krysser hver vifte. Lysere når armen har låst, svakere med stiplet midtlinje mens den sveiper. Den rav trekanten er det som skal lukke seg. Vifta som kommer inn ovenfra tegnes bredere enn regelens sju grader, fordi kula ellers klemmer den til en stripe; hver linje går fortsatt gjennom sitt eget punkt i båndet, og midtlinja er regelens egen.
- **Sikte:** rav i alle faser og det lyseste på glasset: hjørnebraketter som lukker seg, tre ringer, hår og varm midte. Runde 3: den indre ringen er regelens låsesirkel slik den ligger på glasset (en ellipse), i full størrelse når bildet er lukket inn. At siktet står på krysset sies med form (fylt midte, stolpen som fylles), ikke med fargen.
- **Hjørner:** tre korte linjer hver: epoke, modus og kilde; peilinger inne og spredning; siktets asimut og elevasjon, rektascensjon og deklinasjon (B1950); rose med NCP, E og W.
- **Uvanlig kilde:** som før uten ord. Linjene krysser utenfor katalogboksen, den tredje vifta blir aldri lys, siste ring står åpen og stiplet, merket er en naken rombe.

Kortene er skrevet etter den virkelige himmelen 13. april 1986. Selvtesten sjekker at 3C 286, pulsaren og Virgo A står der kortene peker til sine klokkeslett (største avvik under fire grader).

### Det som er annerledes enn på det flate kartet

På en kule er en grad asimut bare cos(elevasjon) grader bue, og det oppreiste bildet viser hele halvkula, så en grad er omtrent to tredeler av hva den var da vinduet fylte ovalen. Regelens låsesirkel (radius 1,6 grader i asimut og elevasjon) er 1,3 til 6,7 enheter på det åpne bildet, mot 9,9 hele veien rundt på det flate kartet. Derfor **lukker bildet seg inn**: når pekeren (eller siktet, med tastene) nærmer seg krysset, vokser kula rundt krysset, opptil 3,6 ganger. Lukket inn er sirkelen minst ni enheter tvers over, som det flate kartets selvtest krevde. Bildet går helt inn mens fiksen tas og åpner seg igjen når den er holdt. Regelens egne tall (låseradius, fangstradius, hint, assistent) er de samme.

Piltaster, gåtaster og styrekors flytter siktet den veien de peker på glasset, med regelens egen fart. Trinnknappene heter LEFT, UP, DOWN og RIGHT 5° og gjør regelens fem graders steg den veien.

## Høyre kolonne og stripene

| Panel | Viser | Ekte verdi |
|---|---|---|
| SIGNAL WATERFALL | Rullende tid mot frekvens, vinduet sentrert på skiva, sekunder langs venstre side. Rader på to enheter tegnet som korn langs raden. Kortets linje står som en lys kjerne med to svakere trinn i midten når skiva er på kortet, ute til siden når den er av, og er borte uten lås | Skiva og kortet (`FS47Rx`), styrke = bærerkvalitet × arrayen på målet × fiks på kartet |
| Tikk i fossefallet | Kort, bred strek i raden som skrives idet båndet passerer tikket | Opptakets egne tikk, bare mens opptaket skrives (som båndstripa). Aldri før |
| FFT SPECTRUM | Liten utgave av TUNE-sidas zoom: støygulv, topp på kortets linje, skivas merke, tre dB-tall og tre MHz-tall | Samme formel som `TuneZoom` |
| ACQUISITION / TRACKING | S/N, residual, forsinkelse per arm, rate, nærmeste katalogkilde og avstand, ett navn og ett tall per linje | `S47SkyAcquire` (teller opp gjennom låsesekvensen som før). Rate er hvor fort lengste forsinkelse endrer seg når himmelen dreier, som frynserate ved kortets frekvens |
| Blink | Siktets avstand fra krysset den veien det ligger på kartet (runde 3: opp på tastene er opp på blinken), med ring for låseradius og fangstradius, og avstanden i grader under | Retning fra kartets projeksjon, avstand `ReticleAt - Cross` i regelens grader |
| I og Q | To spor: støy uten lås, to sinuser en kvart omdreining fra hverandre med lås, med litt støy som rir på dem (0,13 av full skala) | Samme styrke som fossefallet; støyen har fase fra jobbens frø |

**TUNE:** siden fyller hele rammens kropp. Skivene, avlesningene og stripa beholder sin høyde; zoomspekteret og fossefallet ved siden av får resten (omtrent 290 enheter, før 232 over et tomt bånd). I og Q står der båndstripa står, og viker for den mens noe skrives. **ARRAY, RECORD og DATA** har sine kontroller som før inne i ramma (RETAIN TAKE og resten uendret).

Bokser per bilde: fossefallet omtrent 750 ved kula (før 1000 kvadratiske celler i 25 rader) og omtrent 1080 på TUNE (før 1760). Himmelen omtrent 950 til 1200 (før omtrent 270).

## Effekter

Alle effekter følger røret, ikke et filter oppå. Med «Calm flashes» står alt i tabellen stille.

| Effekt | Hva | Med Calm flashes |
|---|---|---|
| Glød | Lyse strøk tegnes tre ganger: bred og svak, middels, tynn og lys. Strålen er litt bredere der den er lysere. Ett trinn til bare for de tre overskriftsordene (SARO OPS, tilstanden, kortets frekvens) og for siktet, som også har nær hvit kjerne | Uendret (ingen bevegelse) |
| Etterglød | Siktets korte spor, sveipets hale, fossefallets nyeste rad litt varmere | Fossefallet uten varm rad; sporet følger bare bevegelse |
| Sveip | En perle som løper langsomt rundt kanten med hale mens søket pågår, sju sekunder per runde, og en svak viser til den | Ingen sveip |
| Ringer ved lås | Låsesekvensens tre ringer lukker seg om krysset og blir siktets ringer. Ved låsen går tre ringer utover fra fiksen og dør ut | Sekvensens ringer lukker seg som ellers (langsomt); ingen går utover |
| Oppblomstring | Hele bildet sveller én gang, mykt, over 0,9 s ved låsen (opp én gang, ned én gang) | Ingen |
| Banneret | Bokstavene skrives inn når tilstanden skifter, siste bokstav varm | Står ferdig |
| Stjernene | Svak skimring på tabellens stjerner, 0,35 til 0,85 Hz | Stille |
| I og Q | Sporene løper, og støyen som rir på dem lever; uten lås driver støyen jevnt (runde 3, før ny 14 ganger i sekundet) | Sporene står, med støyen |
| Lite spektrum og TuneZoom | Støyen beveger seg | Står (runde 3) |
| Bildehopp og mørkt slag på interferens | Hele bildet hopper sidelengs eller mørkner et øyeblikk, høyst tre ganger i sekundet (runde 3, før opptil fem og seks) | Ingen |
| Linjeskjelv | Et bånd av bildet flytter seg én enhet i et tidels sekund, sjelden | Ingen |
| Brumbånd | Rørets sjeldne, svake brumbånd, som før | Som før |

Hele glassets linjer og vignett er skjermens egne fra før (`SS47CrtGlass`) og er ikke gjort sterkere. Farge er aldri eneste tegn: hver tilstand står også i ord eller form.

## Runde 2: hva første blikk i motoren viste

| Funn 13. november | Rettet slik |
|---|---|
| Stjerner og himmel for tynne, ingen Melkevei | Svakt felt, tre stjerneklasser med halo, nær hvite figurlinjer, 900 korn i båndet |
| Små tekster uleselige på glasset | Gulv på Mono 22 for all tekst i ramma, færre ord |
| Kula sto skjevt og trangt | Oppreist halvkule midt i panelet, grader rundt kanten, timer under |
| Viftene ble lyse kiler ved lås | Tynne linjer i alle faser med tverrstreker; siktet rav, større og lysest |
| Fossefallet var klosset | Tynne rader av korn, linje med myke kanter, sekunder langs siden |
| Tomt bånd på TUNE | Siden fyller kroppen |
| Flat glød | Ett trinn til på overskriftsordene og siktet |
| I og Q var rene sinuser | Litt støy rir på dem |
| Hodets høyre blokk kuttet av hjørnet | Flyttet inn, bredden regnet riktig |
| To selvtestpunkter kunne ikke feile; tall kunne vise 47 | Fiksen lages gjennom modellen og øktas steg og kontrolleres som håndfiks og som assistert; `Num` for alle tall |

Valget for kula koster målestokk: det åpne bildet er mindre per grad enn i første runde. Innlukkingen på 3,6 ganger gjør opp for det, men må kjennes rolig i spill.

## Kilde og prøver

`S47SkySphere.h` (tabell, projeksjon, svakt felt), `S47RxLook.h` (banner, effekter, skop, tekstmål, tall uten 47), `SS47RxLook` (malte deler), `SS47SkyChart` (kula), `S47FreeformTerminal::RxFrame` (ramma), `FS47FreeformSession::RxLive` (alle tall). Selvtestene «sky sphere», «receiver look», «sky chart» og «sky acquire» kjøres fra `SkySelfTest`. Økta prøver nå fiksen slik kartet lager den: uten peilinger ingen fiks, modellen låst og øktas steg tatt gir håndfiks, og et kart ingen rører gir assistert fiks.

Mottakerprøven (`-S47InternalControlTest`) finner fortsatt hver knapp på navnet og måler at sidene får plass uten rulling. Den tar bilder av himmelkula før opptaket (`acquire-searching`, `acquire-searching-calm`, `acquire-closing-in`, `acquire-lock-moment`), tar da fiksen for hånd slik et ekte spill gjør, og tar etter runde 2 også `tune-before-take`, `array-tab` og `data-tab`.

## Runde 3: gjennomgang av uavhengig leser 14. november

16 funn, alle kontrollert i koden; 15 rettet, ett var ikke en feil (høyre stikke). Det som endrer oppførsel:

- **Styring og blink:** blinken i ACQUISITION / TRACKING tegner avviket den veien det ligger på kartet (`SS47RxTrack::EyeAt`, samme projeksjon som `SteerOnGlass`). Før var det asimut bortover og elevasjon opp, som skiller seg fra kartets opp med den parallaktiske vinkelen (rundt -64° ved 3C 286), så OPP flyttet merket på skrå.
- **Zoomkurven:** med tastene eller assistenten lukker bildet seg etter `ZoomForTrue`, så trådkorsets avstand på glasset aldri vokser mens det nærmer seg krysset (før gikk den fra 24 til rundt 35 åpne grader mellom 24° og 13°), og `ZoomStep` lar ikke zoomen gli raskere enn det. `ZoomNear` 4 → 6, så hele låsesirkelen står helt lukket inn (3,6). Pekerens kurve er ellers som før. Et kart som bygges på nytt midt i søket står like langt lukket inn (`ZoomAtStart`), i stedet for å hoppe åpent.
- **Fryst stjernetid:** kartet står på kortets sideriske tid (se Projeksjon over).
- **Armnavn:** 1, 2 og 3 ved viftene og DELAY 1 til 3 i panelet.
- **Låsesonen:** regelens avstand er uendret (prøvene og balansen bygger på rene grader; buemål ville gjort låsen bredere på tvers og lettere høyt oppe). I stedet er den indre ringen låsesirkelen på glasset. Vanskeligheten er den samme.
- **Banneret** følger `ReceiverNextAction` nøyaktig, med TEST NOT READY, ARRAY BLOCKED også før pekingen og REFERENCE SCAN under skanningen.
- **Rolige blink:** spekteret, TuneZoom-støyen, bildehoppet og det mørke slaget står stille; uten rolige blink høyst tre i sekundet.
- **Taster:** stegtastene tar ikke tastaturet fra kartet. Sporingspanelets siste rad står hel.
- **Prøver:** ordene de malte delene skriver leses gjennom de samme funksjonene som tegner dem; mottakerprøven går gjennom hele widgettreet på alle faner med skivene feid, sjekker at fiksen holdes, at rolige blink står stille, at DATA, ARRAY og RECORD får plass og at banner og linje sier det samme.

Høyre stikke: den styrer spillets virtuelle peker over glasset (`S47Gamepad`), og kartet følger pekeren innenfor kanten som en mus, så ledeteksten er sann og er ikke endret.

## Gjenstår

Bygg og selvtest av runde 3. Bygg og selvtest av runde 2 er gjort. Se i spill mot målbildet: om teksten leses, om ovalen står rolig med luft, om stjernefeltet og Melkeveien har riktig tetthet, om innlukkingen på 3,6 ganger kjennes rolig, om DATA får plass (loggført av mottakerprøven). Tom vurderer uttrykket mot referansen. Story-nattas mottaker er ikke rørt.
