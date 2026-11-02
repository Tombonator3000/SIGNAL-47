# S-03 original cabinet

Pure-Python extraction reuses the immutable exported field rather than introducing a second cabinet. `prepare.py EXPORT OUT` reads `geo/geo_004.glb` using the existing `trim_web.py` helpers. Only triangles wholly within the S-03 cabinet bounds are extracted. The remaining indices preserve all other cabinets unchanged. Expected source counts make a changed export fail explicitly.

The original body, cap, pad, conduit and atlas front are retained. The body's solid front is removed, an inward steel liner added, and the atlas front becomes a 15 mm thick hinged leaf. Existing front UV coordinates are unchanged. Root-coordinate body geometry is not centered. Door geometry is relative to the world hinge reported in `array-cabinet.json`; negative Y rotation opens west, away from the cabinet. Coordinates are Three metres, converted by the existing Unreal GLB pipeline just like other Web trims.

Run `import_assets.py` inside an authorized Unreal job with `S47_ARRAY_CABINET` and `S47_ARRAY_CABINET_REPORT`. No Blender/FBX conversion is needed. The script imports only `/Game/ArrayCabinet`, validates triangle and material-slot counts and stages metadata. Runtime must bind each slot to its original material ID (including the original atlas), apply all four trim meshes and create the body/door together. A successful preparation/import is not a visual runtime check.

Story's motor-bus inspection cabinet is separate and unaffected. Do not change saved service-state values or consume additional relays on restore. Generated GLBs stay outside Git; the scripts preserve editable extraction instructions and original assets.
