# Gaussian splats and generated characters

Read this when evaluating captured environments, PlayCanvas/SuperSplat, or image-to-3D characters. These are different asset pipelines. Check current versions and the target renderer before adopting either. Sources checked 13 September 2026; local experiments are evidence for their stated fixture only.

## Choose by the asset's role

Gaussian splats represent a scene with overlapping oriented, colored distributions. They are useful candidates for captured static environments, spatial reference and inspection experiences. An ordinary mesh remains the practical baseline for movable, articulated, precisely scaled or heavily edited game objects. This is a production recommendation, not a claim that splats cannot animate.

[PlayCanvas](https://github.com/playcanvas) offers a web engine, editor and related tools. Use the web stack for appropriate browser deliverables; asset preparation tools can be useful without changing an established game's engine. [SuperSplat](https://github.com/playcanvas/supersplat) provides interactive splat editing. Its [format workflow](https://developer.playcanvas.com/user-manual/supersplat/editor/import-export/) distinguishes WebGL editing from WebGPU-dependent exports.

[SplatTransform](https://github.com/playcanvas/splat-transform) handles conversion, transforms and filtering. Pin a local tool version and retain original inputs. SOG CPU compression is available; this does not imply CPU support for voxelization or rendering. A splat `.glb` uses `KHR_gaussian_splatting`: its extension is not evidence of an ordinary triangle mesh or compatibility with a generic GLB importer.

[Collision generation](https://developer.playcanvas.com/user-manual/splat-transform/collision/) produces voxel data and a separate `.collision.glb` triangle mesh. Inspect the actual mesh and player-sized passages; inferred surfaces do not establish accurate interactable bounds. Test thin walls, gaps, floors and doorway clearances in the target engine before using the result for navigation.

[Relighting](https://developer.playcanvas.com/user-manual/gaussian-splatting/building/relighting/) uses a lit proxy mesh and transfers its lighting onto splats in screen space. Proxy silhouette errors affect lighting boundaries. This provides runtime lighting control, but does not automatically recover clean albedo or eliminate captured shadows. Test the project's darkest, brightest and moving-light states rather than assuming an unlit scan behaves like a PBR mesh.

For Unity, assess a concrete implementation. [aras-p/UnityGaussianSplatting](https://github.com/aras-p/UnityGaussianSplatting) documents D3D12, Metal and Vulkan, with OpenGL limitations; it renders precomputed data, not reconstruction training. Do not apply those limits to every Unity splat package, or change a working project's renderer without an isolated compatibility test.

## Generated character workflow

The [Tripo demonstration](https://www.tripo3d.ai/blog/gpt-6-astra-3d-character-workflow) suggests clean full-body references and separately generated body, head and hair. Assemble against one scale in Blender; inspect joins from several directions. Reuse the generated body's working armature where suitable, remove duplicate body geometry and test attachments in poses. Check facial deformation and material maps separately. A slider name does not prove a functioning expression; a short motion test should precede a larger animation set. This is a vendor-reported workflow, not a guarantee of one-click production quality or a reproduced model benchmark.

Our production acceptance additionally requires the actual game's import, units, pivots, material response, rig/avatar mapping, runtime actions and relevant performance checks. Keep service outputs as candidates until these pass. Prefer the simplest character representation needed by the story; do not add a cast merely because a generator is available.

## Reuse and evidence

Keep source-photo rights, reconstruction-tool terms, generated-output terms and runtime-code licenses separate. An MIT viewer does not license arbitrary scans or third-party characters. Preserve attribution and distinguish redistribution of raw assets from embedding them in a game.

For a feasibility probe, use an original small fixture, known dimensions and a fresh output directory. Check conversion counts, finite values, coordinate error, the GLB's actual representation and a rendered view. Record failures and hardware. Those checks do not measure scan fidelity, game integration, collision usability, rig deformation or frame rate; each needs its own relevant test.
