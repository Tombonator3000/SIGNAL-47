# Kirken i Caliche, TB2

Kilde: `tb2/church.py` med `build(lod)`. Nye materialrader: `tb2/materials_church.json`. Ren Python, samme lokale ramme som TB1 (meter, Z opp, framgavlen vender mot negativ X ved x=0). Fotavtrykk, veggens høyde (4,2 m til takfot), dør (2,2 x 2,9 m) og vinduer fra `design.BUILDINGS['Church']` er uendret. Sidevinduene står på samme plass og har samme mål som i TB1. Døren er lukket.

Runde 2 gjaldt skumrings- og nattinntrykket: varmt glødende vinduer, varmere hvitmaling, en tydelig lysflekk fra lampen i inngangen og nye takrenner som ikke lenger ser ut som kroker i endene. Konseptarket `08-kirke.png` styrer retningen: hvit liggende kledning, mørkt shingeltak, klokketårn på framgavlen og inngangsparti med gavl. Fra fotoreferansen 13 er bare den steinlagte adkomsten tatt med.

## Hva som er lagt til

Skall:
* Ekte liggende kledning. Hvert bord er skråstilt med synlig underkant (butt), har skjøter forskjøvet omtrent hver 3,66 m, og er skåret skrått langs spissbuene og gavlskråningen. Omtrent hvert sjette bord, og de fleste nederst mot bakken, bruker et slitt, gråere malingsmateriale.
* Hjørnebord, vannbord med dryppkant, synlig betongsokkel, frisbord og kassert takutstikk med soffitt, vindskier, takfotbord og lukkede endestykker i takutstikket.
* Brattere saltak (mønet er hevet fra 6,4 til omtrent 7,7 m) med 42 trinnvise shingelrader per side. Radene er delt opp i forskjøvne biter med liten variasjon i tykkelse og noen falmede flekker. Mønekappen er lagt som egne overlappende biter.
* Spissbuevinduer med ekte buet lysåpning gjennom veggtykkelsen, utvendige og innvendige foringer med buet overstykke, sluttstein, skrå vannbrett og forkant under, sprosser og Y-stavverk.
* Lameller i begge gavlene.
* Klokketårn: kledd sokkel, gesimsbånd, hjørnestolper, lameller nederst i hver side, spissbueåpninger, bjelkekrans, klokketårnstak med svungen nederkant og shingelrader, knopp og smijernskors. Klokkestolen er av mørkt tre med strevere.
* Inngangsparti med gavl: stolper med sokkel og kapitél, utskårne knekter, rekkverk med spiler mellom stolpe og vegg, panelt tak, gavlfelt med kors, eget tak med shingel og vindskier. Stolpene står 0,45 til 0,52 m ut fra veggen.
* Flat hellelegging foran døren (maks 0,035 m høy) med kant av liggende tegl, innenfor 0,6 m fra veggen.
* Dobbel plankedør med fuger og bakplate, kraftig foring med gesims over, terskel og innvendige løyer.
* Oppslagstavle med lite tak og gulnede ark.

Inventar og beslag (Fixtures):
* Bronseklokke med hulrom inni, kleppert, eikeåk, aksel med lagre, tauhjul og tau.
* Lange smijernsbeslag med spydspiss og stabelhengsler, ringhåndtak og nøkkelskilt.
* Taklampe av glasskrukketypen under inngangstaket. Glasset har et eget, sterkere glødemateriale (ChurchPorchGlow, emisjon 110), så lampen kaster en tydelig varm lysflekk på døren i skumring.
* K-formede takrenner (karniss) med lukket profil som slutter i flukt med taket, nedløp med svanehals, klammer, utkast og spruteblokker.
* Ovnsrør gjennom taket med inndekning, stormkrage og regnhatt.
* Strømmåler med rør opp til takfoten og en bryterboks på sideveggen.
* Seks skolelamper med glasskupler (én av dem er slokket), fire veggarmer og to messingstaker med lys og flammer.

Interiør:
* Pløyd plankegulv, løper i midtgangen, brystpanel med brystlist og fotlist.
* Hvelvet bordtak med lekter og flatt felt i midten, fire takstoler med veggstolper, konsoller, knekter, hanebjelker og korte konger.
* Åtte par benker på samme plass som i TB1, med formede vanger, skrå rygg, rygglist, salmebokhylle og salmebøker.
* Kor på podium med alterring (dreide spiler og knefall), alter med duk og antependium, dorsalfelt med kors, salmetavle med nummerkort, prekestol med skrå pult og bibel.
* Piano med krakk og vase med tørkede blomster, ovn med eget rør opp til taket, sakristi med to vegger, døråpning, skap, stol, bord, kirkebok og messehagel, og en pult med gjestebok ved døren.

Nye materialer i `materials_church.json` gjenbruker bare eksisterende teksturfiler: ChurchWhiteBoard, ChurchWornBoard, ChurchTrim, ChurchGlass (ravfarget og gjennomskinnelig med svak varm emisjon 1,8, slik at spissbuevinduene lyser varmt i skumring og om natten uten å se ut som lamper), ChurchPorchGlow (emisjon 110), ChurchLampGlow (emisjon 10), ChurchFlameGlow (emisjon 10), ChurchBronze, ChurchBrass, ChurchGalv, ChurchStone, ChurchBrick, ChurchShingleFaded og ChurchOldPaper.

## Trekanter

| Del | LOD0 | LOD1 | LOD2 | Budsjett LOD0 |
|---|---:|---:|---:|---:|
| Shell | 20 182 | 12 574 (62 %) | 4 954 (25 %) | 30 000 |
| Glass | 136 | 88 (65 %) | 40 (29 %) | 500 |
| Interior | 8 552 | 5 428 (63 %) | 2 430 (28 %) | 25 000 |
| Fixtures | 5 584 | 3 544 (63 %) | 1 736 (31 %) | 6 000 |

Alle flater har et materiale som finnes i `materials.json` eller i `materials_church.json`. Bygget kjører gjennom meshkits test for degenererte flater. Utvendig geometri lavere enn 2,3 m ligger høyst 0,6 m fra ytterveggen, og dette er kontrollert numerisk for alle tre LOD-ene.

## Bilder

I `/home/tom/scratch-bm/Signal47-townbuildings-tb2-20261027/church/`:
* `Church_tb2_lod0_street.png`, `_oblique.png`, `_door.png`, `_roof.png`, `_dusk.png` (TB2, LOD0)
* `Church_tb2_lod1_street.png`, `Church_tb2_lod2_street.png`
* `Church_tb1_lod0_*.png` til sammenligning
* `check/inside_nave.png` (skipet sett fra døren i skumring), `check/inside_belfry.png` (klokketårnet nært), `check/inside_night.png` (framsiden om natten med glødende vinduer), `check/inside_eave.png` (takfot, takrenne og ovnsrør). Disse er laget med et eget hjelpeskript som åpner den lagrede .blend-filen.

## Kjente svakheter

* Klokketårnet står delvis over gavlveggen, og under taket er sokkelen en tett boks. Det er ingen inndekning av beslag der tårnet møter taket, så skjøten er en ren skjæring.
* Trappen fra konseptet mangler med vilje. Gulvet står på bakkenivå og steg kan være høyst 0,2 m. Inngangen virker derfor lavere enn på konseptarket.
* Interiøret er mørkt i forhåndsvisningen fordi bare de selvlysende materialene gir lys. I spillet må integratoren sette egne punktlys ved lampene. Hvor sterk emisjon (10) skal være, må vurderes i Unreal.
* Nedløpene av galvanisert metall blir nesten svarte i skumring fordi de speiler en mørk himmel.
* Arkene på oppslagstavlen er blanke uten tekst. Vi har ikke noe trygt tekstatlas for kirken.
* Malingsslitasjen er laget med hele bord i et gråere materiale. Det finnes ikke ekte avflassing eller teksturmasker.
* LOD2 har ingen kledningsbord. Kjerneveggen bruker trestrukturen med planprojeksjon og kan på nært hold se pusset ut. På den avstanden LOD2 brukes, ser det greit ut i `Church_tb2_lod2_street.png`.
* Vinduenes glød ligger i glassmaterialet og er derfor alltid på. På dagtid ser glasset lyst ravfarget ut. Hvis vinduene skal kunne slukkes, må integratoren styre emisjonen.
* Lysflekken fra lampen i inngangen kommer bare fra et sterkt emissivt materiale i forhåndsvisningen. I Unreal bør den lages med et ekte punktlys eller spotlys.
* I dagslys gjør den blå himmelen malingen litt kald. Den er varmere enn i første runde, og i skumring ser den varmt off-white ut.
* Det er ingen egen kollisjon. Den eksisterende i spillet gjelder fortsatt.
