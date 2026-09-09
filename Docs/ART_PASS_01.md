# First Unity art and signal-feedback pass — 2026-09-09

## Result

Original OBJ geometry is retained. Blender exports nine refined FBX families into Assets/Signal47/Art/Refined with bevelled edges, weighted normals and named parts. Unity assigns materials by part, including receiver LEDs. The source is Blender/SIGNAL47_refined_assets.blend; regenerate with Blender/Source/refine_export.py.

The room gains tiled linoleum, ceiling grid, physical light diffusers, wall rails, drawer details, chairs, a clipboard table, hooded desk lamp, extra keyboards and two status monitors. The reference remains the original control-room v02 image. Thin dish surfaces now render from both sides, restoring the silhouettes outside the windows. The center chair is offset to keep the console approach open.

Receiver feedback now responds to frequency, gain, bandwidth and azimuth. The curve shows a carrier peak, bandwidth window and tuning marker. A low-volume positional carrier tone strengthens with reception and pulses 4/7 after pattern lock. Fine tuning changes frequency in 0.001 MHz steps. Existing pass thresholds and story order are unchanged. Texture pixel buffers and screen material are reused instead of allocating each update.

## Verification

Local Unity Linux build succeeded. The graphical smoke scenario now includes 21 assertions: the previous complete prologue and restart coverage, independent feedback response for all four instruments, strong aligned reception, controllable receiver LEDs and a raycast confirming that furniture does not obstruct console interaction. Actual in-game screenshots were inspected, then label sizes, standby screens, keyboards and light placement were corrected.

This is still a prototype art pass. It does not claim final sound mixing, exhaustive manual movement tests or a 60 FPS performance guarantee. The automated scenario invokes interaction methods and uses fixed instrument values; the user's previous manual play confirmation applies to the preceding build.

GitHub workflow validation is recorded separately below once the run completes.

First runner run 34393426649 compiled successfully but the Linux player crashed in SDL mouse initialization because a service has no display backend. A reproduction with DISPLAY/WAYLAND_DISPLAY/XDG_RUNTIME_DIR unset confirmed that explicitly selecting `SDL_VIDEODRIVER=dummy` avoids that startup crash. The headless test also disables audio output. This only changes the automation launcher; interactive players retain graphics and audio. SDL driver selection is documented at https://wiki.libsdl.org/SDL2/SDL_HINT_VIDEODRIVER .

The same run hit the GitHub account artifact-storage quota. No existing artifacts were deleted. Upload is now an optional workflow input, off by default; the build and logs remain on the local runner. The interactive Linux package is also available in this working copy's Artifacts directory.

Final result: graphical local run and display-free local reproduction each passed all 21 assertions. Corrected GitHub run https://github.com/Tombonator3000/SIGNAL-47/actions/runs/34393843133 completed successfully in 3m16s, built the Linux executable and passed the same scenario. Upload was intentionally skipped; the runner generated Artifacts/SIGNAL-47-Linux.tar.gz (62 MiB) and retained logs locally. Source commit tested: eb13745.
