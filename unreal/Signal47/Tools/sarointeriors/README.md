# SARO gang og kontrollrom

**Status:** Under arbeid. **Gjelder:** Egne native overflater og dekor, UE5.8.4/Linux. **Sist kontrollert:** 30. oktober 2026.

Gangen kom først, deretter kontrollrommet, med godkjente Dales kontor som visuell referanse. Redigerbare kilder ligger i `unreal/ArtSource/SaroInteriors`. Begge finishmeshes deler originalens verdensnullpunkt og bruker eksisterende egne Office-teksturer i nye materialer. Godkjente Office-assets blir bare lest.

Kjør `corridor_finishes.py` eller `control_finishes.py` i Blender med `-b -t 2 --python`. Importer med Unreal Editors `-run=pythonscript -script=Tools/sarointeriors/import_assets.py`, `-nullrhi` og `-ExecCmds="Interchange.FeatureFlags.Import.FBX 0"`. Standard er gang; `-S47SaroImportControl` velger kontrollrom. Importen går til `/Game/Signal47/Art/SaroInteriors` med prosjektets målte FBX/PBR-oppskrift.

`FS47SaroRooms` legger overflatene inn som NoCollision. Gangens eksisterende armaturer beholder strøm/flimring; kontrollrommets vinduslys følger døgnkurven. Svake Low-reflekser slås av når Lumen gir indirekte lys. De opprinnelige gangsonene, dørene og interaksjonsflatene brukes videre. De små arbeidsgruppene ligger separat i `Config/Props/control-detail.json`.

`-S47SaroRoomsOff` slår av nye finish-/lyslag og gangtint. `-S47SaroControlRoomOff` gjelder kontrollrommets finish-/lyslag alene. Dekorfilen følger vanlig Props-innlasting. Faktiske førbilder tas før dekorfilen aktiveres.

Kameraene i `S47ProductionViews` bruker suite `interiors`: `office-corridor-west-east`, `office-passing-bay`, `interior-corridor-east-west`, `interior-control-wide`, `interior-control-rack` og `interior-control-console`.
