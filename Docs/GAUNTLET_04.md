# Gauntlet 04 — real user journey and measured player

## Scope and acceptance, defined before judging the run

Continue the existing SIGNAL / 47 prologue. The player walks around SARO during a 1986 night shift, tunes a carrier, reads a physical printout, answers a telephone carrying a future event, then experiences that event 47 seconds later. Representation remains free-camera 3D; delivery remains an offline Linux desktop game. No network account, persistence service, purchased resource or engine migration is required.

This increment closes verification gaps and refines the story phone. Required outcomes:

1. Actual keyboard/mouse input takes the player from Start through the clipboard, receiver, three tuning stages, physical printout, phone, notebook, pause, ending and restart. No teleports or direct game-action calls in this journey test.
2. The phone has a recognizable sloping beige case, distinct dark keypad and curved handset, retaining the physical answer animation and reachable interaction. Match phone-desk-v1 as a provisional visual target. The full room concept is a future direction, not a claim that all its detail is implemented.
3. Existing 54 logical/physics regression assertions continue to pass.
4. Measure a non-development Linux player on this PC at 1280×800 with the delivered graphics preset. Warm up 20 seconds and record at least 120 seconds of representative movement, instrument UI, phone/event progression, pause and restart. Target average ≥59 fps, p95 ≤17.2 ms (16.67 ms plus 0.53 ms timer/presentation tolerance), p99 <20 ms, zero frames >50 ms. Nearest-rank percentiles; no frames removed. Unsupported GPU timing stays unverified. Screenshot captures are performed in a separate journey run, not silently excluded from benchmark data.
5. Deliver the local desktop skill, inspectable source/PR and a labeled runnable package, keeping the known working main build available.

Controls: WASD, mouse look, E interaction, Tab notebook, Escape close/pause. Test size 1280×800; controls include written prompts and do not rely solely on color. Full keyboard navigation of every GUI button, remapping, subtitles/audio descriptions and small-screen scaling remain outside this increment's demonstrated accessibility coverage. No save persistence beyond a session is currently required.

## Stack and capability evidence

| Responsibility | Tool/version and reason | Input → output / handoff | Availability |
|---|---|---|---|
| Editable game source and scene | Unity 6000.3.22f1, URP 17.3.0, Input System 1.20.0; retain working native 3D stack | Unity scripts/assets → generated SARO_Prologue → Linux player | VERIFIED WORKING: editor CLI, project load, compile, scene/player and logs tested separately in prior passes |
| Editable geometry | Blender 4.5.13 LTS; existing reproducible source jobs | Blender source script/.blend → FBX/.meta → Unity import and runtime | VERIFIED WORKING CLI path; prior nine-family export and current lamp inspection documented in ART_PASS_01 and ASSET_PASS_03 |
| Optional Blender MCP | ahujasid/blender-mcp, third-party | Host MCP → addon → Blender scene | UNAVAILABLE in this session; current maintainer setup inspected. Executed scripts remain the selected alternative; no MCP connection is claimed |
| Concept targets | Built-in imagegen with imagegen skill | Named existing screenshot references and stored briefs → separate concept PNGs | VERIFIED WORKING; two images generated and inspected |
| Real input | Native Linux uinput through the existing Wayland/XWayland session | Keyboard/mouse events → existing game controls → observer state and screenshots | VERIFIED WORKING: 14 checkpoints through restart; no gameplay setter exposed |
| Measurement | Observation-only GauntletProbe; monotonic Update intervals and optional FrameTimingManager | Actual player frames → frames.csv/performance.json; source/build stamp | VERIFIED WORKING: 138-second native journey; GPU timing unavailable |
| Delivery | Git, GitHub private repo, dedicated systemd runner | Branch/PR → reproducible build and regression logs → local archive | VERIFIED WORKING in previous run 34403502258; fresh checks recorded below |

Hardware observed: Intel Core Ultra 5 225U, integrated Intel Arrow Lake-U graphics, x86_64; Ubuntu 26.04.1 LTS with KDE Wayland/XWayland. Exact runtime GPU/renderer string, preset, memory and build identifier come from the measured player. Review is sequential self-review; no independent reviewers ran.

## Visual target v1

The built-in imagegen tool generated `VisualTargets/control-room-v1.png` and `VisualTargets/phone-desk-v1.png`, using the actual prior build's start/phone frames as layout references. These are provisional concepts, not user-approved art and not runtime evidence. No runtime screenshot is retouched.

Control-room brief: 16:10 eye-level first-person camera; preserve existing room scale, three-CRT central desk, wide antenna windows, phone table and reserve-radio workbench. Muted sage walls, worn teal metal, tan small-pattern linoleum, cream analog equipment; green practical fluorescent light, a small warm task light and a cool dark exterior. Clarify controls and useful silhouettes with modest geometry, no cinematic blur, fog or additional architecture.

Phone brief: same palette, room and first-person desk view; preserve orange articulated lamp and cream mug. Replace the rectangular phone with a simple 1980s beige sloping pushbutton case, dark numbered keypad, curved resting handset and cord. No added UI or props; production-feasible model. The generated numeral arrangement is illustrative; the actual model must use a coherent keypad layout.

| Reference feature | Observed baseline difference | User impact | Correction / evidence |
|---|---|---|---|
| Recognizable telephone | Flat rectangular base, block handset and sparse controls | Central story prop is difficult to identify visually | PASS for required sloping cream case, dark numbered keys, curved handset and cord: [actual player](Evidence/Gauntlet04/phone-resting-final.png). Surface detail remains simpler than the concept |
| Clear walkways and control access | Only isolated raycast tests previously existed | A passing script could conceal an obstructed route | PASS: native keyboard/mouse journey with collision and GUI controls; [checkpoint states](Evidence/Gauntlet04/journey-result.json) |
| Restrained practical lighting and material detail | Existing scene broadly matches palette; chairs/CRT and light response remain simpler than full-room concept | Broader visual polish remains open | Preserve current room scope; do not declare full-room concept matched |

## Sources checked

- [Unity 6.3 FrameTimingManager](https://docs.unity3d.com/6000.3/Documentation/ScriptReference/FrameTimingManager.html)
- [Blender MCP maintainer installation/transport](https://github.com/ahujasid/blender-mcp): third-party addon/server, not installed by this workflow.
- [OpenAI skill structure](https://learn.chatgpt.com/docs/build-skills), plus the locally installed skill-creator instructions. Local skill location: `/home/tombonator3000t/.codex/skills/gauntlet-loop/`.

## First observed failures and corrections

- A focused, non-development OpenGL player with the existing Ultra preset/vsync produced only 33 frames in a 33.004-second diagnostic capture: average 1.00 fps, p95 1001.45 ms. This was a stationary diagnostic, not the required representative benchmark. All frames were retained in `Artifacts/Gauntlet/vsync-baseline-diagnostic.json`. The old smoke test had disabled vsync only for itself, hiding this normal-player defect. Linux startup now disables vsync and requests 60 fps for the actual game; this setting is a proposed fix, not performance proof. Shader, resolution and art quality remain unchanged.
- Native automation initially raised X11 focus without activating the Wayland window. The driver now requests window-manager activation and checks native input focus before any press. Mixed-monitor pointer coordinates also required validation in client space. These are test-connection issues; the real input gate stays open until a complete journey succeeds.
- The initial observer build required the fully qualified `System.Environment` name and an older-compatible file replacement path. The first Blender phone export rejected the `FONT` type enum; the corrected exporter uses supported mesh/other types and reports failure with a nonzero process exit.

- The decisive Start-button defect was `activeInputHandler: 1` (new system only) while all menus used IMGUI/OnGUI. The installed Input System 1.20 documentation explicitly says it cannot generate input for IMGUI (`Library/PackageCache/com.unity.inputsystem@7a4e1a2a8194/Documentation~/KnownLimitations.md`). Both input backends are now enabled, preserving new-system movement and legacy menu events. The old 54 assertions had invoked methods directly and could not detect this.
- Native mouse events are now sent through two temporary Linux uinput devices, using existing per-user device access. No system permissions or persistent input service was added. The devices are removed after the run. XWayland is used only to identify/activate the owned game window and inspect pointer/focus. The previous XTest approach is not claimed working.

- A 68.65-second partial native-input measurement after Linux pacing repair measured 60.00 fps average, p95 16.84 ms, p99 17.04 ms and zero frames >50 ms. This run stopped on an overly strict automation target for a GUI slider, so it is explicitly a partial diagnostic, not the final performance gate. The test now uses realistic coarse-control tolerances within the unchanged puzzle ranges, plus the real ±0.001 MHz buttons. Advancing each stage still requires the actual game's acceptance logic through the real GUI action button.

The new phone FBX was re-imported into Blender read-only: 33 meshes, 10,711 triangles, 12 numeral meshes and 3 independently named handset parts. The source retains editable text and geometry in `Unity/Blender/SIGNAL47_story_phone.blend`; the exporter is `Unity/Blender/Source/build_story_phone.py`. Unity uses the exported sibling `Assets/Signal47/Art/Authored/SM_StoryPhone.fbx`, preserving original recovered phone assets. Uniform materials require no image UV maps. Source bounds/orientation and part counts are in `Artifacts/Gauntlet/phone-export-validation.json`.

## Final native journey and performance

The non-development player passed all 14 native-input checkpoints: Start, clipboard, receiver, three tuning stages, direction solve, physical printout, phone answer/dead line, notebook, pause/resume, 47-second event/title and restart with cleared evidence. The driver uses real GUI drags, fine-adjustment buttons, E and collision-constrained walking. It reads observer state to locate targets and judge outcomes; it never sets instrument values, moves the player directly or calls game interactions. This is automated keyboard/mouse verification, not a claim that a human manually played it.

After 20 seconds of warm-up, all 8,294 frames over 138.267 seconds were retained. The same release build and Ultra preset were used for the final unaltered screenshots; captures ran separately after measurement.

| Measure | Observed | Gate |
|---|---:|---|
| Average fps | 59.9853 | PASS ≥59 |
| p95 frame interval | 16.8320 ms | PASS ≤17.2 ms |
| p99 frame interval | 17.0179 ms | PASS <20 ms |
| Maximum frame interval | 49.7720 ms | Reported; one near-threshold slow frame remains visible in the data |
| Frames >50 ms | 0 | PASS |
| Focus / duration | Focused throughout / 138.267 s | PASS |
| CPU frame-time mean | 16.6708 ms | Includes pacing; not pure processing cost |
| GPU timing | No positive samples returned | UNVERIFIED; JSON zero means unavailable |
| Peak engine allocation | 91.4549 MiB | Unity allocation, not whole-process RSS |

Conditions: Linux x86_64 player, Unity 6000.3.22f1, Intel Core Ultra 5 225U, Mesa Intel Graphics (ARL), Mesa 26.0.8/OpenGLCore, KDE Wayland/XWayland, 1280×800, Ultra, vsync disabled and 60 fps application limit. The input/observer instrumentation was enabled and its overhead remains in the measurement. Frame intervals measure Unity updates, not a hardware display scanout or GPU profiler trace. Other devices, higher resolutions and long thermal sessions remain UNVERIFIED.

Evidence: [raw frames](Evidence/Gauntlet04/frames.csv), [performance report](Evidence/Gauntlet04/performance.json), [native checkpoints](Evidence/Gauntlet04/journey-result.json), [build manifest](Evidence/Gauntlet04/build-manifest.json), [capture states](Evidence/Gauntlet04/visual-evidence.json), [phone export](Evidence/Gauntlet04/phone-export-validation.json).

The tested source was the working tree based on `07eb27d0374236e37e5216475f3675ee325e1d2a`, identified by Unity-source SHA-256 `5642e656d52a2cce95f58fae3e9d30fb7f00a1fca9e3ad2969c6924438ad13b1`. The manifest also hashes the complete build payload, including player data and assemblies; the native launcher hash alone is insufficient. No Unity code, settings or scene changed after this benchmark. Later documentation commits do not change that evidence identifier.

## Delivery and remaining gates

| Requirement | Result |
|---|---|
| Real user journey, ending and restart | PASS — native input, 14 observed checkpoints |
| Phone visual features and runtime import | PASS — inspected actual phone frame and exported geometry |
| Full-room concept match | UNVERIFIED / future scope — [actual room](Evidence/Gauntlet04/control-room-final.png) retains simpler chairs, CRT housings and lighting detail |
| Performance on the stated PC/preset | PASS — measured criteria above |
| 54 existing regression assertions | PASS — [CI 34408377787](https://github.com/Tombonator3000/SIGNAL-47/actions/runs/34408377787), revision `123a88a`, all 54 assertions |
| Subjective audio balance/listening | UNVERIFIED — no listening review claimed |
| Independent review | N/A — sequential self-review selected |

The local `gauntlet-loop` skill was adapted from the supplied Universal Gauntlet text, with a focused SKILL.md, visual/performance reference and desktop metadata. The skill-creator validator passed. It is installed in `/home/tombonator3000t/.codex/skills/gauntlet-loop/` and backed up in `Artifacts/gauntlet-loop-skill.zip`. Desktop picker discovery after refresh is UNVERIFIED; this is not a claim that the mobile skill automatically synced.

### Reproduce

```sh
# Build the normal release player, stamp exact source/payload and package it.
bash Automation/build-gauntlet-linux.sh
# Open normally to play (no test instrumentation).
./Artifacts/GauntletLinux/Signal47.x86_64
# For a separate real-input benchmark, close other SIGNAL 47 instances first.
./Artifacts/GauntletLinux/Signal47.x86_64 --signal47-gauntlet -screen-width 1280 -screen-height 800 -screen-fullscreen 0 -logFile "$PWD/Artifacts/gauntlet-player.log"
# In another terminal, while the game is at its initial Start screen:
python3 Automation/gauntlet-user-journey.py --measure
# A fresh launch without --measure records the screenshot journey instead.
```

The native driver requires a graphical XWayland session, one SIGNAL 47 window, Python 3, libX11/libXtst and existing access to `/dev/uinput`. It stops if focus is lost and destroys its temporary input devices on exit. Do not use the keyboard/mouse during that run. It does not install software or change device permissions. The build wrapper is syntax-checked; its editor-build, stamping and packaging steps were executed separately for the delivered build.

Playable package: `Artifacts/SIGNAL-47-Gauntlet-Linux.tar.gz`; extract into a new directory and launch `Signal47.x86_64`. The previous `Artifacts/Linux` output and original source assets remain available. The shipped package contains third-party notices and the font license. The repository branch remains separate and unmerged.

This is a verified bounded prologue increment, not a complete game. The next bounded slice is improving the central CRT/keyboard/chair silhouettes against the existing full-room concept, followed by an actual sound-balance review and the same native-input/performance gates.


### Final regression and package checkpoint

[GitHub run 34408377787](https://github.com/Tombonator3000/SIGNAL-47/actions/runs/34408377787) succeeded on source commit `123a88a4883c600c9e5ca935b3ecec88932ad649`; all 54 method-driven/physics regression assertions passed. This development/headless run is separate from the measured release journey. [Stored results](Evidence/Gauntlet04/ci-result.json) preserve the individual assertions, including handset lift/return, desk reachability and event timing. GitHub artifact upload was skipped as configured; the archive is available locally.

A further native-input capture run on the unchanged release shows the handset [before answering](Evidence/Gauntlet04/phone-before-answer.png) and [lifted after E](Evidence/Gauntlet04/phone-lifted.png), from the same camera. Both frames were inspected unaltered: the three handset parts lift together, leaving the cradle exposed. The static coiled cord remains a simplified detail; hand animation and full cord deformation are not implemented.

The raw CSV was independently recalculated and matched the reported frame counts, average and percentiles. The packaged archive was read back and its entire payload matched the stored build hash; executable permissions and both notice/license files were present. [Delivery verification](Evidence/Gauntlet04/delivery-validation.json) records the result and archive SHA-256 `8553d7d184c86065acc5b4cf87db9b566f4749ba23c4de58e3bd1beeb5a1254b` (58,583,792 bytes). The previous main archive remains untouched.

The separate screenshot journey also passed all 14 checkpoints; [complete capture states and images](Evidence/Gauntlet04/Journey/journey-result.json) are preserved beside one another. The dedicated runner was verified `online`, `busy: false`, with its systemd service active after CI.
