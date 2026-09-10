# Visual 10 authored laboratory and field assets

Original geometry authored for SIGNAL / 47 with the reproducible Blender source
`Unity/Blender/Source/visual10_props.py` and editable
`Unity/Blender/SIGNAL47_visual10_props.blend`. No external models, textures or
paid services were used for these meshes. Project ownership/license applies.

The FBX exports consolidate copies by material, preserve UVs and real bevels,
and use metric dimensions with -Z forward / Y up. Their source collections
retain separate editable components. See `blender-verification.json` for
executed Blender version, triangle counts, UV presence and bounds. A successful
Blender export does not itself certify Unity import or runtime appearance.

Scene materials, colliders, lighting and placement are assigned by the Visual10 integration. Lamp bulbs use CH09_Cream as a remapping slot; actual emitted light is supplied in Unity. Bench drawers face Blender +X, long axis Blender Y, floor origin Z=0. Wall lamp extends Blender -Y from wall rose origin. Field shade illuminates Blender -Z from aperture origin. Cloth origin is sink rim; drape falls on Blender -Y side. Existing VT323 and
Poly Haven floor resources retain their original notices in the project.

## Corrected dish surface

V10_RadioDish_BowlCorrected.fbx is derived from the existing project-owned Refined/SM_RadioDish_Bowl_A.fbx bowl. Only face winding and smooth normals are corrected; vertex positions, radius, depth and pivot are preserved. Feed/support meshes remain in the original asset and must be preserved during integration. Reproduce independently with visual10_props.py -- --dish-only. The original assets are untouched. See dish-normal-verification.json for measured geometry and FBX roundtrip checks.
