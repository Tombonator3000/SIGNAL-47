# World area / visual language pass 05 — 2026-09-10

## Scope

This slice responds to the supplied radio-telescope night references and motel-neon reference without changing the prologue story order or core interactions. The target is **stylized realism**: strong silhouettes, believable 1986 materials and lighting, and large readable shapes, rather than expensive photorealism.

The current runtime control room remains the playable prologue. This pass starts establishing the larger game's visual and spatial language around it. The motel is a **compressed blockout for a later field location**, placed outside the current playable room; its position in the scene is not canonical geography and does not claim that a motel is physically next to SARO.

## Visual decisions from the supplied references

- Radio array: cold, star-rich New Mexico night; dishes read first as large silhouettes. Selective low-angle amber maintenance light gives the metal structure depth without lighting every dish.
- Desert: broad dark values, low scrub and rock silhouettes, sparse service infrastructure and road reflectors. Detail comes from shape rhythm rather than dense texture layers.
- Roadside motel: low one-storey 1980s motor-court massing, cream/stucco body, black asphalt and a red/green stacked sign. The sign is an original `SIERRA MOTOR COURT` design, not a copy of the photographed Crane Motel sign.
- Interior/exterior contrast: institutional green/cream control room remains visually safe and legible; outside is darker, cooler and larger in scale, with warm amber as the human-maintenance accent.

## Implemented

`WorldAreaPass.cs` hooks the existing vertical-slice scene save, so the current scene builder can stay untouched. It:

1. Re-composes the five nearest existing dishes into a clearer receding perspective seen through the control-room windows. Existing dish pivots and `DishArrayController` remain unchanged.
2. Adds three shadowless amber maintenance lights, emissive red service beacons, small service cabinets/markers and deterministic low-poly scrub/rock breakup.
3. Darkens the desert/asphalt palette and reduces fog density so the existing night-sky asset can read more strongly. The Poly Haven sky remains a visual sky, not an astronomically exact 1986 star map.
4. Adds an inaccessible roadside motel blockout behind the current prologue building for later field-area development: parking, room rhythm, office/canopy and an original red/green `SIERRA MOTOR COURT` sign.
5. Keeps added geometry primitive/simple and avoids new high-resolution textures or shadow-casting point lights.

At scene-save time the imported Qwantani sky exposure is raised from 0.025 to 0.085 with a slightly less muted tint; the source asset and its attribution remain unchanged. Existing third-party credits and source relationships are unchanged.

## Acceptance criteria for this bounded slice

- Existing story constants and sequence are untouched: 1419.900 / 1420.110 / 1420.405 MHz, 4 / 7, -39 LY and the 47-second event.
- Existing player navigation and interaction objects are not moved or replaced.
- `WorldAreaArt`, `ArrayNightArt` and `RoadsideMotel_Blockout` are generated with the prologue scene.
- Added array mood lights do not cast real-time shadows; added visual language relies primarily on emissive materials and simple geometry.
- Existing regression journey must still pass after Unity rebuild. A separate `WorldAreaSmokeChecks` component checks world roots, lightweight array lighting and desert breakup while feeding failures into the existing smoke run.
- Visual comparison and performance measurement remain required on the real graphical Linux player before this pass can be called visually verified.

## Verification

GitHub Actions run `34454251575` completed successfully on the dedicated `signal47-kubuntu` self-hosted runner at source commit `bd3cc5216fefa2c5af2784c0c52709eeda66af0e`. Unity compiled and built the Linux prologue and the existing smoke run completed successfully. A temporary verification step additionally required exactly three `WORLD_PASS_SMOKE_PASS` markers in `Artifacts/smoke.log` plus the existing `SIGNAL47_SMOKE_PASS`; that step passed. The workflow file was then restored to its previous manual-only form, so the final branch does not leave a permanent push trigger.

This is **build/regression verification**, not visual verification. The runner smoke job uses the established display-free path, so no claim is made that the new night-array composition, brighter stars or motel blockout have been inspected in a graphical player yet. The prior 60 fps benchmark is only a baseline; this pass still needs a new graphical capture and performance measurement before its visual/performance gates can pass.

## Area direction after this pass

The larger game should grow as a small number of dense, legible investigation zones joined by road, not a giant empty open world. The first route can be:

`SARO control building → array/service yard → access road → highway junction → SIERRA MOTOR COURT / diner cluster`

Later branches can lead to ranchland, a microwave/radar site, dry wash, restricted military track and the Roswell thread. Distances in design should feel large through sightlines, road pacing and darkness, while actual playable geometry stays compressed enough to keep production and hardware demands sane.

## Performance budget intent

For new exterior art, prefer simple repeated meshes, 512–1024px reusable materials, baked/emissive light cues, no more than a few local real-time lights in a view, no added shadowed point lights, and LOD/instancing when the full array becomes explorable. The established 60 fps reference-PC gate remains the benchmark; this file does not claim the new pass meets it until measured.
