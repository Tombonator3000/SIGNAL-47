# Original Caliche nature: native import and verification

These scripts run **inside an explicitly scheduled Unreal 5.8 editor process**.
They do not start an editor, Blender, a build or a package. The owner must first
reserve the heavy-job slot and use `Scripts/unreal_guard.py` (`require-idle`, or
the Python `require_idle`/`run_owned` wrapper). No installed-game grace is needed.

The importer reuses the proven street material and legacy-FBX helpers through
`runpy`, temporarily selecting `S47_STREET_ONLY=''`. This runs **only** the tiny
asymmetric coordinate probe in `/Game/CalicheStreet/Verification`; no street
model, atmosphere, trim, radio material or backdrop import is selected. Existing
street/cloud import scripts are unchanged. Enable the legacy FBX factory with
`-ini:Engine:[ConsoleVariables]:Interchange.FeatureFlags.Import.FBX=0`.

## Owned assets and contracts

| Source directory | Native destination | Required authored levels |
| --- | --- | --- |
| `ground/` | `/Game/CalicheStreet/NatureGround` | one FBX per terrain/shoulder part |
| `vegetation/` | `/Game/CalicheStreet/NatureVegetation` | twelve base prototypes, three separate FBXs each |

Every source has `model_contract.json` with `parts` and `materials.json` (list or
name-keyed object). Main part names start `SM_CN_` and do not include `_LOD`.
Rows carry `bounds_local_m`, `triangles`, `materials` or `used_materials`.
Vegetation rows additionally contain `lods`: a list with `lod_index=0,1,2`,
`name`, source-relative `fbx`, `triangles`, `materials` and `bounds_local_m`.
Fallback FBX name is `exports/<part>.fbx`. All files must remain inside their
source folder. All selected sources are preflighted before native meshes change.

The coordinate contract is Blender metres `(x,y,z)` to native centimetres
`(x,-y,z)*100`: `MATCH_UP_FORWARD_AXES`, scene/unit conversion, front X and baked
import yaw +90. Sources use the established Blender FBX -Y forward/Z up export.
An asymmetric probe checks translation and size; every actual mesh checks its
own bounds. Runtime ground anchor is supplied by the runtime owner, not baked
again by this importer.

Base native meshes are rebuilt fresh at stable owned paths to remove stale
material-slot remaps. Additional FBXs are imported with `import_lod`; no automatic
mesh reduction or generated substitute LOD is requested. Defaults are explicit
screen sizes `[1.0,0.12,0.025]`. Nanite is disabled. Collision is not generated;
the runtime owner keeps terrain/plants `NoCollision` and preserves existing walk
surfaces, proxies and story collision.

Material rows reuse the existing texture/DirectX-normal/perimeter-alpha path.
`vertex_rgb_tint:true` adds `existing BaseColor * VertexColor.RGB` at BaseColor
without disconnecting the separately connected vertex-alpha/DitherTemporalAA
mask. Texture settings are sRGB only for base color, linear roughness with default
compression, normal-map compression with no green-channel flip. File hashes are
saved in report/asset metadata to bind native meshes, materials and textures to
the current source files.

## Flags and outputs

| Environment variable | Default / purpose |
| --- | --- |
| `S47_NATURE_ONLY` | `ground,vegetation`; select either or both, no duplicates |
| `S47_NATURE_REPORT` | `Tools/calichenature/unreal-import.json` |
| `S47_NATURE_PROBE_REPORT` | import report stem + `-axis-probe.json` |
| `S47_NATURE_VERIFY` | `Tools/calichenature/unreal-verification.json` |
| `S47_NATURE_SCREENS` | JSON array, defaults above; per-part then contract `screen_sizes` take precedence |
| `S47_NATURE_CULL_METERS` | JSON family overrides, e.g. `{"sage":220,"grass":90}` |

Cull policy resolves part `cull_m`, environment family override, contract
`cull_m`, then defaults grass90m/sage220m/yucca260m/juniper1200m/stone110m. A number
means end distance with start at82% of end, rounded to native centimetres,
matching the runtime HISM policy; an explicit pair is also accepted. The
asset stores `CN_RUNTIME_CULL_METERS` metadata only. **Actual HISM component cull
and shadow settings are the runtime owner's responsibility.** The verifier does
not claim metadata proves distance culling.

The current runtime applies the authored placement JSON's `cull_metres` per
family, start at82% of that value: grass73.8..90m, sage180.4..220m,
yucca213.2..260m, juniper984..1200m, stone90.2..110m. Grass and the smallest Caliche_A stones cast no
shadows; the remaining authored variants do. These are component settings, independent
of the asset screen-size defaults.

Use distinct scratch report paths for each frozen run. Import entry point:
`-run=pythonscript -script=<project>/Tools/calichenature/import_assets.py`.
Then exit and verify in a **fresh editor process** with
`-run=pythonscript -script=<project>/Tools/calichenature/verify_unreal.py`.
Both use `-unattended -nullrhi -nosplash -NoSound` and the legacy-FBX flag above.
Check the process return code, log errors, JSON `status=="PASS"` and every
`checks[].pass`; an import process's return code alone is insufficient.

The verifier checks saved LOD counts/triangles/screens, bounds, UVs, every slot
and real section binding, import
color policy, material graph reachability, texture settings and source hashes.
All three vegetation materials must actually store
`used_with_instanced_static_meshes=True`; a missing flag fails native validation.
For ground rows needing RGB/alpha, it exports LOD0 with `export_source_mesh=False`
and `vertex_color=True` into `<verify-report-stem>-render-fbx/`. UE 5.8's exporter
then reads the actual saved render `ColorVertexBuffer`. A dependency-free binary
FBX reader compares source/native RGBA polygon-corner distributions with 2/255
byte-quantization tolerance, retaining both file hashes and actual statistics.
Corner order may differ, so the comparison sorts each channel and explicitly
does not claim a spatial or runtime shader screenshot comparison.

UE5.8 protects `bAutoComputeLODScreenSize` from Python reads. Only that exact
protected-property exception is recorded under `report.unverified`, with status
`UNVERIFIED` and its full reason; it contributes no passing check. A readable
`True` still fails, and missing properties or any other exception still fail.
The verifier neither bypasses the field's protection nor infers it from screen
sizes. Saved screens remain independently verified. The runtime owner's native
gate must check actual cooked `RenderData.ScreenSize` against1/.12/.025 on all12
prototype families before creating components; editor flag status is distinct
from that runtime behavior proof. Top-level PASS covers readable checks only
and does not promote the protected flag to verified status.

Ground additionally requires saved `use_full_precision_u_vs=True` at every LOD.
The same actual render FBX supplies UV0 polygon corners for a source/native
per-channel distribution comparison with absolute `1e-4` UV tolerance. Negative
and repeated coordinates are preserved exactly, without modulo/wrap or offset
normalization; half-precision stepping at large terrain UVs therefore fails.
The report stores UV range, corner count, maximum channel delta and both FBX
hashes separately as `uv0_proof`. This verifies LOD0 render UVs, rather than
claiming the build-policy flag alone proves rendered precision.

Native PASS still requires running these scripts. Packaged eye-height pictures,
story/proxy checks, measured runtime placement/culling and the complete 110-shot
regression remain separate acceptance requirements. No purchased or third-party
models/textures are copied by this pipeline.

## Read-only packaged log audit and optional matched timing

The same verifier has an ordinary-Python log-only entry point, with **no Unreal
import or launch**. Use a fresh output file; existing evidence is never replaced:

```
python3 Tools/calichenature/verify_unreal.py \
  --runtime-log /scratch/capture/runtime.log \
  --placement Tools/calichenature/ground/nature-placement.json \
  --runtime-report /scratch/capture-audit.json
```

It requires12 unique `nature component` records with actual counts/cull distances
and PASS checks, exactly matching the placement JSON. The single final ready
record must agree with their total and3LODs. It also requires exactly12 distinct
`S47: nature cooked LOD screens <Id>: 1.00000,0.12000,0.02500` records with the
same authored family IDs. Missing, duplicate, malformed or wrong thresholds
fail. These values come from actual cooked `RenderData.ScreenSize` and verify
runtime thresholds independently of the protected editor flag. Native component telemetry checks
NoCollision and the first/last actual world transforms after HISM tree building;
the audit does not claim every plant is visible merely because the counts pass.
Owned `CalicheStreet/Nature*` material usage/SMU/default-material/fallback or
shader compilation failure diagnostics also fail the runtime audit and are
preserved in `nature_material_warnings`. Existing warnings for unrelated
`/Game/Web` or Free assets do not trip this scoped check.

`Scripts/caliche-nature-review.py --expect-nature` incorporates this audit into
each existing fixed-pose capture and records the placement file hash. The default
is compatible with the original6103baseline and does not require nature records.
`--performance` adds only the already-supported `S47StartupTrace` flag: the
GraphicsReview warmup stays3600s and no named camera takes over. The trace gives
game-thread **wall-frame intervals**, not isolated CPU work or GPU timing.
Its fixed visible4..10s window requires contiguous frames, at least30samples,
the requested PropsAt and held clock established before3.75s, an active startup
gate and zero shader/material/PSO/RHI queues. Reports include mean/p95/p99,
derived FPS and60fps comparisons. Texture residency/budget is unmeasured; matching
texture/streaming warnings are retained without inventing residency counters.

Capture separate before/after runs with identical `--views`, `--clocks`,
`--timeline`, resolution and `--performance`; add `--expect-nature` only to the
candidate. Candidate `--compare /scratch/before/nature-review.json` reports mean
and p95 changes only when every pose/clock/settings/window/device/driver matches
and both timing captures pass. Timing-evidence PASS is distinct from meeting a
frame-budget target. Existing baseline8 has no trace and remains unchanged, so
its performance is UNVERIFIED and cannot silently become the matched baseline.
