"""Refresh presentation after a source-preserving lighting adjustment.
The complete build_station.py already includes these settings.
"""
import bpy
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parent
bpy.ops.wm.open_mainfile(filepath=str(ROOT/'1420_AM_station.blend'))
scene=bpy.context.scene
for o in scene.objects:
 if o.type=='LIGHT':
  o.visible_glossy=False; o.visible_transmission=False; o.visible_camera=False; o.data.specular_factor=0
# GLB scene origin follows the common FBX building pivot.
parts=[o for o in scene.objects if o.type=='MESH' and o.name.startswith('SM_1420_')]
bpy.ops.object.select_all(action='DESELECT'); saved={o.name:o.location.copy() for o in parts}
for o in parts: o.select_set(True); o.location-=Vector((0,5,0))
bpy.ops.export_scene.gltf(filepath=str(ROOT/'1420_AM_station.glb'),export_format='GLB',use_selection=True,export_materials='EXPORT',export_extras=True)
for o in parts: o.location=saved[o.name]
scene.camera=bpy.data.objects['CAM_Exterior']
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'1420_AM_station.blend'))
scene.render.filepath=str(ROOT/'exterior.png'); bpy.ops.render.render(write_still=True)
scene.camera=bpy.data.objects['CAM_Interior']; scene.render.filepath=str(ROOT/'interior.png'); bpy.ops.render.render(write_still=True)
