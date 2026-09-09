# SIGNAL / 47 — Gauntlet log

## U0/U1 migration pass
**Goal:** move the already-proven vertical slice from browser WebGL to a Unity-first source without losing its gameplay truth.

### BUILD
- Unity project skeleton created.
- Input System first-person controller.
- Raycast interaction contract.
- Procedural/ported physical assets.
- Signal profiles and signal-console gameplay.
- Prologue director with deterministic timing.
- Notebook, paper inspection, pause/restart/title.
- Procedural audio placeholders.
- Auto scene builder.

### REUSE
- `game.js` timing and thresholds retained.
- `graphics.js` proportions / positions used for Unity layout.
- v0.2 control-room screenshot kept as art target.
- Procedural geometry converted into OBJ assets for Unity/Blender.

### STATIC VERIFY
- All expected C# files and model assets generated.
- Prototype story constants checked against source.
- OBJ files load through trimesh after export.

### NOT YET VERIFIED
- Unity C# compile.
- URP material import in the installed 6.3 editor.
- Input feel, collision, render performance and actual frame rate.
- Runtime scene-builder import order.

### NEXT GAUNTLET GATE
Open in Unity 6.3 LTS, allow import, rebuild scene from menu if required, run from 23:41 through title, capture in-game screenshot + Console output, then repair every compile/runtime/visual regression before adding any new location.
