"""Original Hereford production meshes. Blender 5.2, no downloaded geometry.

Run: blender -b -t 2 --python build_cows.py [-- --render]
Model axes: Blender Z up, +Y forward; measured Unreal FBX recipe maps (x,-y,z)*100.
Concept references are preserved in Concepts/ and embedded as image reference objects.
"""
from pathlib import Path
import bpy, bmesh, math, json, sys, hashlib
from mathutils import Vector, Matrix
import numpy as np

HERE=Path(__file__).resolve().parent
sys.path.insert(0,str(HERE))
from mesh_cleanup import clean_geometry
OUT=HERE/'Export'; TEX=HERE/'textures'; PRE=HERE/'Preview'
for p in (OUT,TEX,PRE): p.mkdir(exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
scene=bpy.context.scene;scene.name='SIGNAL47_Herefords'
scene.unit_settings.system='METRIC';scene.unit_settings.scale_length=1
scene.render.engine='CYCLES';scene.cycles.device='CPU';scene.cycles.samples=40
scene.render.threads_mode='FIXED';scene.render.threads=2
scene.render.resolution_x=1400;scene.render.resolution_y=1000;scene.render.resolution_percentage=100
scene.render.image_settings.file_format='PNG'
scene.world=bpy.data.worlds.new('Neutral_studio_world');scene.world.color=(.18,.18,.18)
scene.view_settings.view_transform='AgX'
scene.render.film_transparent=False

def collection(name):
    c=bpy.data.collections.new(name);scene.collection.children.link(c);return c
LIVE=collection('01_LIVE_HEREFORD_EDITABLE')
DEAD=collection('02_ABDUCTED_ALDERETE25_EDITABLE')
STUDIO=collection('03_PREVIEW_STUDIO')
REFERENCES=collection('04_CONCEPT_REFERENCES')
LOD=collection('05_EXPORTED_LODS')
REFERENCES.hide_render=True

ROWS=[];MATS={}
def image(name,rgba):
    n=rgba.shape[0];im=bpy.data.images.new(name,n,n,alpha=True)
    im.pixels.foreach_set(rgba.astype(np.float32).ravel());im.filepath_raw=str(TEX/(name+'.png'));im.file_format='PNG';im.save()
    filepath=im.filepath_raw
    bpy.data.images.remove(im)
    return bpy.data.images.load(filepath,check_existing=True)
N=512;yy,xx=np.mgrid[0:N,0:N].astype(float)/N
rng=np.random.default_rng(47025)
grain=np.zeros((N,N));fibers=np.zeros((N,N))
for k in range(30):
    fx=int(rng.integers(2,48));fy=int(rng.integers(1,8));phase=rng.random()*math.tau
    grain+=np.sin(math.tau*(fx*xx+fy*yy)+phase)/(1+k*.18)
for k in range(18):
    fx=int(rng.integers(48,180));fy=int(rng.integers(3,30));phase=rng.random()*math.tau
    fibers+=np.sin(math.tau*(fx*xx+fy*yy)+phase)/(1+k*.15)
grain/=max(abs(grain.min()),grain.max());fibers/=max(abs(fibers.min()),fibers.max())
detail=grain*.52+fibers*.48

def material(name,color,rough=.75,fur=False,metal=0,spec=.28):
    m=bpy.data.materials.new(name);m.use_nodes=True;p=m.node_tree.nodes.get('Principled BSDF')
    p.inputs['Base Color'].default_value=(*color,1);p.inputs['Roughness'].default_value=rough;p.inputs['Metallic'].default_value=metal;p.inputs['Specular IOR Level'].default_value=spec
    if fur and 'Sheen Weight' in p.inputs:p.inputs['Sheen Weight'].default_value=.16
    var=detail*(.13 if fur else .06)
    base=np.ones((N,N,4),dtype=float)
    base[:,:,:3]=np.clip(np.array(color)[None,None,:]*(1+var[:,:,None]),0,1)
    # PNG RGB is sRGB; material rows use white tint, preserving authored texture colour.
    base[:,:,:3]=np.where(base[:,:,:3]<=.0031308,base[:,:,:3]*12.92,1.055*base[:,:,:3]**(1/2.4)-.055)
    baseim=image(name+'_base_color',base);baseim.colorspace_settings.name='sRGB'
    roughmap=np.ones((N,N,4));roughmap[:,:,:3]=np.clip(rough+detail[:,:,None]*.06,0,1)
    roughim=image(name+'_roughness',roughmap);roughim.colorspace_settings.name='Non-Color'
    h=detail*(.17 if fur else .055);dx=(np.roll(h,-1,axis=1)-np.roll(h,1,axis=1));dy=(np.roll(h,-1,axis=0)-np.roll(h,1,axis=0))
    normal=np.stack([-dx,dy,np.ones_like(dx)],axis=-1);normal/=np.linalg.norm(normal,axis=-1)[:,:,None]
    normrgba=np.ones((N,N,4));normrgba[:,:,:3]=normal*.5+.5
    normalim=image(name+'_normal',normrgba);normalim.colorspace_settings.name='Non-Color'
    nodes=m.node_tree.nodes;links=m.node_tree.links
    b=nodes.new('ShaderNodeTexImage');b.image=baseim;links.new(b.outputs['Color'],p.inputs['Base Color'])
    rr=nodes.new('ShaderNodeTexImage');rr.image=roughim;links.new(rr.outputs['Color'],p.inputs['Roughness'])
    nm=nodes.new('ShaderNodeTexImage');nm.image=normalim
    # Source normal PNG is DirectX. Invert green for Blender's OpenGL tangent convention.
    sep=nodes.new('ShaderNodeSeparateXYZ');comb=nodes.new('ShaderNodeCombineXYZ');inv=nodes.new('ShaderNodeMath');inv.operation='SUBTRACT';inv.inputs[0].default_value=1
    nn=nodes.new('ShaderNodeNormalMap');nn.inputs['Strength'].default_value=.45
    for a,b in [(nm.outputs['Color'],sep.inputs[0]),(sep.outputs['X'],comb.inputs['X']),(sep.outputs['Y'],inv.inputs[1]),(inv.outputs[0],comb.inputs['Y']),(sep.outputs['Z'],comb.inputs['Z']),(comb.outputs[0],nn.inputs['Color']),(nn.outputs[0],p.inputs['Normal'])]:links.new(a,b)
    ROWS.append({'name':name,'base_color_linear':[1,1,1,1],'roughness':rough,'metallic':metal,'specular':spec,'textures':{'base_color':'textures/'+name+'_base_color.png','normal':'textures/'+name+'_normal.png','roughness':'textures/'+name+'_roughness.png'},'normal_convention':'DirectX','original_generation':'deterministic short-fibre height and colour field, seed47025'})
    MATS[name]=m;return m

RED=material('M_Cow_RedHide',(.19,.043,.014),.88,True)
WHITE=material('M_Cow_WhiteHide',(.64,.60,.49),.9,True)
NOSE=material('M_Cow_Muzzle',(.32,.17,.13),.68)
HOOF=material('M_Cow_Hoof',(.052,.044,.032),.72)
HORN=material('M_Cow_Horn',(.44,.36,.23),.67)
EYE=material('M_Cow_Eye',(.008,.006,.004),.2,spec=.5)
EAR=material('M_Cow_EarInner',(.15,.052,.029),.87,True)
TAG=material('M_Cow_YellowTag',(.62,.40,.012),.58)
BONE=material('M_Cow_DryBone',(.45,.40,.28),.84)
CUT=material('M_Cow_DryCut',(.021,.014,.01),.95)
FROST=material('M_Cow_Frost',(.53,.61,.64),.9)

def patterned_hide(name,kind):
    m=RED.copy();m.name=name
    if kind=='body':
        y=xx*2.4-1.2;z=yy*1.6
        edge=.018*np.sin(y*51)+.009*np.sin(y*117)+.006*grain
        belly=np.clip((.68+.045*np.sin(y*7)+edge-z)/.012+.5,0,1)*np.clip((.57-np.abs(y))/.09,0,1)
        chest=np.clip((y-.54)/.09,0,1)*np.clip((1.01+edge-z)/.018,0,1)*np.clip((z-.73)/.06,0,1)
        socks=np.clip((.28+.025*np.sin(y*37)+edge-z)/.012+.5,0,1)
        mask=np.maximum(np.maximum(belly,chest),socks)
    else:
        y=xx*1.3-.2;z=yy*.9-.3
        edge=.012*np.sin(z*61)+.006*np.sin(z*143)
        mask=np.clip((y-.355+edge)/.010+.5,0,1)
    col=np.array([.19,.043,.014])[None,None,:]*(1-mask[:,:,None])+np.array([.64,.60,.49])[None,None,:]*mask[:,:,None]
    col*=1+detail[:,:,None]*.11
    rgba=np.ones((N,N,4));rgba[:,:,:3]=np.where(col<=.0031308,col*12.92,1.055*np.maximum(col,0)**(1/2.4)-.055)
    im=image(name+'_base_color',rgba);im.colorspace_settings.name='sRGB'
    base=next(n for n in m.node_tree.nodes if n.type=='TEX_IMAGE' and 'base_color'in n.image.name);base.image=im
    row=dict(ROWS[0]);row['name']=name;row['textures']=dict(row['textures']);row['textures']['base_color']='textures/'+name+'_base_color.png';row['original_generation']='UV-authored Hereford markings with short-fibre PBR, seed47025';ROWS.append(row);MATS[name]=m
    return m
BODY_HIDE=patterned_hide('M_Cow_BodyHide','body');HEAD_HIDE=patterned_hide('M_Cow_HeadHide','head')

def move_to(ob,c):
    for old in list(ob.users_collection):old.objects.unlink(ob)
    c.objects.link(ob)
def mesh(name,verts,faces,mat=RED,c=LIVE):
    me=bpy.data.meshes.new(name);me.from_pydata(verts,[],faces);me.update();ob=bpy.data.objects.new(name,me);c.objects.link(ob);me.materials.append(mat)
    for f in me.polygons:f.use_smooth=True
    return ob
def active(ob):
    bpy.ops.object.select_all(action='DESELECT');ob.select_set(True);bpy.context.view_layer.objects.active=ob
def apply(ob,mod):active(ob);bpy.ops.object.modifier_apply(modifier=mod.name)
def join(obs,name):
    for o in list(bpy.context.selected_objects):o.select_set(False)
    for o in obs:o.select_set(True)
    bpy.context.view_layer.objects.active=obs[0];bpy.ops.object.join();obs[0].name=name;return obs[0]
def cat(a,b,c,d,t):return .5*((2*b)+(-a+c)*t+(2*a-5*b+4*c-d)*t*t+(-a+3*b-3*c+d)*t*t*t)
def smooth_rows(rows,steps=4):
    rows=[np.array(r,dtype=float)for r in rows];result=[]
    for i in range(len(rows)-1):
        for j in range(steps):result.append(cat(rows[max(i-1,0)],rows[i],rows[i+1],rows[min(i+2,len(rows)-1)],j/steps))
    result.append(rows[-1]);return result
def loft_y(name,rows,mat=RED,c=LIVE,sides=32):
    # y, centre-z, lateral radius, vertical radius; smoothly controlled animal silhouette.
    rr=smooth_rows(rows);v=[];f=[]
    for y,z,rx,rz in rr:
        for k in range(sides):
            a=math.tau*k/sides;v.append((rx*math.cos(a),y,z+rz*math.sin(a)))
    for j in range(len(rr)-1):
        for k in range(sides):f.append((j*sides+k,j*sides+(k+1)%sides,(j+1)*sides+(k+1)%sides,(j+1)*sides+k))
    f.extend([tuple(reversed(range(sides))),tuple((len(rr)-1)*sides+k for k in range(sides))])
    return mesh(name,v,f,mat,c)
def tube(name,rows,mat=RED,c=LIVE,sides=16):
    # Path cross-sections perpendicular to the local tangent; radii taper at each joint.
    rr=smooth_rows(rows,3);v=[];f=[]
    for i,r in enumerate(rr):
        center=Vector(r[:3]);tangent=Vector(rr[min(i+1,len(rr)-1)][:3])-Vector(rr[max(0,i-1)][:3]);tangent.normalize()
        ex=tangent.cross(Vector((0,1,0)))
        if ex.length<.01:ex=tangent.cross(Vector((1,0,0)))
        ex.normalize();ey=tangent.cross(ex).normalized()
        for k in range(sides):
            a=math.tau*k/sides;v.append(center+ex*math.cos(a)*r[3]+ey*math.sin(a)*r[4])
    for j in range(len(rr)-1):
        for k in range(sides):f.append((j*sides+k,j*sides+(k+1)%sides,(j+1)*sides+(k+1)%sides,(j+1)*sides+k))
    f.extend([tuple(reversed(range(sides))),tuple((len(rr)-1)*sides+k for k in range(sides))]);return mesh(name,v,f,mat,c)
def ellipsoid(name,at,scale,mat,c=LIVE,segments=24,rings=12):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=segments,ring_count=rings,location=at);ob=bpy.context.object;ob.name=name;ob.scale=scale;move_to(ob,c);active(ob);bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);ob.data.materials.append(mat)
    for f in ob.data.polygons:f.use_smooth=True
    return ob
def bake_loc(ob):
    active(ob);bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
def material_regions(ob,kind):
    ob.data.materials.clear();ob.data.materials.append(BODY_HIDE if kind=='body'else HEAD_HIDE)
    for f in ob.data.polygons:f.material_index=0

body=loft_y('Body_anatomical_loft',[
    [-.97,1.04,.09,.20],[-.87,1.02,.28,.35],[-.65,1.0,.39,.40],[-.35,.99,.43,.41],
    [0,.99,.455,.41],[.30,1.0,.40,.41],[.53,1.04,.35,.39],[.73,1.07,.27,.33],[.87,1.10,.18,.25],[.94,1.10,.055,.16]])
flesh=[body]
for s in (-1,1):
    flesh.append(tube('Foreleg_'+str(s),[(s*.29,.60,1.1,.14,.16),(s*.29,.61,.86,.115,.13),(s*.28,.63,.60,.084,.09),(s*.27,.64,.46,.055,.065),(s*.25,.69,.24,.045,.054),(s*.25,.70,.13,.065,.065)]))
    flesh.append(tube('Hindleg_'+str(s),[(s*.30,-.62,1.13,.17,.18),(s*.34,-.70,.87,.14,.14),(s*.34,-.80,.60,.08,.085),(s*.31,-.77,.44,.06,.07),(s*.27,-.59,.24,.047,.058),(s*.27,-.57,.13,.068,.065)]))
body=join(flesh,'SM_Cow_Alive_Body')
rem=body.modifiers.new('Connected_anatomical_surface','REMESH');rem.mode='VOXEL';rem.voxel_size=.034;rem.use_smooth_shade=True;apply(body,rem)
sm=body.modifiers.new('Relax_muscle_surface','SMOOTH');sm.factor=.62;sm.iterations=3;apply(body,sm)
material_regions(body,'body');parts=[body]

def hoof(x,y):
    obs=[]
    for side in (-1,1):
        # Split cloven wedge, narrow heel, broad sloped toe. The cleft remains real geometry.
        cx=x+side*.038
        v=[(cx-.033,y-.055,.025),(cx+.033,y-.055,.025),(cx+.037,y+.10,.025),(cx-.037,y+.10,.025),
           (cx-.031,y-.05,.13),(cx+.031,y-.05,.13),(cx+.028,y+.074,.105),(cx-.028,y+.074,.105)]
        ob=mesh('Cloven_hoof',v,[(0,3,2,1),(4,5,6,7),(0,1,5,4),(1,2,6,5),(2,3,7,6),(3,0,4,7)],HOOF)
        bevel=ob.modifiers.new('Worn_hoof_edges','BEVEL');bevel.width=.009;bevel.segments=2;apply(ob,bevel);obs.append(ob)
    return obs
for s in (-1,1):parts+=hoof(s*.25,.70)+hoof(s*.27,-.57)
udder=ellipsoid('Udder',(0,-.48,.61),(.23,.25,.17),NOSE);parts.append(udder)
for x in (-.095,.095):
    for y in (-.40,-.56):parts.append(tube('Teat',[(x,y,.52,.027,.027),(x,y,.45,.024,.024),(x,y,.415,.012,.012)],NOSE,sides=12))
parts.append(tube('Tail',[(0,-.91,1.32,.041,.038),(.015,-1.0,1.13,.028,.027),(.027,-1.02,.87,.020,.022),(.07,-1.04,.63,.018,.019),(.12,-1.02,.48,.025,.026)],RED,sides=12))
parts.append(tube('White_tail_switch',[(.11,-1.02,.54,.025,.027),(.13,-1.03,.44,.047,.044),(.15,-1.04,.33,.038,.033),(.18,-1.05,.29,.012,.012)],WHITE,sides=16))
body=join(parts,'SM_Cow_Alive_Body')

# Neck root is the local origin. Eye centres intentionally match the original glint anchors.
head=loft_y('Head_neck_and_face',[
    [-.16,-.015,.17,.23],[0,.015,.20,.27],[.20,.055,.19,.31],[.36,.09,.18,.30],
    [.50,.05,.215,.26],[.66,0,.20,.225],[.80,-.09,.19,.155],[.92,-.12,.18,.12],[.98,-.12,.13,.09]],sides=32)
material_regions(head,'head');headparts=[head]
headparts.append(ellipsoid('Soft_pink_muzzle',(0,.934,-.13),(.20,.10,.12),NOSE))
for s in (-1,1):
    eye=ellipsoid('Eye', (s*.17,.66,.11),(.021,.040,.028),EYE,segments=24,rings=12);headparts.append(eye)
    # Raised eyelids wrap the eye without hiding the anchor point.
    eyelid=tube('Upper_eyelid',[(s*.177,.616,.106,.014,.014),(s*.185,.635,.142,.014,.014),(s*.184,.67,.148,.012,.012),(s*.174,.700,.120,.010,.010)],EAR,sides=10);headparts.append(eyelid)
    nose=ellipsoid('Nostril',(s*.095,1.025,-.108),(.040,.014,.027),EYE,segments=20,rings=10);headparts.append(nose)
    # Leaf-shaped ears with a concave inset and a narrow base, rather than oval primitives.
    v=[(s*.15,.29,.20),(s*.29,.22,.25),(s*.48,.25,.27),(s*.54,.34,.22),(s*.44,.41,.18),(s*.28,.38,.18),(s*.33,.31,.22)]
    faces=[(6,i,(i+1)%6)for i in range(6)]
    ear=mesh('Ear_outer',v,faces,RED);so=ear.modifiers.new('Ear_thickness','SOLIDIFY');so.thickness=.018;apply(ear,so)
    sub=ear.modifiers.new('Soft_ear_edge','SUBSURF');sub.levels=2;apply(ear,sub);headparts.append(ear)
    inset=mesh('Ear_inner',[(s*.24,.28,.231),(s*.43,.29,.255),(s*.47,.34,.228),(s*.37,.37,.205),(s*.28,.35,.209)],[(0,1,2,3,4)],EAR);headparts.append(inset)
    horn=tube('Short_curved_horn',[(s*.155,.22,.315,.064,.064),(s*.22,.20,.34,.050,.050),(s*.265,.18,.39,.035,.035),(s*.28,.17,.45,.021,.021),(s*.265,.18,.50,.005,.005)],HORN,sides=16);headparts.append(horn)
headparts.append(tube('Mouth_seam',[(-.145,.992,-.201,.005,.005),(0,1.021,-.207,.005,.005),(.145,.992,-.201,.005,.005)],CUT,sides=8))

# Blank yellow plastic on shared head: each cow's actual number remains in its original inspect text.
v=[(-.36,.335,.19),(-.405,.335,.15),(-.405,.335,.075),(-.315,.335,.075),(-.315,.335,.15)]
tag=mesh('Blank_yellow_left_ear_tag',v,[(0,1,2,3,4)],TAG)
solid=tag.modifiers.new('Tag_thickness','SOLIDIFY');solid.thickness=.008;apply(tag,solid)
be=tag.modifiers.new('Tag_rounded_corners','BEVEL');be.width=.005;be.segments=2;apply(tag,be);headparts.append(tag)
head=join(headparts,'SM_Cow_Alive_Head')

def uv_project(ob,preserve=False):
    me=ob.data
    if not me.uv_layers:me.uv_layers.new(name='UVMap')
    uv=me.uv_layers.active.data
    for f in me.polygons:
        mat=me.materials[f.material_index].name
        if preserve and mat in ('M_Cow_BodyHide','M_Cow_HeadHide'):continue
        ax=max(range(3),key=lambda k:abs(f.normal[k]));axes=[k for k in range(3)if k!=ax]
        for li in f.loop_indices:
            p=me.vertices[me.loops[li].vertex_index].co
            if mat=='M_Cow_BodyHide':uv[li].uv=((p.y+1.2)/2.4,p.z/1.6)
            elif mat=='M_Cow_HeadHide':uv[li].uv=((p.y+.2)/1.3,(p.z+.3)/.9)
            else:uv[li].uv=(p[axes[0]]*3.1,p[axes[1]]*3.1)
def clean(ob,preserve=False):
    active(ob);bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
    old=[m or CUT for m in ob.data.materials];indices=[p.material_index for p in ob.data.polygons]
    used=[]
    for k in indices:
        m=old[k] if k<len(old) else CUT
        if m not in used:used.append(m)
    ob.data.materials.clear()
    for m in used:ob.data.materials.append(m)
    for f,k in zip(ob.data.polygons,indices):f.material_index=used.index(old[k]if k<len(old)else CUT)
    bm=bmesh.new();bm.from_mesh(ob.data);bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(ob.data);bm.free()
    uv_project(ob,preserve)
    tri=ob.modifiers.new('Explicit_export_triangles','TRIANGULATE');tri.quad_method='FIXED';apply(ob,tri)
    clean_geometry(ob)
    # End caps can project to a line in the side atlas. Give only those UV triangles a
    # tiny local footprint within the same colour cell, keeping tangent bases defined.
    uv=ob.data.uv_layers.active.data
    for f in ob.data.polygons:
        a,b,c=[uv[li].uv.copy()for li in f.loop_indices]
        if abs((b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x))<2e-12:
            mid=(a+b+c)/3;ax=max(range(3),key=lambda k:abs(f.normal[k]));axes=[k for k in range(3)if k!=ax]
            for li in f.loop_indices:
                v=ob.data.vertices[ob.data.loops[li].vertex_index].co;uv[li].uv=mid+Vector((v[axes[0]]*.001,v[axes[1]]*.001))
for ob in (body,head):clean(ob)

# Fallen counterpart uses the SAME source anatomy and hide, posed on her right side.
dbody=body.copy();dbody.data=body.data.copy();DEAD.objects.link(dbody)
dhead=head.copy();dhead.data=head.data.copy();DEAD.objects.link(dhead)
fall=Matrix.Translation(Vector((-.66,0,.45)))@Matrix.Rotation(math.pi/2,4,'Y')
neck=Matrix.Translation(Vector((0,.74,1.12)))@Matrix.Rotation(-.12,4,'X')@Matrix.Rotation(.10,4,'Z')
dbody.data.transform(fall);dhead.data.transform(fall@neck)
# The live ground pivot is exact: lower only hoof soles, keeping the ankle at 13 cm.
# Pose copies above retain their separately grounded fallen silhouette.
for v in body.data.vertices:
    if v.co.z<.13:v.co.z=max(0,(v.co.z-.025)*.13/.105)
# Gravity bends the free legs differently. The lower pair bears on the ground; the upper
# front leg falls forward and lower, rather than all four projecting in a rigid plane.
for v in dbody.data.vertices:
    w=max(0,min(1,(-.05-v.co.x)/.45))
    upper=v.co.z>.45
    v.co.z-=w*((.59 if v.co.y>0 else .51)if upper else .14)
    if upper:v.co.y+=w*(.08 if v.co.y>0 else -.11)
dead=join([dbody,dhead],'SM_Cow_Abducted')

# Create the round dry flank incision as a real inset surface, flush with the upper flank.
# Cylinder cut removes skin and creates a shallow cylindrical dark cavity, with no red interior.
bpy.ops.mesh.primitive_cylinder_add(vertices=64,radius=.205,depth=.4,location=(.37,-.05,.83))
cutter=bpy.context.object;cutter.name='Flank_cutting_tool'
bo=dead.modifiers.new('Precisely_excised_round_flank','BOOLEAN');bo.operation='DIFFERENCE';bo.solver='EXACT';bo.object=cutter;apply(dead,bo);bpy.data.objects.remove(cutter,do_unlink=True)
cutslot=list(dead.data.materials).index(CUT)
for f in dead.data.polygons:
    x,y,z=f.center;r=math.hypot(x-.37,y+.05)
    if r<.207 and .62<z<.95 and (z<.64 or abs(f.normal.z)<.7):f.material_index=cutslot
# Ring and floor are intentionally dark dry tissue, not glowing or wet flesh.
ring=tube('Hard_dark_round_margin',[(.37,-.05,.641,.01,.01),(.37,-.05,.65,.01,.01)],CUT,c=DEAD,sides=12)
# Replace tube with a true annular lip, slightly sloped to follow the upper side.
bpy.data.objects.remove(ring,do_unlink=True)
rv=[];rf=[]
for rad,drop in ((.208,0),(.196,.012),(.180,.04)):
    for k in range(64):
        a=k*math.tau/64;x=.37+rad*math.cos(a);y=-.05+rad*math.sin(a)
        z=.45+.450*math.sqrt(max(.02,1-((x-.34)/.425)**2))+.004-drop
        rv.append((x,y,z))
for j in range(2):
    for k in range(64):rf.append((j*64+k,j*64+(k+1)%64,(j+1)*64+(k+1)%64,(j+1)*64+k))
lip=mesh('Circular_dry_cut_edge',rv,rf,CUT,DEAD)
floor=ellipsoid('Dry_dark_incision_floor',(.37,-.05,.662),(.198,.198,.012),CUT,DEAD,segments=48,rings=8)
dead=join([dead,lip,floor],'SM_Cow_Abducted')

# Oval jaw window follows the exposed left side. Dry bone patch is recessed, edged dark.
jawcenter=fall@neck@Vector((-.207,.68,-.035))
normal=(fall@neck).to_3x3()@Vector((-1,0,0));normal.normalize()
up=(fall@neck).to_3x3()@Vector((0,0,1));up.normalize();right=normal.cross(up).normalized()
def oval(name,center,rx,ry,mat,offset=0):
    vs=[center+normal*offset]
    for k in range(48):
        a=k*math.tau/48;vs.append(center+right*(rx*math.cos(a))+up*(ry*math.sin(a))+normal*offset)
    return mesh(name,vs,[(0,k+1,(k+1)%48+1)for k in range(48)],mat,DEAD)
rim=oval('Jaw_dry_dark_margin',jawcenter,.146,.086,CUT,.010)
bone=oval('Jaw_oval_dry_bone',jawcenter,.129,.069,BONE,.012)
dead=join([dead,rim,bone],'SM_Cow_Abducted')

# Empty mouth after the tongue is gone. Small dark cavity at the muzzle's bottom, not a red gape.
mouth=fall@neck@Vector((0,1.014,-.18))
empty=ellipsoid('Empty_mouth_no_tongue',mouth,(.024,.012,.10),CUT,DEAD,segments=24,rings=10)
empty.data.transform(Matrix.Identity(4));dead=join([dead,empty],'SM_Cow_Abducted')

# Dead eye is milky and lightly frosted; no green/emissive eyes on the corpse.
eyeat=fall@neck@Vector((-.17,.66,.11))
deadeye=ellipsoid('Frosted_open_eye',eyeat+normal*.025,(.034,.045,.013),FROST,DEAD,segments=20,rings=12)
dead=join([dead,deadeye],'SM_Cow_Abducted')

# Separate raised dark print on the corpse tag only. Text is converted to mesh before export.
tagat=fall@neck@Vector((-.36,.347,.128))
bpy.ops.object.text_add(location=tagat);text=bpy.context.object;move_to(text,DEAD);text.name='ALDERETE25_print'
text.data.body='ALDERETE\n25';text.data.align_x='CENTER';text.data.align_y='CENTER';text.data.size=.018;text.data.space_line=.85;text.data.extrude=.0005;text.data.materials.append(CUT)
text.matrix_world=fall@neck@Matrix.Translation(Vector((-.36,.347,.128)))@Matrix.Rotation(-math.pi/2,4,'X')@Matrix.Rotation(math.pi,4,'Z');active(text);bpy.ops.object.convert(target='MESH');text=bpy.context.object;bake_loc(text)
dead=join([dead,text],'SM_Cow_Abducted');clean(dead,True)

def bounds(ob):return [[float(fn(v.co[k]for v in ob.data.vertices))for k in range(3)]for fn in (min,max)]
def triangles(ob):return len(ob.data.polygons)
def fbx(ob,path):
    active(ob);bpy.ops.export_scene.fbx(filepath=str(path),use_selection=True,object_types={'MESH'},apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',axis_forward='-Y',axis_up='Z',bake_anim=False,add_leaf_bones=False,mesh_smooth_type='FACE')
parts={};lods={str(k):{}for k in range(3)}
for ob in (body,head,dead):
    # Original editable objects stay in source collections, with native metric pivots at zero.
    assert triangles(ob)<30000,(ob.name,triangles(ob))
    alltris=[]
    for l,ratio in enumerate((1,.56,.30)):
        ex=ob.copy();ex.data=ob.data.copy();LOD.objects.link(ex);ex.name=ob.name+(''if l==0 else '_LOD'+str(l))
        if l:
            dec=ex.modifiers.new('Silhouette_preserving_LOD'+str(l),'DECIMATE');dec.ratio=ratio;dec.use_collapse_triangulate=True;apply(ex,dec)
        clean(ex,True);name=ob.name+(''if l==0 else '_LOD'+str(l));ex.name=name
        fbx(ex,OUT/(name+'.fbx'))
        active(ex);bpy.ops.export_scene.gltf(filepath=str(OUT/(name+'.glb')),export_format='GLB',use_selection=True,export_materials='EXPORT')
        row={'triangles':triangles(ex),'bounds_local_m':bounds(ex),'materials':[m.name for m in ex.data.materials]}
        lods[str(l)][ob.name]=row;alltris.append(row['triangles']);ex.hide_render=True;ex.hide_set(True)
    parts[ob.name]={'bounds_local_m':bounds(ob),'materials':[m.name for m in ob.data.materials],'lod_triangles':alltris,'pivot_m':[0,0,0]}

# Asymmetric native axis probe: importer measures (200,-400,600)cm and (20,40,60)cm full sizes.
bpy.ops.mesh.primitive_cube_add(size=1,location=(2,4,6));probe=bpy.context.object;probe.name='axis_probe';probe.scale=(.2,.4,.6);bake_loc(probe);fbx(probe,OUT/'axis_probe.fbx');bpy.data.objects.remove(probe,do_unlink=True)

contract={'source_revision':'COWS1-original-lofted-Hereford','units':'metres','blender_up':'Z','blender_forward':'+Y','mapping':'UE=(100*Bx,-100*By,100*Bz); three=(Bx,Bz,-By)','native_import_policy':{'force_front_x_axis':True,'coordinate_system_policy':'MATCH_UP_FORWARD_AXES','import_rotation_yaw':90},'head_runtime_origin_three_m':[0,1.12,-.74],'head_eye_glints_blender_m':[[-.17,.66,.11],[.17,.66,.11]],'abducted_head_proxy_blender_m':[.417613509,.1+1.171471815,.30],'parts':parts,'lods':lods,'concept_references':['Concepts/ku-levende.png','Concepts/ku-dod-endelig.png'],'authorship':'Original scripted geometry and procedural PBR textures; concepts generated with built-in imagegen; no downloaded model.'}
(HERE/'model_contract.json').write_text(json.dumps(contract,indent=2)+'\n')
# Native fixed-exposure torch response: preserve cream/coat detail at 3-5 metres.
# This is the existing engine recipe's TextureTint parameter, not emissive lighting.
for row in ROWS:
    native_tint = .32 if row['name'] in ('M_Cow_BodyHide','M_Cow_HeadHide','M_Cow_DryBone') else .45 if row['name']=='M_Cow_Muzzle' else .5 if row['name']=='M_Cow_Horn' else 1.0
    row['texture_tint_linear'] = [native_tint]*3
(HERE/'materials.json').write_text(json.dumps(ROWS,indent=2)+'\n')

# Source scene has assembled live animal with its separate turning-head pivot.
head.location=(0,.74,1.12)
body['role']='Editable original live body; ground pivot';head['role']='Editable original live head; neck-root pivot'
dead['role']='Original dead Hereford after abduction; right side down, left side uppermost, no blood'
for name in ('ku-levende.png','ku-dod-endelig.png'):
    im=bpy.data.images.load(str(HERE/'Concepts'/name),check_existing=True)
    ref=bpy.data.objects.new(name,None);ref.empty_display_type='IMAGE';ref.data=im;ref.empty_display_size=3;REFERENCES.objects.link(ref);ref.location=(5,0,2);ref.hide_render=True

def studio_material():
    m=bpy.data.materials.new('Preview_ground_only');m.diffuse_color=(.11,.13,.145,1);m.use_nodes=True;p=m.node_tree.nodes.get('Principled BSDF');p.inputs['Base Color'].default_value=(.11,.13,.145,1);p.inputs['Roughness'].default_value=.8;return m
bpy.ops.mesh.primitive_plane_add(size=200,location=(0,0,-.012));ground=bpy.context.object;move_to(ground,STUDIO);ground.data.materials.append(studio_material())
def light(name,at,power,size,target=(0,0,.8)):
    data=bpy.data.lights.new(name,'AREA');data.energy=power;data.shape='DISK';data.size=size;ob=bpy.data.objects.new(name,data);STUDIO.objects.link(ob);ob.location=at;ob.rotation_euler=(Vector(target)-ob.location).to_track_quat('-Z','Y').to_euler()
light('Large_soft_key',(-3,4,5),650,4);light('Soft_fill',(4,2,3),420,3);light('Back_rim',(0,-4,4),650,3)
def camera(name,at,target,ortho):
    data=bpy.data.cameras.new(name);data.type='ORTHO';data.ortho_scale=ortho;ob=bpy.data.objects.new(name,data);STUDIO.objects.link(ob);ob.location=at;ob.rotation_euler=(Vector(target)-ob.location).to_track_quat('-Z','Y').to_euler();return ob
alivecam=camera('Camera_Live_ThreeQuarter',(-3.5,5,2.6),(0,.05,.85),3.8)
deadcam=camera('Camera_Abducted_ThreeQuarter',(-2.8,4,3.6),(0,.1,.35),3.8)
sidecam=camera('Camera_Live_Side',(-4,.05,1.9),(0,.05,.9),3.6)
scene.camera=alivecam
for ob in DEAD.objects:ob.hide_render=True
for ob in DEAD.objects:ob.hide_set(True)
for ob in REFERENCES.objects:ob.hide_set(True)
for screen in bpy.data.screens:
    for area in screen.areas:
        if area.type=='VIEW_3D':
            area.spaces.active.shading.type='MATERIAL'
            area.spaces.active.region_3d.view_perspective='CAMERA'
for im in bpy.data.images:
    if im.source=='FILE':
        try:im.pack()
        except Exception:pass
bpy.ops.file.make_paths_relative()
bpy.ops.wm.save_as_mainfile(filepath=str(HERE/'Signal47_Cows.blend'))
if '--render'in sys.argv or '--render-dead'in sys.argv:
    if '--render'in sys.argv:
        scene.render.filepath=str(PRE/'cow-alive.png');bpy.ops.render.render(write_still=True)
        scene.camera=sidecam;scene.render.filepath=str(PRE/'cow-alive-side.png');bpy.ops.render.render(write_still=True)
    for ob in LIVE.objects:ob.hide_render=True
    for ob in DEAD.objects:ob.hide_render=False
    scene.camera=deadcam;scene.render.filepath=str(PRE/'cow-abducted.png');bpy.ops.render.render(write_still=True)
    for ob in LIVE.objects:ob.hide_render=False
    for ob in DEAD.objects:ob.hide_render=True
    scene.camera=alivecam
    bpy.ops.wm.save_as_mainfile(filepath=str(HERE/'Signal47_Cows.blend'))
print('COW SOURCE READY',json.dumps({name:r['lod_triangles']for name,r in parts.items()}))
