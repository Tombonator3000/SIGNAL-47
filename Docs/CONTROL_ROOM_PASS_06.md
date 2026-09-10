# SIGNAL / 47 — control-room pass 06

Status: VERIFIED for the bounded criteria below. This is a bounded prototype increment. Existing prologue interactions, signal constants, evidence and array sequence remain the acceptance baseline. Merge is authorized by the user after verification.

## Scope and acceptance

The player works a night shift, powers the receiver, separates three signals, reads a physical printout and receives a telephone recording of an event that occurs 47 seconds later. This pass improves that same journey before an exterior investigation is added.

- Green analysis, amber tracking and blue standby CRTs are visibly distinct; text fits physical screens and is readable at the workstation.
- Individual keyboard keys and chair upholstery improve silhouettes without adding colliders to the upgrades. Night lighting retains readable routes.
- Recorded effects have documented, separate gains. Output must be nonzero with no full-scale samples in the sampled audit; subjective listening remains UNVERIFIED.
- The full native-input journey, restart and relevant development smoke checks pass.
- Same release, Intel ARL, OpenGLCore, 1280×800 Ultra: at least 120 seconds after warm-up, p95 ≤17.2 ms, p99 <20 ms, no frame >50 ms; no frames removed. GPU timing remains UNVERIFIED when the API reports zero.

Representation: full 3D, first-person free camera. Delivery: offline Linux desktop. Controls remain WASD/mouse, E, Tab and Escape. No new services, packages or asset purchases.

## Working pipeline

| Responsibility | Tool | Input → output / integration | Status and reason |
| --- | --- | --- | --- |
| Game, build and runtime | Unity 6000.3.22f1, URP 17.3.0, Input System 1.20.0 | Existing C# and licensed assets → regenerated SARO_Prologue → Linux player | Local compilation and real input VERIFIED; preserves existing stack |
| Editable art | Unity scene-generation code, existing VT323 font | Named furniture/text/light definitions → scene objects and materials | Actual screenshots inspected; no new raster concept required |
| Audio | Existing licensed clips, ffmpeg and Unity AudioListener | PCM source-level analysis → separate gains → sampled output report | Source analysis VERIFIED; sampled output audit PASS |
| Verification | Existing signal47-kubuntu runner and local automation | Commit → smoke → release journey → six screenshots → raw timing CSV | Single desktop lock; full runner verification PASS |

Visual targets remain Docs/VisualTargets/control-room-v1.png and phone-desk-v1.png. These are concepts, not runtime proof; this increment does not claim the whole room matches their final detail level.

## Changes and review

Text now uses the existing VT323 font and fits measured local bounds, replacing the previous arbitrary scale. The first visual trial overflowed the CRTs; it was rejected and corrected. Original trial images are retained locally under Artifacts/Pass06-Iteration01. Final immutable evidence is under Docs/Evidence/ControlRoomPass06/candidate/run-34471305834.

Each keyboard now has separate key silhouettes; chair pads use a muted fabric material. Ceiling lamps are reduced from 2.0 to 0.20–0.45 intensity, and two shadowless task spots establish warm/cool desk areas. No interaction transforms or upgrade colliders changed.

The instrumented baseline (Docs/Evidence/ControlRoomPass06/LocalBaseline) measured 133.61 seconds, 59.986 fps average, p95 19.450 ms, p99 20.003 ms, one 50.865 ms frame. Main-thread work averaged 1.477 ms; reported presentation/target wait averaged 15.186 ms. The slowest frame occurred around restart. CPU work was low enough that reducing visual quality was not supported by this evidence. The new Linux cap follows the display refresh rate within 60–120 fps (75 on the tested display), and restart uses asynchronous scene loading. The spectrum calculates its unchanged lock quality once per render instead of once per column.

Unity documents software frame limiting as susceptible to microstutter: [Application.targetFrameRate](https://docs.unity3d.com/6000.3/Documentation/ScriptReference/Application-targetFrameRate.html). The previous GLX/vsync stall is why this pass preserves vSyncCount=0. Other displays and graphics APIs require separate validation.

## First audio balance

All original license notices and source audio remain intact. Linear AudioSource/one-shot gains:

| Sound | Before | After | Reason |
| --- | ---: | ---: | --- |
| UI click | .35 | .22 | Softer confirmation |
| Receiver switch | .35 | .12 | The source has much greater average energy than the click |
| Printer | .25 | .42 | Physical paper feed must read clearly |
| Telephone ring | 1.0 | .68 | Reduce prominence without losing the call cue |
| Telephone recording | 1.0 | .90 | Keep the important event audible |
| Room hum | .45 | .36 | Space for interactions |
| Exterior wind | .24 | .16 | Lower competing bed |
| Ceramic impact | .85 | .70 | Preserve event contrast with more headroom |
| Title music | .28 | .24 | Retain ending without an abrupt level jump |

Source-level measurements are in the final evidence package. AudioOutputAudit samples 2048 samples about every 40 ms, channel 0. It can detect observed peaks and silence, but windows can overlap or leave gaps. It is separate from the performance test and does not prove continuous absence of clipping, speaker playback or subjective quality.

## Reproduction and remaining work

Run `REVIEW_PHASE=candidate bash Automation/run-pass06.sh` on the graphical desktop with no other SIGNAL 47 player. It acquires the desktop lock, builds development smoke and a release, exercises the real journey, collects a separate screenshot/audio journey, then six fixed review cameras. Fixed camera positions do not prove outdoor traversal. The inactive motel remains only a visual blockout.

Final run [34471305834](https://github.com/Tombonator3000/SIGNAL-47/actions/runs/34471305834) passed development smoke, 14 native journey checkpoints and sampled audio in seven required states. The same release measured 137.06 seconds / 10,277 frames: 74.980 fps average, p95 15.886 ms, p99 16.192 ms, maximum 40.520 ms and zero stalls above 50 ms. Conditions: Intel Core Ultra 5 225U / Intel ARL, OpenGLCore, 1280×800 Ultra, 75 Hz display, target 75 fps, vSync 0. No frames excluded.

Visual self-review PASS for this increment: original 01-control-room-array, 02-workstation, 03-crt-detail, journey-09, journey-12 and journey-13 were inspected. CRT text fits the screens; key silhouettes, chair pads and suspended practical lamps are present; route and modal controls remain readable. This is still prototype art: exterior geometry/detail and complete concept fidelity are unfinished. GPU timing and subjective listening remain UNVERIFIED. The audio sample windows show nonzero output and no full-scale samples, not continuous audio verification.

Tested source revision: 9022f203d755081cb6e4b6617b9a268b90223501 plus builder output. The generated scene has been copied byte-for-byte from that CI checkout into source. The complete Unity input hash is verified equal to the tested build: eb4d673036daa15bcee21be46ddce8532170892e47696c2becfd2be4cdedfbb9. Later evidence/documentation commits do not change that input. Next bounded scope: a playable route from the control room into the service yard with one concrete investigation task, followed by its own full verification. Motel gameplay is later.
