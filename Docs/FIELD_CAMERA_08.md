# SIGNAL / 47 — field camera 08

Current milestone: extend the verified service-yard prototype with physical photographic evidence. The player collects a camera near the east door, reads controller S-03, frames the central antenna from the service apron, takes a real image, returns inside and compares the photograph with the log. The evidence records observations; it does not announce an alien explanation. No changes to the prologue's frequencies, 4/7, -39 LY or 47-second sequence.

## Acceptance recorded before final validation

- Complete the original prologue and physical exterior route with keyboard/mouse; collect the camera through a raycast interaction. C raises/lowers the viewfinder, Space exposes a photograph; E/Tab/Escape remain the existing controls.
- Reject a premature exposure, wrong subject or obstructed frame with readable feedback. A valid exposure captures the player's actual view without UI. It is added once, can be reopened, and is saved locally with metadata; restart clears current investigation state while preserving exported photographs.
- Comparing requires both records and returning inside. The comparison remains reopenable and does not duplicate observations. No debug command performs the journey or injects photographs.
- Runtime screenshots show the CC0 camera with authored materials, readable ivory viewfinder brackets, and an unclipped cream photographic record with actual image and distinct controls. Existing sparse prototype environment remains the baseline; new references are provisional targets, not user approval or running-build evidence.
- Same reference PC and preset as pass07: Intel ARL / OpenGLCore / 1280×800 Ultra, release player, 20-second warm-up then >=120 seconds including actual exposure and comparison. Average >=59 fps, p95 <=17.2 ms (existing desktop timer tolerance), p99 <20 ms, no intervals >50 ms. No frames removed. Evidence screenshot overhead is measured separately; the gameplay photograph is included in performance. GPU timings and subjective listening require separate evidence.

Representation: full 3D, free first-person movement. Delivery: offline Linux desktop. Keyboard/mouse, current 1280×800 reference screen. Photo panel scales down with screen size. Save is an exported JPG/JSON under Unity's persistent data directory (SARO/SIGNAL 47/FieldPhotos); this is not a whole-game resume system. Only the current slice's survey subject is photographable in this increment.

## Stack and verified handoffs

| Responsibility | Tool / input | Output / integration | Availability / reason |
| --- | --- | --- | --- |
| Gameplay and presentation | Unity 6000.3.22f1, URP 17.3.0, Input System 1.20.0 | C# + generated SARO_Prologue → Linux player | Existing verified stack; retain working game |
| Authored camera preparation | Blender 4.5.13, Poly Haven Camera_01 | Editable .blend → FieldCamera08.fbx, original UVs and three 1K PBR sets → URP materials | Executed CLI workflow, 22,319 triangles in exported body; loose strap preserved only in authoring source |
| Shutter feedback | roachpowder CC0 camera shutter HQ preview; ffmpeg | Mono 44.1 kHz WAV → AudioClip | Download/license verified; listening remains separate |
| Visual target | Built-in imagegen, actual pass07 screenshot | Docs/VisualTargets/FieldCamera08 images → visual comparison | Session tool; no paid API or new subscription |
| Evidence | ScreenCapture + AsyncGPUReadback, thread-safe JPG array encoder | Actual rendered image → notebook texture + local JPG/JSON | Native journey will prove runtime; no public service |
| Verification | Existing uinput journey + observation-only probe | Release screenshots, frame intervals, logs and source/payload hashes | Existing verified desktop path, exclusive test lock |

## Sources and Dream Loop adaptation

Reviewed the original conversation “Idéer om kosmisk etterforskning” and “Utvikle spillområde visuelt”: physical investigation tools, a camera whose record can be compared with observations, restrained stylized realism, sparse desert locations, no early explanatory reveal. The motel remains a later location; no new travel system is claimed here.

[Dream Loop](https://github.com/achimala/dream-loop), MIT, was read including SKILL.md, pro-mode/workflow.md and assets-3d.md on 10 September 2026. Useful elements: use an actual baseline image, choose assets before building, compare target and runtime with a fresh critic, revise a stalled hypothesis. Gauntlet's explicit functional/visual/performance gates take precedence over a summed visual score. Paid image-to-3D services are excluded by the user's zero-spend instruction. No installer, API key or public control server was used. Local `.dream-loop` is ignored; durable references live in Docs.

The first evidence-sheet generation invented more detailed antenna geometry and unsupported narrative text. It was rejected before the target was fixed; a correction uses the existing game screenshot. Neither image is proof of runtime fidelity.

Free sources: [Poly Haven Camera_01 — Rajil Jose Macatangay](https://polyhaven.com/a/Camera_01), [roachpowder Camera Shutter](https://freesound.org/people/roachpowder/sounds/170229/), both CC0. Existing Kenney interface sounds, VT323 font, Poly Haven room assets and wind/music are reused. Additional candidate Kenney RPG Audio and plaster textures were researched but not imported because this slice does not need them.

Reproduce: `bash Automation/run-field-camera.sh`. Source preparation: `python3 Automation/fetch-camera-assets.py`, then Blender `--background --python Unity/Blender/Source/prepare_field_camera.py`. The build regenerates the scene. Source/asset manifests retain provenance and hashes. Screenshots and performance are initially UNVERIFIED until their reports and review are recorded below.
