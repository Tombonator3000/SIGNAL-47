# SIGNAL / 47

First-person cosmic investigation at SARO, New Mexico, 1986.

## Current status

Dedicated runner `signal47-kubuntu` is online. Unity 6000.3.22f1, Blender 4.5.13 LTS, Git and Python passed the manual runner check. A separate local Unity project-creation probe passed on 2026-09-09, confirming working editor licensing.

The original project archive `SIGNAL_47_Unity_U1.zip` was found in the ChatGPT conversation “Idéer om kosmisk etterforskning”. Chrome blocked its download with `ERR_BLOCKED_BY_CLIENT`. The source project has not yet been imported or compiled. No replacement assets have been created.

Once the original ZIP is available:

```sh
python3 Automation/import_unity.py /absolute/path/SIGNAL_47_Unity_U1.zip
```

This creates `Unity/`, preserves original source files and refuses to overwrite an existing project. Importing the original must precede project-specific changes.

## Next verification gate

1. Inspect original scripts, scene builder and model assets.
2. Import and compile with Unity 6000.3.22f1; repair actual compile errors.
3. Build `SARO_Prologue` using the existing scene builder.
4. Test movement, interactions, calibration, interference, anomaly, printer, phone and the 47-second sequence through the title card.
5. Produce a Linux build and actual game screenshots before the first graphics pass.

## Story constants from the original design

Calibration 1419.900 MHz; interference 1420.110 MHz; anomaly 1420.405 MHz; pulse pattern 4 / 7; distance -39 LY; future sound followed by the corresponding impact 47 seconds later.

Do not expand into driving, a large world or NPC systems before this prologue works end to end.
