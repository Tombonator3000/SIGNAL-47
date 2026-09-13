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

## Tested Unity compatibility lesson — Splat21

An isolated SIGNAL / 47 fixture on Unity 6000.3.22f1 / URP 17.3.0 / Intel ARL used UnityGaussianSplatting 1.1.1 at `2c6fed37da67a217367261fcfcd3316d34c73e76`. Vulkan rendered 784 and 50,176 synthetic Gaussians without recorded runtime errors. OpenGL reported supported compute/shaders but failed with kernel and Render Graph errors; nominal capability flags are insufficient. Reject this known failed combination before launching it again. These results do not characterize every GPU or splat implementation.

The isolated project needed both XR and VR Unity modules for `XRSettings`, the package URP feature, Render Graph enabled, HDR and MSAA disabled. Keep these changes out of the working game until compatibility and visual tests justify integration. The package is unlit and does not inherit PlayCanvas relighting. Inspect opaque depth boundaries: the fixture visibly contaminated the bottom of an opaque foreground object despite successful Vulkan execution, so integration remained unapproved.

After that failed OpenGL probe, the machine suffered a logged global OOM and reboot; the full causal chain was not established. Do not rerun a known failing graphical combination to obtain more evidence. Bound experimental process memory and elapsed time, verify the boundary before starting, cap error retention and stop at the first runtime error. A shell timeout alone did not reliably protect that distressed session. Linux user cgroups were verified for the recovered probe; they do not guarantee protection from every GPU/driver or out-of-group resource failure. Preserve original failure evidence and separate historical and corrected build identities.


## Tested generated-model comparison — Hybrid23

SIGNAL / 47 Hybrid23 tested [Arloopa UnitySplats](https://github.com/arloopa/UnitySplats) 1.2.0 at `6c0258189a2b124af1282fa9236fd9b6637f1a1a`, Unity 6000.3.22f1 / URP 17.3.0 / Linux Vulkan / Intel ARL, HDR and 1×MSAA. A GPT 2.5 reference became a Magnific/Tripo textured GLB; Blender normalized and clipped its doorway, then sampled 150,000 surface Gaussians. This was **untrained mesh-derived data**, not a generated Marble world or multiview reconstruction. The same derived mesh was the A/B baseline. It looked sharper and used less diagnostic frame time; do not convert an already usable mesh to splats by default.

The checked [MIT license](https://github.com/arloopa/UnitySplats/blob/6c0258189a2b124af1282fa9236fd9b6637f1a1a/LICENSE.md) permits commercial use with notices. Preserve Unity.WebP/libwebp and bundled DLL notices too; this does not make asset-generation services free. Validate the actual package dependencies: this probe needed the builtin UnityWebRequest module. In headless construction, explicitly create/validate `GsplatSettings.Instance` and persist its Resources asset. Mark importer settings dirty, write them, reimport and read back the actual compression/coordinate values; assignment followed by reimport alone had left Spark/Auto instead of Uncompressed/RUF.

The package's actual matched-mesh proxy relighting produced visible lamp response. Its shared light map was camera-bound: the tested separate photo camera used the same pose, projection, viewport and culling, with successful explicit proxy `Refresh`/`RenderNow` before the URP camera request. Other camera poses and production MSAA remain unverified. Do not equate a successful camera request or changed image with correct lighting. Retain a common-only exposure when switching mesh/splat representations, or a missing splat render can appear to pass because the mesh disappeared.

A real CharacterController, common colliders, rotating door and inward mesh lining kept mechanics separate. The generated shell initially bled windows/undersides into the room; aligned opaque interior geometry fixed the large intrusion, with minor seams remaining. Final fixed-pose cyan reference: 2,414 inner pixels, zero changed; this does not prove global occlusion or retest another plugin's identical failing fixture. Final proxy player and its unpacked launcher each passed 22 API checks and five separately rendered JPEGs, with native input unverified.

Keep shader compilation concurrency bounded too: one 4 GiB cgroup-limited build was killed while compiling shaders; two Unity job workers completed inside the same limit. This contained failure is different from Splat21's global OOM history. Detailed recipes, source assets, original captures, limits and identities live in the project's `Automation/Hybrid23/` and `Docs/Research/Hybrid23/` records. Treat these results as evidence for this candidate and device, not as universal renderer or photorealism claims.
