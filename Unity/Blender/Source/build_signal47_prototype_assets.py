"""Run inside Blender 5.x: blender --background --python build_signal47_prototype_assets.py
Imports the prototype-port OBJ files, adds non-destructive bevels and saves a refinement source .blend.
"""
import bpy
from pathlib import Path

HERE=Path(__file__).resolve()
PROJECT=HERE.parents[2]
SRC=PROJECT/'Assets/Signal47/Art/PrototypePort'
OUT=PROJECT/'Blender/SIGNAL47_prototype_assets.blend'

bpy.ops.wm.read_factory_settings(use_empty=True)
collection=bpy.data.collections.new('SIGNAL47_PROTOTYPE_PORT')
bpy.context.scene.collection.children.link(collection)

for path in sorted(SRC.glob('SM_*.obj')):
    before=set(bpy.data.objects)
    bpy.ops.wm.obj_import(filepath=str(path), forward_axis='NEGATIVE_Z', up_axis='Y')
    imported=[o for o in bpy.data.objects if o not in before]
    for obj in imported:
        # Relink into named collection.
        for c in list(obj.users_collection): c.objects.unlink(obj)
        collection.objects.link(obj)
        obj.name=f'{path.stem}__{obj.name}'
        if obj.type=='MESH':
            bevel=obj.modifiers.new('Prototype bevel','BEVEL'); bevel.width=.008; bevel.segments=2
            obj.data.materials.clear()
            mat=bpy.data.materials.get('MAT_PROTOTYPE_NEUTRAL') or bpy.data.materials.new('MAT_PROTOTYPE_NEUTRAL')
            mat.diffuse_color=(.42,.44,.40,1)
            obj.data.materials.append(mat)

# Keep asset families separated and ready for manual refinement.
bpy.context.scene.unit_settings.system='METRIC'
bpy.context.scene.unit_settings.scale_length=1.0
bpy.ops.wm.save_as_mainfile(filepath=str(OUT))
print('Saved',OUT)
