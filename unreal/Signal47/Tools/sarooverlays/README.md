# SARO overlay separation

Regenerates the existing g220/g377 grime atlas geometry with 10 mm separation from floors and ceilings. The measured g220 lower-wall overlays on the south, east and west walls move 12 mm along their normals, leaving at least 11 mm clearance from the control-room finish. Whole triangles move together. Original triangle count, UVs, normals and vertex colours are retained. This is project-owned exported geometry, not a new texture asset.

From the repository root, supply the original export folder containing `manifest.json` and `geo/`:

```sh
python3 unreal/Signal47/Tools/sarooverlays/prepare_overlays.py --export /path/to/export --out /tmp/saro-overlays
```

Copy the generated `S47SaroOverlayGeometry.inl` into `Source/Signal47/Private` after reviewing its geometry report. `S47SaroOverlays.cpp` retains each component's live material. The separate downpipe repair clones the existing CPU-readable AccessStains mesh with all three LODs and moves only the six wet marks from inside the pipe volume onto the wall.

Developer comparison: `-S47SaroOverlayOriginal` keeps the old geometry. `-S47ProductionViews -S47ProductionSuite=saro-overlays -S47ProductionLight=0 -S47ProductionExit -S47ProductionShots=/absolute/output` captures ceiling, floor and downpipe cameras in an isolated developer session. These are fixed visual fixtures, not a human playthrough.

Lower-wall diagnosis: `-S47ProductionSuite=saro-wall-base` captures four close/oblique views. Add `-S47WallBaseMotion` to the single `saro-overlays` / `saro-overlay-floor` view to capture five frames over a three-second camera slide. These fixtures do not simulate walking input. Use identical before/after views without `-S47SaroOverlayOriginal`, which also reverts the earlier horizontal repair.
