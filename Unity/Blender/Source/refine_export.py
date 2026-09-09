"""Non-destructive refinement: original OBJ -> bevelled, named FBX parts."""
import bpy
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'Assets/Signal47/Art/Refined'
OUT.mkdir(parents=True,exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.context.scene.unit_settings.system='METRIC'
for source in sorted((ROOT/'Assets/Signal47/Art/PrototypePort').glob('SM_*.obj')):
    bpy.ops.object.select_all(action='DESELECT')
    before=set(bpy.data.objects)
    bpy.ops.wm.obj_import(filepath=str(source),forward_axis='NEGATIVE_Z',up_axis='Y')
    meshes=[o for o in bpy.data.objects if o not in before and o.type=='MESH']
    for o in meshes:
        o.name=source.stem+'__'+o.name
        bevel=o.modifiers.new('Soft manufactured edges','BEVEL')
        bevel.width=.005 if 'Dish' not in source.name else .012
        bevel.segments=2
        bevel.limit_method='ANGLE'
        o.modifiers.new('Weighted corner normals','WEIGHTED_NORMAL')
        o.select_set(True)
    bpy.ops.export_scene.fbx(filepath=str(OUT/(source.stem+'.fbx')),use_selection=True,object_types={'MESH'},add_leaf_bones=False,bake_anim=False,axis_forward='-Z',axis_up='Y',apply_unit_scale=True,use_mesh_modifiers=True)
    print('REFINED',source.name,'parts',len(meshes))
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'Blender/SIGNAL47_refined_assets.blend'))
