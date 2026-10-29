"""Three bounded Cycles previews of the saved source (not game pictures). Context poles and the
road are PREVIEW_CONTEXT only and are never exported."""
from pathlib import Path
import math
import bpy
from mathutils import Vector
HERE=Path(__file__).resolve().parent
bpy.ops.wm.open_mainfile(filepath=str(HERE/'Caliche_TrafficSignal.blend'))
sc=bpy.context.scene; sc.render.engine='CYCLES'; sc.cycles.device='CPU'; sc.cycles.samples=32
sc.render.threads_mode='FIXED'; sc.render.threads=2
sc.render.resolution_x=1200; sc.render.resolution_y=900; sc.render.resolution_percentage=100
sc.world.use_nodes=True; bg=sc.world.node_tree.nodes['Background']
for c in bpy.data.collections:
    if '_EDIT_' in c.name: c.hide_render=True
def xyz(p): return Vector((p[0],-p[2],p[1]))
def aim(o,p): o.rotation_euler=(xyz(p)-o.location).to_track_quat('-Z','Y').to_euler()
def mat(name,c,r=.9):
    m=bpy.data.materials.new(name); m.use_nodes=True; p=m.node_tree.nodes['Principled BSDF']
    p.inputs['Base Color'].default_value=(*c,1); p.inputs['Roughness'].default_value=r; return m
wood=mat('PREVIEW_CONTEXT_wood',(.16,.11,.07)); road=mat('PREVIEW_CONTEXT_asphalt',(.06,.06,.06))
for P in ((-9.1,19.0),(9.1,-19.0)):
    bpy.ops.mesh.primitive_cone_add(vertices=7,radius1=.17,radius2=.13,depth=8,location=xyz((P[0],-7.249+4,P[1])))
    o=bpy.context.object; o.name='PREVIEW_CONTEXT_pole'; o.data.materials.append(wood)
bpy.ops.mesh.primitive_plane_add(size=80,location=xyz((0,-7.25,0))); bpy.context.object.data.materials.append(road)
head=bpy.data.objects['SM_Caliche_Signal_Head']; span=bpy.data.objects['SM_Caliche_Signal_Span']
for o in (head,span): o.hide_render=False; o.hide_set(False)
sun_d=bpy.data.lights.new('Preview_sun','SUN'); sun_d.energy=3.5; sun_d.angle=math.radians(1.5)
sun=bpy.data.objects.new('Preview_sun',sun_d); sc.collection.objects.link(sun); sun.rotation_euler=(math.radians(50),0,math.radians(35))
d=bpy.data.cameras.new('Preview_camera'); cam=bpy.data.objects.new(d.name,d); sc.collection.objects.link(cam); sc.camera=cam
out=HERE/'Preview'; out.mkdir(exist_ok=True)
lens=bpy.data.materials['M_TownSignal_Lens'].node_tree.nodes['Principled BSDF']
def shot(file,eye,target,focal,sky,night=False):
    bg.inputs['Color'].default_value=sky; bg.inputs['Strength'].default_value=.05 if night else 1.0
    sun.hide_render=night
    lens.inputs['Emission Color'].default_value=(1,.42,.03,1); lens.inputs['Emission Strength'].default_value=25 if night else 0
    d.lens=focal; cam.location=xyz(eye); aim(cam,target)
    sc.render.filepath=str(out/file); bpy.ops.render.render(write_still=True)
shot('blender-head-day.png',(1.6,-1.35,1.9),(0,-.5,0),55,(.55,.66,.85,1))
shot('blender-head-night.png',(1.6,-1.35,1.9),(0,-.5,0),55,(.02,.025,.04,1),True)
shot('blender-span-day.png',(10,-5.4,-9),(-2,-.6,4),28,(.55,.66,.85,1))
print('TRAFFIC SIGNAL BLENDER PREVIEWS COMPLETE')
