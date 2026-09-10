# Visual pass 10 — reference-driven production

Base: main c48616a (Chapter09 merged). User supplied targets: Docs/VisualTargets/Visual10/user-photolab.png and user-b12.png. These are targets, not runtime evidence.

Keep the completed investigation, world positions, input, immutable photographs and save compatibility. Main correction: textured manufactured surfaces with credible scale, contact shadows, warm practical lamps against cool night, and close-range silhouettes. No new gameplay system.

## Gates registered before implementation
1. Photolab: cream/green textured walls, worn floor, credible drawer benches, enlarger/sink details and distinct red safelight / warm archive task light; no floating props or dark unreadable actions.
2. B-12: textured weathered cabinet and concrete, warm local lighting with contact shadows, readable single physical stripe, cool antenna silhouettes and visible night sky. Both photographic experimental routes remain legible.
3. Presentation: free licensed PBR and appropriate physical FX integrated; existing Scott Buckley music retained with credits; no continuous music masking the mystery. Matched original runtime screenshots reviewed against supplied targets.
4. Regression: native full chapter, photo inspection, normal routes and save/restart on the new release; original archives preserved.
5. Delivery/performance: new identified Linux package and launcher; 1280x800 Ultra on reference Kubuntu, >=20s warm-up and >=120s representative measure; mean>=59fps, p95<=17.2ms, p99<20ms, no frames>50ms. No screenshot overhead removed from a mixed measurement; screenshots in separate run.

## Production availability
Files, terminal, Unity6000.3.22f1 build, Blender4.5.13 export/import, native input, runtime captures, sampled audio output and release measurements: VERIFIED. Subjective audio listening: UNVERIFIED.

Status: verified playable graphics update packaged as Visual10-eb0748660027. Defined static features and runtime/delivery checks pass; full reference quality remains FAIL with the specific differences below. Previous Chapter09 release remains available.

## Observed corrections

- Candidate01 inspection runner used an exterior-only return route from an indoor saved pose and hit the building boundary. Runner now chooses the normal interior route first. This failed tool attempt is retained separately from successful native visual walks.
- Candidate02 exposed reversed new bench fronts and a safelight placed behind its mounting wall. Corrected placement, preserved original colliders and game action positions. Cabinet label moved off its analogue meter; local reading light moved in front of the cabinet.
- Old dish is an exact parabola but all triangle normals point away from its concave side. New bowl-only mesh reverses winding and uses analytic parabola normals without moving any original vertex/pivot/feed support. Blender FBX roundtrip is retained in dish-normal-verification.json; runtime shows curved illumination. Original FBX remains unchanged.
- WorldAreaPass ran as a sceneSaving hook after visual authoring, resetting sky, fog and desert material. Execute it explicitly before Visual10; its existing idempotence guard then prevents overwriting the final presentation. This is the reason repeated sky adjustments initially did not reach runtime.
- Derived PBR maps are regenerated deterministically when values change. Outdoor concrete and the lab's sealed floor share original CC0 maps, with subdued albedo contrast/roughness adaptation for the interior finish. Normal maps are imported as normals, roughness/metal maps as linear data.
- Visible emitter follows actual experiment lamp intensity, including passive blackout and restored saves. The two captured photographic images still originate from the actual scene at exposure, and imported audio is not destroyed as if it were a generated clip.

## Interim profiling (not the final delivery result)
Source ce5456c3a579cf993e1dad8e9d6123baf82ad76e3525151d30ae777d016af5ef: 74.988fps/121.513s, p95 13.544ms, p99 13.839ms, max17.854ms, zero intervals>50ms on reference 1280x800 Ultra/OpenGLCore/75Hz. Includes physical processing, active control, exposure/export and conclusion from a copied historical checkpoint; 20s warm-up, nearest-rank percentiles, all9112 samples retained, screenshots disabled. This justified retaining contact effects. Changes after this profile require a new final measurement.

## Further visual corrections

Candidate05 proved that the world order correction takes effect, but its full intended sky settings were too bright. Recalibrated from that actual output to 0.075 exposure / (0.34,0.46,0.66) tint. Retained target dark blue night, locally warm field light and visible terrain. Replaced only visible old cube-mesa renderers with a continuous low distant ridge; old source objects remain. Corrected two-sided plant normals, moved stars inside the unchanged camera far clip, tilted the field lamp toward its work area and enclosed the ceiling seam with normal coving.

The final native comparison camera is reached by walking, including an adjusted B-12 overview standing at (11.35,-18.0), looking toward (10.8,1.4,-21.4), to include the whole cabinet and reference. Left/right placement follows the established accessible game layout.

## Native visual iterations 07–09

- The independent reviewer confirmed stronger warm field-metal response, but identified the flat lab floor and source-light construction as defining remaining differences. Candidate07 is not claimed to meet full reference quality.
- Added CC0 Dry Ground Rocks and reduced original Grass Medium02 blade geometry. Candidate08 exposed invisible tufts: the single-mesh FBX root carries -90° X and scale100. Placement code had overwritten both. Preserve the imported transform under a separate placement parent. The same axis correction applies to the one-node cloth and funnel; other original source files remain untouched.
- Replaced four solid glowing ceiling boxes with original two-tube fluorescent fixtures. Paired real sources below the fixtures soften the shelf silhouette. Added a physical measuring jug, funnel and print tongs, and brought existing stools to their workstations while retaining the central route.
- Reflection influence now extends beyond the physical floor/wall bounds, instead of fading exactly at those surfaces. Runtime logs confirm three actual128px cubemaps after normal loading. A bounded refresh after the B12 lamp/vane settles avoids stale experiment reflections; no per-frame cubemap rendering. Null-renderer smoke tests skip this graphics-only component.
- More polished sealed-floor response and pale fluorescent tube emission require the next actual image comparison. Cabinet meter paper has a separate subdued material; etched graduations remain visible. Paved service road receives its own texture while retaining layout.

All candidate images are original runtime captures from normal walking with a copied historical predevelopment checkpoint. They demonstrate visual appearance and routes, not the final new photographic journey. Final new-game, two real exposures, branch/restore, audio output and release performance checks still follow.

## Baked-light and geometry correction

The repeated dark/flat lab comparison prompted an architectural correction: four actual baked Rectangle area sources, 27 classic light probes and a Progressive CPU bake (10 texels/m, two 512 directional atlases). The editor must run with its graphics device, without `-nographics`; use `Automation/build-visual10-linux.sh`. Its BuildCurrentGauntletLinux entrypoint preserves the baked scene. Ordinary scene-generation builds regenerate an unbaked scene and are not the Visual10 release path.

UV2 generation uses a verified two-pixel manual margin. Required surfaces are checked for degenerate UV area and valid baked atlas assignments. Tiny objects and open slender metal frames explicitly use probes. An earlier failing required-frame UV guard is retained in Artifacts/Visual10BakeFailures; it was not silently accepted. Bake02/Bake04 original native walks confirm softer lighting, active fluorescent emission and readable sink/stool steel. URP's installed material validator requires an emissive GI flag: RealtimeEmissive with realtime GI disabled retains visible emission without baked source contribution; keyword validation is checked before and after BuildPlayer.

Independent review found the black procedural twig clusters visually worse than the reference. Their renderer is disabled; their mesh remains preserved. Actual CC0 grass geometry now uses layered crowns, under2500 triangles per model, with original atlas islands and FBX roundtrip checks. Walking-space exclusions remain unchanged.

Known comparison limits remain explicit: the established multi-antenna array composition differs from the reference's single dominant dish; broad floor-reflection shapes and fine material/prop density still require judgment in the final captures. Full reference equivalence is not established by the narrower five production gates.

The locally installed Gauntlet visual reference was updated through the read skill-creator workflow with tested FBX-root preservation, scene-pass overwrite, bowl-normal and derived-map lessons. Local validation passed; that skill directory is not a Git repository, and no sync to another machine is claimed. Dream Loop was read from the pinned original source and used as a comparison method, not newly installed. No paid asset service was used.

## Controlled local release

`Visual10-eb0748660027`, source commit `d026cc3d431ed0e99b43e06e2e6e204ef842cdba`; Unity SHA `eb0748660027744a8ad717c16a51a1e8801f4908b758e4ca191e24108c9a2c01`, build payload `eb2bbdcc925e3e016b6e5c90996cb386ada41bb2004e42e1954b07713601ebb6`. Archive SHA `8f83fba8197dd3650d6ca608efdaefc05c44f12eb138bc3f64b42b98121368dd`. Subsequent documentation/evidence commits do not change that tested source. Evidence: [final-d026cc3](Evidence/Visual10/final-d026cc3/README.md).

PASS: actual new-game prologue and first exposure, restart before development, active control, restart after second exposure, second development/conclusion and repeated original-photo reopening/loupe; a separate passive branch completes from the same first real checkpoint. Wrong subjects, premature control, wrong references/hypotheses/conclusions and lab pause are exercised. Settings sliders affect native mouse input, window/fullscreen toggles work and audio/mouse values survive process restart. No runtime exception markers in seven new runtime runs. Earlier unchanged Chapter09 core smoke/negative-save reports are preserved as historical regression evidence, not relabeled as new runs.

PASS: the new archive was unpacked into a new directory; all173 files, hashes and executable modes checked, normal launcher started without observer/smoke flags, first-person play, focus loss/return, pause and Quit inspected. Original package-start PNGs were reviewed separately after automation. Root `Spill-SIGNAL47.sh` points to this fixed release. Previous Chapter09 archive hash remains unchanged.

PASS performance on Intel Core Ultra5 225U / Mesa Intel ARL, Linux7.0 Ubuntu26.04 / OpenGLCore /1280×800 Ultra /75Hz, target75, vSync0. After20s warm-up: 8922 unfiltered intervals over121.533s, mean73.412fps, p95 15.368ms, p99 19.364ms, max30.718ms, zero>50ms. Nearest-rank percentiles and mean independently reproduced from the full CSV. Route includes lab, dense grass, active reference, new photo/export and conclusion; screenshots/audio audit separate. Engine peak135.52MiB; whole-process peak RSS380.68MiB includes startup/loading. GPU timing UNVERIFIED. This is not a performance claim for other machines/presets.

Sampled audio output PASS for observed sound output/no sampled clipping: 48kHz, channel0/2048 samples about every40ms, 4030 windows across active control/finish, maximum amplitude0.25628, no full-scale samples. Paused-state windows include transitions and are not proof of sample-perfect silence. Subjective listening remains UNVERIFIED.

The independent critic confirms coherent reference art direction and the registered static features. **Full reference quality remains FAIL:** prop/surface density, broad floor-reflection shapes, cabinet paint damage and terrain variation remain lower than the target images. This delivery is a substantial playable graphics checkpoint, not a claim that the entire requested reference target is finished. The next coherent art pass should address those remaining material/geometry differences; moving working antennas merely to duplicate one reference composition is unnecessary.

Unity's post-build analytics callback emitted a Core RP VolumeProfileOverridesAnalytic NullReference. It is recorded in the unmodified build log; successful build, emitter validation and actual package runtime are separately checked. No PackageCache or engine source was changed to hide it.
