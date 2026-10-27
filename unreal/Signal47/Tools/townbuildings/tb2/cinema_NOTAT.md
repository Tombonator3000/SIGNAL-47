# TB2 Kino (Plaza), Caliche

Kilde: `tb2/cinema.py` med `build(lod)` for LOD 0, 1 og 2. Nye materialer ligger i `tb2/materials_cinema.json` og bruker bare eksisterende teksturfiler i `textures/`. Geometrien er skrevet på nytt fra bunnen i samme lokale ramme som TB1. TB1 brukes bare til lesing (hjelpere fra `geometry.py`, ankre fra `design.py`).

## Uendret
Fotavtrykk, vegghøyde 6,2 m, fasaden mot +x, døren (y 2,6) og de to dørhøye åpningene (y 1,0 og y −2,6) er nøyaktig som i `design.BUILDINGS['Cinema']`. Alle tre åpningene er lukkede glassdører. Ingen ny adgang for spilleren. Under 2,3 m stikker ingenting mer enn 0,6 m ut fra en vegg: billettluken går 0,51 m ut, plakatkassene 0,27 m, trappetrinnene ved nødutgangene 0,5 m. Terrassoforplassen under marquee er 2 cm høy.

## Hva som er lagt til
* Fasade i varm, falmet puss med ekte tilbaketrukket, trinnvis parapet: høyere midtparti, lavere fløyer, to trinnvise pyloner med terrakottariller, et sentralt trinntårn bak bladskiltet og tre terrakottafinner på hver side med trinnvise topper. Avrundede finner og riller i LOD0.
* Støpt ornamentikk som enkel geometrisk relieff: trinnvise rombemedaljonger i terrakotta, turkis og keramikk, sikksakkfrise under gesimsen, ziggurat på pylonhodene, «speed lines» på fløyene. Diskré regnstriper under beslaget og en lappet puss.
* Sokkel i svart glasert flis under marquee, mørk sprutsone på fløyene.
* Marquee med ekte dybde: undertak med skjøter, røde emaljerte pærelister foran og på sidene, aluminiumsrammer, lysende melkehvite tavler foran og på begge sider, skinner for løsbokstaver og fysiske svarte bokstaver CLOSED (egen original blokkskrift i geometri). Trinnvis gesims med tannsnitt, røde neonrør på hjørnene og under gesimsen, strekkstag med strammeskruer og veggplater, blinkerskap og koblingsboks på taket.
* Pærer: 34 i fronten, 6 på hver side, 3 rader med 13 hengende pærer i undertaket. Noen få er døde (eget umalt materiale), slik et slitt 1986 skilt ville vært.
* Høyt bladskilt med rund topp: emaljert kropp, kremfarget kantlist, kanalbokstaver P L A Z A på begge sider med eksponerte røde neonrør, neonkontur, pærerekke i forkanten, vinkelbeslag og to rørstag ned til taket, føderør.
* Billettluke senket inn i fasaden: buet glass (eget glass), svart flisbase, valnøttdisk med messingkant, messingposter, trinnvis krone, nedrullet skodde bak glasset, CLOSED kort, snakkerist med messingring, pengeskål. På foajésiden en panelt boks med dør.
* Tre dørpar i bronseanodisert aluminium med glass, skyvestang og avstandsholdere, sparkeplate i messing, hengsler og låssylindre (LOD0), messingterskel. Bronsegitter i transomfeltet over hver dør.
* Plakatkasser med aluminiumsramme, glass, trinnvis krone, sil og svanehalslampe.
* Terrassoforplass med rød ramme, krembunn og rød midtstripe, lagt som ikke overlappende felt (ingen z kamp).
* Sidevegger med pilastre, sprutsone, ståldør for nødutgang med trinn og lampe, lamellrister, nedløp med takrenneutløp og skvettblokk. Elektrisk inntak synlig fra gaten (måler, bryterboks, mast med værhode, rør over parapeten til skiltene).
* Tak: tjærepapp med skjøter, projeksjonsrom med dør, sopphatt og luftestokk, fordampningskjøler på vinkeljernstativ med kanal, takluke, luftestokker, svanehalsventil, rør på trebukker til skiltene.
* Foajé: terrassogulv med innlagt mønster, vegg mot salen med to åpninger og lukkede fløyelsgardiner med folder, valnøtt brystpanel med flissokkel, himling med valnøttlist, takplafonder, vegglamper, innrammede plakater med bildelamper, kiosk med terrassoplate, godterimonter i glass, popkornmaskin med glass, kasseapparat, bakdisk med sirupsdispenser og meny, messingstolper med fløyelstau, askebeger, søppelkasse, radiator og benk.

## Trekanter
| Del | LOD0 | LOD1 | LOD2 | Budsjett LOD0 |
|---|---:|---:|---:|---:|
| Shell | 9 922 | 6 352 (64 %) | 3 770 (38 %) | 30 000 |
| Glass | 50 | 42 | 38 | 600 |
| Interior | 3 426 | 1 866 (54 %) | 934 (27 %) | 25 000 |
| Fixtures | 7 534 | 4 338 (58 %) | 2 326 (31 %) | 8 000 |
| Sum | 20 932 | 12 598 (60 %) | 7 068 (34 %) | |

LOD lages på to måter: færre segmenter og pærer per LOD, og en merkebasert fjerning av små deler (`DROP_LOD1`, `DROP_LOD2`) slik at alle LOD har nøyaktig samme plassering. LOD1 og LOD2 bruker flate bokstaver i stedet for bokser.

## Bilder
Mappe `/home/tom/scratch-bm/Signal47-townbuildings-tb2-20261027/cinema/`:
* `r4/Cinema_tb2_lod0_street.png`, `_oblique`, `_door`, `_roof`, `_dusk` (endelig LOD0)
* `r4/Cinema_tb2_lod1_street.png`, `r4/Cinema_tb2_lod1_dusk.png`, `r4/Cinema_tb2_lod2_street.png`, `r4/Cinema_tb2_lod2_dusk.png`
* `r4/cu_lobby.png` (nærbilde inne i foajeen, eget kamera)
* Sammenligning TB1: `Cinema_tb1_lod0_*.png` i rotmappen. Tidligere runder i `r1` til `r3`.

## Svakheter som gjenstår
* Bladskiltets topp er utenfor bildet i gatevisningen; bokstavene er bare lesbare fra siden (riktig for et bladskilt), og neonbokstavene er enkle polylinjer uten bøyde hjørner.
* Pussen er en jevn tekstur med få flekker. Konseptets flekkete, avskallede puss krever en egen tekstur eller dekaler.
* FloorTile teksturen gir terrassoen synlige flisfuger, mer flis enn ekte støpt terrasso.
* Foajeen er mørk i forhåndsvisningen fordi bare emitterende flater lyser; integrator må sette ekte lys. Bakdisk og kiosk sees bare svakt gjennom glassdørene.
* Glass LOD forholdet er høyt (84 % og 76 %), men tallene er så små at det ikke betyr noe.
* Noen få koplanare sideflater gjenstår der bokstavstrøk møtes; frontflatene er forskjøvet og viser ingen artefakter i bildene.
* I LOD2 er veggplatene til strekkstagene fjernet, så stagene går rett inn i pussen; medaljongene og sikksakkfrisen finnes ikke i LOD2.
* LOD1 og LOD2 skumringsbildene er rendret, men bare gatebildene er kontrollert med øyet.
* Emisjonsstyrkene (pærer 7, neon 11, tavle 2,8) er satt for Blender forhåndsvisningen og må justeres i Unreal.
