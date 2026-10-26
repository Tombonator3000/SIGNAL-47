"""Upgrade the existing Blender/export assembly without regenerating or rendering it.
build_station.py applies the same operation when regenerating from scratch.
"""
import bpy, json
from pathlib import Path
from mathutils import Vector
import sys
ROOT=Path(__file__).resolve().parent; sys.path.insert(0,str(ROOT))
from station_mesh_cleanup import clean_triangulated_mesh
bpy.ops.wm.open_mainfile(filepath=str(ROOT/'1420_AM_station.blend'))
parts=[o for o in bpy.context.scene.objects if o.type=='MESH' and o.name.startswith('SM_1420_')]
contract=json.loads((ROOT/'model_contract.json').read_text())
proof={o.name:clean_triangulated_mesh(o) for o in parts}
for o in parts:
 bpy.ops.object.select_all(action='DESELECT'); o.select_set(True); bpy.context.view_layer.objects.active=o
 location=o.location.copy(); o.location=(0,0,0)
 bpy.ops.export_scene.fbx(filepath=str(ROOT/'exports'/(o.name+'.fbx')),use_selection=True,object_types={'MESH'},apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',axis_forward='-Y',axis_up='Z',bake_anim=False,add_leaf_bones=False,mesh_smooth_type='FACE')
 o.location=location
 o.data.calc_loop_triangles(); contract['parts'][o.name]['triangles']=len(o.data.loop_triangles)
 contract['parts'][o.name]['vertices']=len(o.data.vertices)
 contract['parts'][o.name]['zero_area_triangles']=0
 contract['parts'][o.name]['triangulated']=True
bpy.ops.object.select_all(action='DESELECT'); saved={o.name:o.location.copy() for o in parts}
for o in parts: o.select_set(True); o.location-=Vector((0,5,0))
bpy.ops.export_scene.gltf(filepath=str(ROOT/'1420_AM_station.glb'),export_format='GLB',use_selection=True,export_materials='EXPORT',export_extras=True)
for o in parts: o.location=saved[o.name]
contract['export_triangulation']='Explicit Blender loop triangulation, exact area=0 triangles omitted; visible triangle coordinates/UVs/materials unchanged.'
(ROOT/'model_contract.json').write_text(json.dumps(contract,indent=2))
(ROOT/'mesh_cleanup.json').write_text(json.dumps(proof,indent=2))
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'1420_AM_station.blend'))
print('CLEANUP_PROOF',json.dumps(proof))
