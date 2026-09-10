# SIGNAL / 47 — service yard investigation 07

Status: VERIFIED for the bounded prototype criteria below. Continues the verified pass06 Linux first-person prototype, with the same Unity 6000.3.22f1 / URP 17.3.0 / Input System 1.20.0 stack. No asset purchases or new packages. The previous source tree is preserved; this branch extends the canonical generated scene.

The player follows the unauthorized antenna alignment out of the control room, opens the east service door and checks local controller S-03. Its encoder reports 026 degrees, while no steering command was received. The player files the log and returns inside. This is one small investigation; the motel remains a visual blockout.

## Acceptance before merge

- The existing prologue remains playable; the service door stays physically locked until its ending.
- Continue opens a complete keyboard/mouse route: door, illuminated walkway, readable S-03 cabinet, document, notebook and return to control room. No teleport or test-only activation proves this gate.
- The motor log is collected once, can be reread, preserves earlier evidence and has a recognized return outcome. Restart restores the closed/locked door and empty notebook.
- Original release screenshots show warm pools of light guiding the path, readable world labels and ordinary editable UI. Existing references in Docs/VisualTargets establish the same restrained green/amber night palette and industrial materials. Geometry remains explicitly prototype level.
- Same tested PC/preset as pass06: Intel ARL, OpenGLCore, 1280×800 Ultra; >=120 seconds after warm-up, average >=59 fps, p95 <=17.2 ms (existing timer tolerance), p99 <20 ms, zero frames >50 ms. No excluded frames. GPU timing / subjective listening remain separate if unavailable.

Representation is full 3D with free first-person camera; delivery is offline Linux desktop. WASD/mouse/E/Tab/Escape remain unchanged. Evidence persists within the current playthrough; no disk save is introduced. The ending gains Continue while retaining Restart.

## Pipeline and baseline

The existing authored dishes and licensed VT323 font are reused. ServiceYardPass is the editable source for the doorway, fitted signs, path, rails, three shadowless practical lamps and cabinet; scene builder writes SARO_Prologue and the Linux player imports it. Runtime C# owns progression, animated physical door and evidence. The native journey adds actual walking and raycast interactions. Baseline: Docs/Evidence/ControlRoomPass06/candidate/run-34471305834/Snapshots/05-service-yard-array.png — an exterior camera view, without traversal.

Run `bash Automation/run-service-yard.sh` on the graphical PC with no other player. It locks desktop testing, builds development smoke and release, runs a separate measured journey and screenshot/audio journey, then fixed visual regression cameras. Evidence is written to Artifacts/Pass07. Fixed cameras do not substitute for actual outdoor journey images. Final results and original images are retained in Docs/Evidence/ServiceYard07/final-140bd0c.

First native-input trial: prologue, Continue, door opening and room exit passed, but the open south-hinged leaf blocked the turn along the walkway at x10.60/z2.73. Corrected by hinging at the north jamb and opening toward the end rail. Failure evidence is retained locally in Artifacts/Pass07-DoorBlocked. This is a real traversal finding, not an assumed visual issue.

Second trial: all 22 native checkpoints and frame-time gates passed (167.38 seconds, 74.983 fps, p95 15.927 ms, p99 16.241 ms, max 40.296 ms). Visual review rejected weak practical light pools and unlit-looking lamp faces. Preserved under Artifacts/Pass07-ReadableRouteDimLights. Correction keeps URP's four-light budget, splits the long path mesh into short slabs, concentrates the warm spotlights and uses explicit unlit diffuser faces. The corrected build requires fresh visual/performance evidence.

## Final verification — 10 September 2026

PASS: development smoke, 22 native keyboard/mouse checkpoints in both the measured run and the separate screenshot/audio run, evidence preservation/deduplication, physical outward-and-return route, restart, sampled audio, frame-time gates and scoped visual self-review. The final gate record is `final-verification.json`; `quality-gates.json` retains the automated pre-review output.

Same release and preset: Intel Core Ultra 5 225U / Intel ARL, OpenGLCore, 1280×800 Ultra, 75 Hz display, target 75, vsync 0. After 20 seconds warm-up, all 12,583 frames over 167.814 seconds were included: 74.982 fps average, p95 15.912 ms, p99 16.198 ms, maximum 38.210 ms, zero frames above 50 ms. Engine allocation counter peaked at 94.11 MiB; this is not total OS process memory. GPU timing and subjective listening remain UNVERIFIED. No performance claim for other devices.

The tested base revision was 140bd0cf299af290882cb49a420375896929d81e plus generated scene/material output. The complete checked-in Unity source hash matches the tested player inputs: c47dc1e198a4a8593c2e39bb8a9d0d334a23a2a902f83e0bc031f3b5e7b83708. Full build payload hash: 301863ef18f3337198f11ec3c9438e2f9a99ecf0246a2a08858fe1a62895e88b. Later commits preserve that same generated scene and add records only.

| Visual/experience criterion | Observed gap | Correction and final evidence |
| --- | --- | --- |
| Open route through service door | First leaf obstructed the turn | North hinge opens toward end rail; native journey-15/16 and return checkpoint PASS |
| Warm pools guide the player | First lamps were too weak | Short slabs, focused warm spots, visible diffuser faces; actual player journey-16 PASS |
| Readable controller and paper | Simple prototype cabinet, no detail polish | Fitted S-03 label, complete legible document in journey-18; PASS for readability |
| Evidence and outcome remain usable | None blocking in final inspection | Four documents visible in journey-20; return confirmation in journey-21; PASS |
| Established night art direction | Exterior remains simple geometry | Palette and route readability PASS for this slice; full concept detail remains unfinished |

Native screenshots are actual play, reached with WASD/mouse/E. The six Snapshots images use fixed cameras and explicit setup, including an inactive motel blockout; they are not traversal proof. The optional --signal47-yard-capture is a quick fixed-camera lighting check, not a gameplay test.

The delivered Linux archive is Artifacts/SIGNAL-47-Gauntlet-Linux.tar.gz. Extract into a fresh directory, launch Signal47.x86_64, finish the prologue and choose CONTINUE / SERVICE YARD. Follow the east door to S-03, read its log and return inside. E interacts, Tab reopens the notebook, Escape pauses/closes; Restart clears this playthrough. No disk save is included.

Next bounded scope: choose the next investigation beat, then build its full user journey. Motel gameplay is still unimplemented; additional weathering, environment detail and subjective audio review remain future work.
