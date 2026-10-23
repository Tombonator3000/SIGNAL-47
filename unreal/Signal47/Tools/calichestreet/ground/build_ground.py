"""Original Caliche street surface dressing, Blender 5.2.2, no new collisions.

Run prepare_ground.py with system Python before this script. Generation does
not render by default. Pass -- --render only when the shared GPU is free.
"""
import bpy
import json
import math
from pathlib import Path
import random
import sys
from mathutils import Vector

ROOT=Path(__file__).resolve().parent
sys.path.insert(0,str(ROOT))
from terrain import ANCHOR, REGION, floor_y, clear_surface, clear_dressing, pavement, in_box, BUILDINGS

OUT=ROOT/'exports';OUT.mkdir(exist_ok=True)
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
bpy.context.scene.unit_settings.system='METRIC'
bpy.context.scene.unit_settings.scale_length=1
spec=json.loads((ROOT/'materials.json').read_text())
MATS=[]
for row in spec:
    m=bpy.data.materials.new(row['name']);m.use_nodes=True
    bs=m.node_tree.nodes.get('Principled BSDF')
    bs.inputs['Roughness'].default_value=row['roughness']
    for role,filename in row['textures'].items():
        img=bpy.data.images.load(str(ROOT/filename),check_existing=True)
        if role!='base_color':img.colorspace_settings.name='Non-Color'
        tex=m.node_tree.nodes.new('ShaderNodeTexImage');tex.image=img;tex.extension='REPEAT'
        if role=='normal':
            # Disk maps are DirectX. Blender's normal node expects OpenGL.
            sep=m.node_tree.nodes.new('ShaderNodeSeparateColor')
            inv=m.node_tree.nodes.new('ShaderNodeMath');inv.operation='SUBTRACT';inv.inputs[0].default_value=1
            combine=m.node_tree.nodes.new('ShaderNodeCombineColor')
            m.node_tree.links.new(tex.outputs['Color'],sep.inputs[0])
            m.node_tree.links.new(sep.outputs['Red'],combine.inputs['Red'])
            m.node_tree.links.new(sep.outputs['Green'],inv.inputs[1])
            m.node_tree.links.new(inv.outputs[0],combine.inputs['Green'])
            m.node_tree.links.new(sep.outputs['Blue'],combine.inputs['Blue'])
            norm=m.node_tree.nodes.new('ShaderNodeNormalMap')
            m.node_tree.links.new(combine.outputs[0],norm.inputs['Color'])
            m.node_tree.links.new(norm.outputs['Normal'],bs.inputs['Normal'])
        else:m.node_tree.links.new(tex.outputs['Color'],bs.inputs['Base Color' if role=='base_color' else 'Roughness'])
    MATS.append(m)

# A spatial index of the unchanged rendered triangles. The top sample uses
# road/desert/pavement only, never roof, plant card, prop or new terrain.
source=json.loads((ROOT/'contact_triangles.json').read_text())
BINS={}
for name,ts in source.items():
    for t in ts:
        a,b,c=[Vector(p) for p in t];cross=(b-a).cross(c-a)
        if cross.length<1e-10 or abs(cross.y)/cross.length<.94:continue
        for ix in range(math.floor(min(p[0] for p in t)/2),math.floor(max(p[0] for p in t)/2)+1):
            for iz in range(math.floor(min(p[2] for p in t)/2),math.floor(max(p[2] for p in t)/2)+1):
                BINS.setdefault((ix,iz),[]).append(t)

def top(x,z):
    hits=[];floor=floor_y(x,z)
    for a,b,c in BINS.get((math.floor(x/2),math.floor(z/2)),[]):
        den=(b[2]-c[2])*(a[0]-c[0])+(c[0]-b[0])*(a[2]-c[2])
        if abs(den)<1e-10:continue
        u=((b[2]-c[2])*(x-c[0])+(c[0]-b[0])*(z-c[2]))/den
        v=((c[2]-a[2])*(x-c[0])+(a[0]-c[0])*(z-c[2]))/den
        if min(u,v,1-u-v)<-1e-6:continue
        h=u*a[1]+v*b[1]+(1-u-v)*c[1]
        if floor-.12<=h<=floor+.16:hits.append(h)
    return max(hits,default=floor)

def point(x,z,lift=.008):return (x,-(z-2060),top(x,z)+lift)

class Mesh:
    def __init__(self):self.v=[];self.f=[];self.mat=[];self.uv=[];self.clearance=[]
    def face(self,vs,mat=0,uv=None):
        if len(vs)<3:return
        if (Vector(vs[1])-Vector(vs[0])).cross(Vector(vs[2])-Vector(vs[0])).length<1e-9:return
        n=len(self.v);self.v.extend(vs);self.f.append(tuple(range(n,n+len(vs))));self.mat.append(mat)
        self.uv.extend(uv or [(p[0]/4,-p[1]/4) for p in vs])
    def overlay(self,x,z,lift):
        p=point(x,z,lift);self.clearance.append({'xz':[x,z],'above_source_m':lift,'above_floor_m':p[2]-floor_y(x,z)})
        return p
    def grid(self,box,mat,step=.5,lift=.008,mask=lambda x,z:True):
        x0,x1,z0,z1=box;nx=math.ceil((x1-x0)/step);nz=math.ceil((z1-z0)/step)
        for j in range(nz):
            for i in range(nx):
                xa=x0+(x1-x0)*i/nx;xb=x0+(x1-x0)*(i+1)/nx
                za=z0+(z1-z0)*j/nz;zb=z0+(z1-z0)*(j+1)/nz
                if not all(mask(x,z) for x,z in [(xa,za),(xb,za),(xa,zb),(xb,zb),((xa+xb)/2,(za+zb)/2)]):continue
                a=self.overlay(xa,za,lift);b=self.overlay(xb,za,lift);c=self.overlay(xa,zb,lift);d=self.overlay(xb,zb,lift)
                # Blender's local Y is negative road Z; face normals point up.
                self.face([a,c,b],mat);self.face([b,c,d],mat)
    def ribbon(self,centers,width,mat,lift=.012,mask=clear_surface):
        for i in range(len(centers)-1):
            a,b=centers[i:i+2];dx=b[0]-a[0];dz=b[1]-a[1];ll=math.hypot(dx,dz)
            if ll<.001:continue
            px=-dz/ll*width*.5;pz=dx/ll*width*.5
            xzs=[(a[0]+px,a[1]+pz),(b[0]+px,b[1]+pz),(b[0]-px,b[1]-pz),(a[0]-px,a[1]-pz)]
            if not all(-30<=x<=40 and 2020<=z<=2090 and mask(x,z) for x,z in xzs):continue
            vs=[self.overlay(x,z,lift) for x,z in xzs]
            if Vector(vs[1])-Vector(vs[0])==Vector((0,0,0)):continue
            self.face(vs,mat)
    def object(self,name):
        me=bpy.data.meshes.new(name);me.from_pydata(self.v,[],self.f);me.update()
        for m in MATS:me.materials.append(m)
        for f,idx in zip(me.polygons,self.mat):f.material_index=idx
        uv=me.uv_layers.new(name='UVMap')
        for loop in me.loops:uv.data[loop.index].uv=self.uv[loop.vertex_index]
        ob=bpy.data.objects.new(name,me);bpy.context.collection.objects.link(ob)
        ob['collision']='disabled_visual_dressing';ob['anchor_world_three_m']=ANCHOR
        return ob

surfaces=Mesh();road=Mesh();rocks=Mesh();plants=Mesh();rng=random.Random(470047)
# Warm caliche fills the spaces between the already paved strips. The upper
# plate is 8 mm above the measured original surface. Perimeter cells retreat
# irregularly so the 70 m cell never appears as a rectangle from street level.
def soilmask(x,z):
    border=min(x+30,40-x,z-2020,2090-z)
    return border>.7+.45*math.sin(z*.61+x*.47) and clear_surface(x,z) and abs(x)>6
surfaces.grid(REGION,0,step=.65,mask=soilmask)
# Graded shoulders stop at the actual sidewalk/apron. These flat aggregate
# bands give the road edge much more readable material and scale variation.
for side in [-1,1]:
    box=[4.83*side,8.4*side,2020.5,2089.5];box[:2]=sorted(box[:2])
    surfaces.grid(box,1,step=.25,lift=.010,mask=lambda x,z:clear_surface(x,z) and not (z<2021 or z>2089))
# Pavement wear is a thin overlay over g1004. Existing wet/oil wear is +8 mm;
# the concrete field sits at +6 mm so those authored story/light marks remain.
for box in [(-16.9,-14.3,2041,2062.5),(-16.8,-13.8,2072,2087.8),(16.75,19.5,2025,2028.8),(16.75,18.85,2048.2,2056.2),(16.75,18.7,2072.2,2088.8)]:
    surfaces.grid(box,7,step=.35,lift=.006,mask=lambda x,z:not any(in_box(x,z,b,.1) for b in BUILDINGS))
# Keep both the yellow center marking and its immediate blacktop context.
# Outside 1.05 m from the center, worn aggregate varies in broad irregular
# fields rather than repeating a uniformly clean road strip.
for side in [-1,1]:
    def roadmask(x,z):
        inner=1.10+.13*math.sin(z*.31);outer=4.42+.035*math.sin(z*1.7)
        return inner<abs(x)<outer and 2020.5<z<2089.5
    bb=sorted([1.08*side,4.48*side])+[2020.25,2089.75]
    road.grid(bb,2,step=.28,lift=.008,mask=roadmask)
    for k in range(12):
        z=2022+k*5.35+rng.random();x=side*(1.8+rng.random()*1.6)
        centers=[(x+math.sin(t*.6+k)*.15,z+t*.43) for t in range(5)]
        road.ribbon(centers,.16+rng.random()*.38,6,lift=.012,mask=lambda x,z:1.1<abs(x)<4.43)
    # Thin dust fingers cross onto the outside blacktop edge, under 12 mm high.
    for k in range(15):
        z=2021+k*4.7;road.ribbon([(side*4.30,z),(side*4.72,z+.3),(side*5.15,z+.4)],.06+rng.random()*.09,1,lift=.013,mask=lambda x,z:abs(x)>4.1 and not pavement(x,z))
# Dry drainage is texture/pebbly sediment, with no excavated channel or new
# height field. Its branching outline follows the unchanged source surface.
for x,z,l in [(-25.6,2023,35),(-22.9,2038,22),(33,2044,40),(37.8,2053,31)]:
    pts=[(x+math.sin(i*.33)*.75,z+i*.65) for i in range(int(l/.65))]
    surfaces.ribbon(pts,.3,3,lift=.012)
    for i in range(6,len(pts)-6,12):
        a=pts[i];surfaces.ribbon([a,(a[0]+.7,a[1]-1),(a[0]+1.3,a[1]-1.8)],.14,1,lift=.012)

def rock(x,z,s):
    h=top(x,z)-s*.28;verts=[];n=7
    for zz,rr in [(0,.70),(.5,1),(1,.35)]:
        for i in range(n):
            a=math.tau*i/n;f=.84+rng.random()*.28
            verts.append((x+math.cos(a)*s*rr*f,-(z-2060)+math.sin(a)*s*rr*f,h+zz*s*.62))
    for j in range(2):
        for i in range(n):
            k=(i+1)%n;rocks.face([verts[j*n+i],verts[j*n+k],verts[(j+1)*n+k],verts[(j+1)*n+i]],4)
    rocks.face(verts[-n:],4)

def bush(x,z,s):
    h=top(x,z)-.015
    for i in range(15):
        a=rng.random()*math.tau;length=s*(.35+rng.random()*.6);lean=s*(.18+rng.random()*.22)
        root=(x,-(z-2060),h);tip=(x+math.cos(a)*lean,-(z-2060)+math.sin(a)*lean,h+length)
        w=.008+s*.015
        plants.face([(root[0]-w,root[1],root[2]),(root[0]+w,root[1],root[2]),tip],5,[(0,0),(1,0),(.5,1)])
        for j in [0,1]:
            p=Vector(root).lerp(Vector(tip),.4+j*.23);leaf=p+Vector((math.cos(a+.8)*s*.22,math.sin(a+.8)*s*.22,s*.11))
            plants.face([p,p+Vector((w*.9,w*.7,.01)),leaf],5,[(0,0),(1,0),(.5,1)])

clusters=[(-26,2027),(-22,2043),(-28.5,2059),(-29.2,2087),(34,2025),(35,2047),(33,2055),(35,2074),(38,2085)]
dressing=[]
for cx,cz in clusters:
    for k in range(16):
        a=rng.random()*math.tau;r=math.sqrt(rng.random())*2.1;x=cx+math.cos(a)*r;z=cz+math.sin(a)*r;s=.07+rng.random()*.22
        if -29.5<x<39.5 and 2020.5<z<2089.5 and clear_dressing(x,z,s) and math.hypot(x-38,z-2036)>2:
            rock(x,z,s);dressing.append({'kind':'rock','xz':[x,z],'radius_m':s})
    for k in range(5):
        x=cx+rng.uniform(-1.5,1.5);z=cz+rng.uniform(-1.5,1.5);s=rng.uniform(.22,.55)
        if -29.5<x<39.5 and 2020.5<z<2089.5 and clear_dressing(x,z,s) and math.hypot(x-38,z-2036)>2:
            bush(x,z,s);dressing.append({'kind':'dryplant','xz':[x,z],'radius_m':s})

manifest={'anchor_world_three_m':ANCHOR,'coordinate_mapping':'worldThree=(8000+bx,bz,2060-by)','import_policy':{'force_front_x_axis':True,'coordinate_system_policy':'MATCH_UP_FORWARD_AXES','import_rotation_yaw':90,'runtime_yaw':0},'collision':'NoCollision for every part','parts':{},'dressing':dressing}
for name,mesh in [('SM_Caliche_Surface',surfaces),('SM_Caliche_RoadWear',road),('SM_Caliche_Rocks',rocks),('SM_Caliche_DryPlants',plants)]:
    ob=mesh.object(name);ob.data.calc_loop_triangles()
    bounds=[[min(v.co[i] for v in ob.data.vertices) for i in range(3)],[max(v.co[i] for v in ob.data.vertices) for i in range(3)]]
    manifest['parts'][name]={'bounds_local_m':bounds,'triangles':len(ob.data.loop_triangles),'origin_m':list(ob.location),'material_slots':[m.name for m in ob.data.materials],'uv_layers':len(ob.data.uv_layers),'max_above_source_m':max((r['above_source_m'] for r in mesh.clearance),default=None),'max_above_floor_m':max((r['above_floor_m'] for r in mesh.clearance),default=None)}
    bpy.ops.object.select_all(action='DESELECT');ob.select_set(True);bpy.context.view_layer.objects.active=ob
    bpy.ops.export_scene.fbx(filepath=str(OUT/(name+'.fbx')),use_selection=True,object_types={'MESH'},axis_forward='-Y',axis_up='Z',global_scale=1,apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',use_triangles=True,mesh_smooth_type='FACE',bake_space_transform=False,add_leaf_bones=False,path_mode='AUTO')
    bpy.ops.export_scene.gltf(filepath=str(OUT/(name+'.glb')),use_selection=True,export_format='GLB')
(ROOT/'model_contract.json').write_text(json.dumps(manifest,indent=2)+'\n')
(ROOT/'surface_samples.json').write_text(json.dumps(surfaces.clearance+road.clearance,separators=(',',':'))+'\n')
# Portable Blender file contains the original texture images as packed data.
for image in bpy.data.images:
    if image.source=='FILE':image.pack()
bpy.ops.object.select_all(action='SELECT')
bpy.ops.export_scene.gltf(filepath=str(OUT/'Caliche_Ground_Assembly.glb'),use_selection=True,export_format='GLB')
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'Caliche_Ground.blend'),compress=True)
print('CALICHE_GROUND_GENERATED',json.dumps({k:v['triangles'] for k,v in manifest['parts'].items()}))

if '--render' in sys.argv:
    scene=bpy.context.scene;scene.render.engine='CYCLES';scene.cycles.device='CPU';scene.cycles.samples=16
    scene.render.resolution_x=1600;scene.render.resolution_y=1000;scene.render.resolution_percentage=100
    d=bpy.data.lights.new('InspectionSun','SUN');d.energy=3;ob=bpy.data.objects.new('InspectionSun',d);bpy.context.collection.objects.link(ob);ob.rotation_euler=(.4,-.6,-.6)
    d=bpy.data.cameras.new('InspectionCamera');cam=bpy.data.objects.new('InspectionCamera',d);bpy.context.collection.objects.link(cam);scene.camera=cam;d.lens=28
    for name,pos,target in [('ground_overview',(-31,43,32),(0,0,4.5)),('shoulder_player',(-3,8,6.0),(-13,-2,4.5)),('east_player',(8,12,6.0),(22,2,4.5))]:
        cam.location=pos;cam.rotation_euler=(Vector(target)-cam.location).to_track_quat('-Z','Y').to_euler();scene.render.filepath=str(ROOT/(name+'.png'));bpy.ops.render.render(write_still=True)
