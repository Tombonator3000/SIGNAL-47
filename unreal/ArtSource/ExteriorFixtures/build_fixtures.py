"""Original 1986 industrial luminaires. Run with Blender 5.2, --background -t 2.

Editable parts, three authored LODs, metric FBX and original PBR maps. The local
contract is Three metres (x,y up,z), Blender=(x,-z,y), measured UE=(x,z,y)*100.
No downloaded mesh, generated concept image, or third-party texture is used.
"""
from pathlib import Path
import json, math
import bpy, bmesh, numpy as np
from mathutils import Vector

HERE=Path(__file__).resolve().parent
OUT=HERE/'Export'; TEX=HERE/'textures'
OUT.mkdir(parents=True,exist_ok=True); TEX.mkdir(exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
SC=bpy.context.scene; SC.unit_settings.system='METRIC'; SC.unit_settings.scale_length=1
SC.render.threads_mode='FIXED'; SC.render.threads=2
MATS={}; ROWS=[]; LOD=0; COL=None; PART=[]

def image(name,data,linear=False):
    im=bpy.data.images.new(name,width=data.shape[1],height=data.shape[0],alpha=True)
    if linear: im.colorspace_settings.name='Non-Color'
    im.pixels.foreach_set(np.asarray(data,dtype=np.float32).ravel())
    im.filepath_raw=str(TEX/(name+'.png')); im.file_format='PNG'; im.save(); im.pack()
    return im

def material(short,col,rough,metal,kind='paint'):
    name='M_ExteriorFixtures_'+short; n=512
    rng=np.random.default_rng(1986+len(ROWS)); v,u=np.mgrid[:n,:n]/n
    fine=rng.normal(0,1,(n,n)); broad=np.sin(u*31+np.sin(v*13))*.35+np.sin(v*21)*.25
    if kind=='lens':
        ridges=np.cos(u*math.tau*24); field=ridges*.09+fine*.012
        height=np.sin(u*math.tau*24)*.004+np.sin(v*math.tau*78)*.0007
    elif kind=='metal':
        field=fine*.016+np.sin(v*math.tau*185)*.019+broad*.035
        height=fine*.0003+np.sin(v*math.tau*185)*.0002
    else:
        field=fine*.017+broad*.045
        # Scattered restrained oxidation, not heavy dirt baked into all surfaces.
        field-=np.maximum(0,np.sin(u*47+np.sin(v*31))*np.cos(v*39)-.91)*.35
        height=fine*.00020+broad*.0003
    base=np.ones((n,n,4),dtype=np.float32)
    lin=np.clip(np.asarray(col)[None,None,:]*(1+field[:,:,None]),.001,.95)
    base[:,:,:3]=np.where(lin<.0031308,lin*12.92,1.055*lin**(1/2.4)-.055)
    ri=np.ones_like(base);ri[:,:,:3]=np.clip(rough+field[:,:,None]*.15,.08,.97)
    dy,dx=np.gradient(height); normal=np.stack((-dx*n,-dy*n,np.ones_like(dx)),axis=2)
    normal/=np.linalg.norm(normal,axis=2)[:,:,None]
    nm=np.ones_like(base);nm[:,:,:3]=normal*.5+.5;nm[:,:,1]=1-nm[:,:,1] # DirectX
    bi=image(name+'_base',base); rr=image(name+'_rough',ri,True); ni=image(name+'_normal',nm,True)
    m=bpy.data.materials.new(name);m.use_nodes=True;nodes=m.node_tree.nodes;links=m.node_tree.links
    p=nodes.get('Principled BSDF');p.inputs['Metallic'].default_value=metal;p.inputs['Roughness'].default_value=rough
    for im,socket in ((bi,'Base Color'),(rr,'Roughness')):
        t=nodes.new('ShaderNodeTexImage');t.image=im;links.new(t.outputs['Color'],p.inputs[socket])
    t=nodes.new('ShaderNodeTexImage');t.image=ni
    split=nodes.new('ShaderNodeSeparateColor');combine=nodes.new('ShaderNodeCombineColor');inv=nodes.new('ShaderNodeMath');inv.operation='SUBTRACT';inv.inputs[0].default_value=1
    links.new(t.outputs['Color'],split.inputs['Color']);links.new(split.outputs['Red'],combine.inputs['Red']);links.new(split.outputs['Blue'],combine.inputs['Blue'])
    links.new(split.outputs['Green'],inv.inputs[1]);links.new(inv.outputs[0],combine.inputs['Green'])
    no=nodes.new('ShaderNodeNormalMap');no.inputs['Strength'].default_value=.30 if kind=='lens' else .16
    links.new(combine.outputs['Color'],no.inputs['Color']);links.new(no.outputs['Normal'],p.inputs['Normal'])
    MATS[short]=m
    ROWS.append({'name':name,'base_color_linear':[1,1,1,1],'metallic':metal,'roughness':rough,'specular':.35,
                 'emission':[0,0,0,1],'emission_strength':0,'normal_convention':'DirectX','normal_strength':.30 if kind=='lens' else .16,
                 'texture_tint_linear':[1,1,1], 'textures':{'base_color':'textures/'+bi.name+'.png','roughness':'textures/'+rr.name+'.png','normal':'textures/'+ni.name+'.png'}})
    return m

material('Bronze',(.105,.099,.076),.55,.62)
material('Aluminium',(.31,.33,.30),.36,.82,'metal')
material('Steel',(.19,.22,.22),.40,.84,'metal')
material('Gasket',(.012,.015,.014),.84,.02)
material('Lens',(.51,.49,.40),.34,.0,'lens')

def xyz(p): return Vector((p[0],-p[2],p[1]))
def active(o):
    bpy.ops.object.select_all(action='DESELECT');o.select_set(True);bpy.context.view_layer.objects.active=o
def finish(o,name,mat):
    o.name=name
    for c in list(o.users_collection):c.objects.unlink(o)
    COL.objects.link(o);o.data.materials.append(MATS[mat]);PART.append(o);return o
def bevel(o,r):
    if r<=0:return o
    active(o);m=o.modifiers.new('Cast_edge_radius','BEVEL');m.width=r;m.segments=(3,2,1)[LOD]
    bpy.ops.object.modifier_apply(modifier=m.name)
    m=o.modifiers.new('Face_weighted_normals','WEIGHTED_NORMAL');m.keep_sharp=True
    bpy.ops.object.modifier_apply(modifier=m.name);return o
def mesh(name,points,faces,mat,r=0):
    me=bpy.data.meshes.new(name);me.from_pydata([xyz(p) for p in points],[],faces);me.update()
    bm=bmesh.new();bm.from_mesh(me);bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(me);bm.free()
    o=bpy.data.objects.new(name,me);COL.objects.link(o);o.data.materials.append(MATS[mat]);PART.append(o)
    return bevel(o,r)
def box(name,at,size,mat,r=.002):
    bpy.ops.mesh.primitive_cube_add(size=1,location=xyz(at));o=finish(bpy.context.object,name,mat)
    o.dimensions=(size[0],size[2],size[1]);active(o);bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    return bevel(o,min(r,min(size)*.24))
def cyl(name,a,b,r,mat,sides=None):
    aa,bb=xyz(a),xyz(b);d=bb-aa
    bpy.ops.mesh.primitive_cylinder_add(vertices=sides or (16,10,8)[LOD],radius=r,depth=d.length,location=(aa+bb)/2)
    o=finish(bpy.context.object,name,mat);o.rotation_euler=d.to_track_quat('Z','Y').to_euler()
    for p in o.data.polygons:p.use_smooth=len(p.vertices)==4
    return o
def prism_x(name,width,yz,mat,r=.002):
    n=len(yz);p=[(x,y,z) for x in (-width/2,width/2) for y,z in yz]
    f=[tuple(range(n-1,-1,-1)),tuple(range(n,n*2))]+[(i,(i+1)%n,(i+1)%n+n,i+n) for i in range(n)]
    return mesh(name,p,f,mat,r)
def vecadd(p,n,k):return tuple(p[i]+n[i]*k for i in range(3))
def rectangle(w,h,r):
    # True rounded cast corners retained in all LODs; segment counts decrease.
    points=[];segments=(5,3,2)[LOD]
    for cx,cy,start in ((w/2-r,h/2-r,0),(-w/2+r,h/2-r,90),(-w/2+r,-h/2+r,180),(w/2-r,-h/2+r,270)):
        for i in range(segments+1):
            a=math.radians(start+i*90/segments);points.append((cx+r*math.cos(a),cy+r*math.sin(a)))
    return points
def plane_point(c,u,v,n,a,b,d):return tuple(c[i]+u[i]*a+v[i]*b+n[i]*d for i in range(3))
def plate(name,c,u,v,w,h,r,thick,mat):
    n=tuple(Vector(u).cross(Vector(v)));rr=rectangle(w,h,r);num=len(rr)
    p=[plane_point(c,u,v,n,a,b,d) for d in (-thick/2,thick/2) for a,b in rr]
    f=[tuple(range(num-1,-1,-1)),tuple(range(num,num*2))]+[(i,(i+1)%num,(i+1)%num+num,i+num) for i in range(num)]
    return mesh(name,p,f,mat)
def ring(name,c,u,v,w,h,r,border,thick,mat):
    n=tuple(Vector(u).cross(Vector(v)));outer=rectangle(w,h,r);inner=rectangle(w-2*border,h-2*border,max(.001,r-border))
    count=len(outer);p=[]
    for d in (-thick/2,thick/2):
        for rr in (outer,inner):p += [plane_point(c,u,v,n,a,b,d) for a,b in rr]
    f=[]
    for i in range(count):
        j=(i+1)%count
        f += [(i,j,j+count,i+count),(i+2*count,i+3*count,j+3*count,j+2*count),
              (i,j,j+2*count,i+2*count),(i+count,i+3*count,j+3*count,j+count)]
    return mesh(name,p,f,mat)
def screw(name,c,n,r=.004):
    cyl(name+'_washer',vecadd(c,n,-.001),vecadd(c,n,.001),r*1.55,'Steel',12 if LOD==0 else 8)
    cyl(name+'_slotted_head',vecadd(c,n,.001),vecadd(c,n,.003),r,'Aluminium',10 if LOD==0 else 8)
    if LOD==0:
        # Recessed dark slot and bright rim establish the period slotted screw head.
        u=Vector((1,0,0)) if abs(n[0])<.8 else Vector((0,1,0));u=(u-Vector(n)*u.dot(Vector(n))).normalized();v=Vector(n).cross(u)
        plate(name+'_recess',vecadd(c,n,.0031),u,v,r*1.5,.0011,.0004,.00015,'Gasket')
def flutes(name,c,u,v,w,h,nr):
    if LOD==2:return
    n=tuple(Vector(u).cross(Vector(v)));count=nr if LOD==0 else max(6,nr//2)
    for i in range(count):
        x=-w/2+(i+.5)*w/count;rad=min(.004,w/count*.23)
        profile=[(-rad,0),(-rad*.65,.0011),(0,.0018),(rad*.65,.0011),(rad,0)] if LOD==0 else [(-rad,0),(0,.0017),(rad,0)]
        p=[plane_point(c,u,v,n,x+a,b,d) for b in (-h/2,h/2) for a,d in profile];k=len(profile)
        f=[tuple(range(k-1,-1,-1)),tuple(range(k,k*2))]+[(j,(j+1)%k,(j+1)%k+k,j+k) for j in range(k)]
        mesh(name+str(i),p,f,'Lens')

WP_N=(0,-.287347886,.957826285);WP_V=(0,.957826285,.287347886);U=(1,0,0)
WP_C=(0,-.035,.1775)
def wallpack():
    plate('Flush_wall_mount_gasket',(0,0,.0015),U,(0,1,0),.312,.262,.021,.003,'Gasket')
    plate('Cast_backplate',(0,0,.009),U,(0,1,0),.334,.278,.024,.012,'Bronze')
    prism_x('Cast_bronze_wedge_body',.344,[(-.139,.014),(.140,.014),(.140,.145),(.082,.191),(-.135,.128)],'Bronze',.004)
    prism_x('Folded_rain_hood',.36,[(.099,.051),(.148,.030),(.148,.170),(.103,.202),(.096,.190)],'Bronze',.002)
    ring('Recessed_lens_gasket',vecadd(WP_C,WP_N,-.006),U,WP_V,.312,.206,.020,.015,.007,'Gasket')
    plate('Moulded_glass_diffuser',WP_C,U,WP_V,.285,.18,.013,.004,'Lens')
    ring('Cast_lens_retainer',vecadd(WP_C,WP_N,.002),U,WP_V,.317,.210,.022,.011,.008,'Bronze')
    flutes('Vertical_glass_prism_',vecadd(WP_C,WP_N,.0021),U,WP_V,.267,.153,20)
    if LOD<2:
        for x in (-.146,.146):
            for y in (-.092,.092):screw('Lens_cover_fixing',plane_point(WP_C,U,WP_V,WP_N,x,y,.0061),WP_N,.0032)
        for side in (-1,1):
            cyl('Side_conduit_blank',(side*.172,.055,.065),(side*.176,.055,.065),.018,'Bronze')
            cyl('Cast_knockout_seam',(side*.1762,.055,.065),(side*.1768,.055,.065),.014,'Steel')
            for i in range(4 if LOD==0 else 2):
                box('Casting_heat_fin',(side*.173,.085-i*.023,.055),(.008,.008,.061),'Bronze',.0015)
        for x in (-.12,.12):screw('Wall_anchor',(x,.108,.019),(0,0,1),.004)
    # A lower drain and captive cover tongue are visible in silhouette at close range.
    box('Lower_cover_catch',(0,-.143,.126),(.036,.012,.020),'Steel',.002)
    if LOD==0:box('Drain_slot',(0,-.149,.120),(.018,.0008,.006),'Gasket',0)

def canopy():
    # Plane U X V points down; mounting gasket reaches exactly y=0.
    v=(0,0,1);normal=(0,-1,0)
    plate('Roof_mount_gasket',(0,-.0015,0),U,v,.574,.214,.024,.003,'Gasket')
    plate('Die_cast_ceiling_tray',(0,-.014,0),U,v,.60,.24,.027,.025,'Bronze')
    ring('Tray_reveal',(0,-.033,0),U,v,.577,.217,.022,.009,.005,'Gasket')
    ring('Cast_shoulder_frame',(0,-.051,0),U,v,.573,.214,.025,.016,.040,'Bronze')
    plate('Prismatic_opal_bowl',(0,-.081,0),U,v,.532,.187,.021,.028,'Lens')
    flutes('Long_diffuser_prism_',(0,-.0953,0),U,v,.503,.161,26)
    ring('Captive_diffuser_rim',(0,-.088,0),U,v,.558,.210,.024,.012,.011,'Aluminium')
    for x in (-.282,.282):
        box('Spring_retaining_clip',(x,-.066,0),(.017,.059,.056),'Steel',.002)
        if LOD<2:screw('Clip_captive_screw',(x,-.096,0),normal,.0037)
    if LOD<2:
        for x in (-.242,.242):
            for z in (-.086,.086):screw('Canopy_anchor',(x,-.029,z),normal,.0033)
        for side in (-1,1):
            cyl('Sealed_conduit_knockout',(side*.296,-.018,0),(side*.300,-.018,0),.009,'Bronze')
    if LOD==0:
        for x in (-.23,.23):box('Raised_cast_parting_line',(x,-.037,.113),(.062,.0015,.0018),'Aluminium',0)

A=math.radians(45);SN=(0,math.sin(A),math.cos(A));SV=(0,math.cos(A),-math.sin(A))
def hp(p):return (p[0],.065+p[1]*math.cos(A)+p[2]*math.sin(A),.005-p[1]*math.sin(A)+p[2]*math.cos(A))
SG_C=hp((0,0,.068))
def signwash():
    plate('Ground_mount_neoprene',(0,.0015,-.02),U,(0,0,-1),.13,.11,.013,.003,'Gasket')
    plate('Cast_anchor_foot',(0,.008,-.02),U,(0,0,-1),.13,.11,.013,.010,'Bronze')
    for x in (-.091,.091):
        # Actual bent U-yoke with broad base, taper, and pivot clearance.
        o=prism_x('Yoke_steel_arm',.012,[(.013,-.042),(.013,.019),(.073,.022),(.087,.012),(.077,-.014),(.035,-.023)],'Steel',.0014)
        o.location.x=x
    plate('Cast_head_back',hp((0,0,-.031)),U,SV,.18,.09,.021,.016,'Bronze')
    ring('Cast_projector_body',hp((0,0,.014)),U,SV,.18,.09,.021,.012,.082,'Bronze')
    plate('Recessed_reflector',hp((0,0,.052)),U,SV,.156,.069,.014,.003,'Aluminium')
    ring('Optical_seal',hp((0,0,.060)),U,SV,.177,.087,.018,.010,.004,'Gasket')
    plate('Tempered_prismatic_lens',SG_C,U,SV,.16,.068,.012,.003,'Lens')
    ring('Front_cast_bezel',hp((0,0,.065)),U,SV,.183,.093,.019,.010,.012,'Bronze')
    flutes('Sign_optical_flute_',vecadd(SG_C,SN,.0016),U,SV,.143,.051,12)
    for side in (-1,1):
        cyl('Angle_pivot_boss',(side*.088,.065,.005),(side*.100,.065,.005),.014,'Bronze')
        if LOD<2:screw('Yoke_pivot_lock',(side*.100,.065,.005),(side,0,0),.006)
    if LOD<2:
        for x in (-.047,.047):
            for z in (-.055,.014):screw('Foundation_anchor',(x,.014,z),(0,1,0),.004)
        for x in (-.080,.080):screw('Bezel_retaining_screw',hp((x,0,.073)),SN,.003)
        cyl('Cable_gland',(0,.022,-.047),(0,.022,-.067),.008,'Steel',12)
        cyl('Sealed_cable_stub',(0,.022,-.066),(0,.017,-.075),.004,'Gasket',10)
    if LOD==0:
        for x in (-.051,-.017,.017,.051):
            # Shallow cast fins on the head back, with head rotation baked into their vertices.
            points=[hp((x+dx,y,z)) for dx in (-.002,.002) for y,z in [(-.027,-.039),(.027,-.039),(.027,-.047),(-.027,-.047)]]
            mesh('Rear_cast_cooling_fin',points,[(0,1,2,3),(4,7,6,5),(0,4,5,1),(1,5,6,2),(2,6,7,3),(3,7,4,0)],'Bronze')

BUILDERS={'SM_Exterior_WallPack':wallpack,'SM_Exterior_Canopy':canopy,'SM_Exterior_SignWash':signwash}
OPTICS={
 'SM_Exterior_WallPack':{'mount_plane':{'axis':'z','value':0,'solid_side':'+z'},'lens_center_three_m':WP_C,'lens_normal_three':WP_N,'aperture_m':[.285,.18],'emitter_three_m':vecadd(WP_C,WP_N,.008)},
 'SM_Exterior_Canopy':{'mount_plane':{'axis':'y','value':0,'solid_side':'-y'},'lens_center_three_m':[0,-.096,0],'lens_normal_three':[0,-1,0],'aperture_m':[.51,.18],'emitter_three_m':[0,-.105,0]},
 'SM_Exterior_SignWash':{'mount_plane':{'axis':'y','value':0,'solid_side':'+y'},'lens_center_three_m':SG_C,'lens_normal_three':SN,'aperture_m':[.16,.068],'emitter_three_m':vecadd(SG_C,SN,.006),'head_tilt_degrees':45}}
CONTRACT={'status':'SOURCE_GENERATED_NATIVE_UNVERIFIED','units':'metres','mapping':'Three (x,y,z) -> Blender (x,-z,y) -> UE centimetres (x,z,y)*100','destination':'/Game/Signal47/Art/ExteriorFixtures',
 'originality':'Original metric bpy geometry and procedural PBR textures. No third-party imagery or meshes.',
 'lens_material':'M_ExteriorFixtures_Lens','lens_runtime_parameters':{'EmissiveFactor':'linear RGB, zero when off, multiplies texture','TextureTint':'linear RGB, default one'},'parts':{}}
EXPORT=bpy.data.collections.new('DELIVERY_FBX');SC.collection.children.link(EXPORT)
for name,builder in BUILDERS.items():
    counts=[];data=None;components=[]
    for LOD in range(3):
        COL=bpy.data.collections.new(name+'_EDIT_LOD'+str(LOD));SC.collection.children.link(COL);PART=[]
        builder();components.append(len(PART))
        for o in PART:
            active(o);bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
        bpy.ops.object.select_all(action='DESELECT')
        for o in PART:o.select_set(True)
        bpy.context.view_layer.objects.active=PART[0];bpy.ops.object.duplicate();copies=list(bpy.context.selected_objects)
        bpy.context.view_layer.objects.active=copies[0];bpy.ops.object.join();ob=bpy.context.object
        ob.name=name if LOD==0 else name+'_LOD'+str(LOD)
        for collection in list(ob.users_collection):collection.objects.unlink(ob)
        EXPORT.objects.link(ob);active(ob)
        SC.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
        bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT');bpy.ops.uv.cube_project(cube_size=1);bpy.ops.object.mode_set(mode='OBJECT')
        t=ob.modifiers.new('Explicit_delivery_triangles','TRIANGULATE');bpy.ops.object.modifier_apply(modifier=t.name)
        points=[ob.matrix_world@v.co for v in ob.data.vertices]
        bounds=[[float(fn(p[k] for p in points)) for k in range(3)] for fn in (min,max)]
        count=len(ob.data.polygons);counts.append(count)
        assert all(p.area>1e-12 for p in ob.data.polygons),name+' degenerate triangle'
        assert all(math.isfinite(f) for p in points for f in p),name+' nonfinite vertex'
        if LOD==0:
            three=[[(bounds[0][0]),bounds[0][2],-bounds[1][1]],[bounds[1][0],bounds[1][2],-bounds[0][1]]]
            data={'bounds_local_m':bounds,'bounds_three_m':three,'materials':[m.name for m in ob.data.materials], 'origin':[0,0,0],**OPTICS[name]}
        bpy.ops.export_scene.fbx(filepath=str(OUT/(ob.name+'.fbx')),use_selection=True,object_types={'MESH'},apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',axis_forward='-Y',axis_up='Z',bake_anim=False,add_leaf_bones=False,mesh_smooth_type='FACE',use_tspace=True)
        ob.hide_render=True;ob.hide_set(True)
        COL.hide_viewport=LOD>0 or name!='SM_Exterior_WallPack';COL.hide_render=LOD>0 or name!='SM_Exterior_WallPack'
    assert counts[0]>counts[1]>counts[2]>0,(name,counts)
    data['lod_triangles']=counts;data['editable_components']=components;CONTRACT['parts'][name]=data
CONTRACT['total_LOD0_triangles']=sum(r['lod_triangles'][0] for r in CONTRACT['parts'].values())
assert CONTRACT['total_LOD0_triangles']<12000,CONTRACT['total_LOD0_triangles']
(HERE/'materials.json').write_text(json.dumps(ROWS,indent=2)+'\n')
(HERE/'model_contract.json').write_text(json.dumps(CONTRACT,indent=2)+'\n')
SC.world=bpy.data.worlds.new('Neutral_fixture_world');SC.world.color=(.18,.18,.18)
SC.render.engine='CYCLES';SC.cycles.device='CPU';SC.cycles.samples=20
SC.view_settings.view_transform='AgX'
bpy.ops.wm.save_as_mainfile(filepath=str(HERE/'SARO_ExteriorFixtures.blend'))
print('EXTERIOR FIXTURES SOURCE PASS',json.dumps({k:v['lod_triangles'] for k,v in CONTRACT['parts'].items()}))
