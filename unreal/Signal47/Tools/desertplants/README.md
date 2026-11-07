# Ørkenplantene: cholla, yucca og mesquite

**Status:** Kilde bygd og kontrollert i Blender, FBX lest tilbake i Blender. `import_assets.py` og spillkoden er skrevet, men ikke kjørt, kompilert eller sett i spillet. **Gjelder:** Egne plantemodeller til flora-systemet (`S47Flora`), Freeform først. **Sist kontrollert:** 7. november 2026.

Originale prosedyrale `bpy`-modeller med egne prosedyrale atlas. Ingen nedlastede modeller, foto eller uttrekk. Binærfiler ligger privat i `~/Documents/ChatGPT/Signal 47/modeller/desertplants-2026-11-07/` (`fbx/`, `blend/`, `textures/`, `renders/`, `milepæler/`, `model_contract.json`, `materials.json`, `reimport.json`), aldri i Git.

## Filer

| Fil | Innhold |
|---|---|
| `spec.py` | Mål, trekantbudsjett, navn og atlasregioner, fastsatt før bygging. |
| `dpkit.py` | Mesh-klosser uten `bpy`: rør, kort og perler med UV, vind og farge per hjørne. |
| `cholla.py`, `yucca.py`, `mesquite.py` | Selve plantene. `build(variant, lod)`. Fast frø, så bygg kan gjentas. |
| `textures.py` | Atlasene, `T_DP_<Art>_BC.png` og `_R.png`. Trenger numpy og Pillow. |
| `run_blender.py` | Moduser `clay`, `mat`, `wire`, `lods`, `wind`, `export`, `verify`. |
| `heavy.py` | Kjører `run_blender.py` gjennom `HeavyReservation` og `unreal_guard.run_owned`. Stopper aldri andre. |
| `check.py` | Rask kontroll uten Blender: trekanter, mål og vindtall mot `spec.py`. |
| `import_assets.py` | Bygger materialinstansene og importerer de åtte meshene til `/Game/DesertPlants`. Kontrollerer hver LOD mot `reimport.json`. |

## Kjøring

```sh
python3 check.py
~/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/bin/python3 textures.py "<utrot>/textures"
python3 ~/.claude/skills/signal47-nattskift/scripts/q.py python3 heavy.py mat all line,close,macro,far
python3 ~/.claude/skills/signal47-nattskift/scripts/q.py python3 heavy.py export all + verify all
```

Kjør alltid Blender gjennom `heavy.py`. En løs Blender-prosess stopper andres vernede kjøringer.

## Kontrakt mot flora-materialene

Plantene er laget for `M_WebFlora_Card` (cholla og mesquite, masked) og `M_WebFlora_Succulent` (yucca, opaque), altså FLORA_WPO i `Tools/webexport/import_web.py`.

- **Vind ligger i UV-kanal 1, ikke i vertex-farge.** `UVChannel_2.x` er `aSway` fra `flora.ts`: 0 ved roten, 1 i en grasstrå-topp. `UVChannel_2.y` er 0, og `Parts` må være 0 på materialet, ellers bretter skyggeren bort deler av planten.
- **Ingenting løsner i vinden.** Vindtallet er kontinuerlig: et barn starter med forelderens tall i festepunktet, og et alpha-kort har rothjørnene på greinens tall. WPO-en flytter hjørner rent etter tallet. Prøvd i Blender med `wind`-modus på 0,5 m, langt over spillets 7 cm, ikke i Unreal.
- **Rot i origo:** roten står 5 til 8 cm under bakken, fordi `FLORA_WPO` krymper kopien mot origo, og spillet setter kopien 3,5 cm ned ganger skala.
- **Vertex-farge** er en lineær tone som ganges med atlaset (`UseVertexColor` 1), alpha alltid 1. Eksporteres med `colors_type='LINEAR'`, så tallene skal nå skyggeren uendret. Ikke kontrollert i Unreal.
- **Normaler:** egne normaler, opp og ut på blad og kort. Materialet gir begge sider samme lys via `TwoSidedSign`; `run_blender.py` etterligner det i forhåndsvisningen.
- **Normalkart finnes ikke:** flora-mastrene leser ikke normalkart, så detaljene er malt i fargen.
- **Klipp:** forhåndsvisningen bruker 0,42, som `flora.ts`.
- **Mip:** tynne kvister og pigger mister dekning i mipkart. RGB under gjennomsiktige piksler er regionens snittfarge, så kantene ikke blir mørke. LOD2 på mesquite bruker egne `far_*`-regioner med tykkere streker.

## Import til Unreal

Fra `unreal/Signal47/`, i en kopi der `Content/Web/Base` finnes (mastrene er generert og ligger ikke i Git):

```sh
python3 Scripts/unreal_guard.py require-idle && S47_DESERT_REPORT="$HOME/scratch-bm/Signal47-desertplants/import-$(date +%H%M%S).json" "$HOME/.local/opt/UnrealEngine-5.8.4/Engine/Binaries/Linux/UnrealEditor-Cmd" "$PWD/Signal47.uproject" -run=pythonscript -script="$PWD/Tools/desertplants/import_assets.py" -unattended -nullrhi -nosplash -NoSound '-ini:Engine:[ConsoleVariables]:Interchange.FeatureFlags.Import.FBX=0'
```

Resultat: `Content/DesertPlants/` (`SM_DP_*`, `Materials/MI_DP_*`, `Textures/T_DP_*`, LFS). Rapporten har trekanter, UV-kanaler, vindtall og materialer per LOD, og loggen har én `DESERTPLANTS`-linje per mesh. Samme løype som `Tools/farmbuildings/import_assets.py` (legacy FBX, `MATCH_UP_FORWARD_AXES`, `import_rotation` yaw +90, `import_lod` for `_LOD1` og `_LOD2`), med disse forskjellene:

- `generate_lightmap_u_vs = False` på importen og på hver LOD, ellers overskrives UV-kanal 1 og vinden blir feil. FBX-importen snur V, så `UVChannel_2.y` er 1 i Unreal, ikke 0. Det er ufarlig så lenge `Parts` er 0.
- `normal_import_method = FBXNIM_IMPORT_NORMALS`, `vertex_color_import_option = REPLACE`.
- `use_full_precision_u_vs = True` i kildemodellene, så vindtallene holder.
- Ingen kollisjon, ingen Nanite. Skjermstørrelser som `calichenature`: `[1.0, 0.12, 0.025]`.
- Nye `MaterialInstance` av mastrene over: `UseMap` 1, `Map` = BC, `UseRoughnessMap` 1, `RoughnessMap` = R, `UseVertexColor` 1, `UseInstanceColor` 1, `Parts` 0, `Shade` 1. `Back` 1 på mesquite, 0 på cholla. Klipp 0,42 på de maskerte.

Mål etter import: `SM_DP_Cholla_A` skal være 189 cm over null og `SM_DP_Cholla_B` 162 cm (bounds går 6 cm under null), og `UVChannel_2` skal fortsatt finnes som kanal 1. Alle tall står i `reimport.json`.

## I spillet

Bare i Freeform (`FS47Flora::EnsureDesert`, `DESERT_PARTS`, `NATURAL` i `S47Flora.cpp`). Hver art er én `ES47FloraKind`, hver variant ett instansert mesh, og alle åtte må finnes, ellers blir de gamle sukkulentene stående. Områdemiksen står i `S47GroundCover.cpp` (`DesertMix`). Tallene er anslag fra modellene, ikke målt i spillet.

## Kilder og lisens

- Modeller og atlas: egne, ingen fremmed lisens.
- Referanser, bare sett på, ikke brukt i filene: `nature-referanser-2026-11-07/KILDER.md`. Bilde 07 er CC BY 2.0 (Judy Gallagher), 08 er CC BY 4.0 (Larry D. Moore), 03 og 10 er CC0 (Patrick Alexander).
- Vurdert og valgt bort, Poly Haven (CC0): `wild_rooibos_bush` (76 521 trekanter, tette barnålsdusker), `searsia_burchellii` (1 427 042 trekanter, tette mørke løvklumper, nærmest i greinform) og `searsia_lucida` (841 648 trekanter, tett busk). Ingen ligner en sørvestlig mesquite i løv, og alle måtte bygges om for å nå budsjettet.
