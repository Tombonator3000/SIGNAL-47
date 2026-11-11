# Tools/cosmosshop: kjøpte Cosmos-rekvisitter i bensinstasjonens butikk (GAS-STATION-LIFT-01)

**Status:** Under arbeid · **Gjelder:** Cosmos-rekvisittene i butikken på bensinstasjonen i Caliche (`S47StationDressing`) · **Sist kontrollert:** 11. november 2026

Toms Cosmos-pakker «Post Apocalyptic Coffee Shop» (UE 5.3) og «Abandoned Hospital» (UE 4.22) ligger som zip på skrivebordet. Butikken får herfra kasseapparat, kaffemaskin, kjøleskap, søppelbøtte, pappesker, ventiler, sikringsskap og skilt, så rommet blir tettere og mer levd i. Ingen ny spillmekanikk: tingene bare står der.

```sh
bash unreal/Signal47/Tools/cosmosshop/import.sh
```

- `extract.py` (kjøres med `python3 -I`) tar bare ut pakkene de valgte meshene trenger (`selection.json`: mesh, materialer, mastermaterialer, funksjoner og teksturer), funnet fra `/Game/<pakke>/...`-stiene i hver pakkes navnetabell. Zip-filene er utrygge: hver pakke pakkes ut i sin egen nye, tomme mappe, hvert navn kontrolleres mot utmappen, og ingenting i dem kjøres. Rundt 0,4 GB av kaffebarpakken og 0,07 GB av sykehuspakken.
- `migrate.py` kjøres i et kasserbart UE 5.8-prosjekt (`proj58` i arbeidsmappen), som `Tools/militarybarriers`: pakkerøttene flyttes til `/Game/Licensed/CosmosShop/CoffeeShop` og `.../Hospital` uten redirectorer, Nanite slås av, basismaterialene kompileres på nytt og alt lagres i 5.8-format. Deretter kontrolleres hver valgt mesh i en ny prosess: den laster, hvert spor har et materiale under `CosmosShop`, alle pakker den når finnes (ingen manglende tekstur), og målene i cm. Skriptet nekter å kjøre i `Signal47.uproject`.
- Resultatet kopieres til `Content/Licensed/CosmosShop`, som Git ignorerer (`unreal/.gitignore`), og `DefaultGame.ini` koker hele `/Game/Licensed`. Skriptet stopper hvis Git ville sett filene og hvis kopien ville gitt under 100 GB ledig.
- `verification.json` har bare navn og mål (ingen lisensierte filer) og kan ligge i Git.
- Miljø: `S47_COSMOS_ZIPS` (mappen med zip-filene, standard `~/Desktop`), `S47_COSMOS_WORK` (arbeidsmappe, standard `~/scratch-bm/cosmos-shop`; må tømmes før ny kjøring).
- Lisens: Cosmos-filer kommer aldri i Git, og Cosmos-modeller og -bilder brukes aldri som referanse for AI-verktøy (se [Cosmos-oversikten](../../../../Docs/Research/Assets/README.md)). Skriptene åpner ingen bilder. Å se på skjermbilder fra spillet er vanlig bruk.

## Plassering

`Config/Props/gas_cosmos.json` sier hvor hver ting står, i byens meter slik butikkens `interior_layout.json` har dem: `at` er bunnens midtpunkt for tingens dreide og skalerte omriss (på disken, gulvet eller mot veggen), så pakkenes hjørnepivoter ikke spiller noen rolle. `yaw`, `pitch`, `roll` er Unreal-grader i modellens ramme (yaw 0: meshens +X mot byens +x; yaw 90: mot +z, bakveggen). `anchor_of` plasserer etter en annen tings omriss (skuffen med kasseapparatet), `pivot` bruker meshens egen pivot, `collide` gir en gangblokk. Fila leses når stasjonen bygges, så plasseringene kan justeres uten nytt bygg (ny spillstart holder). `S47Props` hopper over fila.

`S47StationDressing` henger tingene under stasjonens egen mesh (`AS47Props::PlacedMesh("gasStation")`), bare i Freeform. Mangler en asset, hoppes den over og loggen sier én gang `S47: station dressing: N of M Cosmos pieces not here`; uten pakken er butikken som før. Selvtesten skriver `station dressing`-linjer: fila leses, hver ting har lisensiert sti, fornuftig skala og står i butikken utenom kortflekken på disken og ekspeditørens plass, og hvor mange assets som finnes (bestått med og uten).

## Valgt bort

| Mesh | Hvorfor |
|---|---|
| SM_Fire_Extinguisher | Materialet (`MI_Fire_Extinguisher`) mangler i zip-fila; meshen har to tomme spor. Blender-modellen har allerede et apparat. |
| SM_VendingMachine01 | 1,33 m bred: ingen ledig vegg i butikken (plassen mellom fryseboksen og kjøledørene er 1,05 m). |
| SM_CashRegister | Et flatt diskstykke (71 x 65 x 18 cm), ikke et kasseapparat; `SM_Cash_Register_Body` og `_Drawer` brukes. |
| Kopper, servietter, kaffeposer, sigarettpakke, `SM_Tissue_Box`, `SM_Coffee_Box` | Kaffebarens egne logoer eller søppel; disken og kaffebenken er fulle fra før. |
| SM_Notice_Board, SM_Stool, SM_Safe, SM_Secchio_con_manico | Finnes i Blender-modellen (korktavle, stol, bøtte) eller for store. |
| SM_Interruttore_Luce | Italiensk bryterplate, ikke amerikansk 1986. |
| VintageRoom-pakken (UE 4.19, 4,7 GB) | Ikke pakket ut: stuerekvisitter, lite som hører hjemme i en bensinstasjon. |
