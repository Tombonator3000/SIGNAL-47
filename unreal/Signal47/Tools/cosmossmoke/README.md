# Tools/cosmossmoke: Cosmos-røyk over glørne i brente ringer (D45-28)

Toms Cosmos-pakke «VFX Smoke» (UE 5.0) ligger på skrivebordet i `SIGNAL 47/Content from Cosmos/Smoke`. Spillet bruker skorsteinsrøyken `NS_NS1_Smoke_Chimney`, liten, over hvert av de tre glørredene i ringen (`S47Hap::CosmosSmoke` i `S47Happenings.cpp`). Uten pakken, eller med `-S47CosmosSmokeOff`, tegnes røyken som tynne tråder av motorens egen volumtåke, slik støvvirvelen tegnes.

```sh
bash unreal/Signal47/Tools/cosmossmoke/import.sh
```

- Kopierer `Content/Rocket` med reflink til `unreal/Signal47/Content/Rocket`, med pakkens egne pakkestier (`/Game/Rocket/...`), slik Fab-pakkene Northwood og JustBStudios ligger. Ingenting døpes om og ingen referanser skrives om.
- Stopper før kopiering hvis Git ville sett filene (`unreal/.gitignore` har `Signal47/Content/Rocket/`), og hvis kopien ville gitt under 100 GB ledig.
- `import_smoke.py` laster alle assets, lagrer røyksystemet én gang i 5.8 og feiler hvis det trenger noe utenfor `/Game/Rocket` og motoren. Rapporten (`S47_COSMOS_WORK`, standard `~/scratch-bm/Signal47-cosmossmoke`) har noen fakta om systemet til å sette `S47Hap::CosmosSmokeScale` (nå 0,12).
- `Config/DefaultGame.ini` koker `/Game/Rocket/NS1_Smoke` alltid, siden spillet finner systemet ved navn.
- Lisens: Cosmos-filer kommer aldri i Git, og bildene åpnes eller mates aldri til AI-verktøy (se `Docs/Research/Assets/COSMOS_LEARTES_2026-11-10.md`). Skriptet leser ingen bilder.

Kontroll i spillet: F10, «Night events», «Scorched ring: coals, smoke and crackle» og «Scorched ring: four hours on». Loggen skriver `S47: happening embers in road: ... smoke from the Cosmos pack` eller `... as volumetric wisps`.
