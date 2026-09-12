# Visual production and performance gates

Read for visual or realtime projects; substitute correctness, response time, throughput, render time or file size for non-realtime work.

## Concept and asset handoff

Use separate images for a representative gameplay/main screen, an important second state and an asset detail only when needed. Reuse supplied identity/layout references. Specify subject, camera/projection/aspect ratio, composition, scale, palette, materials, light direction, density, motion/interaction and practical constraints. Show intended use; promotional art is a separate deliverable. Preserve exact final UI text as editable components, not baked lettering.

For 2D define sprite scale, silhouette, outline/brush treatment, animation, background layering and readability. For 3D define viewpoint, shape language, material response and atmosphere. For interfaces define hierarchy, spacing, density, accessibility and meaningful states. Identify expensive features early; concept imagery does not prove realtime feasibility.

Keep a concise versioned visual specification tied to named reference files and saved prompts. Resolve contradictions before implementation. Do not lower the reference standard to excuse a weak build.

Build only assets needed by the current slice. Choose code/vector, raster, procedural content, authored meshes or suitable licensed resources by actual requirements. For Blender retain editable source and verify export scale, orientation, origin, normals, UVs, material/texture references and animation. Set budgets from target hardware and measured scene cost. Inspect imported assets in the runtime. For sprites inspect transparency, dimensions, pivots, animation consistency, sorting, filtering and runtime scale. Preserve licenses, source/export relationships and engine metadata such as Unity .meta files.

## Runtime comparison

Capture unaltered screenshots from the actual build. Match reference camera, aspect ratio, scene/state and quality preset where possible. Use real recordings or frame sequences for transitions and animation. Never retouch runtime evidence or present a concept as functioning software.

Use a short comparison table: reference feature, observed difference, impact, next correction and resulting evidence. Review camera/composition/scale/silhouettes; palette/contrast/light/materials/depth; required density/style consistency; UI readability/focus/controls; motion/feedback/transitions. Fix largest gaps first. Numerical image comparisons are supporting evidence, not proof of interaction or an invented similarity percentage.

Visual completion requires the agreed explicit criteria. If a defining feature cannot meet the platform or performance limit, document the tested limitation and concrete alternatives; do not silently change hardware, style or scope.

## Measured performance

Default realtime target: sustained 60 fps. Define device, OS, renderer, resolution, quality preset, workload and measurement method before judging results. Initial thresholds: p95 frame time ≤16.67 ms and p99 <20 ms; state any timer/vsync tolerance in advance. Define a stall threshold, for example >50 ms, and its allowance explicitly. These are starting project criteria, not universal guarantees.

Use a representative player/production build. A typical first benchmark has warm-up followed by about two minutes of normal and demanding use: movement, effects, UI and transitions. Extend for thermal behavior or memory growth when warranted. Record average fps, p95/p99 frame times, stalls and memory where available; inspect CPU/GPU timing when supported. Record sample counts and percentile method. Include problematic frames; label deliberate screenshot or loading overhead separately rather than silently discarding it.

Editor, emulation, headless, software-renderer or cloud results need limitations. A target-fps setting, average-only counter or fewer triangles does not establish stable 60 fps. Missing target-device measurement remains UNVERIFIED. Scope performance claims to the measured device and workload.

Profile before optimization. Address observed bottlenecks with suitable batching/culling/LOD/budgets/baked lights/asynchronous work or reduced unnecessary updates. Record quality/resolution changes and recapture visual evidence. Visual and performance gates must pass on the same final build and preset.

## Tested Unity/Blender handoff traps

Observed in SIGNAL / 47 with Unity6000.3.22f1/URP17.3 and Blender4.5.13; recheck behavior for another importer/version.

- A one-mesh FBX can put unit/axis conversion on its imported root (in this case -90° X and scale100), while a multi-part FBX has an extra container. Do not replace the asset root transform with placement yaw/scale. Put the unchanged imported instance under a placement parent; apply authored position/yaw/variation to that parent. Verify actual Unity world bounds and a close runtime view, not only the Blender roundtrip. In the tested scene, overwriting these transforms made grass100× too small and rotated a draped cloth into the sink.
- Procedural scene passes may also run through scene-saving callbacks. If new material/sky settings disappear despite correct authoring code, inspect the final saved scene and callback order before tweaking their values again. The tested world pass reset sky/fog/ground after the visual pass; explicitly ordering its idempotent execution fixed the overwrite.
- A visible bowl can retain the intended shape but have normals facing its back. Inspect winding, normal direction and actual vertex positions before changing the entire light rig. A corrected bowl-only mesh can preserve the original source, pivot, movement and supports.
- Derived packed roughness/metalness maps need deterministic regeneration when their parameters change. An existence-only file cache can leave repeated material corrections absent from the running result. Verify the actual imported data and retain original licensed maps separately.

These are tested production diagnostics, not a requirement to use a particular shader, coordinate convention or scene generator in every project.
