# SIGNAL / 47

First-person cosmic investigation at SARO, New Mexico, 1986.

## Play on Linux

Run `Artifacts/Linux/Signal47.x86_64` from the local checkout, or extract `SIGNAL-47-Linux.tar.gz` from a successful **Build and test SIGNAL 47 Linux** GitHub Actions run and launch `Signal47.x86_64`.

WASD moves, mouse looks, E interacts, Tab opens the notebook and Esc closes the console or pauses. Start with the shift clipboard and receiver bank. The CRT reference card explains calibration, interference rejection and the anomalous carrier. Finish the direction solve, wait for the physical paper feed, inspect the printout, answer the phone and remain in the room for the ending. E picks up the coffee mug and returns it to the desk. Collected documents can be reopened from the scrollable notebook; observations carry local timestamps.

## Latest pass

The control room now has refined Blender meshes, differentiated materials, tile and ceiling detail, auxiliary screens and furniture. Signal strength, spectrum and a positional receiver tone respond to tuning; fine adjustment buttons step by 0.001 MHz. See `Docs/ART_PASS_01.md`. The investigation pass adds physical paper, an animated handset, a holdable mug, collected evidence and a synchronized 47-second event sequence; see `Docs/INVESTIGATION_PASS_02.md`.

## Source and tools

`Unity/` contains the original `SIGNAL_47_Unity_U1.zip` project, retrieved by the user from the original ChatGPT conversation and imported on 2026-09-09. The nine original OBJ assets are reused. Blender successfully imported all nine and saved `Unity/Blender/SIGNAL47_prototype_assets.blend`.

Unity 6000.3.22f1, URP 17.3.0, Input System 1.20.0; Blender 4.5.13 LTS. Open `Unity/` in Unity Hub and open `Assets/Signal47/Scenes/Prototype/SARO_Prologue.unity`.

## Build and test

```sh
bash Automation/build-linux.sh
```

Requires the configured Unity editor and Linux build support. `UNITY_EDITOR` may override the local editor path. The manual GitHub workflow uses the dedicated `signal47-kubuntu` runner and saves a compressed Linux build with logs on the PC. GitHub upload is optional (off by default because account artifact storage was full). The headless test uses SDL’s dummy video driver and disables audio output; graphical/audio runs remain separate. Builds are development builds; the automated scenario runs only when `--signal47-smoke` is supplied.

See `Docs/VALIDATION.md` for test scope and remaining limitations. This is the recovered first playable prologue, not final art or a complete game.
