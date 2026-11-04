# SARO parking surface

The garage forecourt and its access strip use `M_S47_ParkingAsphalt`; bay lines use `M_S47_ParkingPaint`. Both are built by `import_parking.py` and loaded by `S47SaroSite.cpp`. `/Game/Backdrop` is included in cook configuration because runtime loads these materials by path.

## Source and scale

Reuse the project's original Caliche `OldAsphalt` color, normal and roughness maps. Sources and authorship are documented in [Caliche ground](../../Signal47/Tools/calichestreet/ground/README.md). No new downloaded images or licensed material are introduced. Existing Caliche assets remain unchanged.

The previous Mesa asphalt image contained a dusty border around a complete parking area. Repeating that image on differently scaled cube UVs produced the visible rectangular grid. The replacement uses world coordinates at 3.6 m, with an 18 percent rotated 8.3 m color sample. Both paving meshes share scale and phase. Since the retained source maps are not periodic at their edges, sampling mirrors and clamps them rather than joining unlike edges. Normal directions follow the mirrored UV derivatives, with restrained relief at the turns to avoid lighting seams. Dust follows the outside of their combined L-shaped footprint, not the shared mesh seam. Paint has restrained surface wear; all three staff bays end west of the garage entrance.

## Rebuild

```sh
python3 unreal/ArtSource/Backdrop/run_parking_import.py --apply --output /absolute/fresh/output
```

The runner uses the global production lease and idle guard. The importer checks source texture color spaces, DirectX-normal settings, ownership, graph connections, compilation and save results. NullRHI import must be followed by the normal runtime visual check; it does not by itself validate GPU shader rendering.

## Geometry and driving

The physical footprint stays in `S47SaroForecourt` in `S47SaroSite.h`: forecourt X 20.1–39.8 / Z 5.9–18, access X 10–39.8 / Z 18–23, in Three metres. The driving area now includes the whole paving in both modes. Actual walls, cars and door leaves remain obstacles. Garage access retains the existing Freeform requirement. Outside Jeep parking uses a bounded region that fits the whole vehicle footprint; the existing Story commute arrival pad is retained.

`-S47EastWingDriveTest` checks access, cross-aisle and western frontage with the production controller, as well as staff-car collision, garage/ramp/door behavior and the Story lock. `-S47EastWingShots` includes access, aisle and close surface views 19–21.
