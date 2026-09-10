# SIGNAL / 47 — service yard investigation 07

Status: IMPLEMENTED, verification in progress. Continues the verified pass06 Linux first-person prototype, with the same Unity 6000.3.22f1 / URP 17.3.0 / Input System 1.20.0 stack. No asset purchases or new packages. The previous source tree is preserved; this branch extends the canonical generated scene.

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

Run `bash Automation/run-service-yard.sh` on the graphical PC with no other player. It locks desktop testing, builds development smoke and release, runs a separate measured journey and screenshot/audio journey, then fixed visual regression cameras. Evidence is written to Artifacts/Pass07. Fixed cameras do not substitute for actual outdoor journey images. Final results will be recorded after inspection.

First native-input trial: prologue, Continue, door opening and room exit passed, but the open south-hinged leaf blocked the turn along the walkway at x10.60/z2.73. Corrected by hinging at the north jamb and opening toward the end rail. Failure evidence is retained locally in Artifacts/Pass07-DoorBlocked. This is a real traversal finding, not an assumed visual issue.

Second trial: all 22 native checkpoints and frame-time gates passed (167.38 seconds, 74.983 fps, p95 15.927 ms, p99 16.241 ms, max 40.296 ms). Visual review rejected weak practical light pools and unlit-looking lamp faces. Preserved under Artifacts/Pass07-ReadableRouteDimLights. Correction keeps URP's four-light budget, splits the long path mesh into short slabs, concentrates the warm spotlights and uses explicit unlit diffuser faces. The corrected build requires fresh visual/performance evidence.
