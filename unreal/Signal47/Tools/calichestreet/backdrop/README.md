# Original Caliche geology — CN5

Current original Blender source, PBR, FBX and GLB exports are **CN5_SOURCE_VERIFIED, PASS**. Native visual appearance is **UNVERIFIED**; the parent owns native import and camera acceptance. CN4 resolved the bright repetitive benches but actual source-corresponded rays showed that OldRoad protection depressed a broad intact cap into a pale ramp. CN5 treats only that transition as eroded rubble and exposed rock. Export proof is not native visual acceptance.

`SM_Caliche_Mesas` remains one editable visual `NoCollision` mesh with all six stable material names `M_Caliche_Mesa_0` through `_5` used. Blender uses metres, Z up and origin zero. Anchor is Three `(8000,0,2060)`, runtime yaw zero, mapping `worldThree=(8000+bx,bz,2060-by)`. FBX retains `axis_forward=-Z`, `axis_up=Y`. Sky/clouds, native code, valley ground, vegetation and collision floors are outside this source scope.

The four actual belts retain CN4's narrow unequal benches, matched warmgray-brown rock/cap tones and continuous east escarpment. CN5 adds angular 31m drainage cuts and broader 90m cross-ridges only to the raised east OldRoad transition, with rubble/exposed rock islands in stable material slots 4/1/3 replacing intact cap. Intended appearance is broken matte stone/talus rather than one smooth pale plane. Actual local height changes are −13.470028 to +11.551635m; no new collision is created.

`cn5_source_proof.py` and `cn5_source_proof.json` establish that all 53,213 source XY points, belts, anchor, six material names, road-mask/path/distance function ASTs and all nine PNG bytes are unchanged from preserved CN4. Only 1,253 heights change, all 286.027240–434.731975m from OldRoad. The original 180m corridor and original protection function remain intact. Actual GLB rays at the previously identified pale cap targets `(2450,590)` and `(2500,625)` now hit exposed rock `Mesa_3`; `(2430,520)` now hits talus `Mesa_4`. These rays use the requested camera and source-backed eye height; final native eye telemetry was unavailable, so no exact raster or visual acceptance is claimed.

Actual source mesh: **103,550 triangles** against a 125,000 budget. Local bounds remain `[-3071.326904,-1850,-4]` to `[2250,3759.061523,234.650040]` metres. Fresh independent FBX/GLB/Blend verification passes exact triangles, six used material slots, pivots/scale, finite noncollapsed UVs, strictly upward nondegenerate faces, whole-triangle road clearances and packed current images. Minimum exported UV triangle area is **0.00288571786129**. Conservative raised-triangle clearance remains **224.555255m from OldRoad** and **237.934143m from the main road**, both above 180m. FBX bounds error is 0.612259mm within the unchanged 1mm tolerance; GLB bounds error is 0.

Three original PBR sets contain 2048px base colors and 1024px DirectX normal/roughness maps. All nine actual PNGs are packed byte-identically in `Caliche_Mesas.blend`; CN5 changes no image bytes or external dependencies. `model_contract.json`/`geology_descriptor.json` describe actual exports; `verification.json` is the current independent PASS report. `generation_status.json` records actual source/artifact/guard hashes. The full sequential owned run was uncontended: textures 5.555s, build 4.541s, verify 2.522s.

Rebuild only after an explicit shared heavy-job slot:

```sh
cd unreal/Signal47/Tools/calichestreet/backdrop
python3 run_source.py
```

The runner uses bundled NumPy/Pillow Python for `texture_geology.py`, then Blender background with two threads for `build_backdrop.py` and `verify_backdrop.py`, one job at a time. Every step has fresh Unreal-guard preflight and an owned contention monitor. No render, Unreal import or package runs here. The light exported proof can be rerun with the bundled Python and `cn5_source_proof.py`. Historical pre-generation checkpoint scripts are not current delivery verifiers.

Prior local evidence remains intact. Before CN5 mutation, the actual CN4 Blend/FBX/GLB, nine PNGs, source, contracts, PASS/guards and day/dusk camera frames were preserved with hashes under `/home/tom/scratch-bm/Signal47-caliche-nature/evidence/CN4-rejected-oldroad-cap-20261024T084318Z`. Earlier CN2/CN3 visual rejection and UV/fold proofs remain under `evidence/`. No protection, upward-face or noncollapsed-UV gate was weakened. DCC slot is released; fresh native CN5 camera review is still required.
