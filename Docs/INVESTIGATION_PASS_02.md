# Investigation pass 02 — 2026-09-09

## Playable changes

- The printer feeds out a visible sheet over 1.2 seconds. Reading it files the distance result as evidence; the console does not reveal -39 LY in advance.
- The telephone handset shakes when ringing, lifts during the call, and settles when the connection ends. Its parts reuse the original refined phone mesh.
- E picks up the coffee mug; E again returns it to its original desk position. The impact drops it from the camera when held, or slides it off the desk when left alone. The resulting fragments can be examined.
- The notebook timestamps and deduplicates observations, scrolls, and allows collected documents to be reopened. Telephone notes are explicitly written observations, not a recorded audio asset.
- The impact uses an absolute deadline of 47 game seconds after the line goes dead, avoiding accumulated waits. Pause freezes the deadline. The call uses the same generated impact and ceramic break waveforms, separated by the same 0.7-second interval as the later event.
- Eleven dishes begin moving with a short stagger and finish at the same azimuth/elevation. The title waits for the last dish to finish.

## Source preservation

Original OBJ assets, source ZIP, other projects, and the separate runner are unchanged. The scene builder regenerates only this project's prologue scene. Blender exports remain the existing assets; the new handset pivot is added to the scene.

## Verification

The graphical Linux scenario passed 45 assertions with exit 0 and SIGNAL47_SMOKE_PASS. Coverage includes paper-feed readiness, evidence collection without duplicates, handset lift/return, mug pickup/return and both impact branches, the 47-second deadline (within a 0.25-second frame tolerance), pause, all eleven dish headings, and restart. The final paper position and text orientation were checked in screenshots, and the final graphical build again passed all 45 assertions. Automated interactions are invoked programmatically, with screenshots from the actual Linux executable; this is not a manual keyboard/mouse playthrough or a performance benchmark.

The first graphical attempt hit its watchdog while desktop presentation throttled the test. The QA harness now disables vertical synchronization, caps rendering at 60 FPS, and allows 180 seconds; normal game settings are unchanged. The subsequent graphical scenario passed.

## Local commands and outputs

Working directory: `/home/tombonator3000t/.codex/.chatgpt-projects/g-p-6aa05604a4708191a65139c1d5f89363/SIGNAL-47`.

```sh
/home/tombonator3000t/Unity/Hub/Editor/6000.3.22f1/Editor/Unity -batchmode -nographics -quit -projectPath "$PWD/Unity" -executeMethod Signal47.Editor.Automation.BuildLinux -logFile "$PWD/Artifacts/story-build.log"
./Artifacts/Linux/Signal47.x86_64 --signal47-smoke -screen-fullscreen 0 -screen-width 1280 -screen-height 800 -logFile "$PWD/Artifacts/story-play.log"
tar -czf Artifacts/SIGNAL-47-Linux.tar.gz -C Artifacts/Linux .
```

Generated scene: `Unity/Assets/Signal47/Scenes/Prototype/SARO_Prologue.unity`. Playable executable: `Artifacts/Linux/Signal47.x86_64`. Screenshots and result: `Artifacts/Screenshots/`.

## Runner environment repair

GitHub run 34397231080 compiled successfully but stalled in native FMOD initialization, before the managed watchdog. Its systemd process lacked XDG_RUNTIME_DIR and PULSE_SERVER. It was canceled. A local headless launch with `/run/user/1000/pulse/native` explicitly selected reached the test immediately and passed all 45 assertions with exit 0. The build script now discovers this same-user socket if present and supplies its environment; a 240-second process timeout with a 10-second termination grace period also bounds hangs before Unity scripting starts. No system sound configuration or other runner was modified.

```sh
gh workflow run build-linux.yml -f upload_artifact=false
gh run cancel 34397231080
SDL_VIDEODRIVER=dummy PULSE_SERVER=unix:/run/user/1000/pulse/native XDG_RUNTIME_DIR=/run/user/1000 ./Artifacts/Linux/Signal47.x86_64 -batchmode -nographics -noaudio --signal47-smoke -logFile "$PWD/Artifacts/story-headless.log"
```

## Final GitHub verification

[Run 34397712654](https://github.com/Tombonator3000/SIGNAL-47/actions/runs/34397712654) completed successfully on commit `9737357`. Unity built the Linux player, the headless scenario passed all 45 assertions, and the Linux archive was created. The dedicated `signal47-kubuntu` runner was verified online and idle afterward. GitHub artifact upload remained off because of the existing account storage limit; both local and runner copies remain available on the PC.

Final local artifact: `Artifacts/SIGNAL-47-Linux.tar.gz` (62 MiB), executable permission verified in the archive. Its checksum is in `Artifacts/SIGNAL-47-Linux.sha256`. Graphical and headless local logs both contain `SIGNAL47_SMOKE_PASS`.
