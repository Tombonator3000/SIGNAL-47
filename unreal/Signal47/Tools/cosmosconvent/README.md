# Tools/cosmosconvent: kolonikirken San Isidro av kjøpte Cosmos-deler

**Status:** Under arbeid · **Gjelder:** Cosmos-meshene under `/Game/Licensed/CosmosConvent` · **Sist kontrollert:** 13. november 2026

Toms Cosmos-pakke «Colonial Convent» ligger som zip (`~/Desktop/SIGNAL 47/Content from Cosmos/Cosmos assets/ColonialConvent.zip`). Herfra kommer delene til en liten spansk misjonskirke som tar plassen til den hvite bordkirken i Caliche (Tom 13. november: «bytt til kolonikirka»). Verktøyet henter meshene inn i prosjektet og skriver plasseringslisten; bygging og regler i spillet ligger i C++ (`S47Convent`, `S47CalicheAccess`) og `Config/Props/church_convent.json`.

```sh
bash unreal/Signal47/Tools/cosmosconvent/import.sh
python3 unreal/Signal47/Tools/cosmosconvent/layout.py
```

- Løypa er den samme som i [Tools/cosmoschurch](../cosmoschurch/README.md): `import.sh` har stegene `extract` (uten motor), `editor` (to kjøringer av `UnrealEditor-Cmd` i et kasserbart 5.8-prosjekt, gjennom motorkøen) og `copy`. Zip-filen er utrygg: `extract.py` kjøres med `python3 -I`, pakker ut i en ny, tom mappe og kjører ingenting fra den. Bare pakkene de valgte meshene trenger tas ut (`selection.json`, omtrent 1,2 GB av 4,5).
- `migrate.py` flytter pakkeroten til `/Game/Licensed/CosmosConvent/Convent`, slår av Nanite, lagrer i 5.8-format og kontrollerer hver mesh. Resultatet kopieres til `Content/Licensed/CosmosConvent`, som Git ignorerer; skriptet stopper hvis Git ville sett filene eller hvis kopien ville gitt under 100 GB ledig.
- `verification.json` har bare navn, mål, pivot, trekanter og materialspor og kan ligge i Git.
- `layout.py` skriver `Config/Props/church_convent.json` (deler, egne bokser, gangvegger, lamper, dørmål og bakkeflate). Håndendringer i JSON-filen går tapt ved neste kjøring.
- Lisens: Cosmos-filer kommer aldri i Git, og Cosmos-modeller og -bilder brukes aldri som referanse for AI-verktøy. Delene er valgt etter navn, mål og trekanttall (pakkens egne FBX-filer målt med Blender, ingen bilder åpnet) og deretter sett på plass i spillet.

## Valgt og valgt bort

Pakkens egen kirke (`SM_ChurchWall*`, `SM_ChurchDome`, `SM_BellTower`, `SM_ChurchButtress*`) er 15 m bred og 18 til 34 m høy: en katedral, ikke hentet. Kirken er satt sammen av klosterets byggesett:

| Del | Mål (cm) | Brukt til |
|---|---|---|
| `SM_FlatWallTall_01`, `SM_WindowTall_01` | 300 x 20 x 450 | pussede vegger og vindusfelt, gjort 40 cm tykke |
| `SM_ChapelPosaArcade_01` | 450 x 53 x 450, bue 1,8 m bred | hoveddør, bue til sakristiet, bakdør; i skala 0,6 som klokkegavl |
| `SM_OutsideTrims_01` | 300 x 10 x 75 | gesims øverst på veggene |
| `SM_Bell`, `SM_Cross` | | klokke i gavlen, kors på toppen |
| `SM_PavingStoneTileBig_01` | 300 x 300 x 10 | gulv og gangen over forgården |
| `SM_Fence_01`, `SM_FenceColumn_01` | 450 x 40 x 252 | forgårdens lave mur og stolper (halv høyde) |
| `SM_Log` | 26 x 121 x 26 | takbjelker (vigas) med endene ut gjennom langveggene |
| `SM_Bench`, `SM_Desk`, `SM_AltarWallOrnament`, `SM_ChurchTableCross`, `SM_ChurchCandle`, `SM_ChurchChalice`, `SM_Podium`, `SM_Chair`, `SM_ChurchFrame01_1` til `03_1`, `SM_Chandelier`, `SM_ChurchCurtain`, `SM_Rope`, `SM_Barrel01_1` | | benker, alterbord, altertavle, kors, lys, lesepult, stoler, bilder, lysekrone, forheng, klokketau, tønner |
| `SM_BenchExterior`, `SM_Agave01_1`, `SM_Agave02_1` | | benker og agaver i forgården |
| `SM_WallTallDoor_01`, `SM_MainEntrance_01` | | ikke brukt: døren og porten er lukket og støpt inn i meshen |
| `SM_AngelStatue`, `SM_ChurchOrnamentTable`, `SM_InteriorFountain`, palmer, steiner, sekker | | ikke hentet |

Pakken har ingen skriftestol, skap eller taktegl; taket er en egen flat plate under veggkronen, og dørbladene er spillets egne (`SM_Access_Church_Door`).

## Plan

Kirkens egne meter: x øst fra frontveggens midtplan, z sør for aksen. Rommet er `Church` i `S47CalicheAccess` (rot verden x 8021, z 2095,5; gulvet i rotens høyde).

- Forgård x −9 til 0, z ±6: lav mur på tre sider, portåpning 2,6 m i vest, åpent ved begge kirkehjørnene.
- Skip x 0 til 12, z ±4,5 (innvendig 11,6 x 8,6 m, 4,1 m under taket): hoveddør i vestveggens bue, to benkerader med midtgang og smale sideganger, alterbord og frittstående altertavle 1 m foran østveggen.
- Sakristi x 12 til 15, z ±2,25: inn gjennom buen bak altertavlen, ut bakdøren i østveggen, 2 m fra kirkegårdsporten. Grusgangen på kirkegården starter ved bakdøren.
- Bakke: flaten `church` i `S47GroundEdits` (vei x 11,2 til 37,0, z 2087,9 til 2103,1, 2 cm under gulvet, 3 m skråning), skåret inn i bakkemeshen med `Tools/freeformterrain/site_ground.py`.
