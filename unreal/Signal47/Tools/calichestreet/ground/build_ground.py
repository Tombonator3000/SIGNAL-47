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
    if 'specular' in row:
        bs.inputs['Specular IOR Level'].default_value=row['specular']
    if row.get('vertex_alpha_opacity'):
        vertex=m.node_tree.nodes.new('ShaderNodeVertexColor');vertex.layer_name='Color'
        m.node_tree.links.new(vertex.outputs['Alpha'],bs.inputs['Alpha'])
        m.surface_render_method='DITHERED'
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
            norm.inputs['Strength'].default_value=row.get('normal_strength',.2)
            m.node_tree.links.new(combine.outputs[0],norm.inputs['Color'])
            m.node_tree.links.new(norm.outputs['Normal'],bs.inputs['Normal'])
        else:
            m.node_tree.links.new(tex.outputs['Color'],bs.inputs['Base Color' if role=='base_color' else 'Roughness'])
            if role=='base_color' and row.get('opacity_from_base_color'):
                if row.get('alpha_mode')=='masked':
                    clip=m.node_tree.nodes.new('ShaderNodeMath');clip.operation='GREATER_THAN';clip.inputs[1].default_value=.333
                    m.node_tree.links.new(tex.outputs['Alpha'],clip.inputs[0]);m.node_tree.links.new(clip.outputs[0],bs.inputs['Alpha'])
                else:m.node_tree.links.new(tex.outputs['Alpha'],bs.inputs['Alpha'])
                m.surface_render_method='DITHERED'
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
    def __init__(self):self.v=[];self.f=[];self.mat=[];self.uv=[];self.clearance=[];self.alpha=[];self.fade_stats={}
    def face(self,vs,mat=0,uv=None):
        # Sword-leaf tips converge to one point. Keep a real triangle rather
        # than emitting a zero-area triangle from a repeated quad endpoint.
        clean=[];clean_uv=[]
        source_uv=uv or [(p[0]/6,-p[1]/6) for p in vs]
        for p,t in zip(vs,source_uv):
            if not clean or (Vector(p)-Vector(clean[-1])).length_squared>1e-20:
                clean.append(p);clean_uv.append(t)
        if len(clean)>1 and (Vector(clean[0])-Vector(clean[-1])).length_squared<1e-20:
            clean.pop();clean_uv.pop()
        vs=clean;uv=clean_uv
        if len(vs)<3:return
        if (Vector(vs[1])-Vector(vs[0])).cross(Vector(vs[2])-Vector(vs[0])).length<1e-9:return
        n=len(self.v);self.v.extend(vs);self.f.append(tuple(range(n,n+len(vs))));self.mat.append(mat)
        self.uv.extend(uv)
        self.alpha.extend([1.0]*len(vs))
    def fade_edges(self,mat,width):
        # Determine every actual open edge from the accepted source grid,
        # including the holes around pavement and buildings. Duplicated face
        # vertices use the same XY key and therefore get identical alpha.
        # Alpha reaches zero AT the polygon boundary, not one cell before it.
        def key(p):return (round(p[0],6),round(p[1],6))
        edges={};positions={}
        for face,idx in zip(self.f,self.mat):
            if idx!=mat:continue
            for i in range(len(face)):
                a=key(self.v[face[i]]);b=key(self.v[face[(i+1)%len(face)]])
                edge=tuple(sorted([a,b]));edges[edge]=edges.get(edge,0)+1
                positions[a]=None;positions[b]=None
        boundary=[edge for edge,count in edges.items() if count==1]
        bins={}
        for a,b in boundary:
            for ix in range(math.floor((min(a[0],b[0])-width)/width),math.floor((max(a[0],b[0])+width)/width)+1):
                for iy in range(math.floor((min(a[1],b[1])-width)/width),math.floor((max(a[1],b[1])+width)/width)+1):
                    bins.setdefault((ix,iy),[]).append((a,b))
        for p in positions:
            distance=width
            for a,b in bins.get((math.floor(p[0]/width),math.floor(p[1]/width)),[]):
                dx=b[0]-a[0];dy=b[1]-a[1];den=dx*dx+dy*dy
                t=max(0,min(1,((p[0]-a[0])*dx+(p[1]-a[1])*dy)/den)) if den else 0
                distance=min(distance,math.hypot(p[0]-a[0]-t*dx,p[1]-a[1]-t*dy))
            t=max(0,min(1,distance/width));positions[p]=t*t*(3-2*t)
        for face,idx in zip(self.f,self.mat):
            if idx==mat:
                for vi in face:self.alpha[vi]=positions[key(self.v[vi])]
        self.fade_stats[MATS[mat].name]={'width_m':width,'open_edges':len(boundary),'boundary_alpha_max':max((positions[a] for edge in boundary for a in edge),default=0),'unique_points':len(positions),'alpha_zero':sum(a<1e-5 for a in positions.values()),'alpha_partial':sum(1e-5<a<.99999 for a in positions.values()),'alpha_one':sum(a>=.99999 for a in positions.values())}
    def overlay(self,x,z,lift):
        p=point(x,z,lift);self.clearance.append({'xz':[x,z],'above_source_m':lift,'above_floor_m':p[2]-floor_y(x,z)})
        return p
    def grid(self,box,mat,step=.5,lift=.008,mask=lambda x,z:True,x_breaks=()):
        x0,x1,z0,z1=box;nx=math.ceil((x1-x0)/step);nz=math.ceil((z1-z0)/step)
        xs=sorted(set([round(x0+(x1-x0)*i/nx,12) for i in range(nx+1)]+list(x_breaks)))
        for j in range(nz):
            for i in range(len(xs)-1):
                xa=xs[i];xb=xs[i+1]
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
    def blob(self,x,z,rx,rz,mat,lift=.012,seed=0,mask=clear_surface):
        # Radial, ragged perimeter: no long straight sides or rectangular
        # ribbon joins when viewed from a shallow player-camera angle.
        rr=random.Random(470000+seed);ring=[];count=19
        angle=rr.random()*math.tau
        for i in range(count):
            a=angle+math.tau*i/count;r=.78+rr.random()*.26
            ring.append((x+math.cos(a)*rx*r,z+math.sin(a)*rz*r))
        if not all(-30<=a<=40 and 2020<=b<=2090 and mask(a,b) for a,b in ring):return
        center=self.overlay(x,z,lift)
        for i,(a,b) in enumerate(ring):
            c,d=ring[(i+1)%count]
            self.face([center,self.overlay(c,d,lift),self.overlay(a,b,lift)],mat)
    def patch(self,x,z,width,length,mat,angle=0,lift=.012):
        cs=math.cos(angle);sn=math.sin(angle);vs=[]
        for xx,zz in [(-.5,-.5),(-.5,.5),(.5,.5),(.5,-.5)]:
            px=x+xx*width*cs-zz*length*sn;pz=z+xx*width*sn+zz*length*cs
            if not (-30<=px<=40 and 2020<=pz<=2090):return
            vs.append(self.overlay(px,pz,lift))
        self.face(vs,mat,[(0,0),(0,1),(1,1),(1,0)])
    def beam(self,a,b,r,mat=5,sides=4):
        a=Vector(a);b=Vector(b);d=(b-a).normalized();u=d.cross(Vector((0,0,1)))
        if u.length<.01:u=d.cross(Vector((0,1,0)))
        u.normalize();v=d.cross(u)
        for i in range(sides):
            aa=math.tau*i/sides;bb=math.tau*(i+1)/sides
            self.face([a+r*(u*math.cos(aa)+v*math.sin(aa)),a+r*(u*math.cos(bb)+v*math.sin(bb)),b+r*(u*math.cos(bb)+v*math.sin(bb)),b+r*(u*math.cos(aa)+v*math.sin(aa))],mat)
    def object(self,name):
        me=bpy.data.meshes.new(name);me.from_pydata(self.v,[],self.f);me.update()
        for m in MATS:me.materials.append(m)
        for f,idx in zip(me.polygons,self.mat):f.material_index=idx
        uv=me.uv_layers.new(name='UVMap')
        for loop in me.loops:uv.data[loop.index].uv=self.uv[loop.vertex_index]
        colors=me.color_attributes.new(name='Color',type='FLOAT_COLOR',domain='POINT')
        for i,c in enumerate(colors.data):c.color=(1,1,1,self.alpha[i])
        me.color_attributes.active_color=colors
        ob=bpy.data.objects.new(name,me);bpy.context.collection.objects.link(ob)
        ob['collision']='disabled_visual_dressing';ob['anchor_world_three_m']=ANCHOR
        return ob

surfaces=Mesh();road=Mesh();rocks=Mesh();plants=Mesh();rng=random.Random(470047)
# Caliche follows the existing rendered ground +8 mm. Every open mesh edge
# fades over 1.8 m through vertex alpha, including holes around original pave.
# A ragged opaque grid silhouette is NOT a soft transition by itself.
def soilmask(x,z):
    border=min(x+30,40-x,z-2020,2090-z)
    return border>.7 and clear_surface(x,z) and abs(x)>4.9
surfaces.grid(REGION,0,step=.65,mask=soilmask)
surfaces.fade_edges(0,1.8)
# Local alpha-feathered patches replace the old broad, hard-edged gravel bands.
# They reveal underlying soil between chips instead of drawing a new rectangle.
for side in [-1,1]:
    for i in range(32):
        z=2022.0+i*2.08;x=side*(5.9+.48*math.sin(i*1.71))
        if pavement(x,z):continue
        surfaces.patch(x,z,1.35,2.2,8,angle=.22*math.sin(i*1.43),lift=.012)
# Pavement wear is a thin overlay over g1004. Existing wet/oil wear is +8 mm;
# the concrete field sits at +6 mm so those authored story/light marks remain.
for x,z in [(-16.6,2044),(-16.4,2059),(-16.5,2075),(-16.3,2086),(18.8,2027),(19.1,2034.5),(18.9,2040.7),(18.6,2052.4),(19.0,2058.8),(19.0,2069.7),(18.5,2079.4),(18.4,2086.7)]:
    surfaces.patch(x,z,1.2,1.7,9,angle=.4*math.sin(z),lift=.006)
# Continuous lane wear has no 2.2m-wide old-blacktop centre stripe. Actual
# roadTexture.ts markings are only +/-0.1m x0.1m and +/-3.6m x0.12m.
# A fine cross-road grid samples the alpha holes without covering their paint.
road.grid([-4.7,4.7,2020.25,2089.75],2,step=.26,lift=.008,
          x_breaks=[-3.85,-3.69,-3.51,-3.35,-.38,-.18,0,.18,.38,3.35,3.51,3.69,3.85])
road.fade_edges(2,.45)
for face,mat in zip(road.f,road.mat):
    if mat!=2:continue
    for vi in face:
        x=abs(road.v[vi][0])
        # Entire yellow paint and a 2cm antialias margin stay fully uncovered.
        t=max(0,min(1,(x-.18)/.20));yellow=t*t*(3-2*t)
        # Preserve existing white edge paint too, with no broad colour stripe.
        t=max(0,min(1,(abs(x-3.6)-.09)/.16));white=t*t*(3-2*t)
        road.alpha[vi]=min(road.alpha[vi],yellow,white)
for side in [-1,1]:
    # Repairs/cracks now live inside the detailed asphalt image. Large opaque
    # blobs read as featureless polygons at player height, so none are drawn.
    # Thin dust fingers cross onto the outside blacktop edge, under 12 mm high.
    for k in range(15):
        z=2021.6+k*4.65
        if not pavement(side*5.0,z):road.patch(side*4.95,z,.8,1.2,9,angle=.18*math.sin(k),lift=.013)
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

def bush(x,z,s,sage=False,width=None):
    h=top(x,z)-.025;width=width or s*.95;tile=rng.randrange(4)
    u0=(tile%2)*.5+.002;u1=u0+.496;v0=1-(tile//2+1)*.5+.002;v1=v0+.496
    for i in range(3):
        a=i*math.pi/3+rng.uniform(-.1,.1);dx=math.cos(a)*width*.5;dy=math.sin(a)*width*.5
        plants.face([(x-dx,-(z-2060)-dy,h),(x+dx,-(z-2060)+dy,h),(x+dx,-(z-2060)+dy,h+s),(x-dx,-(z-2060)-dy,h+s)],11 if sage else 10,[(u0,v0),(u1,v0),(u1,v1),(u0,v1)])

def yucca(x,z,s):
    h=top(x,z)-.03
    for i in range(22):
        a=i*math.tau/22+rng.uniform(-.11,.11);length=s*rng.uniform(.65,1.1);last=None
        for j in range(4):
            t=j/3;r=length*(.12+t*.8);zz=h+length*(.22+math.sin(t*math.pi*.8)*.65)
            w=length*.035*math.sin((t*.9+.1)*math.pi)
            left=(x+math.cos(a)*r-math.sin(a)*w,-(z-2060)+math.sin(a)*r+math.cos(a)*w,zz)
            right=(x+math.cos(a)*r+math.sin(a)*w,-(z-2060)+math.sin(a)*r-math.cos(a)*w,zz)
            if last:plants.face([last[0],last[1],right,left],12,[(0,(j-1)/3),(1,(j-1)/3),(1,t),(0,t)])
            last=(left,right)

def tumbleweed(x,z,r):
    h=top(x,z)+r*.65-.04
    for i in range(8):
        a=i*math.pi/8+rng.uniform(-.09,.09);last=None
        for j in range(13):
            t=j*math.tau/12;rr=r*(.86+rng.random()*.22)
            p=(x+math.cos(a)*math.cos(t)*rr,-(z-2060)+math.sin(a)*math.cos(t)*rr,h+math.sin(t)*rr*.74)
            if last:plants.beam(last,p,.0045,5,3)
            last=p

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
            bush(x,z,s,sage=k%3==0);dressing.append({'kind':'dryplant','xz':[x,z],'radius_m':s})

# Three deliberately placed, compact clusters visible through the gap between
# radio and souvenir. They remain well outside the full main walk rectangle
# and the A02/studio/door protected boxes, rather than hiding behind façades.
for hero,(cx,cz) in enumerate([(23.8,2049.0),(26.3,2051.5),(28.3,2048.6)]):
    hero_rng=random.Random(47900+hero)
    for k in range(7):
        a=hero_rng.random()*math.tau;r=hero_rng.uniform(.15,.85)
        x=cx+math.cos(a)*r;z=cz+math.sin(a)*r;s=hero_rng.uniform(.22,.45)
        if clear_dressing(x,z,s):
            rock(x,z,s);dressing.append({'kind':'rock','xz':[x,z],'radius_m':s,'hero_cluster':hero})
    for k in range(5):
        x=cx+hero_rng.uniform(-.65,.65);z=cz+hero_rng.uniform(-.65,.65);s=hero_rng.uniform(.48,.78)
        if clear_dressing(x,z,s):
            bush(x,z,s,sage=k%2==0);dressing.append({'kind':'dryplant','xz':[x,z],'radius_m':s,'hero_cluster':hero})

# Façade skirts from the actual concept: low straw tufts and dusty sage at
# wall/porch corners, outside the real main-walk rectangle. Specific protected
# boxes leave the buzzer, entrances, window subject and moved A02 figure clear.
facade_clusters=[(19.82,2034.4),(19.84,2040.1),(20.7,2042.8),(22.2,2042.9),(24.2,2042.9),(27.2,2043.0),(29.6,2043.1),(20.1,2059.0),(20.1,2064.8),(20.1,2069.4),(21.0,2057.1),(23.3,2057.1),(26.0,2057.1),(28.9,2057.1),(29.9,2059.3),(30.1,2067.4),(-24.8,2065.0),(-20.8,2064.9),(-28.2,2064.9),(-28.8,2071.2)]
for cluster,(cx,cz) in enumerate(facade_clusters):
    rr=random.Random(5300+cluster)
    # Short grasses fit between the sidewalk and wall; larger sage is
    # concentrated round the exposed side corners and building gap.
    for k in range(8):
        x=cx+rr.uniform(-.22,.22);z=cz+rr.uniform(-.43,.43)
        width=rr.uniform(.18,.32);r=width*.5;s=rr.uniform(.25,.46)
        if clear_dressing(x,z,r,facade=True):
            bush(x,z,s,sage=k%4==0,width=width)
            dressing.append({'kind':'sage' if k%4==0 else 'grass','xz':[x,z],'radius_m':r,'facade':True,'facade_cluster':cluster})
    for k in range(7):
        x=cx+rr.uniform(-.4,.4);z=cz+rr.uniform(-.55,.55);s=rr.uniform(.055,.18)
        if clear_dressing(x,z,s,facade=True):
            rock(x,z,s);dressing.append({'kind':'rock','xz':[x,z],'radius_m':s,'facade':True})
    surfaces.patch(cx,cz,.75,1.25,9,angle=rr.uniform(-.5,.5),lift=.012)

for x,z,s in [(21.5,2047.2,.61),(28.9,2054.2,.75),(31.0,2058.0,.65),(-28.8,2062.8,.58),(31.3,2044.5,.67),(22.5,2072.5,.52)]:
    r=s*1.0
    if clear_dressing(x,z,r,facade=True):
        yucca(x,z,s);dressing.append({'kind':'yucca','xz':[x,z],'radius_m':r,'facade':True})
for x,z,r in [(23.0,2055.0,.34),(30.9,2048.0,.39),(-27.8,2063.6,.32),(30.9,2072.0,.36),(-22.0,2046.5,.31),(33.0,2079.0,.40)]:
    if clear_dressing(x,z,r,facade=True):
        tumbleweed(x,z,r);dressing.append({'kind':'tumbleweed','xz':[x,z],'radius_m':r,'facade':True})

# R3 native view: make a few readable organic groups at exposed building
# corners, with a broad sage silhouette and smaller grass satellites. Never
# scatter tall details across the street or photo/door approach rectangle.
for hero,(cx,cz) in enumerate([(20.92,2043.15),(20.32,2056.48),(20.28,2071.08),(-18.42,2089.02)]):
    rr=random.Random(6500+hero)
    if clear_dressing(cx,cz,.50,facade=True):
        bush(cx,cz,.69,sage=True,width=1.0)
        dressing.append({'kind':'sage','xz':[cx,cz],'radius_m':.50,'facade':True,'visible_corner_group':hero,'primary_sage':True,'height_m':.69,'width_m':1.0})
    for k in range(8):
        a=rr.random()*math.tau;r=rr.uniform(.16,.82)
        x=cx+math.cos(a)*r;z=cz+math.sin(a)*r
        width=rr.uniform(.45,.62) if k<2 else rr.uniform(.19,.34)
        radius=width*.5;height=rr.uniform(.42,.58) if k<2 else rr.uniform(.28,.47)
        if clear_dressing(x,z,radius,facade=True):
            bush(x,z,height,sage=k<2,width=width)
            dressing.append({'kind':'sage' if k<2 else 'grass','xz':[x,z],'radius_m':radius,'facade':True,'visible_corner_group':hero})
    surfaces.patch(cx,cz,1.35,1.8,9,angle=rr.uniform(-.8,.8),lift=.012)

# Tiny roots beside the radio bench's north end fit in the real 0.2m strip
# outside the walking rectangle. They do not project into the bench approach.
for x,z in [(19.73,2041.72),(19.72,2041.90),(19.74,2039.68)]:
    r=.065
    if clear_dressing(x,z,r,facade=True):
        bush(x,z,.33,width=r*2)
        dressing.append({'kind':'grass','xz':[x,z],'radius_m':r,'facade':True,'beside_bench':True})

manifest={'anchor_world_three_m':ANCHOR,'coordinate_mapping':'worldThree=(8000+bx,bz,2060-by)','import_policy':{'force_front_x_axis':True,'coordinate_system_policy':'MATCH_UP_FORWARD_AXES','import_rotation_yaw':90,'runtime_yaw':0},'collision':'NoCollision for every part','parts':{},'dressing':dressing}
for name,mesh in [('SM_Caliche_Surface',surfaces),('SM_Caliche_RoadWear',road),('SM_Caliche_Rocks',rocks),('SM_Caliche_DryPlants',plants)]:
    ob=mesh.object(name);ob.data.calc_loop_triangles()
    bounds=[[min(v.co[i] for v in ob.data.vertices) for i in range(3)],[max(v.co[i] for v in ob.data.vertices) for i in range(3)]]
    manifest['parts'][name]={'bounds_local_m':bounds,'triangles':len(ob.data.loop_triangles),'origin_m':list(ob.location),'material_slots':[m.name for m in ob.data.materials],'used_materials':sorted(set(MATS[i].name for i in mesh.mat)),'uv_layers':len(ob.data.uv_layers),'vertex_color_attribute':'Color','vertex_alpha_min':min(mesh.alpha),'vertex_alpha_max':max(mesh.alpha),'edge_fade':mesh.fade_stats,'max_above_source_m':max((r['above_source_m'] for r in mesh.clearance),default=None),'max_above_floor_m':max((r['above_floor_m'] for r in mesh.clearance),default=None)}
    bpy.ops.object.select_all(action='DESELECT');ob.select_set(True);bpy.context.view_layer.objects.active=ob
    bpy.ops.export_scene.fbx(filepath=str(OUT/(name+'.fbx')),use_selection=True,object_types={'MESH'},axis_forward='-Y',axis_up='Z',global_scale=1,apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',use_triangles=True,mesh_smooth_type='FACE',bake_space_transform=False,add_leaf_bones=False,path_mode='AUTO',colors_type='LINEAR',prioritize_active_color=True)
    bpy.ops.export_scene.gltf(filepath=str(OUT/(name+'.glb')),use_selection=True,export_format='GLB',export_vertex_color='ACTIVE',export_all_vertex_colors=False)
(ROOT/'model_contract.json').write_text(json.dumps(manifest,indent=2)+'\n')
(ROOT/'surface_samples.json').write_text(json.dumps(surfaces.clearance+road.clearance,separators=(',',':'))+'\n')
# Portable Blender file contains the original texture images as packed data.
for image in bpy.data.images:
    if image.source=='FILE':image.pack()
bpy.ops.object.select_all(action='SELECT')
bpy.ops.export_scene.gltf(filepath=str(OUT/'Caliche_Ground_Assembly.glb'),use_selection=True,export_format='GLB',export_vertex_color='ACTIVE',export_all_vertex_colors=False)
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'Caliche_Ground.blend'),compress=True)
print('CALICHE_GROUND_GENERATED',json.dumps({k:v['triangles'] for k,v in manifest['parts'].items()}))

if '--render' in sys.argv:
    scene=bpy.context.scene;scene.render.engine='CYCLES';scene.cycles.device='CPU';scene.cycles.samples=16
    scene.render.resolution_x=1600;scene.render.resolution_y=1000;scene.render.resolution_percentage=100
    d=bpy.data.lights.new('InspectionSun','SUN');d.energy=3;ob=bpy.data.objects.new('InspectionSun',d);bpy.context.collection.objects.link(ob);ob.rotation_euler=(.4,-.6,-.6)
    d=bpy.data.cameras.new('InspectionCamera');cam=bpy.data.objects.new('InspectionCamera',d);bpy.context.collection.objects.link(cam);scene.camera=cam;d.lens=28
    for name,pos,target in [('ground_overview',(-31,43,32),(0,0,4.5)),('shoulder_player',(-3,8,6.0),(-13,-2,4.5)),('east_player',(8,12,6.0),(22,2,4.5))]:
        cam.location=pos;cam.rotation_euler=(Vector(target)-cam.location).to_track_quat('-Z','Y').to_euler();scene.render.filepath=str(ROOT/(name+'.png'));bpy.ops.render.render(write_still=True)
