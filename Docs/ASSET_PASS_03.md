# Asset pass 03 — 2026-09-09

## Implemented

Twelve selected sources/packs from FREE_ASSET_CATALOG.md are now used by the Unity prologue:

- Poly Haven: metal office desk (phone desk and radio workbench), vintage radio transceiver, articulated desk lamp, old linoleum floor and Qwantani night sky. Models use 1K material maps; floor and sky use 2K. URP materials combine metalness and inverted roughness into the required texture channels. The lamp's rig guide curves and loose exported wire segments are hidden; its bulb has a separate emissive material and a downward work light.
- Freesound: dot-matrix printer, mechanical telephone ring, ceramic break and desert wind. These are the public HQ MP3 previews, not original WAV masters. The selected clips are converted to mono 44.1 kHz PCM, trimmed, normalized and faded; the wind has an overlapping loop transition.
- Kenney: two Interface Sounds clips for switches and buttons.
- VT323: terminal and title typography, with the original OFL notice bundled.
- Scott Buckley: a 45-second excerpt of the No Piano Melody version of “Signal to Noise,” faded at both ends, only on the final title. Attribution, license link and adaptation notice appear on screen and in the game package.

Printer audio lasts the same 1.2 seconds as the paper feed. The recorded ceramic break is mixed into the future call and played again at the actual impact. The future call remains 4.3 seconds, its impact-to-ceramic interval remains 0.7 seconds, and the later impact still occurs 47 game seconds after the line goes dead. The actual ceramic effect uses a separate positional audio source so it does not truncate the boom's decay.

The new desks have collision boxes and the phone/mug rest on the imported surface. The original prototype meshes and Blender refinements remain in the project. Other projects, synced references and the source ZIP were not changed.

## Test and repair cycle

The requested gauntlet-loop skill/plugin was not installed. This pass used the existing Unity build, graphical scenario, screenshots and GitHub runner to perform the build–test–repair cycle.

- Initial compilation exposed a Unity audio importer API mismatch; preload settings were moved to AudioImporterSampleSettings.
- New reachability checks found that a downward interaction ray could hit the player's own CharacterController within four centimetres. The player now uses Unity's Ignore Raycast layer and interaction rays use Physics.DefaultRaycastLayers. Ordinary movement collision is retained. The tests still require the first hit to be the actual printer or phone.
- Screenshots exposed an excessively bright sky/lamp and visible lamp rig curves. Sky exposure/tint, lamp material assignment, lighting and guide visibility were corrected.
- All 31 downloaded source-file SHA-256 hashes were checked against the import manifest. Four prepared PCM effects have zero clipped samples; measured peaks are 0.65, 0.70, 0.90 and 0.30. The wind's boundary step is approximately 0.00266 of full scale. Audio was checked through file metadata/sample analysis; subjective listening remains outstanding.

The final Unity Linux build succeeded and the final graphical scenario passed all 54 assertions, exited 0, and wrote SIGNAL47_SMOKE_PASS. Actual screenshots were inspected at 1280×800, including terminal text, the phone/lamp, radio workbench and music attribution. The scenario invokes interactions and instrument values programmatically. It includes real physics raycasts and actual player screenshots, but is not a manual keyboard/mouse playthrough or a performance benchmark.

## Files and executed commands

Working directory: `/home/tombonator3000t/.codex/.chatgpt-projects/g-p-6aa05604a4708191a65139c1d5f89363/SIGNAL-47`.

Sources, generated materials and prepared audio: `Unity/Assets/Signal47/Art/ThirdParty/`. Exact URLs, authors, license IDs, byte counts and hashes: `manifest.json` inside that folder. Effect processing: `audio-processing.json`. Full credits: `Docs/THIRD_PARTY_NOTICES.md`.

```sh
python3 Automation/fetch-curated-assets.py
python3 Automation/prepare-curated-audio.py
python3 -m py_compile Automation/fetch-curated-assets.py Automation/prepare-curated-audio.py
/home/tombonator3000t/Unity/Hub/Editor/6000.3.22f1/Editor/Unity -batchmode -nographics -quit -projectPath "$PWD/Unity" -executeMethod Signal47.Editor.Automation.BuildLinux -logFile "$PWD/Artifacts/imported-assets-build.log"
./Artifacts/Linux/Signal47.x86_64 --signal47-smoke -screen-fullscreen 0 -screen-width 1280 -screen-height 800 -logFile "$PWD/Artifacts/imported-assets-play.log"
```

Blender 4.5.13 LTS was also used read-only to inspect the lamp FBX material slots and rig guide objects. No source model was overwritten.

Build logs: `Artifacts/imported-assets-build.log`, `Artifacts/imported-assets-play.log`. Sample measurements: `Artifacts/audio-validation.json`. Screenshots: `Artifacts/Screenshots/`, including the imported workbench in `11-imported-radio.png`. Playable executable: `Artifacts/Linux/Signal47.x86_64`.

## Local package

`Artifacts/SIGNAL-47-Linux.tar.gz` is 84.3 MiB. Its executable permission and included THIRD_PARTY_NOTICES.md / VT323-OFL.txt were verified. SHA-256 is recorded in `Artifacts/SIGNAL-47-Linux.sha256`.

```sh
tar -czf Artifacts/SIGNAL-47-Linux.tar.gz -C Artifacts/Linux .
sha256sum Artifacts/SIGNAL-47-Linux.tar.gz > Artifacts/SIGNAL-47-Linux.sha256
```

## GitHub verification

[Run 34403502258](https://github.com/Tombonator3000/SIGNAL-47/actions/runs/34403502258) completed successfully on commit `580ccbac7b3cb74ea0174ac97eca685a2f589a4c`. The dedicated `signal47-kubuntu` runner built the project from GitHub, passed all 54 headless assertions with SIGNAL47_SMOKE_PASS, and created the Linux archive. This run had artifact upload disabled under the existing account storage limit. The downloadable package in the local checkout is the graphically checked build; the runner has its own build in `/home/tombonator3000t/signal47-tools/actions-runner/_work/SIGNAL-47/SIGNAL-47/Artifacts/`.

```sh
gh workflow run build-linux.yml --repo Tombonator3000/SIGNAL-47 -f upload_artifact=false
gh run watch 34403502258 --repo Tombonator3000/SIGNAL-47 --interval 30 --exit-status
gh run view 34403502258 --repo Tombonator3000/SIGNAL-47 --json status,conclusion,headSha,url
```
