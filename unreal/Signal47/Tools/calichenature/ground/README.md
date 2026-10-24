# Caliche nature ground

Original terrain dressing for the user reference in
`unreal/Concepts/Caliche-Nature-2026-10-24/`. No reference pixels, licensed textures
or downloaded geometry are used. The Blender source and the procedural material
sources are editable. Cloud generation belongs to the separate sky work.

## Coordinates and protection

Blender uses metres and Z up. Its `(x,y,z)` maps to world Three metres
`(8000+x,z,2060-y)`. Native import is independently checked as `(x,-y,z)*100`.
All tile object origins stay at zero; the runtime prop anchor supplies the offset.

`terrain_contract.py` reproduces the unchanged native variable terrain axes,
float32 grid samples and triangle interpolation. The old road's complete local
curve is used for placement exclusion; its historical height-query shortcut is
retained only where matching the original floor requires it. Terrain overlays
are lifted 16 mm and have no collision. Existing asphalt, town sidewalks,
building footprints and the accepted central street slice remain authoritative.

Original road colliders are recorded with their source manifest hash in
`protection.json`. The placement baker additionally excludes complete building
interiors, the town pedestrian band, radio mast/wires and both road routes.
Each plant's conservative horizontal radius includes both axes of its actual
prototype bounds and instance scale. Stones/grass do not cast shadows.

## Reproduce

Obtain the shared heavy-job slot before texture preparation or Blender. Use
`Scripts/unreal_guard.py` and its owned-process monitoring wrapper. Run only one
heavy Unreal/Blender process at a time, with at least 10 GiB available.

1. Run `prepare_textures.py` using a Python runtime with NumPy and Pillow.
2. Run Blender background `build_ground.py`, then `verify_ground.py`.
3. After the separate vegetation kit exists, run `prepare_placement.py`.
4. Run `stage_placements.py` to write the prop manifest and staged JSON.
5. Import/verify with the parent `Tools/calichenature/` pipeline in separate
   editor processes, then package and inspect matched player-height views.

`verification.json` covers the saved source, portable image dependencies and
actual GLB color/alpha accessors. It explicitly does not stand in for Unreal
render-buffer verification or packaged visual, collision and performance tests.
Do not report a generated source or planned scatter as an accepted game result.
