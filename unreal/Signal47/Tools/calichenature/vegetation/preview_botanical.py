"""One CPU contact sheet: same-scale Sage/Juniper LODs, plus grass/yucca/stones.
Presentation transforms are never saved into the source Blend.
"""
import bpy,math
from mathutils import Vector
from pathlib import Path
ROOT=Path(__file__).resolve().parent
bpy.ops.wm.open_mainfile(filepath=str(ROOT/'caliche_vegetation.blend'))
scene=bpy.context.scene
samples=[]
for i,lod in enumerate(range(3)):
 suffix=f'_LOD{lod}' if lod else ''
 samples.append(('Sage_B'+suffix,(-4+i*4,4,0),2.0,f'SAGE B / LOD{lod} / 0.62 m'))
 samples.append(('Juniper_B'+suffix,(-4+i*4,0,0),.55,f'JUNIPER B / LOD{lod} / 2.65 m'))
samples += [('Grass_B',(-4,-4,0),2.0,'BUNCHGRASS / 0.64 m'),('Yucca_B',(0,-4,0),1.3,'YUCCA / 1.05 m'),('Caliche_B',(3.6,-4,0),1.8,'CALICHE / SANDSTONE')]
keep={'SM_CN_'+name for name,_,_,_ in samples}|{'SM_CN_Sandstone_A'}
for o in list(bpy.data.objects):
 if o.type=='MESH' and o.name not in keep:bpy.data.objects.remove(o,do_unlink=True)
for col in bpy.data.collections:col.hide_render=False;col.hide_viewport=False
ink=bpy.data.materials.new('PREVIEW_ink');ink.use_nodes=True;ink.node_tree.nodes['Principled BSDF'].inputs['Base Color'].default_value=(.012,.012,.012,1)
for name,loc,scale,label in samples:
 o=bpy.data.objects['SM_CN_'+name];o.hide_render=False;o.hide_set(False);o.location=loc;o.scale=(scale,scale,scale);o.rotation_euler[2]=.25
 d=bpy.data.curves.new('Label','FONT');d.body=label;d.size=.16;d.align_x='CENTER';text=bpy.data.objects.new('Label_'+name,d);scene.collection.objects.link(text);text.location=(loc[0],loc[1]-1.60,.025);d.materials.append(ink)
stone=bpy.data.objects['SM_CN_Sandstone_A'];stone.hide_render=False;stone.hide_set(False);stone.location=(4.45,-4.05,0);stone.scale=(1.2,1.2,1.2)
bpy.ops.mesh.primitive_plane_add(size=200,location=(0,0,-.02));ground=bpy.context.object;mat=bpy.data.materials.new('PREVIEW_neutral');mat.diffuse_color=(.23,.22,.20,1);ground.data.materials.append(mat)
world=scene.world;world.use_nodes=True;world.node_tree.nodes['Background'].inputs['Color'].default_value=(.32,.36,.43,1);world.node_tree.nodes['Background'].inputs['Strength'].default_value=.5
light=bpy.data.lights.new('Preview_softbox','AREA');light.energy=1700;light.shape='DISK';light.size=10;o=bpy.data.objects.new('Preview_softbox',light);scene.collection.objects.link(o);o.location=(-5,-7,12)
d=bpy.data.cameras.new('Preview_camera');d.type='ORTHO';d.ortho_scale=15.0;o=bpy.data.objects.new('Preview_camera',d);scene.collection.objects.link(o);o.location=(0,-18,22);o.rotation_euler=(Vector((0,0,.50))-o.location).to_track_quat('-Z','Y').to_euler();scene.camera=o
scene.render.engine='CYCLES';scene.render.threads_mode='FIXED';scene.render.threads=2;scene.cycles.samples=8;scene.cycles.transparent_max_bounces=32;scene.cycles.use_denoising=True
scene.render.resolution_x=1280;scene.render.resolution_y=1200;scene.render.resolution_percentage=100;scene.view_settings.view_transform='AgX'
scene.render.filepath=str(ROOT/'botanical_preview.png');bpy.ops.render.render(write_still=True)
print('PREVIEW_ONLY: same scale across each family LOD row, original source unchanged; physical heights printed in metres.')
