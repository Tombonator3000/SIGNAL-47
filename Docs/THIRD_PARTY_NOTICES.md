# SIGNAL / 47 — third-party resources

## NightSky12 — NASA catalogue map

[Deep Star Maps 2020](https://svs.gsfc.nasa.gov/4851/), visualization by Ernie Wright (USRA).
Credit: NASA/Goddard Space Flight Center Scientific Visualization Studio. Gaia DR2: ESA/Gaia/DPAC.
The map also uses Hipparcos-2, Tycho-2, Yale Bright Star, UCAC3 and XHIP catalogue data.

Source file: `starmap_2020_8k.exr`. The game's `NASA_DeepStarMap_8k.png` is an 8192×4096 sRGB derivative, re-encoded in Blender; shader exposure, atmosphere and orientation are artistic adaptations. It is not an astronomically dated reconstruction of New Mexico in 1986. No constellation diagrams, NASA logos or identifiable persons are included. Source and derivative hashes, the conversion and its measured error are in `Art/NightSky12/provenance.json`; the reproducible conversion is `Blender/Source/prepare_night_sky.py`.

Use follows the [NASA media guidelines](https://www.nasa.gov/nasa-brand-center/images-and-media/), not a CC0 designation. Source acknowledgement does not imply NASA endorsement of SIGNAL / 47.

## Poly Haven — CC0 1.0

- Old Linoleum Flooring 01 — Charlotte Baglioni: https://polyhaven.com/a/old_linoleum_flooring_01
- Metal Office Desk — Ulan Cabanilla: https://polyhaven.com/a/metal_office_desk
- Vintage Radio Transceiver — Mateusz Sadek: https://polyhaven.com/a/vintage_radio_transceiver
- Desk Lamp Arm 01 — Kuutti Siitonen (model/textures), Yann Kervran (rig): https://polyhaven.com/a/desk_lamp_arm_01
- Qwantani Night (Pure Sky) — Greg Zaal and Jarod Guest: https://polyhaven.com/a/qwantani_night_puresky

License: https://creativecommons.org/publicdomain/zero/1.0/
Provider terms: https://polyhaven.com/license
These assets are free and public domain under CC0. Unity materials, texture-channel packing, scale, placement and lighting were adapted for SIGNAL / 47. Discovery/download metadata powered by Poly Haven's public API.

## Freesound — CC0 1.0

- old dot-matrix printer — viertelnachvier: https://freesound.org/people/viertelnachvier/sounds/181420/
- telephonering.wav — transitking: https://freesound.org/people/transitking/sounds/15826/
- ceramic cup shatters on tile floor — geraldfiebig: https://freesound.org/people/geraldfiebig/sounds/524999/
- desert_wind.wav — DarkShroom: https://freesound.org/people/DarkShroom/sounds/645305/

License: https://creativecommons.org/publicdomain/zero/1.0/
Inputs are the publicly available HQ MP3 previews, not the original WAV masters. Effects were excerpted, converted to mono PCM, normalized, faded and, for wind, crossfaded for looping. Ceramic is also mixed into the fictional future telephone call.

## Kenney — CC0 1.0

Interface Sounds: https://kenney.nl/assets/interface-sounds
Used files: click_001.ogg and switch_003.ogg.
License: https://creativecommons.org/publicdomain/zero/1.0/

## VT323 — SIL Open Font License 1.1

Copyright 2011, The VT323 Project Authors (peter.hull@oikoi.com)
Source: https://github.com/google/fonts/tree/main/ofl/vt323
The complete supplied font license and copyright notice are included in VT323-OFL.txt beside the game executable. The font is bundled under its original license.

## Scott Buckley — CC BY 4.0

'Signal to Noise' by Scott Buckley — released under CC-BY 4.0. www.scottbuckley.com.au
Source: https://www.scottbuckley.com.au/library/signal-to-noise/
License: https://creativecommons.org/licenses/by/4.0/
This game uses a 45-second excerpt from the supplied No Piano Melody mix, with fade-in, fade-out and volume adjustment, for the prologue transition and chapter ending. The same credit appears on both screens. Credit the composer and source in the description of videos featuring this music, including trailers.

## Field camera 08 additions

- **Camera_01**, Rajil Jose Macatangay, [Poly Haven](https://polyhaven.com/a/Camera_01), CC0-1.0. Imported body geometry and 1K body/lens-body/strap PBR textures. Prepared in Blender with base origin and metre scale; loose strap retained in the editable .blend and omitted from runtime FBX. URP metallic/smoothness maps pack source metallic and inverted roughness. 22,319 exported triangles. Powered by Poly Haven (asset metadata retrieval).
- **Camera Shutter**, roachpowder, [Freesound 170229](https://freesound.org/people/roachpowder/sounds/170229/), CC0-1.0. Public HQ MP3 preview converted to mono 44.1 kHz WAV, high-pass 100 Hz and non-amplifying limiter at 0.85, played at 0.28 gain. This is not the original master recording.
- Provenance, source hashes and downloaded variants: Unity/Assets/Signal47/Art/ThirdParty/PolyHaven/Camera_01/source-manifest.json.

## Chapter 09 original work

The enlarger, processing trays, sink, stool, bottles and B-12 cabinet are original geometry authored for SIGNAL / 47 in Blender4.5.13. Reproducible source and editable .blend are kept in Unity/Blender; FBX meshes, UV/bounds verification and provenance are in Unity/Assets/Signal47/Art/Chapter09. Project ownership/license applies. The chapter also uses original procedural film-handling, paper-transfer and restrained discovery tones. No new third-party downloads or paid asset service were used for this chapter.

The two generated pictures in Docs/VisualTargets/Chapter09 are design references only. The game's photographs are captured from the actual Unity scene, with authored film-response geometry rendered only during exposure. The reference images are not substituted for gameplay evidence. Existing original imported prototype meshes and all licensed resources above remain preserved.

## Visual pass 10 additions

- Concrete Floor 02 — Rob Tuytel, https://polyhaven.com/a/concrete_floor_02
- Green Metal Rust — Rob Tuytel, https://polyhaven.com/a/green_metal_rust
- Painted Plaster Wall — Amal Kumar, https://polyhaven.com/a/painted_plaster_wall

All three PBR assets: CC0-1.0, https://polyhaven.com/license and https://creativecommons.org/publicdomain/zero/1.0/. Downloaded 2K diffuse/OpenGL normal/roughness textures through the public API; powered by Poly Haven. Colour tint, normal strength and surface-specific roughness remapping are adaptations. Green lakk remains primarily dielectric. Original source and SHA256 are recorded in Unity/Assets/Signal47/Art/Visual10/source-manifest.json. No website preview render is bundled.

Kenney Impact Sounds 1.0 — https://kenney.nl/assets/impact-sounds, CC0-1.0. Used impactMetal_light_000.ogg, impactSoft_medium_000.ogg and footstep_concrete_000/001/002.ogg. Mono import, restrained gain and small step-pitch variation are adaptations. Soft impact is used as film handling contact, not claimed to be a field recording of photographic processing. Full pack license is preserved beside the imported audio.

New drawer benches, rounded lamp shades, utility bin, sink cloth, twin-tube fluorescent fixtures, measuring jug, funnel and photographic tongs are original Blender geometry. Reproducible script, editable source, metric FBX roundtrip checks and license notes are retained under Unity/Blender and Art/Visual10/Models. Existing original models and prior licensed sources remain preserved. Existing Scott Buckley music and on-screen credit are unchanged.

### Visual pass 10 terrain refinement

- **Dry Ground Rocks** — Rob Tuytel, https://polyhaven.com/a/dry_ground_rocks, CC0-1.0. Original 2K diffuse, OpenGL normal and roughness maps are preserved under Unity/Assets/Signal47/Art/Visual10/Textures/dry_ground_rocks. The source surface spans 4 metres. Terrain material tint, tiling and roughness/smoothness adaptation are project modifications.
- **Grass Medium 02** — Rico Cilliers, https://polyhaven.com/a/grass_medium_02, CC0-1.0. The original 1K Blender source is preserved under Unity/Blender/ThirdParty/grass_medium_02; original diffuse, dry diffuse, alpha, normal and roughness maps are preserved under Unity/Assets/Signal47/Art/Visual10/Textures/grass_medium_02. The game's reduced tuft geometry and atlas-based dry-grass presentation are adaptations of this source; they must not be described as wholly original project assets. The original complete scatter configuration is distinct from the runtime tufts. No claim is made that the unspecified grass species is botanically verified as native to New Mexico.

Both assets were retrieved through Poly Haven's public API; powered by Poly Haven. License: https://polyhaven.com/license and https://creativecommons.org/publicdomain/zero/1.0/. Source URLs, authors, original file sizes and SHA256 hashes for all nine added files are recorded in Unity/Assets/Signal47/Art/Visual10/source-manifest.json. Provider preview renders are not bundled as game resources.
