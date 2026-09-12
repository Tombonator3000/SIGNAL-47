# Unity chapter validation on Linux

Use for an existing Unity desktop game that adds persistent photographic evidence, chapter progression, or legacy IMGUI settings on Kubuntu/XWayland. This is a tested workflow, not a connected editor/input service.

## Inputs and prerequisites

Identify the authoritative checkout, exact Unity/package versions, retained release, current local changes, normal player entry point, and acceptance criteria. Establish an isolated save/archive directory, a working native keyboard/mouse path, read-only runtime observation and a single exclusive lock for graphical tests. Keep editor/build jobs separate from input runs and performance measurement.

## Workflow

1. Freeze the gameplay build identity using both the source tree and full player payload hashes. The small Linux executable alone does not identify Unity game content. Later evidence/documentation commits may have a different Git ID while retaining identical verified game sources.
2. Capture photographic evidence from the actual scene at exposure time. Store camera/world-method metadata, stable evidence IDs and image SHA-256. Development and reopening reveal the stored image; they must not recapture the current world. Serialize completed export metadata after encoding/hash computation using a detached copy; do not let a worker thread mutate live Unity state.
3. Exercise a normal input journey. Quit the actual process before development and after the control exposure, then Continue in new processes. Compare image bytes and metadata across checkpoints. Treat save-file edits and direct game-method tests as separate fault/contract checks, never as normal-journey proof.
4. Test unreadable primary with valid backup, invalid primary and backup, missing images, duplicate evidence IDs and unwritable storage in isolated copies. Preserve the previous durable file on failure. Queued checkpoint work and an in-flight write both need draining before Quit; a failed save must leave the player a usable recovery/exit choice.
5. Inspect the real settings overlay, native fullscreen and mouse movement after relaunch. In the tested Unity 6000.3/XWayland setup, boolean fullscreen left a 1280×800 render surface inside a 1920×1080 window and legacy IMGUI scaled input incorrectly. Requesting native FullScreenWindow resolution and remembering the prior window dimensions fixed the actual clicks. Verify on the current compositor; do not compensate in the test by clicking different invisible coordinates.
6. Check scene-save callbacks when authored lighting changes disappear. In the tested project, an older dressing hook ran after the new lighting pass. Make the ordering explicit/idempotent and inspect a fresh runtime image. Repeatedly increasing light intensity before checking callback order did not address the cause.
7. On the tested KDE Wayland desktop, X11 root GetImage capture failed and root-window focus requests were ignored. The installed Spectacle active-window command (`--background --nonotify --activewindow --no-decoration --no-shadow --output`) produced original native client images. Normal Alt+Tab and guarded reactivation proved actual focus loss/return. Use current local tool help and an authorized desktop session; do not treat these as portable APIs.
8. Separate unchanged runtime screenshots, native interaction evidence, audio-output measurements, listening, and frame measurements. Warm up, include real photo capture/export and scene/UI transitions, retain every recorded frame interval, and report loading separately under a declared boundary. GPU timing may be unavailable even when CPU timing works.

## Expected evidence

Native new-game-to-ending and actual process restart reports; exact immutable photo hashes; readable scene/image comparisons; isolated failure cases with recovery; settings persistence and real mouse response; full-frame performance CSV with conditions; packaged payload manifest and an extracted normal-start check. No green score substitutes for any required missing gate.

## Proven scope and limits

Applied in SIGNAL / 47 Chapter09 on 2026-09-10, Unity 6000.3.22f1/URP 17.3.0/Input System 1.20.0, Blender 4.5.13, Kubuntu/XWayland/Intel ARL. The passive complete journey and two process restarts, native fullscreen/settings correction, immutable photo checks and scene-save ordering were observed on source revision 9329a2d. Generalization to another Unity version, compositor, platform or project remains to be tested. Subjective listening and external player discovery are not established by automation.

Project evidence: canonical private Tombonator3000/SIGNAL-47 repository, Docs/CHAPTER_09.md and Docs/Evidence/Chapter09/. This local reference does not synchronize itself to another machine or install a new integration.
