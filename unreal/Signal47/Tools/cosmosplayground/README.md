# Tools/cosmosplayground: kjøpte Cosmos-lekeapparater

**Status:** Under arbeid · **Gjelder:** Cosmos-meshene under `/Game/Licensed/CosmosPlayground` · **Sist kontrollert:** 12. november 2026

Toms Cosmos-pakker «Post Apocalyptic Playground» Vol 1 og Vol 3 ligger som zip på skrivebordet (`PostApocPlaygroundVol1.zip`, `PostApocPlaygroundVol3.zip`). Herfra kommer klatrestativ, sklier, lekesett, karusell, basketstativ, benk, lekehus og fjærdyr. Verktøyet henter bare meshene inn i prosjektet; lasting og plassering i spillet ligger i C++ og `Config`, ikke her.

```sh
bash unreal/Signal47/Tools/cosmosplayground/import.sh
```

- `import.sh` har tre steg som også kan kjøres hver for seg: `extract` (uten motor), `editor` (det tunge steget, to kjøringer av `UnrealEditor-Cmd`) og `copy`. Uten argument kjøres alle. Når flere bruker motoren samtidig, pakkes `editor`-steget inn i motorkøen (`q.py`).
- `extract.py` (kjøres med `python3 -I`) tar bare ut pakkene de valgte meshene trenger (`selection.json`: mesh, materialer, mastermaterialer og teksturer), funnet fra `/Game/<pakke>/...`-stiene i hver pakkes navnetabell. Zip-filene er utrygge: hver pakke pakkes ut i sin egen nye, tomme mappe, hvert navn kontrolleres mot utmappen, og ingenting i dem kjøres. Rundt 0,14 GB av Vol 1 og 0,18 GB av Vol 3.
- `migrate.py` kjøres i et kasserbart UE 5.8-prosjekt (`proj58` i arbeidsmappen), som `Tools/cosmosshop`: pakkerøttene flyttes til `/Game/Licensed/CosmosPlayground/Vol1` og `.../Vol3` uten redirectorer, Nanite slås av, basismaterialene kompileres på nytt og alt lagres i 5.8-format. Deretter kontrolleres hver valgt mesh i en ny prosess: den laster, hvert spor har et materiale under `CosmosPlayground`, alle pakker den når finnes (ingen manglende tekstur), og målene i cm. Skriptet nekter å kjøre i `Signal47.uproject`.
- Resultatet kopieres til `Content/Licensed/CosmosPlayground`, som Git ignorerer (`unreal/.gitignore`), og `DefaultGame.ini` koker hele `/Game/Licensed`. Skriptet stopper hvis Git ville sett filene og hvis kopien ville gitt under 100 GB ledig.
- `verification.json` har bare navn, mål i cm (min og maks per akse fra pivoten), pivotens plass, lengste akse, antall materialspor og Nanite-tilstand. Ingen lisensierte filer, og den kan ligge i Git.
- Miljø: `S47_COSMOS_ZIPS` (mappen med zip-filene, standard `~/Desktop`), `S47_COSMOS_WORK` (arbeidsmappe, standard `~/scratch-bm/cosmos-playground`; må tømmes før ny kjøring).
- Lisens: Cosmos-filer kommer aldri i Git, og Cosmos-modeller og -bilder brukes aldri som referanse for AI-verktøy (se [Cosmos-oversikten](../../../../Docs/Research/Assets/README.md)). Skriptene åpner ingen bilder, og meshene er valgt etter navn og mål alene.

## Valgt

| Pakke | Mesh |
|---|---|
| Vol 1 | `SM_JungleGym_01a`, `SM_JungleGym_03b`, `SM_Playground_Set_02b`, `SM_Playground_Set_04b`, `SM_Slide_03b` |
| Vol 3 | `Basketball_Goal_01a`, `SM_Bench_01a`, `SM_House_01a`, `SM_MerryGoRound_01a`, `SM_Slide_02a`, `SM_Spring_Rider_Chicken_02a`, `SM_Spring_Rider_Pig_01a` |

Hver ting er én mesh. Ingen er delt i ramme, seter og kjettinger, så ingenting skal settes sammen.

## Valgt bort

| Mesh | Hvorfor |
|---|---|
| `SM_ZBackdrop_01` (begge pakker) | Bakteppe til pakkenes visningskart, ikke et lekeapparat. |
| `SM_Sign_01a` (Vol 3) | Skilt, ikke et lekeapparat; teksten på det er ukjent og kan bryte med 1986. |
| Kartene under `Maps` | Pakkenes egne visningskart trengs ikke. |
| Huskestativ | Finnes ikke som egen mesh i noen av de to pakkene. |

## Plassering

`Config/Props/school_cosmos.json` sier hvor hver ting står ved skolen i Caliche, i tomtas meter (x øst, z sør fra skolens punkt, vei x 48, z 2245): `at` er bunnens midtpunkt for tingens dreide omriss, så pakkenes skjeve pivoter ikke spiller noen rolle. `yaw` er Unreal-grader oppå tomtas, `collide` gir en gangblokk, og `replaces` navngir gruppen av egne bokser som skjules når tingen finnes (`swings`, `merry`, `seesaw`, `monkey`, `benches`). Fila leses når tomtene bygges (`S47TownSites`, `FDress`), så plasseringene kan justeres uten nytt bygg. `S47Props` hopper over fila.

Mangler en asset, står boksene som før, og loggen sier én gang hvor mange som mangler. Pakkene har ingen huske og ingen vippe: huskene er fortsatt egne bokser. Selvtesten har en linje for fila (bestått med og uten assets). Tingene står på bakken slik `S47RoadTerrain::SurfaceY` gir den, altså på lekeplassens flate (`S47GroundEdits`).
