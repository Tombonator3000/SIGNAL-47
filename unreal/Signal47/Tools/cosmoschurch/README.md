# Tools/cosmoschurch: kjøpte Cosmos-gravminner til kirkegården

**Status:** Under arbeid · **Gjelder:** Cosmos-meshene under `/Game/Licensed/CosmosChurch` · **Sist kontrollert:** 13. november 2026

Toms Cosmos-pakke «Demonic Village» ligger som zip på skrivebordet (`DemonicVillage.zip`). Herfra kommer bare de enkle gravminnene og en benk til San Isidro-kirkegården bak kirken i Caliche. Verktøyet henter meshene inn i prosjektet og skriver plasseringslisten; lasting og plassering i spillet ligger i C++ (`S47TownSites`, tomten `cemetery`) og `Config/Props/church_cosmos.json`.

```sh
bash unreal/Signal47/Tools/cosmoschurch/import.sh
python3 unreal/Signal47/Tools/cosmoschurch/layout.py
```

- `import.sh` har tre steg som også kan kjøres hver for seg: `extract` (uten motor), `editor` (det tunge steget, to kjøringer av `UnrealEditor-Cmd`) og `copy`. Når flere bruker motoren samtidig, pakkes `editor`-steget inn i motorkøen (`q.py`). Løypa er den samme som i [Tools/cosmosplayground](../cosmosplayground/README.md).
- `extract.py` (kjøres med `python3 -I`) tar bare ut pakkene de valgte meshene trenger (`selection.json`). Zip-filen er utrygg: den pakkes ut i en ny, tom mappe, hvert navn kontrolleres mot utmappen, og ingenting i den kjøres.
- `migrate.py` kjøres i et kasserbart UE 5.8-prosjekt (`proj58` i arbeidsmappen): pakkeroten flyttes til `/Game/Licensed/CosmosChurch/Village` uten redirectorer, Nanite slås av, basismaterialet kompileres på nytt og alt lagres i 5.8-format. Deretter kontrolleres hver valgt mesh i en ny prosess. Skriptet nekter å kjøre i `Signal47.uproject`.
- Resultatet kopieres til `Content/Licensed/CosmosChurch`, som Git ignorerer. Skriptet stopper hvis Git ville sett filene, og hvis kopien ville gitt under 100 GB ledig.
- `verification.json` har bare navn, mål i cm, pivotens plass, antall trekanter og materialspor. Ingen lisensierte filer, og den kan ligge i Git.
- `layout.py` skriver `Config/Props/church_cosmos.json` fra `kinds.json` (hvilke gravminner, hvor mange av hver og hvilken vei de vender) med fast frø, så listen blir lik hver gang. Håndendringer i JSON-filen går tapt ved neste kjøring; endre `kinds.json` eller `layout.py`.
- Miljø: `S47_COSMOS_ZIPS` (mappen med zip-filen, standard `~/Desktop`), `S47_COSMOS_WORK` (arbeidsmappe, standard `~/scratch-bm/cosmos-church`; må tømmes før ny kjøring).
- Lisens: Cosmos-filer kommer aldri i Git, og Cosmos-modeller og -bilder brukes aldri som referanse for AI-verktøy (se [Cosmos-oversikten](../../../../Docs/Research/Assets/README.md)). Skriptene åpner ingen bilder. Pakkens navn sier ingenting (`SM_Cemetery01` til `10`), så meshene er valgt etter mål og trekanttall og deretter kontrollert på plass i spillet.

## Valgt og valgt bort

| Mesh | Mål (cm) | Trekanter | Hva | Valg |
|---|---|---:|---|---|
| `SM_Cemetery08` | 90 x 65 x 93 | 8 496 | lite kors på trappesokkel | brukt, flest |
| `SM_Cemetery09` | 101 x 64 x 45 | 1 008 | lav steinblokk | brukt |
| `SM_Cemetery10` | 74 x 15 x 111 | 45 120, ingen LOD | ringkors i stein | brukt, høyst fem |
| `SM_Cemetery04` | 74 x 26 x 176 | 65 110 | stående gravstein med rund topp | brukt, høyst fem |
| `SM_Cemetery01` | 106 x 250 x 70 | 6 936 | liggende gravlokk | brukt, høyst tre |
| `SM_bench` (Props) | 31 x 200 x 45 | 1 284 | enkel trebenk | brukt, to |
| `SM_Cemetery02` | 69 x 68 x 215 | 7 440 | søyle med mørk spiss | sett på plass og tatt ut: for gotisk |
| `SM_Cemetery03`, `05`, `06` | opptil 102 x 87 x 228 | 17 116 til 97 110 | høye monumenter | ikke hentet: for store og tunge for en liten bykirkegård |
| `SM_Cemetery07` | 189 x 410 x 395 | 87 308 | gravkammer eller portal | ikke hentet |
| `SM_fence01` til `03` (ModularKit) | 353 x 22 x 151 | 292 | tregjerde, 1,5 m | målt, ikke brukt: gjerdet er lavt rørgjerde av egne bokser |
| `Church`-gruppen (16 mesher) | | | byggedeler til steinkirke, dør med blod, statue | ikke hentet: ingen benker, alter eller lysestaker blant dem; kirken i Caliche har eget interiør og låst dør |
| `Ritual`, `Hangman`, `Bones`, `Cage`, `Decals` | | | | ikke hentet: okkult og skrekk |

## Plassering

Tomten `cemetery` i `S47TownSites` (vei x 49, z 2095,5) er en fast del av Caliche i Freeform, ikke en midlertidig boks: 22 x 21 m bak kirkens bakvegg, med lavt rørgjerde av egne bokser, port mot kirken, åpning i østsiden mot åpent land og en åpning i nordsiden. Grusgangen er samme grusdekal som tomtene langs gaten (`M_Lived_LotGravel`) og følger bakken; den går fra kirkens sørøstre hjørne langs bakveggen, inn porten og tvers gjennom. Kirken har ingen bakdør i modellen; gangen går derfor rundt hjørnet.

`at` i `church_cosmos.json` er bunnens midtpunkt for tingens dreide omriss i tomtas meter, `yaw` er Unreal-grader, `collide` gir en gangblokk, og `box` er størrelsen på den grå boksen som står der når asseten mangler. Hver ting står på bakken slik `S47RoadTerrain::SurfaceY` gir den. Bakken stiger 0,6 m fra nord til sør over tomten og er ikke planert: ingen flate er meldt inn til `S47GroundEdits`, og bakkemeshen er ikke bakt på nytt. Planter inne på tomten vokser ikke (`S47GroundEdits::SetBare`: bar bakke uten at høyden endres) eller skjules (`HidePlants`). Selvtesten har en linje for fila og en for gjerdets tre åpninger.
