"""Blender-only assembly previews with labelled context proxies, never game evidence."""
from pathlib import Path
import bpy,math
from mathutils import Vector
HERE=Path(__file__).resolve().parent
bpy.ops.wm.open_mainfile(filepath=str(HERE/'SARO_ArrivalEntrance.blend'))
scene=bpy.context.scene
scene.render.engine='CYCLES';scene.cycles.samples=20;scene.cycles.device='CPU'
scene.render.threads_mode='FIXED';scene.render.threads=2
scene.render.resolution_x=1200;scene.render.resolution_y=1000;scene.render.resolution_percentage=100
scene.world.use_nodes=True;scene.world.node_tree.nodes['Background'].inputs['Color'].default_value=(.5,.58,.7,1)
scene.world.node_tree.nodes['Background'].inputs['Strength'].default_value=.35
out=HERE/'Preview';out.mkdir(exist_ok=True)
def xyz(v):return Vector((v[0],-v[2],v[1]))
def mat(name,col):
    m=bpy.data.materials.new(name);m.diffuse_color=(*col,1);m.use_nodes=True
    m.node_tree.nodes['Principled BSDF'].inputs['Base Color'].default_value=(*col,1)
    m.node_tree.nodes['Principled BSDF'].inputs['Roughness'].default_value=.8;return m
def box(name,at,size,material):
    bpy.ops.mesh.primitive_cube_add(size=1,location=xyz(at));o=bpy.context.object;o.name='CONTEXT_ONLY_'+name
    o.dimensions=(size[0],size[2],size[1]);o.data.materials.append(material);return o
def aim(o,at):o.rotation_euler=(xyz(at)-o.location).to_track_quat('-Z','Y').to_euler()
def light(name,at,power,size):
    d=bpy.data.lights.new(name,'AREA');d.energy=power;d.shape='DISK';d.size=size
    o=bpy.data.objects.new(name,d);scene.collection.objects.link(o);o.location=xyz(at);aim(o,(-.8,1,.5));return o
light('large_soft_key',(-4,5,4),1000,5);light('soft_fill',(-1,3,-4),500,4)
data=bpy.data.cameras.new('PreviewCamera');cam=bpy.data.objects.new('PreviewCamera',data);scene.collection.objects.link(cam);scene.camera=cam
for ob in scene.objects:
    if ob.type=='MESH':ob.hide_render=ob.name not in ('SM_Arrival_Entrance','SM_Arrival_DoorDetails')
cream=mat('Context_plain_stucco',(.5,.46,.36));door=mat('Context_existing_door',(.12,.15,.13));concrete=mat('Context_existing_concrete',(.30,.29,.26))
context=[]
context.append(box('wall_left',(.10,1.4,-.98),(.20,2.8,1.90),cream))
context.append(box('wall_right',(.10,1.4,1.93),(.20,2.8,1.90),cream))
context.append(box('wall_header',(.10,2.49,.48),(.20,.62,1.04),cream))
context.append(box('original_leaf',(0,1.04,.48),(.05,2.08,.96),door))
context.append(box('existing_landing',(-.715,-.05,.48),(1.30,.10,1.90),concrete))
context.append(box('grade',(-2.5,-.64,.5),(9,.06,6),concrete))
# Existing access ramp as a neutral contextual solid with the measured slope.
verts=[xyz(v)for v in [(-4.33,-.6,-.27),(-1.33,0,-.27),(-1.33,0,1.43),(-4.33,-.6,1.43),(-4.33,-.66,-.27),(-1.33,-.06,-.27),(-1.33,-.06,1.43),(-4.33,-.66,1.43)]]
mesh=bpy.data.meshes.new('CONTEXT_ONLY_existing_ramp');mesh.from_pydata(verts,[],[(0,1,2,3),(4,7,6,5),(0,4,5,1),(3,2,6,7),(0,3,7,4),(1,5,6,2)]);mesh.materials.append(concrete)
o=bpy.data.objects.new(mesh.name,mesh);scene.collection.objects.link(o);context.append(o)
cam.location=xyz((-6.4,2.6,4.8));aim(cam,(-1.3,1.0,.48));data.type='PERSP';data.lens=44
scene.render.filepath=str(out/'blender-entrance-context.png');bpy.ops.render.render(write_still=True)
for o in context:o.hide_render=True
for ob in scene.objects:
    if ob.type=='MESH':ob.hide_render=ob.name!='SM_Arrival_VisitorSign'
cam.location=xyz((1.1,1.55,3.3));aim(cam,(0,.94,0));data.lens=62
scene.render.filepath=str(out/'blender-visitor-sign.png');bpy.ops.render.render(write_still=True)
print('ARRIVAL BLENDER PREVIEWS READY')
