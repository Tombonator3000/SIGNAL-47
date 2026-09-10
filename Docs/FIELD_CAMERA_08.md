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

## Final verification — 10 September 2026

PASS: development smoke, 34 native keyboard/mouse checkpoints in each of the measured and screenshot runs, camera pickup, premature/wrong-frame rejection, actual JPG and metadata, preserved earlier evidence, deduplication, comparison inside, reopening and reset. PASS: independent bounded visual review after correcting the FBX import scale, mirrored kit label and inverted top brackets. The sheet now uses the project font and flat controls. All original evidence is in `Docs/Evidence/FieldCamera08/final-af5a85d`; `final-verification.json` supersedes the automatic pre-review visual status in `quality-gates.json`.

Final release: Intel Core Ultra 5 225U / Intel ARL / OpenGLCore, 1280×800 Ultra, 75 Hz display, target 75, vsync 0. After 20-second warm-up, **13,935 frames over 185.832 seconds**: **74.987 fps**, **p95 15.875 ms**, **p99 16.181 ms**, maximum 43.051 ms, **zero frames >50 ms**. No intervals removed; actual in-game photo capture, encoding and saving are included. Engine allocation peak 96.10 MiB, not OS process total. GPU timing and subjective listening remain UNVERIFIED. Existing seven audio-state samples pass; the new shutter asset itself has no full-scale samples, but its subjective mix was not heard.

The checked-in Unity inputs match source SHA-256 `af5a85d7a25f43b2adcf8ee07d5b7dedf5935c921e9879c676288ee97decb396`; full player payload SHA-256 `672a8a768f0069dc1e06e561c50ed496cb1bca7d8ca3772374aeea88bdb8a020`. Source checkpoint `6457265` contains those exact inputs. The build was produced from base 3d37c84 plus the working changes; later documentation commits do not change tested inputs. No purchased asset or paid API was used.

| Target feature | First observed difference | Correction | Final original evidence |
| --- | --- | --- | --- |
| Recognizable field camera | Imported body appeared absent because root unit transform was overwritten | Preserve imported transform below metre-scale wrapper | Journey/journey-15.png |
| Readable physical instructions | Label mirrored | Face TextMesh toward south approach | Journey/journey-15.png |
| Framing marks clear of title | Float comparison inverted upper brackets | Explicit top/bottom indexing | Journey/journey-25.png |
| Calm photographic record | Generic glossy buttons/system font | VT323 typography, flat controls, pale paper and separated columns | Journey/journey-28.png and journey-31.png |
| Actual visual evidence | None: exported image matches native scene, without interface | Preserve original JPG and capture metadata | Journey/ExportedPhoto/S03.jpg |

Remaining limits: one survey subject, a still-prototype exterior and motel blockout, no whole-game resume or film-development mechanic. Exported photos survive restart; notebook state does not. Next bounded investigation: physical film processing and a second clue requiring visual interpretation of a photograph.
