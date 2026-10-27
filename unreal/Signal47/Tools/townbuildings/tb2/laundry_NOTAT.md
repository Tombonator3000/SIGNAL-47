# Vaskeriet (Laundry), TB2

Kilde: `tb2/laundry.py` med `build(lod)`, nye materialer i `tb2/materials_laundry.json` (16 rader `M_Town_Laundry*`, bare eksisterende teksturer Stucco, Cloth, FloorTile og MetalPaint). Fotavtrykk, vegghøyde 3,4 m, front, dør og begge vindusåpninger er uendret fra `design.BUILDINGS['Laundry']`. Døren er fortsatt lukket. Skallet er skrevet på nytt; TB1 brukes bare for delte hjelpere i `geometry.py`.

## Hva som er lagt til

Skall:
* Ekte betongblokkforband (406 x 200 mm, løperforband) som relieff på alle fire yttervegger, med fuger der vegkjernen leses som mørtel. Malt krem foran og på sidene, falmet blått bånd opp til vinduskarmen, rå grå blokk bak.
* Avflasset maling som små uregelmessige flak (krem og rå blokk under blått, rå blokk på krem), blå maling som har krøpet over båndet, sprut og jordskitt nederst, regnstriper under beslaget.
* Høyere falsk front (4,05 m) med glatt pusset skiltbånd og store malte, falmede blå bokstaver LAUNDROMAT som i konseptarket, med avskallede partier. Sidevangene trapper ned til 3,75 m.
* Galvanisert murkrone med dryppkant, tjærepapptak med kilelister og mørke lappinger, takluke.
* Aluminiums butikkfront: karm, midtpost, glasslister, beslått vinduskarm med dryppnese, stålbjelke over åpningene. Aluminiumsdør med glass, sparkeplate, trekkhåndtak ute, skyvestang og dørlukker inne, terskel.
* Smal blikkmarkise med mørkegrønn front, atlasskiltet (celle 1) på fronten, fire strekkstag med strammeskruer, varm lampe over døren.
* Fordunstningskjøler på vinkeljernsstativ med lameller, kanal og takgjennomføring, vannrør. Vannvarmerpipe med kjeglehatt, to avløpsluftere, turbinventil.
* Tre tørketrommelhetter på høyre sidevegg og fem på bakveggen, med lo og sotflekker under. Tre takrenner med nedløp, bøyler, sko og sprutstein. Bakdør i stål med lampe.
* Gateutstyr innenfor 0,6 m fra veggen: grønn benk på støpejernsramme, søppeldunk, askebeger med sand, avisautomat, telefonautomat med kappe, lameller for avtrekksvifte, betongtrinn ved døren (3 cm høyt).

Glass: fire vindusruter og dørruten, 10 trekanter.

Interiør: grønn og krem rutete vinylflis, himling med T profiler, veggpanel i blekgrønt med list og krem over, 15 frontmatede vaskemaskiner mot vinduene (noen med tøy i trommelen), 12 toppmatede rygg mot rygg, åtte stablede tørketromler med avtrekk, sammenleggingsbord og veggbenk med tøystabler og kurv, sju oransje plaststoler under vinduene, tre trådtraller med henger, vekslemaskin, vaskemiddelautomat, hylle med flasker, oppslagstavle med lapper, klokke, prisoppslag, bod med grønn dør, søppelbøtter, dørmatte, liten TV i hjørnet, prislapper teipet innenfor glasset.

Fixtures: tolv lysrørsarmaturer med emitterende opalglass (`M_Town_LaundryTubeLens`), neonskilt OPEN i høyre vindu med kjeder og trafo, dørbeslag, strøminntak med måler, hovedbryter, mast, værhatt og dryppsløyfer, gassmåler med regulator og rør, utekran, veggarmaturer.

## Trekanter

| Del | LOD0 | LOD1 | LOD2 | Budsjett LOD0 |
|---|---:|---:|---:|---:|
| Shell | 9 935 | 5 463 (55 %) | 2 584 (26 %) | 30 000 |
| Glass | 10 | 10 | 10 | 500 |
| Interior | 21 552 | 13 908 (65 %) | 7 342 (34 %) | 25 000 |
| Fixtures | 2 572 | 1 644 (64 %) | 892 (35 %) | 6 000 |

LOD1 slår sammen blokkfugene på side og bakvegger til hele skift, halverer flakene og forenkler maskinene. LOD2 har flate malte vegger bortsett fra fronten, rette kasser for maskinene og et flatt neonfelt. Glass kan ikke reduseres meningsfullt.

## Bilder

Mappe: `/home/tom/scratch-bm/Signal47-townbuildings-tb2-20261027/laundry/`
* `Laundry_tb2_lod0_street.png`, `_oblique.png`, `_door.png`, `_roof.png`, `_dusk.png`
* `Laundry_tb2_lod1_street.png`, `Laundry_tb2_lod2_street.png`
* Sammenligning: `Laundry_tb1_lod0_*.png`

Se først `dusk` og `door`.

## Svakheter som gjenstår

* Stikkledningen fra strømmasten til stolpen er ikke modellert; bare dryppsløyfer til isolator. Tilkobling til stolpe må gjøres i nivået.
* Smussflekker er harde polygoner uten gradient; på avstand leser de som flekker, ikke som rennende smuss. Loflekkene under hettene er svake i dagslys.
* Blokkmalingen blir lys og kald i sterk sol i forhåndsvisningen; konseptet er varmere. Fargen kan justeres i `materials_laundry.json`.
* Interiørlyset i forhåndsvisningen kommer bare fra emitterende flater. I Unreal må integrator sette ekte lys i armaturene; `TubeLens` har styrke 10 for Cycles og må kanskje tones ned.
* Telefonautomaten og avisautomaten er enkle kasser uten trykk. Skiltet på markisen bruker eksisterende atlascelle (mørkegrønn), ikke den blå malte stilen.
* Overflater som ligger tett (1 til 2 mm) over veggen kan flimre på stor avstand i Unreal; LOD1 og LOD2 har færre slike lag.
* Ingen visuell godkjenning i Unreal er gjort. Bare Cycles forhåndsvisning.
