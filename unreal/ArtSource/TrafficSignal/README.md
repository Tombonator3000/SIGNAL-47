# Caliche-krysset: hengende gult blinksignal (ENV-03), original Blender-kilde

Erstatter den enkle boksen fra `web/src/world/Town.ts` `crossing()` med et detaljert
firveis blinksignal fra 1986: støpt seksjon med hengslede dører, 12-tommers ravfargede
Fresnel-linser, tunnelvisirer med mørk innside, spennklemme med U-bolter, gaffelfeste,
balansejustering, kabelinngang og dryppsløyfe, samt fast spennwire med forkabel,
stolpebånd, øyebolter, dead-end-grep, kabel ned vestre stolpe, blinkskap og rør.
Værslitasje (falmet emalje, støv, flekkvis avskalling, galvanisert bloom og rustprikker)
ligger i originale prosedyrale PBR-kart. Ingen tredjepartsmodeller eller -bilder.

- `build_signal.py` (Blender 5.2, `-b --factory-startup -t 2`) skriver `.blend`, seks FBX
  (hode og spenn, tre håndlagde LOD), kart, `materials.json` og `model_contract.json`.
- `verify_source.py` kontrollerer den lagrede fila på nytt; `preview.py` lager tre
  Blender-forhåndsvisninger (ikke spillbilder; stolper og vei er bare kontekst).
- Origo for begge mesher er festepunktet der klemmen griper spennwiren, byens lokale
  `(0.5, 11.459, 2000)`. Three `(x,y,z)` → Blender `(x,-z,y)` → UE `(x,z,y)*100`.

Native import: `Signal47/Tools/townsignal/import_assets.py` til `/Game/TownSignal`.
Den gamle boksen, visirene, hengestangen og wirene kuttes av
`Signal47/Tools/townintegration/prepare_trims.py` (merke `signal`, 96+12 trekanter);
trestolpene beholdes. Runtime: `S47TownSignal.cpp`, installert atomisk sammen med
bykuttet i `S47TownBuildings.cpp`.
