# Current verification

The latest scoped result is [service yard 07](SERVICE_YARD_07.md): 22 native checkpoints, inspected release images and the 167.814-second frame-time benchmark. [Control room 06](CONTROL_ROOM_PASS_06.md) records the preceding merged visual/audio/pacing improvement. Both preserve raw evidence and tested input hashes. Earlier results below are historical.

See [Gauntlet 04](GAUNTLET_04.md) for the earlier native keyboard/mouse journey, repaired menu input and Linux pacing, authored story phone, inspected release screenshots and the 138-second performance benchmark. That evidence is separate from earlier method-driven smoke checks. Remaining limits include subjective audio review, full-room concept polish and performance on other devices.

See [asset pass 03](ASSET_PASS_03.md) for the imported graphics/audio and current checks, [investigation pass 02](INVESTIGATION_PASS_02.md) for the latest event timing, evidence and interaction checks, and [art pass 01](ART_PASS_01.md) for the earlier CI results. The remainder of this file records the initial recovery checks.

# Prologue verification — 2026-09-09

## Recovery

Imported original SIGNAL_47_Unity_U1.zip without overwriting another project. Preserved original code, geometry, reference images and legacy prototype files. Source archive remains in the user's Downloads directory.

## Repairs

- Enabled built-in Audio and IMGUI modules missing from the original package manifest. Enabled ScreenCapture for development QA.
- Prevented automatic scene rebuilding during batch import; explicit build command generates the scene.
- Player faces the room instead of the rear wall; gravity is independent of walking speed.
- Pause freezes game time and audio; restart restores time/audio and clears console state.
- Escape exits console before opening another modal.
- Console scales for smaller windows, separates sliders from buttons, has an opaque background and shows the full frequency label.
- Corrected CRT and receiver orientation, brighter work lights, unobstructed exterior view with window collision.
- Corrected clipped start heading.

## Executed checks

Unity generated SARO_Prologue and compiled a Linux development build successfully. Blender 4.5.13 LTS imported all nine original OBJ assets and saved the source blend file.

The development executable's automated scenario checks: session/console initialization; unpowered console rejection; powered console access; wrong parameter rejection; all three profile stages; direction solve/printout; phone ring and answer; pause freezing; delayed future sequence; mug break; title; restart; cleared modal state and restored timescale. The first graphical run passed all 14 assertions, with no captured Unity Error/Exception/Assert. Screenshots exposed visual issues listed above, which were corrected for the follow-up run.

## Limits

The scenario invokes interaction methods and sets instrument values programmatically. It is not a manual mouse/keyboard playthrough, nor a measured performance benchmark. Exact 47-second timing to a single frame, movement/collision coverage and subjective sound quality are not fully certified. Art remains prototype quality; no claim of final lighting or 60 FPS. The original procedural sounds and story sequence are retained.

Local logs and game screenshots are in Artifacts/. Build archives preserve executable permissions. GitHub workflow is manual only.

Additional visual repairs: an explicit CRT display surface carries the generated spectrum while preserving the original OBJ housing; the printout uses a light paper background. Automated runs explicitly continue in the background so desktop focus changes cannot stall QA. The player’s normal focus behavior is unchanged.

Final verification: the last Linux build completed successfully and the final graphical run exited 0 with SIGNAL47_SMOKE_PASS (all 14 assertions). Actual screenshots were inspected; the CRT now displays the spectrum and menu/control text fits. The manual GitHub build workflow has been added but has not yet been executed; these build and graphical checks ran directly on the PC.
