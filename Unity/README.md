# SIGNAL / 47 — Unity vertical slice migration

This is the Unity migration of the playable web prototype. The original game logic, geometry proportions and visual reference are preserved under `Docs/LegacyPrototype` and `Assets/Signal47/Art/References`.

## Open
1. Add this folder in Unity Hub.
2. Open with **Unity 6000.3.22f1 (Unity 6.3 LTS)**. The package versions match the existing Unity project already used on this account: Input System 1.20.0, URP 17.3.0, UGUI 2.0.0.
3. On first import, `Signal47SceneBuilder` automatically creates `Assets/Signal47/Scenes/Prototype/SARO_Prologue.unity`.
4. If it does not, run **SIGNAL 47 → Build or Rebuild Vertical Slice**.
5. Open the scene and press Play.

Controls: WASD, mouse, E interact, Tab notebook, Esc pause/close.

## Prototype behavior preserved
- Player starts at `(0, 6.7)` in the original room footprint.
- Receiver → calibration → interference → 1420.405 MHz anomaly → 4/7 pulse → -39 LY printout.
- Phone starts after 3.6 s.
- Future-call audio ends after 4.3 s, then the hidden event waits exactly 47 s.
- Boom → mug drop/break → dish synchronization → SIGNAL ACQUIRED → title.
- The five primary interaction locations and room proportions are ported from `game.js` / `graphics.js`.

## Ported prototype assets
The browser prototype generated its 3D content in code rather than as FBX files. This migration converts the useful geometry into real OBJ assets Unity can import: control desk, CRT terminal, receiver rack, printer, telephone, mug/intact + broken, radio dish bowl + pedestal. These can also be opened and refined in Blender.

## Original migration validation note (superseded)
The current execution environment did not expose a Unity or Blender binary, so the project files/assets were generated and statically checked here but have **not** been compiled or played inside the Unity Editor yet. The first Unity Editor import is the next required Gauntlet gate.

## Local verification update — 2026-09-09

The project has now compiled and produced a Linux development build on the user’s PC. The first automated prologue run passed through the title and restart. Subsequent visual and interaction repairs are documented in `../Docs/VALIDATION.md`.
