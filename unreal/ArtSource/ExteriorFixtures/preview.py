"""Two bounded Blender assembly previews. Context stands are not exported assets."""
from pathlib import Path
import bpy
from mathutils import Vector
HERE=Path(__file__).resolve().parent
bpy.ops.wm.open_mainfile(filepath=str(HERE/'SARO_ExteriorFixtures.blend'))
sc=bpy.context.scene;sc.render.engine='CYCLES';sc.cycles.device='CPU';sc.cycles.samples=24
sc.render.threads_mode='FIXED';sc.render.threads=2
sc.render.resolution_x=1200;sc.render.resolution_y=900;sc.render.resolution_percentage=100
sc.world.use_nodes=True;sc.world.node_tree.nodes['Background'].inputs['Color'].default_value=(.50,.59,.72,1)
sc.world.node_tree.nodes['Background'].inputs['Strength'].default_value=.30
for c in bpy.data.collections:
    if '_EDIT_' in c.name:c.hide_render=True
for o in bpy.data.objects:
    if o.type=='MESH':o.hide_render=True
def xyz(p):return Vector((p[0],-p[2],p[1]))
def aim(o,p):o.rotation_euler=(xyz(p)-o.location).to_track_quat('-Z','Y').to_euler()
def mat(name,c):
    m=bpy.data.materials.new(name);m.use_nodes=True
    p=m.node_tree.nodes['Principled BSDF'];p.inputs['Base Color'].default_value=(*c,1);p.inputs['Roughness'].default_value=.9
    return m
cream=mat('PREVIEW_CONTEXT_stucco',(.33,.31,.255))
gray=mat('PREVIEW_CONTEXT_plinth',(.15,.17,.17))
context=[]
def box(name,p,s,m):
    bpy.ops.mesh.primitive_cube_add(size=1,location=xyz(p));o=bpy.context.object;o.name='PREVIEW_CONTEXT_'+name
    o.dimensions=(s[0],s[2],s[1]);o.data.materials.append(m);context.append(o);return o
def light(p,w,size):
    d=bpy.data.lights.new('Preview_softbox','AREA');d.energy=w;d.shape='DISK';d.size=size
    o=bpy.data.objects.new(d.name,d);sc.collection.objects.link(o);o.location=xyz(p);aim(o,(0,.10,.03));return o
light((-.7,1.3,1.4),140,1.4);light((1.0,.55,.4),55,1.0);light((-.3,.4,-.4),30,.7)
d=bpy.data.cameras.new('Blender_preview_camera');cam=bpy.data.objects.new(d.name,d);sc.collection.objects.link(cam);sc.camera=cam
d.type='ORTHO';d.ortho_scale=.59
wall=bpy.data.objects['SM_Exterior_WallPack'];wall.hide_render=False;wall.hide_set(False)
box('mounting_wall',(0,0,-.020),(1.1,1.1,.040),cream)
cam.location=xyz((.52,.24,.82));aim(cam,(0,0,.08))
out=HERE/'Preview';out.mkdir(exist_ok=True)
sc.render.filepath=str(out/'blender-wallpack-unlit.png');bpy.ops.render.render(write_still=True)
wall.hide_render=True
for o in context:o.hide_render=True
can=bpy.data.objects['SM_Exterior_Canopy'];can.hide_render=False;can.hide_set(False);can.location=xyz((-.31,.55,0))
sign=bpy.data.objects['SM_Exterior_SignWash'];sign.hide_render=False;sign.hide_set(False);sign.location=xyz((.27,-.14,.04))
box('canopy_underside',(-.31,.575,0),(.67,.05,.32),cream)
box('sign_curb',(.27,-.16,.02),(.32,.04,.31),cream)
box('neutral_backdrop',(0,.15,-.30),(2.2,1.4,.05),gray)
d.type='PERSP';d.lens=38;cam.location=xyz((.50,.10,1.25));aim(cam,(0,.22,.015))
sc.render.filepath=str(out/'blender-canopy-signwash-unlit.png');bpy.ops.render.render(write_still=True)
print('EXTERIOR FIXTURE BLENDER PREVIEWS COMPLETE')
