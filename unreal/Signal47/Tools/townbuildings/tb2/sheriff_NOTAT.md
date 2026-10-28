# Sheriff TB2, notat

Kilde: `tb2/sheriff.py` (`build(lod)`) og `tb2/materials_sheriff.json`. Ren Python som TB1, ingen Blender i selve bygget. Fotavtrykk, hovedveggens høyde (3,5 m), front, dør og vindu fra `design.BUILDINGS['Sheriff']` er uendret. Døren er lukket. Interiøret fra TB1 brukes skrivebeskyttet via `town_geometry.sheriff(lod)` og bygges videre på. Skall, glass og armaturer er nye.

## Hva er lagt til

Skall: vegger med ekte åpninger. De fire hjørnene er egne blokker med avrundet ytterkant (radius 9 cm), og sokkel og avdekning har tilsvarende runde hjørnestykker (TB1 hadde et hakk på 11 x 11 cm i hvert fronthjørne). Trappet parapet med hevet midtparti over vinduet. Avdekningen har avrundet profil og en topplinje som bølger et par centimeter, så parapeten ikke er linjal rett. Sokkel i mørkere puss rundt hele bygget (stopper ved døren), forsterket fundament. Frontvinduet har nå dyp lysåpning med 7 cm synlig smyg, innfelt mørk karm med to poster, glasslister, prefabrikert vindusbrett med fall og dryppnese, smidd gitter med flattjern, veggører og to tverrjern, og avrenningsflekk under brettet. Skiltkassen CALICHE COUNTY fra TB1 er fjernet etter konseptet. På den hevede parapeten står en stor støpt messingstjerne (radius 0,28 m) med fasetterte armer som reiser seg 11 cm fra veggen, kuler, ring og mørk bakplate, og hevede bokstaver SHERIFF (0,40 m høye, 4,5 cm dype). To svanehalslamper montert under avdekningen lyser ned på bokstavene.

Inngang: betongtrinn 12 cm foran døren og videre inn i åpningen, aluminiumsterskel, karm og listverk, dør med ramtre, 2 x 3 glassruter, innfelt nedre fylling med X stag, CLOSED kort på snor bak glasset (atlas celle 15). Messing sparkeplate, hengsler, dørknott, sylinderlås og dørlukker i Fixtures. Utkraget baldakin over døren (underkant 2,42 m) med vegglekt, sperrer, bordtak, rustent blikk med korrugering, vindskier, beslag mot vegg og to strekkstag med strammeskrue. Takarmatur med opal glob under baldakinen.

Gate: glassert oppslagstavle med korkbunn og oppslag, benk med jernramme og værbitte lekter under vinduet, galvanisert søppeldunk med lokk.

Sidevegger: et lite sprossevindu med gitter, tømmeroverligger og nedtrukket rullegardin som gløder svakt på hver side. Høyre side: målerskap med målerglass, hovedbryter, rør, servicemast med festebånd, værhatt, isolatorspole og dryppsløyfer, jordledning og en vegglampe med gitter. Venstre side: gassmåler med stigerør, regulator og stengeventil, utekran med dryppflekk. Kanaler i tre (canales) på begge sider og to bak, med vannstriper og spruteblokk. Reparerte pussflekker med 7 mm relieff og setningssprekker fra vindus og dørhjørner.

Tak: tjærepapptak med kileliste og beslag mot parapeten, lappede felt, fordampningskjøler (swamp cooler) på stativ med lameller, kanal ned i takkant og kobberrør for vann, to luftestusser, takluke, og et barduneret trekantet gittertårn for fylkets radio (til 11,6 m, pisk til 15 m) med jordplanantenne, sidemontert dipol, koaksialkabel inn i taket og rødt hinderlys (Fixtures).

Interiør: to oliven arkivskap med mapper og en arkiveske ved kartet, ny bordlampe med lysende skjerm på radiokommoden rett innenfor vinduet, malt kledning på innsiden av front og sidevegger (olivengrønt brystpanel, brystlist, varm puss over), ben under fylkeskartet så det ikke svever, lav kommode under vinduet med radiostasjon, mikrofon, loggbok og kaffekopp, askebeger, blyantkopp og skriveunderlag på pulten, veggur og fire skriveplater på skilleveggen, vannkjøler, stumtjener med hatt og jakke, rullegardiner. TB1 sitt arkivhylleparti ved y 5,7 stakk 9 cm ut gjennom sideveggen og er flyttet 0,4 m inn. Tre lysrørarmaturer i kontoret pluss den gamle i bakrommet, alle med kjeder, og to bordlamper med lysende skjerm som synes fra gaten om kvelden.

Nye materialer (alle på eksisterende teksturer): SheriffStucco (varm krem), SheriffStuccoBase (støvete brun), SheriffStuccoPatch, SheriffStreak, SheriffCrack, SheriffCoping, SheriffRoofing, SheriffLetters, SheriffBrass, SheriffCork, SheriffWainscot, SheriffWeatheredWood, SheriffLampGlow (12), SheriffTubeGlow (14, varmhvit), SheriffShadeGlow (4).

## Trekanter

| Del | LOD0 | LOD1 | LOD2 |
|---|---:|---:|---:|
| Shell | 9703 | 5904 (61 %) | 2916 (30 %) |
| Glass | 100 | 64 (64 %) | 28 (28 %) |
| Interior | 7560 | 4940 (65 %) | 2020 (27 %) |
| Fixtures | 1734 | 982 (57 %) | 562 (32 %) |

Alle under budsjett (30000 / 500 / 25000 / 6000). LOD1 og LOD2 fjerner smådetaljer (glasslister, stjernekuler, bølger i avdekningen, ribber, lameller, hengsler, kjeder, sidevinduenes gitter på LOD2) og bruker færre segmenter. Interiøret på LOD1 dropper TB1 sine skrivemaskintaster og skuffehåndtak, på LOD2 også bøker, etikettkort, viftegitter og fingerhull.

Kontroll: alle flater er plane (avvik under 0,1 mm), ingen degenererte flater, alle materialer finnes. Under 2,3 m stikker ingenting mer enn 0,58 m ut fra veggflaten bortsett fra betongtrinnet (0,84 m ut, 0,12 m høyt).

## Bilder

`/home/tom/scratch-bm/Signal47-townbuildings-tb2-20261027/sheriff/`

Se først `Sheriff_tb2_lod0_street.png`, `Sheriff_tb2_lod0_dusk.png` og `Sheriff_tb2_lod0_door.png`, deretter `Sheriff_tb2_lod0_oblique.png` og `Sheriff_tb2_lod0_roof.png`. LOD kontroll: `Sheriff_tb2_lod1_street.png` og `Sheriff_tb2_lod2_street.png` (runde 2). `Sheriff_tb2_lod1_dusk.png` og `Sheriff_tb2_lod2_oblique.png` er fra runde 1, før skiltkassen ble fjernet. TB1 til sammenligning ligger i `../baseline/`.

## Svakheter som gjenstår

1. Kveldsbildet: interiøret er fortsatt mørkt i forhåndsvisningen, som bare har emissive flater og ingen ekte lys. Etter runde 2 er verdiene satt ned til et fornuftig nivå (lysrør 14, pærer 12, lampeskjermer 4), så det varme inntrykket kommer mest fra de to lampeskjermene. Integrator bør sette ekte lys ved Fixtures.
2. Dagslysbildene i forhåndsvisningen er overeksponerte og blåstikkende, så pussen ser lysere ut der enn i kveldsbildet. Fargen er sjekket både dag og kveld.
3. Bare ytterhjørner, sokkel og avdekning er avrundet. Lysåpninger, parapetens trinn og selve veggflaten er fortsatt skarpe og plane.
4. Det finnes ingen inntaksledning fra stolpe til servicemasten, fordi stolpene i byen ikke er kjent her. Masten ender i isolator og dryppsløyfer.
5. Sidevinduenes rullegardiner gløder litt også i dagslys siden materialet er emissivt.
6. Radiotårnet er høyt (15 m med pisk) og kan være synlig langt bortover i byen. Det er bevisst etter konseptet, men bør vurderes i spillet.
7. Tegn på slitasje er geometriske (flekker, sprekker, striper). Det finnes ingen egne smuss eller kantslitasjeteksturer, siden bare eksisterende teksturer kunne brukes.
8. Arkivskapene og de fleste nye rekvisittene finnes ikke på LOD2.
