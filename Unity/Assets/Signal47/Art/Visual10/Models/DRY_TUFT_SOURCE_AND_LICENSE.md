# Dry tuft source and license

Derived from **Grass Medium 02**, Rico Cilliers, Poly Haven:
https://polyhaven.com/a/grass_medium_02 . CC0-1.0:
https://polyhaven.com/license and https://creativecommons.org/publicdomain/zero/1.0/ .

Three reduced tufts retain complete actual source blade islands and unchanged
atlas UV corners. No meadow Geometry Nodes system is exported or evaluated.
Source manifest and original downloads: Artifacts/Visual10Research/TerrainRefinement/.
Source SHA256 is recorded in dry-tuft-verification.json. Rebuild with
Unity/Blender/Source/visual10_dry_tufts.py; editable output is
Unity/Blender/SIGNAL47_visual10_dry_tufts.blend, with dry color and alpha packed.

Unity material slot V10_DryGrass_Atlas requires source dry_diff RGB and separate
alpha combined into base-map alpha, alpha clipping (start at .4), double-sided
rendering and rough nonmetal shading. Preserve atlas UVs, use local scale 1,
floor pivot and Y-up import (-Z forward / Y up FBX). Heights are .20/.30/.45 m; two or three layered crowns retain 54/59/63 complete blade islands (2142/2464/2378 triangles).
Use restrained small clumps, avoid dense carpet and verify alpha overdraw in
runtime. No claim of a specific native New Mexico plant species is made.

Blender roundtrip checks are not Unity or performance verification.
