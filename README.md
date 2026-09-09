# SIGNAL / 47

First-person cosmic investigation at SARO, New Mexico, 1986.

## Play on Linux

Run `Artifacts/Linux/Signal47.x86_64` from the local checkout, or extract `SIGNAL-47-Linux.tar.gz` from a successful **Build and test SIGNAL 47 Linux** GitHub Actions run and launch `Signal47.x86_64`.

WASD moves, mouse looks, E interacts, Tab opens the notebook and Esc closes the console or pauses. Start with the shift clipboard and receiver bank. The CRT reference card explains calibration, interference rejection and the anomalous carrier. Finish the direction solve, inspect the printout, answer the phone and remain in the room for the ending.

## Source and tools

`Unity/` contains the original `SIGNAL_47_Unity_U1.zip` project, retrieved by the user from the original ChatGPT conversation and imported on 2026-09-09. The nine original OBJ assets are reused. Blender successfully imported all nine and saved `Unity/Blender/SIGNAL47_prototype_assets.blend`.

Unity 6000.3.22f1, URP 17.3.0, Input System 1.20.0; Blender 4.5.13 LTS. Open `Unity/` in Unity Hub and open `Assets/Signal47/Scenes/Prototype/SARO_Prologue.unity`.

## Build and test

```sh
bash Automation/build-linux.sh
```

Requires the configured Unity editor and Linux build support. `UNITY_EDITOR` may override the local editor path. The manual GitHub workflow uses the dedicated `signal47-kubuntu` runner and uploads a compressed Linux build with logs. Builds are development builds; the automated scenario runs only when `--signal47-smoke` is supplied.

See `Docs/VALIDATION.md` for test scope and remaining limitations. This is the recovered first playable prologue, not final art or a complete game.
