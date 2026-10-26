"""Original SARO radio telescope. Run: blender -b --python build_dish.py.

Geometry is authored in meters, Z up. Each animation component is exported
at the origin; assembly offsets are stored in verification.json.
"""
import bpy
import math
import json
import sys
from pathlib import Path
from mathutils import Vector

ROOT = Path(__file__).resolve().parent
OUT = ROOT / 'exports'
PREVIEW = ROOT / 'previews'
OUT.mkdir(exist_ok=True)
PREVIEW.mkdir(exist_ok=True)
SKIP_RENDER = '--skip-render' in sys.argv
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete(use_global=False)
for dat in list(bpy.data.materials):
    bpy.data.materials.remove(dat)

PALETTE = [
    ('M_Array_Enamel', (.58,.61,.59,1), .18, .58),
    ('M_Array_Reflector', (.77,.79,.74,1), .3, .39),
    ('M_Array_Steel', (.29,.34,.33,1), .75, .43),
    ('M_Array_Dark', (.055,.075,.071,1), .55, .5),
    ('M_Array_Concrete', (.30,.28,.23,1), .0, .94),
    ('M_Array_Oxide', (.30,.13,.055,1), .4, .85),
    ('M_Array_Safety', (.65,.39,.07,1), .2, .64),
    ('M_Array_Red', (.40,.017,.012,1), .1, .3),
]
MATS=[]
for name,color,metal,rough in PALETTE:
    m=bpy.data.materials.new(name)
    m.diffuse_color=color
    m.use_nodes=True
    bs=m.node_tree.nodes.get('Principled BSDF')
    bs.inputs['Base Color'].default_value=color
    bs.inputs['Metallic'].default_value=metal
    bs.inputs['Roughness'].default_value=rough
    MATS.append(m)

class MeshBuilder:
    def __init__(self): self.v=[]; self.f=[]; self.m=[]
    def face(self, coords, mat=0):
        # The first parabolic bay converges on one center vertex. Emit an
        # actual triangle there, rather than a quad with a repeated corner.
        # Unreal removes that quad's zero-area triangle on import otherwise.
        clean=[]
        for co in coords:
            co=Vector(co)
            if not clean or (co-clean[-1]).length_squared>1e-20:
                clean.append(co)
        if len(clean)>1 and (clean[0]-clean[-1]).length_squared<1e-20:
            clean.pop()
        if len(clean)<3:
            return
        n=len(self.v); self.v.extend(clean); self.f.append(tuple(range(n,n+len(clean)))); self.m.append(mat)
    def box(self,c,s,mat=0):
        c=Vector(c); x,y,z=[t*.5 for t in s]
        vs=[c+Vector(q) for q in [(-x,-y,-z),(x,-y,-z),(x,y,-z),(-x,y,-z),(-x,-y,z),(x,-y,z),(x,y,z),(-x,y,z)]]
        for ix in [(0,3,2,1),(0,1,5,4),(1,2,6,5),(2,3,7,6),(3,0,4,7),(4,5,6,7)]: self.face([vs[i] for i in ix],mat)
    def beam(self,a,b,r,mat=2,n=6,r2=None):
        a=Vector(a); b=Vector(b); d=(b-a).normalized(); u=d.cross(Vector((0,0,1)))
        if u.length<.1: u=d.cross(Vector((0,1,0)))
        u.normalize(); v=d.cross(u); r2=r if r2 is None else r2
        ar=[a+r*(u*math.cos(i*math.tau/n)+v*math.sin(i*math.tau/n)) for i in range(n)]
        br=[b+r2*(u*math.cos(i*math.tau/n)+v*math.sin(i*math.tau/n)) for i in range(n)]
        self.face(list(reversed(ar)),mat); self.face(br,mat)
        for i in range(n): j=(i+1)%n; self.face([ar[i],ar[j],br[j],br[i]],mat)
    def ring(self,r,z,t=.05,mat=2,n=64,center=(0,0)):
        for i in range(n):
            a=i*math.tau/n; b=(i+1)*math.tau/n
            self.beam((center[0]+r*math.cos(a),center[1]+r*math.sin(a),z),(center[0]+r*math.cos(b),center[1]+r*math.sin(b),z),t,mat,n=4)
    def tube(self,lo,hi,r0,r1,mat=0,n=16):
        for i in range(n):
            a=i*math.tau/n; b=(i+1)*math.tau/n
            self.face([(r0*math.cos(a),r0*math.sin(a),lo),(r0*math.cos(b),r0*math.sin(b),lo),(r1*math.cos(b),r1*math.sin(b),hi),(r1*math.cos(a),r1*math.sin(a),hi)],mat)
        self.face([(r1*math.cos(i*math.tau/n),r1*math.sin(i*math.tau/n),hi) for i in range(n)],mat)
    def obj(self,name):
        mesh=bpy.data.meshes.new(name); mesh.from_pydata(self.v,[],self.f); mesh.materials.clear()
        for m in MATS: mesh.materials.append(m)
        mesh.update()
        obj=bpy.data.objects.new(name,mesh); bpy.context.collection.objects.link(obj)
        for p,m in zip(mesh.polygons,self.m): p.material_index=m
        # UVs remain useful for later dirt/detail overlays even though materials
        # intentionally rely on engine-compatible constant PBR parameters.
        uv=mesh.uv_layers.new(name='UVMap')
        for p in mesh.polygons:
            for li in p.loop_indices:
                co=mesh.vertices[mesh.loops[li].vertex_index].co
                uv.data[li].uv=(co.x*.125,co.y*.125+co.z*.125)
        return obj

def base(lod):
    q=MeshBuilder(); sides=[16,12,8][lod]
    # Four anchor plinths leave the real open-legged machine silhouette.
    for x in [-2.35,2.35]:
        for y in [-2.35,2.35]:
            q.box((x,y,.42),(1.45,1.45,.84),4)
            q.box((x,y,.90),(1.0,1.0,.14),3)
            q.beam((x,y,.98),(x*.52,y*.52,3.5),.18,0,6)
            if lod==0:
                for dx in [-.35,.35]:
                    for dy in [-.35,.35]: q.beam((x+dx,y+dy,.97),(x+dx,y+dy,1.12),.05,2,6)
    q.tube(1.4,3.4,1.53,1.48,0,sides)
    q.tube(3.4,7.12,1.48,1.02,0,sides)
    q.tube(7.12,7.64,1.2,1.2,3,sides*2)
    q.tube(7.64,7.8,1.35,1.35,2,sides*2)
    q.ring(1.5,3.4,.09,2,sides)
    for a in [math.pi*.25+i*math.pi*.5 for i in range(4)]:
        x,y=math.cos(a),math.sin(a)
        q.beam((2.35*x,2.35*y,1.25),(1.06*x,1.06*y,6.85),.14,0,6)
        if lod<2: q.beam((2.35*x,2.35*y,1.25),(1.48*x,1.48*y,3.5),.075,2,6)
    # North-facing controller box and cabling make orientation unambiguous.
    q.box((0,-1.54,4.25),(1.12,.47,1.65),0)
    q.box((0,-1.79,4.34),(.82,.045,1.25),2)
    q.box((.34,-1.84,4.17),(.04,.06,.25),3)
    if lod<2:
        q.beam((.81,-1.26,2),(.81,-1.26,6.7),.055,3,6)
        # Fixed ladder, opposite the controller door.
        for x in [-.40,.40]: q.beam((x,1.62,1.0),(x,1.62,7.7),.055,2,6)
        for i in range(22): q.beam((-.40,1.64,1.1+i*.3),(.40,1.64,1.1+i*.3),.038,2,6)
        if lod==0:
            for z in [3,4,5,6,7]:
                for i in range(8):
                    a=math.pi*i/8; b=math.pi*(i+1)/8
                    q.beam((.62*math.cos(a),1.59+.62*math.sin(a),z),(.62*math.cos(b),1.59+.62*math.sin(b),z),.024,2,5)
    return q

def azimuth(lod):
    q=MeshBuilder(); n=[48,32,16][lod]
    q.tube(0,.34,1.38,1.38,0,n)
    q.box((0,0,.65),(4.8,2.4,.65),0)
    # Tall split yoke with open center and inclined cheek struts.
    for x in [-2.15,2.15]:
        for y in [-.76,.76]:
            q.beam((x,y,.78),(x,y*.37,4.6),.29,0,6,r2=.25)
        q.box((x,0,4.38),(.64,1.04,.68),0)
        q.beam((x-.40,0,4.6),(x+.40,0,4.6),.52,2,n//2)
        q.beam((x-.44,0,4.6),(x+.44,0,4.6),.32,3,n//2)
        q.beam((x,-.76,1.15),(x,.75,3.4),.12,2,6)
        q.beam((x,.76,1.15),(x,-.75,3.4),.12,2,6)
    q.box((0,-1.72,.38),(5.6,1.25,.16),2)
    q.box((0,-1.7,.25),(5.5,1.12,.10),3)
    # Service platform follows azimuth, fixed ladder terminates below it.
    for x in [-2.72,-1.35,0,1.35,2.72]:
        q.beam((x,-2.27,.45),(x,-2.27,1.57),.04,2,6)
    for z in [.98,1.57]: q.beam((-2.72,-2.27,z),(2.72,-2.27,z),.04,2,6)
    for x in [-2.72,2.72]:
        q.beam((x,-2.27,1.57),(x,-1.15,1.57),.04,2,6)
        q.beam((x,-1.15,.45),(x,-1.15,1.57),.04,2,6)
    if lod<2:
        for i in range(18): q.box((-2.5+i*.294,-1.72,.48),(.04,1.1,.025),2)
        q.box((1.6,-1.5,.92),(.65,.65,.8),0)
        q.beam((-.75,-1.25,.9),(-.75,-1.25,3.65),.07,3,6)
        for x in [-2.5,-1.8]: q.beam((x,.50,1),(x,.50,4.5),.04,2,6)
        for i in range(11): q.beam((-2.5,.5,1.1+i*.3),(-1.8,.5,1.1+i*.3),.028,2,5)
    return q

def surface(r,a,extra=0): return (r*math.cos(a),r*math.sin(a),1.1+r*r/28.8+extra)

def elevation(lod):
    q=MeshBuilder(); sectors=[48,32,24][lod]; rings=[8,5,3][lod]
    # Individually surfaced panel bays and narrow inset seams.
    for j in range(rings):
        r0=9*j/rings; r1=9*(j+1)/rings
        for i in range(sectors):
            a0=math.tau*i/sectors; a1=math.tau*(i+1)/sectors
            gap=.008 if lod==0 else .004
            rr0=r0+gap if j else 0
            rr1=r1-gap
            aa=gap/max(1,r0)
            q.face([surface(rr0,a0+aa),surface(rr1,a0+aa),surface(rr1,a1-aa),surface(rr0,a1-aa)],1)
            # Bottom skin gives solid geometry and under-surface shadowing.
            q.face([surface(r0,a1,-.055),surface(r1,a1,-.055),surface(r1,a0,-.055),surface(r0,a0,-.055)],0)
    q.ring(9,surface(9,0)[2]-.025,.095,0,sectors*2)
    ribn=[32,24,16][lod]; ringrs=[1.8,3.2,4.8,6.4,7.8,9]
    if lod==2: ringrs=[2.5,5.5,9]
    def back(r,a): return surface(r,a,-(.35+.60*math.sin(math.pi*r/18)))
    # Deep radial triangular trusses; all struts are actual open geometry.
    for i in range(ribn):
        a=math.tau*i/ribn
        radii=[.85]+ringrs
        for j in range(len(radii)-1):
            r0,r1=radii[j:j+2]
            q.beam(surface(r0,a,-.10),surface(r1,a,-.10),.062,2,5)
            q.beam(back(r0,a),back(r1,a),.072,0,5)
            q.beam(surface(r0,a,-.10),back(r1,a),.044,2,5)
            q.beam(back(r0,a),surface(r0,a,-.10),.047,0,5)
        q.beam(back(radii[-1],a),surface(9,a,-.1),.048,0,5)
    for j,r in enumerate(ringrs):
        for i in range(ribn):
            a=math.tau*i/ribn; b=math.tau*(i+1)/ribn
            q.beam(back(r,a),back(r,b),.060,0,5)
            if lod<2 and j<len(ringrs)-1:
                q.beam(back(r,a),back(ringrs[j+1],b),.035,2,5)
    # Central elevation cradle and X-axis trunnion match the pivot contract.
    q.box((0,0,.39),(3.4,2.45,.75),0)
    q.beam((-2.6,0,0),(2.6,0,0),.28,2,16)
    for x in [-1.4,1.4]:
        q.beam((x,0,0),(x*2.1,0,1.0),.19,0,8)
        q.beam((x,-.8,.38),(x*2.1,-2.2,1.05),.12,0,6)
        q.beam((x,.8,.38),(x*2.1,2.2,1.05),.12,0,6)
    # Four quadrupod feed legs converge on a compact receiver, not a mast.
    for i in range(4):
        a=math.pi/4+i*math.pi/2
        foot=surface(7.5,a,.03)
        top=(.34*math.cos(a),.34*math.sin(a),8.3)
        q.beam(foot,top,.105,0,8,r2=.065)
        if lod<2:
            q.beam(surface(7.13,a,.03),(.54*math.cos(a),.54*math.sin(a),7.7),.035,2,5)
            q.beam((foot[0],foot[1],foot[2]-.1),(foot[0],foot[1],foot[2]+.3),.16,2,6)
    q.tube(7.72,8.3,.35,.43,0,16)
    q.tube(7.2,7.72,.12,.30,2,12)
    q.box((0,0,8.4),(.8,.72,.24),0)
    q.beam((0,0,8.48),(0,0,9.09),.042,2,8)
    q.beam((0,0,9.08),(0,0,9.2),.095,7,10)
    if lod==0:
        for z in [7.85,7.98,8.11,8.24]: q.ring(.43,z,.015,2,16)
        # Receiver access hoop and cable conduit, visible in silhouette.
        for i in range(4):
            a=math.pi*i*.5; b=math.pi*(i+1)*.5
            q.beam((.62*math.cos(a),.62*math.sin(a),8.67),(.62*math.cos(b),.62*math.sin(b),8.67),.025,2,5)
        q.beam((.5,0,8.4),(.5,0,8.67),.025,2,5)
    return q

builders={'Base':base,'Azimuth':azimuth,'Elevation':elevation}
offsets={'Base':0.,'Azimuth':7.8,'Elevation':12.4}
objects={}
manifest={'author':'Original procedural geometry for SIGNAL / 47, 2026-10-23','license':'Original project asset; no photo pixels or third-party geometry included.','units':'meters','up_axis':'Z','fbx_axis_forward':'-Y','fbx_axis_up':'Z','fbx_apply_unit_scale':True,'pivot_world_z_m':offsets,'elevation_rotation_axis':'X','azimuth_rotation_axis':'Z','materials':[],'meshes':{}}
for name,color,metal,rough in PALETTE: manifest['materials'].append({'name':name,'base_color_linear':color,'metallic':metal,'roughness':rough,'emission':[0,0,0,1],'emission_strength':0,'alpha':1,'textures':{}})
(ROOT/'materials.json').write_text(json.dumps(manifest['materials'],indent=2)+'\n')
for part,builder in builders.items():
    for lod in range(3):
        name='SM_Array_'+part+('' if lod==0 else '_LOD'+str(lod))
        obj=builder(lod).obj(name)
        objects[name]=obj
        bpy.ops.object.select_all(action='DESELECT'); obj.select_set(True); bpy.context.view_layer.objects.active=obj
        bpy.ops.export_scene.fbx(filepath=str(OUT/(name+'.fbx')),use_selection=True,object_types={'MESH'},global_scale=1.,apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',axis_forward='-Y',axis_up='Z',bake_space_transform=False,use_mesh_modifiers=True,mesh_smooth_type='FACE',use_triangles=True,add_leaf_bones=False,path_mode='AUTO')
        bpy.ops.export_scene.gltf(filepath=str(OUT/(name+'.glb')),use_selection=True,export_format='GLB',export_materials='EXPORT')
        me=obj.data; me.calc_loop_triangles()
        mins=[min(v.co[i] for v in me.vertices) for i in range(3)]
        maxs=[max(v.co[i] for v in me.vertices) for i in range(3)]
        manifest['meshes'][name]={'triangles':len(me.loop_triangles),'vertices':len(me.vertices),'materials':len(me.materials),'bounds_local_m':[mins,maxs],'origin_local_m':list(obj.location),'fbx':name+'.fbx','glb':name+'.glb'}
        obj.location.z=offsets[part]
        obj.hide_render=lod!=0; obj.hide_viewport=lod!=0
        obj['assembly_pivot_z_m']=offsets[part]
        obj['lod']=lod

# Save a clean authored assembly with all LODs and readable part names.
scene=bpy.context.scene
scene.unit_settings.system='METRIC'; scene.unit_settings.scale_length=1.0
scene.render.engine='CYCLES'; scene.cycles.device='CPU'; scene.cycles.samples=24
scene.cycles.use_denoising=True
scene.render.resolution_x=1280; scene.render.resolution_y=1280; scene.render.resolution_percentage=100
scene.world.color=(.2,.2,.2)
scene.view_settings.view_transform='AgX'
for part in builders:
    for lod in [1,2]: objects['SM_Array_'+part+'_LOD'+str(lod)].hide_set(True)
bpy.ops.object.select_all(action='DESELECT')
for p in builders: objects['SM_Array_'+p].select_set(True)
bpy.ops.export_scene.gltf(filepath=str(OUT/'SARO_Array_Assembly.glb'),use_selection=True,export_format='GLB',export_materials='EXPORT')
manifest['lod_triangle_totals']={str(lod):sum(manifest['meshes']['SM_Array_'+p+('' if lod==0 else '_LOD'+str(lod))]['triangles'] for p in builders) for lod in range(3)}
manifest['asymmetric_alignment_landmarks']={'base_controller_front_center_m':[0,-1.79,4.34],'azimuth_service_platform_front_center_m':[0,-2.27,.38],'elevation_trunnion_positive_X_end_m':[2.6,0,0],'elevation_reflector_vertex_m':[0,0,1.1],'elevation_feed_top_m':[0,0,8.3],'elevation_beacon_top_m':[0,0,9.2]}
(ROOT/'verification.json').write_text(json.dumps(manifest,indent=2)+'\n')

def light(name,loc,power,size,color):
    d=bpy.data.lights.new(name,'AREA'); d.energy=power; d.shape='DISK'; d.size=size; d.color=color
    ob=bpy.data.objects.new(name,d); bpy.context.collection.objects.link(ob); ob.location=loc
    ob.rotation_euler=(Vector((0,0,10))-ob.location).to_track_quat('-Z','Y').to_euler()
light('Key_softbox',(6,-16,30),9000,18,(1.0,.87,.69))
light('Fill_softbox',(-20,-7,13),6000,15,(.65,.78,1.0))
light('Rim_softbox',(10,16,20),9000,12,(.85,.92,1.0))
bpy.ops.mesh.primitive_plane_add(size=200,location=(0,0,-.03))
ground=bpy.context.object; ground.name='Preview_Ground'; ground.data.materials.append(MATS[4])
camdata=bpy.data.cameras.new('Preview_Camera'); cam=bpy.data.objects.new('Preview_Camera',camdata); bpy.context.collection.objects.link(cam); scene.camera=cam
camdata.type='ORTHO'; camdata.ortho_scale=29

def render(name,pos,target):
    cam.location=pos; cam.rotation_euler=(Vector(target)-cam.location).to_track_quat('-Z','Y').to_euler()
    scene.render.filepath=str(PREVIEW/(name+'.png'))
    if not SKIP_RENDER:
        bpy.ops.render.render(write_still=True)

# The source opens in its assembled engineering pose, dish pointing to zenith.
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'SARO_Array.blend'),compress=True)
render('array_underside',(27,-35,17),(0,0,10.4))
render('array_front',(24,-31,34),(0,0,11.4))
# A low pointing view demonstrates that all elevation geometry moves together.
objects['SM_Array_Elevation'].rotation_euler.x=math.radians(43)
render('array_tracking',(29,-37,18),(0,0,11.2))
render('array_back',(25,34,13),(0,0,11.2))
objects['SM_Array_Elevation'].rotation_euler.x=0
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'SARO_Array.blend'),compress=True)
print('ARRAY_DONE',json.dumps(manifest['lod_triangle_totals']))
