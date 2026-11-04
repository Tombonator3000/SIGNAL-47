# SARO ground texture source

Built-in ImageGen, 4 November 2026. Originals are in `photo_sources/`; import PNGs are in `exports/ground/`. All nine originals are 1254 square, resampled with FFmpeg Lanczos to 2048 square for Unreal. Upsampling does not add source detail. `ground_sources.json` records the original files.

## Prompt set

Common albedo request: photorealistic production PBR BASECOLOR, square 2048 x 2048, seamless on all four edges, orthographic overhead, flat albedo without baked directional light, shadows or ambient occlusion. No vegetation, perspective, labels, borders or collage.

- Sand: 2.4 metre patch of loose pale reddish tan New Mexico desert sand, fine grains and subtle wind ripples, slight natural irregularity; light desaturated warm beige, not orange.
- Crust: 3.2 metre patch of hard pale caliche crust, irregular small shallow cracks in compact chalky sandy soil, scattered small embedded pale stones; light warm cream beige without dramatic dark cracks.
- Gravel: 1.6 metre patch of dry desert gravel, sandstone and limestone pebbles in light dusty sand, varied 5 to 35 mm sizes and occasional 50 mm stone; muted beige, warm grey and pale reddish brown, no large rocks or wetness.

Each normal-map edit references its matching albedo: convert to OpenGL tangent-space NORMAL MAP Y+, preserve stone/crack/ripple layout, restrained shallow relief at the specified physical span. Normalized blue-purple slope vectors, flat RGB 128/128/255, seamless square, no albedo or labels. These are generated estimates, not measured displacement or scan-derived normals.

Each roughness-map edit references its matching albedo: matching linear grayscale PBR ROUGHNESS DATA, preserve surface layout and suppress albedo pigmentation/illumination. Sand grains approximately 0.93 to 0.98, smoother ripple faces 0.86; caliche 0.88 to 0.96, cracks 0.97, stones 0.76 to 0.86; gravel dust 0.94 to 0.98, pebbles 0.80 to 0.90 and occasional smooth faces 0.72 to 0.80. Generated values are approximate, not calibrated measurements.

## Import

```sh
S47_GROUND_TEXTURES=1 python3 unreal/ArtSource/Backdrop/run_ground_import.py --apply --output /absolute/fresh/output
```

The runner holds the production lease and idle guard. Only albedo uses sRGB. Normal imports flip green for Unreal; roughness remains linear grayscale. World projection spans are 2.4 m sand, 3.2 m crust, 1.6 m gravel. T4 supplies the established large-scale coverage masks. The texture-disabled path retains the original placeholder material.

The generated sand uses an 80/20 linear mix of the 2.4 m and 9 m samples instead of multiplying albedo by itself. Neutral GroundTint 0.65 and crust/gravel tint 0.94 avoid adding the old placeholder pigmentation a second time. Existing vertex colour and coverage masks remain intact.

## Estimated height data, 4 November 2026

`derive_ground_height.py` reconstructs `T_Ground{Sand,Crust,Gravel}_H.png` from the corresponding original OpenGL normal imports. These three 2048-square PNGs contain **16-bit linear scalar data**, with the bounded range 0–65535 representing 0–1. They are not additional ImageGen outputs, scan-derived geometry or measured centimetres. The material sets physical relief amplitude separately.

The numerical reconstruction integrates normal slopes by a periodic screened Poisson solve. Image-column slope is `-Nx/Nz`; image-row slope is `+Ny/Nz`, matching the OpenGL source and Unreal's green-channel import conversion. The solve removes the mean tilt, projects away nonintegrable curl in the generated normals and damps broad drift beyond roughly one third of a tile. It preserves the sources and normalizes without clipping. Wrapping is periodic; no painted border or repeated edge strip is introduced.

Regenerate with Python, NumPy and Pillow:

```sh
python3 unreal/ArtSource/Backdrop/derive_ground_height.py
```

The tool checks PNG dimensions, positive slope agreement and an exact 16-bit PNG roundtrip. Initial reconstruction has positive slope correlations (U/V: sand 0.37/0.88, crust 0.65/0.91, gravel 0.70/0.92). Mean steps across wrapped edges are lower than ordinary interior steps for all three maps. Remaining normal/height differences reflect the generated normals' nonintegrable components; do not describe these estimates as a calibrated PBR scan.

## Runtime relief

The three layers share world-projected UVs across base colour, normal and roughness. Near-field parallax uses eight height steps, a 1.2 cm micro-depth (`MicroReliefCm`, capped at 2 cm), a grazing-angle clamp and a fade from 6 to 16 metres. The displacement path samples unshifted height coordinates, independent of the camera. Native material attributes expose Displacement to the Python importer in UE 5.8.

Nanite tessellation uses magnitude 7 cm and centre 0.5: at most ±3.5 cm around the existing surface, 7 cm total relief. Only the five exported desert meshes are enabled; the importer validates unit scale through the parent chain and retains the complete original fallback mesh. Collision geometry and gameplay floor/driving rules are unchanged. Regenerating the Web geometry reapplies these settings through `ground_nanite.py`.

Implementation was checked against local UE 5.8 source and [Epic's Nanite guidance](https://dev.epicgames.com/documentation/unreal-engine/working-with-naniteenabled-content). The project library's `kevinpbuckley/unreal-engine-skills` profile supplied a Nanite reference (catalogued UNVERIFIED, no code copied); compatibility is checked in this project's Linux import and runtime.
