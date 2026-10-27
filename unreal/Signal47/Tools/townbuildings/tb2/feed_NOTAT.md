# Feed TB2, notat

Kilde: `tb2/feed.py` (`build(lod)`), egne materialer i `tb2/materials_feed.json`. Ren Python, samme lokale ramme som TB1 (meter, Z opp, fasadeplan x=0, ut mot +x). Fotavtrykk, vegghøyde 5,2 m, dør og vinduer leses fra `design.BUILDINGS['Feed']` og er ikke endret. TB1 sin interiør og skall er skrevet på nytt; bare hjelpere fra `geometry.py` (Mesh, front_wall, seg) gjenbrukes.

## Hva som er lagt til

Skall (Shell)
* Falsk front i tre trinn (6,25 / 6,75 / 7,35 m) med korbelbånd, malt profillist og galvanisert beslag med dryppkant på hvert trinn og hver trinnvange.
* Varm kremfarget puss (eget materiale FeedStucco), hjørnepilastre med sokkel og kapitél, slitt sementsokkel og et mørkere skittbelte med ujevn overkant opp til rundt 1 m på fasade og pussede sidevegger. Opphøyde reparasjonsflekker i lysere puss (6 til 14 mm), noen få flekker med avskallet puss der adobe synes, tynne regnstriper under beslag og vinduskarmer.
* FEED & SEED er malt rett på den falske fronten med store mørke bokstaver med seriffer, modellert som 9 til 13 mm opphøyd maling (rette strøk som bjelker med forskjøvet dybde, buer som glatte bånd). Ingen skiltplate. Svanehalslampene står over teksten.
* Butikkfront i døråpningen: postamenter med fyllinger, to lukkede glassdører med sprosser, sidelys og overlysrekke. Døren er lukket.
* Vinduer med dype karmer, midtpost, overlys med sprosser, sprossede rammer, betongsålbenk og overligger i tre.
* Pusset butikkdel 9,5 m dyp, deretter lager i bølgeblikk på sider og bakvegg med ekte bølgeprofil, to plateløp med overlapp, galvaniserte lappeplater, hjørnebeslag og sokkelbeslag.
* Lavt saltak 1:12 bak den falske fronten, bølgeblikk i tre plateløp per takflate med trinnvis overlapp, mønebeslag, vindski, gesimsbord, kassetakrenner med endelokk, nedløp med albuer, klammer og sprutstein.
* Avstivere i tre bak den falske fronten, innfestingsbeslag mot taket.
* Swamp cooler på plattform med lameller og kanal, tre turbinventiler, ovnsrør med regnhatt og bardunering.
* Markise i rustent bølgeblikk (FeedRustTin, rusttonet RustTin) med ekte profil og underside, vegglekter, utkragede sperrer, åser, sternbord, knekter der veggen er hel og strekkstag til den falske fronten ved endene. Ingen stolper.
* Lasterampe med nøyaktig samme volum som TB1 sin `Existing_loading_dock`: betongkropp, plankedekke, stålvinkel i kant og ender, fire gummifendere.
* Rekvisitter: to paller med flate fôrsekker, saltsteiner på pall, kasse, galvaniserte bøtter og balje, benk ved venstre vindu, høyballer ved høyre hjørne, propanbur med fire flasker på sideveggen. Skyvedør i tre på lagerveggen med Z-avstivning.

Glass: syv ruter (to vinduer, to sidelys, overlys, to dørblad).

Interiør: skilleveggen mot lageret har en svakt lysende varm flate (FeedWallGlow, emisjon 0,13) slik at butikken leser opplyst innenfra i skumring uten å bli påtrengende om dagen. Plankegulv på rampenivå 0,95, panelt himling med synlige bjelker, skillevegg med bred åpning til lageret, brystpanel, disk med fyllinger, kassaapparat, vekt og ordrelapper, hyller langs sideveggene med bokser og esker, seletøy og tau på knagger, redskapsstativ, sekkestabler på paller, galvaniserte binger, åpne frøbinger med korn og priskort, sekker og høy i lageret.

Inventar (Fixtures): tre svanehalslamper over skiltet og to på sideveggene med emissive linser (WarmGlow), to pendler under markisen, seks pendler inne med sterkere pære (FeedBulbGlow) og svakt lysende emaljeinnside (FeedShadeGlow), dørhåndtak, sparkeplater, lås, CLOSED-kort, strekkstag, sekketralle, propanbur, måler, bryter, rørmast med værhette, dryppsløyfer og isolatorer, skinne og hjul til skyvedøren, hengelås.

## Trekanter

| Del | LOD0 | LOD1 | LOD2 |
|---|---:|---:|---:|
| Shell | 24026 | 15190 (63 %) | 7289 (30 %) |
| Glass | 14 | 14 | 14 |
| Interior | 22464 | 14304 (64 %) | 5896 (26 %) |
| Fixtures | 5954 | 3462 (58 %) | 2006 (34 %) |

Alle innenfor budsjett (30000 / 500 / 25000 / 6000). Glass er bare syv firkanter og reduseres ikke.

## Bilder

`/home/tom/scratch-bm/Signal47-townbuildings-tb2-20261027/feed/`
* `Feed_tb2_lod0_street.png`, `_oblique.png`, `_door.png`, `_roof.png`, `_dusk.png`
* `Feed_tb2_lod1_street.png`, `Feed_tb2_lod2_street.png` (rendret før runde 2, viser fortsatt skiltplaten og den kalde pussen; door og roof er også fra før runde 2)
* Sammenligning: `Feed_tb1_lod0_*.png`

## Avvik og gjenstående svakheter

* Lasterampen er 0,95 m høy og stikker 2,6 m ut, slik den allerede var i TB1 (`Existing_loading_dock`, interiøret står på samme nivå). Volumet er beholdt nøyaktig, ikke senket til 0,2 m, fordi gulvnivå og eksisterende kollisjon bygger på det. Fendrene ligger innenfor volumet. Rekvisittene på rampen står oppå eksisterende kollisjon.
* Markisen henger på knekter og strekkstag uten stolper. Høyballer, benk, propanbur, nedløp og sprutsteiner står innenfor 0,6 m fra veggen.
* Nattlyset er emissive flater. Vinduene gløder varmt i skumringsbildet og hyller og sekker synes, men det er ingen ekte lyskilder. Utelampene lyser ikke opp veggen; det må integrator gjøre i Unreal. Emisjonsstyrken på FeedBulbGlow (5) og FeedWallGlow (0,13) er avstemt i Cycles og må kontrolleres i Unreal.
* Takets bølgeprofil (19 cm periode, litt grovere enn ekte 7,6 cm for å lese på avstand) synes på nært hold, men på lang avstand dominerer RustTin-teksturen, som minner litt om singel.
* Serviceledningen ender i isolatorer på masten. Spennet til gatestolpen finnes ikke i denne modellen.
* Bokstavene er en enkel egen strekfont: seriffene er korte staver, og & er forenklet. Saltsteinene er enkle avrundede kuber, og flatsekkene har ingen trykk. Regnstripene er geometri, ikke dekaler, og kan virke litt regelmessige på nært hold.
* Den pussede sideveggen er stor og jevn. Venstre side (-y) og bakveggen er ikke sjekket med egne kameravinkler, bare i tak- og skråbildet.
