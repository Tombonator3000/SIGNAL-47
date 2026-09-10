# Chapter 09 — The second exposure

Status: BUILDING. This is the full first-chapter assignment, not a film-processing-only pass. The eight acceptance criteria in KUBUNTU_GAUNTLET_SPILL_MASTERPROMPT.md remain binding. Existing prologue and field-camera input were exercised again before integration: all 34 keyboard/mouse checkpoints passed, in Docs/Evidence/Chapter09/Baseline/native-journey.json. No new chapter runtime/performance gate is yet claimed.

## Chosen investigation

Continue the established prologue into a physical north photolab and B-12 reference station beyond S-03. The first exposure preserves a second pale reference stripe that is absent from normal eyesight. The player develops and inspects the image, identifies the location from the field legend, proposes a mundane explanation and performs a controlled test. Shielded illumination and active vane calibration are distinct methods with different visible setup and observations. A second actual scene exposure preserves the unexplained companion mark. Compare the immutable exposures, reject unsupported claims and file a bounded local conclusion. The cause remains unknown.

This is new chapter design, not a retcon claiming earlier canon. Larger motel/driving/Roswell content remains future scope. Prologue frequencies, 4/7, -39 LY and the exact existing 47-second event remain unchanged. First-time length is an untested 20–30 minute content ambition; real input automation duration is recorded separately.

The image phenomenon is authored geometry present only in the photographic render. It must be captured from the player's actual camera/scene and frozen at exposure. Developing/reopening/restoring never re-photographs the current scene. Actual QA screenshots remain unedited runtime evidence.

## Delivery gates, fixed before final review

| Gate | Observable requirement | Current status |
| --- | --- | --- |
| Whole journey | Normal new game through original prologue, lab, hypothesis, field experiment, comparison and new ending | UNVERIFIED |
| Space | Walkable connected control room, yard, north photolab and B-12 pad; reachable tools and return paths | UNVERIFIED |
| Investigation | Different observations/actions for two experimental methods; wrong explanation recoverable | UNVERIFIED |
| Photography | Two immutable actual exposures, physical processing, visible clue, inspection and stable evidence IDs | UNVERIFIED |
| Signature/outcome | Player-driven paired-image discovery and justified local report; larger cause open | UNVERIFIED |
| Persistence | Quit/relaunch before development and after field test; preserved images/state, fresh new game, robust invalid/missing data | UNVERIFIED |
| Presentation | Detailed lab equipment and readable B-12 vane/cabinet; clear route, fitted editable UI, settings and audio transitions | UNVERIFIED |
| Build/delivery | Regression + both actual input paths + same-release visual/performance checks and fresh archive launch | UNVERIFIED |

Reference performance conditions: Intel Core Ultra 5 225U / Intel ARL / OpenGLCore / 1280×800 Ultra, current 75Hz screen if verified unchanged; release build; 20s warm-up then >=120s representative new gameplay. Average >=59 fps, p95 <=17.2ms (existing timer tolerance), p99 <20ms, zero intervals >50ms. No excluded frames. Photograph rendering/readback/encoding/saving is part of gameplay timing; QA screenshots occur in another run. Explicit full program restart is evaluated separately as load time, never silently dropped from a continuous recording. GPU timing and actual subjective listening remain separate evidence requirements.

## Visual targets and assets

Existing control-room-v1 and phone-desk-v1 establish the retained style. New generated targets are Docs/VisualTargets/Chapter09/photolab-target-v1.png and field-target-v1.png. These are provisional concept targets, not user-approved runtime images. Their key criteria are an authored enlarger with bellows/trays/sink; cream/green architectural continuity; red localized safelight versus warm archive illumination; clear central path; a readable single reference stripe with ruler, an industrial B-12 panel and amber-lit walkable pad. Full material fidelity must be assessed honestly after runtime capture.

New geometry is authored in Blender by reproducible scripts, retaining .blend and FBX. Existing licensed floor, room assets, font and effects are reused. No purchases or paid API calls are authorized. Current runtime/build stack stays Unity 6000.3.22f1 / URP 17.3.0 / Input System 1.20.0; Blender 4.5.13 is available at the established local tool path. Save-slot tests use --signal47-save-dir with a separate task directory.

## Skills and responsibility

- Local gauntlet-loop: read with visual/performance reference; governing workflow.
- Brainstorming: original obra/superpowers commit b36e0829c6d0140e93cfef2ca599b1b07d4a7797, reviewed before installation through the bundled skill-installer into ~/.codex/skills/brainstorming. Context, options and choice applied with user's explicit routine-decision authorization; next-turn catalogue availability does not imply another machine is synced.
- Dream Loop: original achimala/dream-loop commit 9bddb901f7d071cfefdd21e264267c757177a9df read during preparation; baseline-based generated targets and fresh critique adapted into Gauntlet. No paid image-to-3D path.
- Imagegen: built-in tool generated the two provisional targets; editable labels and gameplay evidence stay in Unity.
- Skill-creator: read for recording a reusable method only after it has actually worked.
- Runtime, persistence and environment agents have separate file ownership; main agent owns integration, HUD, the single editor/build path, native input testing and final evidence. Their completion reports alone are not runtime verification.

## Integration notes

Source branch gauntlet/chapter-one-09 starts from 1c572b1 (main 8a6b4d9 plus current handoff/masterprompt documentation). Old runtime is preserved in Artifacts/Releases/FieldCamera08-af5a85d. Building may replace only ordinary ignored build outputs, not that immutable release. Sources/ references remain unchanged. No merge or public publish is authorized.

## First integrated runtime checkpoint — candidate 01

Development build and existing smoke regression passed. Candidate release source SHA-256 `cd4038a37df99bf112c58922db8dc07aad7f1792ce4f9446d963de9729a61ef1` completed the passive chapter through native keyboard/mouse, including separate process exits/resumes before development and after the controlled exposure. Both exported scene images were loaded from their original files. Wrong reference, motor-command hypothesis and unsupported conclusions were recoverable. Lab pause froze processing. The middle runner initially expected the wrong archive prompt; it was corrected and the diagnostic journey continued without moving or solving through code. A clean final journey is still required.

Independent visual review confirmed the actual double stripe and fitted photograph UI. Presentation is FAIL pending correction: interior cream walls, enlarger and processing card are underlit, with an overly saturated red patch. Inspection traced existing ceiling light reductions to the control-room scene-saving hook; the chapter lighting must run after that dressing.

Independent code review found and triggered corrections to queued saves during Quit, powered receiver LED restoration and photo sidecar metadata. Nine focused checks using the actual save code with mocked Unity APIs passed; their scope and source hashes are in Evidence/Chapter09/Contracts. These do not certify actual player input or file recovery. New source changes invalidate related final checks until the next build.

Current next run: corrected release build, fresh passive and active routes, corrupt/missing data and settings checks, then representative performance and archive startup. No finished-chapter claim is made at this checkpoint.

Candidate02 visibly corrected the enlarger, CRT surfaces and restored receiver LEDs. Remaining signed/card text, ceiling glare and static tracking label were corrected for candidate03. Actual native testing found an additional Kubuntu fullscreen defect: boolean fullscreen preserved a downscaled1280×800 render surface inside1920×1080 XWayland, and legacy IMGUI input was scaled twice. The corrected path requests native borderless fullscreen and restores the recorded window size, with version2 settings and version1 migration. This requires fresh native testing; neither the failed fullscreen test nor read-only contract checks count as PASS.
