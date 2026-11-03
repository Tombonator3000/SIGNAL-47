# Concrete Block Wall — source candidate

Three original 2K maps (27,161,240 bytes total): diffuse JPG, lossless normal GL PNG,
and roughness JPG. All decode to 2048 × 2048; hashes and source URLs are recorded in
`source-manifest.json`. The Shell importer now defines the native adaptation below; native import and shader compilation passed on 3 November 2026. Visual acceptance remains pending.

Poly Haven identifies Charlotte Baglioni as author, CC0 as license and 2 m tile
width. The square texture is treated as a 2 × 2 m tile. Its linked context photograph
shows the same exterior masonry. That supports a photographic source inference;
the page does not explicitly establish the capture/processing method. Do not label
it a verified photogrammetry scan or an original cream-painted laboratory surface.

## Proposed native recipe (root owns importer)

Target only `EW_Shell_Masonry`, using a new Shell-owned material; do not mutate the
shared `EW_CinderblockCream` Props material. Current shell UV divides metre
coordinates by 2.4 for this slot. Multiply UV0 by 1.2 to reproduce the source's 2 m
span. The source has approximately 400 × 200 mm running-bond courses.

Diffuse is sRGB, roughness linear, normal is GL: use the existing normal import
recipe with `flip_green_channel=True` (verify readback), and metallic=0. Preserve
original files. The source is bare weathered exterior block, visibly darker and
coarser than the intended 1986 cream-painted lab. A direct diffuse hookup is not
appropriate. Implemented importer adaptation: desaturate the sampled diffuse, multiply by 0.25,
add 0.8, then multiply by cream linear color [0.68, 0.63, 0.49] and connect the result
to BaseColor. Roughness is 0.70 + 0.15 * source R. Normal strength is 0.3. These are
look-development starting values awaiting native visual review. Keep all maps on
the same UVs; do not combine this mortar with the old procedural mortar normal.
No displacement or new collision is needed.

The lab screenshot `04-test-lab.png` was inspected. Preserve its block scale, cream
paint, green trim and readable surface; compare before/after in actual gameplay.
If the coarse relief still reads as outdoor retaining wall after paint adaptation,
keep the current material pending a better source. Downloading this candidate does
not establish visual acceptance. No Unreal or Blender process was run.

Library-first searches `Unreal concrete block wall CC0 texture` and `CC0 tekstur`
returned no relevant ready-made source; no external tool or skill was installed.
