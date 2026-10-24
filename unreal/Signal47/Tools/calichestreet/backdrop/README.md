# Original Caliche geology — CN3

Current original Blender source and FBX/GLB exports are **CN3_SOURCE_VERIFIED, PASS**. Native visual appearance is **UNVERIFIED**; the parent owns native import and camera acceptance. The first CN2 native capture was rejected for pale blank ramps, weak strata and similar close rims. CN3 addresses those source problems; this is not a claim that the concept has been matched in game.

`SM_Caliche_Mesas` is one editable mesh with the six stable material names `M_Caliche_Mesa_0` through `_5`, all used. Blender uses metres, Z up and origin zero. Anchor is Three `(8000,0,2060)`, runtime yaw zero, mapping `worldThree=(8000+bx,bz,2060-by)`. FBX retains the proven `axis_forward=-Z`, `axis_up=Y` route. The mesh is visual `NoCollision`; sky/clouds, native code, valley ground and vegetation are outside this source scope.

The five real belts combine a lower, farther western rim, taller eastern escarpments and lower distant ridges. Shorter talus carries the lower 22% of the relief, with angular drainage and five resistant rock risers above it. Recessive gray beds, reddish gray sandstone, pale caps and retained plateau coves provide structural variation. Rock faces retain planar shading; talus is smooth. Three original PBR sets provide 2048px base colors and 1024px DirectX normal/roughness maps, with all nine current image files packed byte-identically into the saved Blend. There are no external asset dependencies.

Actual source mesh: **98,730 triangles** against a 125,000 budget. Local bounds are `[-3055.107666,-1850,-4]` to `[2250,3737.951660,235.254791]` metres. Independent FBX bounds deviation is 0.608921 mm within the documented 1.0 mm tolerance; GLB bounds deviation is 0. The 1.0 mm FBX gate accounts for float32 axis/unit conversion over kilometre coordinates and remains below the parent's 1.2 mm native gate.

Actual independent checks pass exact FBX/GLB triangles, six ordered/used material slots, finite and noncollapsed UVs, strictly upward nondegenerate triangles, origin/scale, PBR texture bindings, and current packed Blend images. Minimum exported UV triangle area is **0.008777431769**. Conservative clearance for every elevated triangle is **225.363195m from OldRoad** and **237.708441m from the main road**, exceeding both 180 m exclusions. OldRoad follows the reproduced source 4 m midpoint/backward integration; the whole-triangle proof uses nearest vertex distance minus the longest projected edge, not only centroids. The town corridor, walking/driving floors and collision remain outside the geology.

`model_contract.json` and `geology_descriptor.json` describe actual geometry, belts and protection. `verification.json` is the fresh independent PASS report. `generation_status.json` records current artifact/source hashes and actual owned run results. The successful guarded build took 3.538 s and verification 2.022 s without contention; the earlier interrupted attempt is retained in the parent's run report.

Rebuild only after an explicit shared heavy-job slot:

```sh
cd unreal/Signal47/Tools/calichestreet/backdrop
python3 run_source.py
```

The runner sequentially uses the bundled NumPy/Pillow Python for `texture_geology.py`, then Blender background with two threads for `build_backdrop.py` and `verify_backdrop.py`. Each step uses `Scripts/unreal_guard.py` fresh preflight and an owned process monitor. No render or Unreal launch occurs. Pre-generation checkpoint scripts are historical design tools; do not run them as delivery verifiers after exports change.

History is preserved under `evidence/`: original seven mesas, visually rejected CN2, CN3 rejected collapsed UVs and CN3 UV2 rejected folds. UV projection now follows the actual dominant face normal and checks both quad diagonals. Retained cove offsets and continuous erosion weights remove the backslope folds; `profile_fold_fix.json` reproduces 779 old reversed intervals and zero corrected intervals. Neither the UV-area nor upward-face gate was weakened. Native camera review remains required before visual acceptance.
