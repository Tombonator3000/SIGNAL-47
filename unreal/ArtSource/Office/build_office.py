"""Original SARO Operations office, Blender 5.2. Metric, editable, no downloaded geometry.
Run blender -b -t 2 --python build_office.py [-- --render]
All helpers take game coordinates (x, height, z); stored Blender coordinates are (x,-z,height).
Native measured FBX recipe therefore yields exact game-world positions.
"""
from pathlib import Path
import bpy, math, json, sys
from mathutils import Vector, Matrix
import numpy as np
ROOT=Path(__file__).resolve().parent
FRONT_OFFSET=.50  # Operations passing bay; keep rear furniture/story anchors fixed.
OUT=ROOT/'Export'; TEX=ROOT/'textures'; PRE=ROOT/'Preview'
for p in (OUT,TEX,PRE):p.mkdir(parents=True,exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
SC=bpy.context.scene;SC.unit_settings.system='METRIC';SC.unit_settings.scale_length=1
SC.render.engine='CYCLES';SC.cycles.device='CPU';SC.cycles.samples=28
SC.render.threads_mode='FIXED';SC.render.threads=2
SC.render.resolution_x=1400;SC.render.resolution_y=1050;SC.render.resolution_percentage=100
SC.world=bpy.data.worlds.new('Office_preview_world');SC.world.color=(.06,.07,.09)
SC.view_settings.view_transform='AgX'
EDIT=bpy.data.collections.new('01_EDITABLE_ORIGINAL_COMPONENTS');SC.collection.children.link(EDIT)
EXPORT=bpy.data.collections.new('02_NATIVE_EXPORT_PARTS');SC.collection.children.link(EXPORT)
STUDIO=bpy.data.collections.new('03_PREVIEW_ONLY');SC.collection.children.link(STUDIO)
MATS={};ROWS=[];PARTS={};CURRENT=[];ORIGINALS={}
rng=np.random.default_rng(47026);N=1024
v,u=np.mgrid[0:N,0:N]/N
fine=rng.normal(0,1,(N,N));slow=np.sin(u*51)*np.sin(v*39)+np.sin(u*173+v*21)*.3

def imwrite(name,arr):
    # Arrays already contain encoded albedo or raw linear data. Do not apply a
    # second colour transform while writing roughness and tangent normals.
    im=bpy.data.images.new(name,N,N,alpha=True);im.colorspace_settings.name='Non-Color';im.pixels.foreach_set(arr.astype(np.float32).ravel());im.filepath_raw=str(TEX/(name+'.png'));im.file_format='PNG';im.save();bpy.data.images.remove(im)
    return bpy.data.images.load(str(TEX/(name+'.png')),check_existing=True)
def cork_fields(size):
    # Independent seed: changing Cork must not advance the room's shared RNG or
    # alter any other material. At 1024px per metre these correlated random
    # grains span roughly 2..6mm, without directional/periodic veneer stripes.
    cork_rng=np.random.default_rng(47026030)
    grains=cork_rng.normal(0,1,(size,size))
    for axis in (0,1):
        grains=sum(weight*np.roll(grains,offset,axis=axis)
                   for offset,weight in ((-2,1),(-1,4),(0,6),(1,4),(2,1)))/16
    grains=np.tanh(grains/max(float(grains.std()),1e-9))
    fine_cork=cork_rng.normal(0,1,(size,size))
    for axis in (0,1):
        fine_cork=(np.roll(fine_cork,-1,axis=axis)+2*fine_cork+
                   np.roll(fine_cork,1,axis=axis))/4
    pores=np.clip((-fine_cork-.35)*1.4,0,1)
    field=grains*.13+np.tanh(fine_cork*2)*.025
    # Physical microroughness, not displacement: <=80 micrometres relief.
    height=grains*.000045-pores*.000035
    return field,height

def value_field(rows,columns,seed):
    """Seamless, gently filtered noise; no repeated sine-wave surface pattern."""
    grid=np.random.default_rng(seed).uniform(-1,1,(rows,columns))
    yy=v*rows;xx=u*columns
    iy=yy.astype(np.int32);ix=xx.astype(np.int32)
    fy=yy-iy;fx=xx-ix
    fy=fy*fy*(3-2*fy);fx=fx*fx*(3-2*fx)
    a=grid[iy%rows,ix%columns]*(1-fx)+grid[iy%rows,(ix+1)%columns]*fx
    b=grid[(iy+1)%rows,ix%columns]*(1-fx)+grid[(iy+1)%rows,(ix+1)%columns]*fx
    return a*(1-fy)+b*fy

def mat(name,color,rough=.7,metal=0,kind='paint'):
    name='M_Office_'+name;m=bpy.data.materials.new(name);m.use_nodes=True
    p=m.node_tree.nodes.get('Principled BSDF');p.inputs['Base Color'].default_value=(*color,1);p.inputs['Roughness'].default_value=rough;p.inputs['Metallic'].default_value=metal
    # Cube projection uses a one-metre span. Heights are metres, independent of
    # colour: painted steel, woven cloth and wood must not share a noisy bump.
    rough_field=None
    if kind=='wood':
        # Long interrupted pores in varnished walnut, rather than evenly spaced
        # black stripes. Wear changes the varnish sheen more than its colour.
        grain=value_field(3,93,4703101)
        fibres=value_field(13,389,4703102)
        pores=np.clip((-fibres-.40)*1.7,0,1)
        wear=value_field(7,9,4703103)
        field=grain*.16+fibres*.035-pores*.055+wear*.025
        height=grain*.000025-pores*.000012
        rough_field=grain*.035+wear*.070+pores*.05
    elif kind=='wall':
        # Old matte paint has broad, quiet tonal variation and fine orange peel;
        # a coarse, high-frequency normal makes grazing light look like soot.
        clouds=value_field(5,7,4703111)
        roller=value_field(93,71,4703112)
        peel=value_field(267,251,4703113)
        field=clouds*.032+roller*.010+peel*.006
        height=peel*.000012+roller*.000006
        rough_field=clouds*.023+roller*.014
    elif kind=='linoleum':
        # Mineral flecks and shallow polish wear in an old resilient floor.
        # Marks live mainly in roughness, not a baked shadow/dirt overlay.
        body=value_field(19,23,4703121)
        flecks=value_field(271,293,4703122)
        polish=value_field(7,11,4703123)
        field=body*.028+flecks*.055
        height=flecks*.000008
        rough_field=polish*.095+body*.035+flecks*.018
    elif kind=='acoustic':
        pores=value_field(373,347,4703131)
        pits=np.clip((-pores-.47)*2.8,0,1)
        field=value_field(9,11,4703132)*.018-pits*.085
        height=-pits*.000035
        rough_field=pits*.025
    elif kind=='cork':
        field,height=cork_fields(N)
    elif kind=='fabric':
        weave=np.sin(math.tau*u*250)*np.sin(math.tau*v*250)
        field=weave*.055+fine*.018
        height=weave*.00010
    elif kind=='tile':
        field=slow*.018+fine*.012
        height=np.sin(math.tau*u*17)*np.sin(math.tau*v*19)*.000018
    else:
        field=slow*.025+fine*.009
        height=np.sin(math.tau*u*73)*np.sin(math.tau*v*67)*.000015
    rgba=np.ones((N,N,4));linear=np.clip(np.array(color)[None,None,:]*(1+field[:,:,None]),.001,1)
    rgba[:,:,:3]=np.where(linear<.0031308,linear*12.92,1.055*linear**(1/2.4)-.055)
    base=imwrite(name+'_base',rgba);base.colorspace_settings.name='sRGB'
    if rough_field is None:rough_field=field*.35
    r=np.ones((N,N,4));r[:,:,:3]=np.clip(rough+rough_field[:,:,None],.1,1);rim=imwrite(name+'_rough',r);rim.colorspace_settings.name='Non-Color'
    # Match the existing native import recipe's DirectX tangent convention.
    dx=(np.roll(height,-1,axis=1)-np.roll(height,1,axis=1))*(N*.5)
    dy=(np.roll(height,-1,axis=0)-np.roll(height,1,axis=0))*(N*.5)
    normal=np.stack((-dx,dy,np.ones_like(dx)),axis=2)
    normal/=np.linalg.norm(normal,axis=2,keepdims=True)
    nr=np.ones((N,N,4));nr[:,:,:3]=normal*.5+.5
    nim=imwrite(name+'_normal_dx',nr);nim.colorspace_settings.name='Non-Color'
    nodes=m.node_tree.nodes;links=m.node_tree.links
    for image,socket in ((base,'Base Color'),(rim,'Roughness')):
        n=nodes.new('ShaderNodeTexImage');n.image=image;links.new(n.outputs['Color'],p.inputs[socket])
    n=nodes.new('ShaderNodeTexImage');n.image=nim
    # Blender previews use OpenGL normals; invert G only in the preview graph.
    separate=nodes.new('ShaderNodeSeparateColor');links.new(n.outputs['Color'],separate.inputs['Color'])
    inv=nodes.new('ShaderNodeMath');inv.operation='SUBTRACT';inv.inputs[0].default_value=1;links.new(separate.outputs['Green'],inv.inputs[1])
    combine=nodes.new('ShaderNodeCombineColor');links.new(separate.outputs['Red'],combine.inputs['Red']);links.new(inv.outputs[0],combine.inputs['Green']);links.new(separate.outputs['Blue'],combine.inputs['Blue'])
    bump=nodes.new('ShaderNodeNormalMap');links.new(combine.outputs[0],bump.inputs['Color']);links.new(bump.outputs['Normal'],p.inputs['Normal'])
    ROWS.append({'name':name,'base_color_linear':[1,1,1,1],'roughness':rough,'metallic':metal,'specular':.33,'normal_strength':1.,'normal_convention':'DirectX','textures':{'base_color':'textures/'+name+'_base.png','roughness':'textures/'+name+'_rough.png','normal':'textures/'+name+'_normal_dx.png'}})
    MATS[name]=m;return m
WALL=mat('SageWall',(.255,.282,.250),.91,kind='wall')
TRIM=mat('CreamEnamel',(.49,.46,.36),.63)
WOOD=mat('Walnut',(.10,.053,.026),.54,kind='wood')
DESK=mat('TankerGreen',(.105,.15,.13),.49)
STEEL=mat('CabinetEnamel',(.19,.23,.21),.57)
DARK=mat('BlackBakelite',(.018,.023,.021),.49)
BRASS=mat('AgedBrass',(.32,.22,.075),.31,.82)
CHROME=mat('BrushedMetal',(.34,.37,.36),.29,.84)
CLOTH=mat('ChairFabric',(.12,.19,.16),.89,kind='fabric')
PAPER=mat('Paper',(.64,.59,.44),.96)
CORK=mat('Cork',(.21,.14,.075),.93,kind='cork')
TILE=mat('FloorLinoleum',(.235,.255,.232),.65,kind='linoleum')
CEILING=mat('CeilingTile',(.46,.47,.43),.98,kind='acoustic')
RED=mat('BinderBurgundy',(.16,.038,.025),.73)
BLUE=mat('BinderBlue',(.032,.069,.10),.76)
OLIVE=mat('BinderOlive',(.084,.090,.049),.83)
GRAY=mat('BinderFadedGray',(.145,.155,.145),.85)
TAN=mat('BinderTan',(.28,.22,.12),.80)
LIGHT=mat('Diffuser',(.65,.62,.48),.56)
mat('CorridorTubeWest',(.65,.62,.48),.56)
mat('CorridorTubeEast',(.65,.62,.48),.56)

def xyz(p):return Vector((p[0],-p[2],p[1]))
def active(o):
    bpy.ops.object.select_all(action='DESELECT');o.select_set(True);bpy.context.view_layer.objects.active=o

def put(o,name,material):
    o.name=name
    for c in list(o.users_collection):c.objects.unlink(o)
    EDIT.objects.link(o);o.data.materials.append(material);CURRENT.append(o);return o

def apply(o,mod):active(o);bpy.ops.object.modifier_apply(modifier=mod.name)
def box(name,at,size,material,bevel=.008):
    bpy.ops.mesh.primitive_cube_add(size=1,location=xyz(at));o=put(bpy.context.object,name,material)
    o.dimensions=(size[0],size[2],size[1]);active(o);bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    if bevel:
        mod=o.modifiers.new('Crafted_edge_radius','BEVEL');mod.width=min(bevel,min(size)*.24);mod.segments=3;apply(o,mod)
        mod=o.modifiers.new('Weighted_corner_normals','WEIGHTED_NORMAL');mod.keep_sharp=True;apply(o,mod)
    return o

def cyl(name,a,b,r,material,segments=20):
    aa=xyz(a);bb=xyz(b);d=bb-aa
    bpy.ops.mesh.primitive_cylinder_add(vertices=segments,radius=r,depth=d.length,location=(aa+bb)/2);o=put(bpy.context.object,name,material);o.rotation_euler=d.to_track_quat('Z','Y').to_euler()
    active(o);bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
    for p in o.data.polygons:p.use_smooth=len(p.vertices)==4
    return o

def torus(name,at,r,wire,material,axis='Y',major=28):
    bpy.ops.mesh.primitive_torus_add(major_segments=major,minor_segments=8,location=xyz(at),major_radius=r,minor_radius=wire);o=put(bpy.context.object,name,material)
    if axis=='Z':o.rotation_euler.x=math.pi/2
    if axis=='X':o.rotation_euler.y=math.pi/2
    for p in o.data.polygons:p.use_smooth=True
    return o

def curve(name,points,r,material):
    c=bpy.data.curves.new(name,'CURVE');c.dimensions='3D';c.resolution_u=10;c.bevel_depth=r;c.bevel_resolution=2
    sp=c.splines.new('BEZIER');sp.bezier_points.add(len(points)-1)
    for b,p in zip(sp.bezier_points,points):b.co=xyz(p);b.handle_left_type='AUTO';b.handle_right_type='AUTO'
    o=bpy.data.objects.new(name,c);EDIT.objects.link(o);o.data.materials.append(material);active(o);bpy.ops.object.convert(target='MESH');CURRENT.append(o);return o

def finish(name):
    originals=list(CURRENT);CURRENT.clear();ORIGINALS[name]=originals
    for o in originals:
        active(o);bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
    bpy.ops.object.select_all(action='DESELECT')
    for o in originals:o.select_set(True)
    bpy.context.view_layer.objects.active=originals[0];bpy.ops.object.duplicate();copies=list(bpy.context.selected_objects);bpy.context.view_layer.objects.active=copies[0];bpy.ops.object.join();o=bpy.context.object;o.name=name
    for c in list(o.users_collection):c.objects.unlink(o)
    EXPORT.objects.link(o);active(o);bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT');bpy.ops.uv.cube_project(cube_size=1);bpy.ops.object.mode_set(mode='OBJECT')
    tri=o.modifiers.new('Triangulated_delivery','TRIANGULATE');apply(o,tri)
    bpy.ops.export_scene.fbx(filepath=str(OUT/(name+'.fbx')),use_selection=True,object_types={'MESH'},apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',axis_forward='-Y',axis_up='Z',bake_anim=False,mesh_smooth_type='FACE')
    bounds=[[float(fn(v.co[k] for v in o.data.vertices))for k in range(3)]for fn in (min,max)]
    PARTS[name]={'triangles':len(o.data.polygons),'bounds_local_m':bounds,'materials':[m.name for m in o.data.materials],'original_components':len(originals)}
    o.hide_render=True;o.hide_set(True)
    return originals

# Service cabinet. Full-height welded shell, genuine open drawer cavity, recessed lower door.
box('Cabinet_back',(0,0,.222),(.62,.99,.025),STEEL)
for x in (-.303,.303):box('Folded_cabinet_side',(x,0,0),(.024,.99,.46),STEEL)
box('Cabinet_bottom',(0,-.48,0),(.62,.028,.46),STEEL)
box('Cabinet_lower_door',(0,-.10,-.238),(.55,.60,.025),STEEL)
box('Cabinet_lower_reveal',(0,-.10,-.223),(.58,.64,.014),DARK,.003)
box('Cabinet_lower_panel',(0,-.10,-.246),(.52,.57,.013),STEEL)
for y in (-.34,.13):cyl('Hinge_barrel',(-.265,y-.035,-.259),(-.265,y+.035,-.259),.012,CHROME,16)
box('Lower_pull',(0,.10,-.274),(.12,.018,.035),CHROME)
cyl('Lower_lock',(0,.18,-.259),(0,.18,-.27),.018,CHROME)
box('Worktop_core',(0,.516,0),(.67,.050,.55),WOOD,.014)
box('Worktop_edge',(0,.491,-.267),(.67,.022,.020),DARK,.003)
for x in (-.26,.26):
    for z in (-.18,.18):cyl('Adjustable_foot',(x,-.495,z),(x,-.525,z),.024,DARK)
for x in (-.277,.277):box('Drawer_steel_slide',(x,.315,-.02),(.015,.024,.38),CHROME,.003)
box('Cabinet_upper_crossrail',(0,.434,-.23),(.59,.035,.022),STEEL)
finish('SM_Office_ServiceCabinet')

# Moving tray. Open volume is modeled; the spare lies on a felt liner inside it.
box('Drawer_front',(0,0,-.19),(.55,.17,.032),STEEL,.008)
box('Drawer_base',(0,-.065,0),(.50,.016,.35),STEEL,.003)
box('Drawer_felt',(0,-.054,0),(.46,.004,.31),CLOTH,.001)
for x in (-.244,.244):box('Drawer_side',(x,-.01,0),(.012,.12,.35),STEEL,.003)
box('Drawer_back',(0,-.01,.17),(.50,.12,.012),STEEL,.003)
for x in (-.075,.075):cyl('Handle_standoff',(x,0,-.207),(x,0,-.243),.011,CHROME)
cyl('Drawer_pull',(-.075,0,-.243),(.075,0,-.243),.012,CHROME)
box('Drawer_label_holder',(0,.050,-.209),(.11,.027,.006),CHROME,.003)
box('Drawer_label_paper',(0,.050,-.214),(.093,.018,.002),PAPER,.001)
# Fuses and stub pencil have real rounded bodies rather than colored rectangles.
for x in (-.18,-.14):
    cyl('Ceramic_fuse',(x,-.039,.075),(x,-.039,.115),.011,TRIM,16)
    for z in (.073,.115):cyl('Fuse_cap',(x,-.039,z-.004),(x,-.039,z+.004),.012,CHROME,16)
cyl('Yellow_pencil',(.17,-.039,-.06),(.17,-.039,.09),.004,WOOD,8)
finish('SM_Office_ServiceDrawer')

# Door leaf is centered on the same live pivot/interaction surface as before.
box('Solid_core_walnut_leaf',(0,0,0),(1,2.07,.055),WOOD,.006)
for face in (-1,1):
    for x in (-.482,.482):box('Door_edge_lipping',(x,0,face*.029),(.025,2.035,.006),WOOD,.002)
    box('Kick_plate',(0,-.83,face*.032),(.88,.25,.004),CHROME,.002)
    cyl('Handle_round_rose',(.36,-.025,face*.033),(.36,-.025,face*.045),.037,BRASS)
    cyl('Lever_shank',(.36,-.025,face*.045),(.36,-.025,face*.085),.013,BRASS)
    cyl('Lever_return',(.36,-.025,face*.085),(.25,-.025,face*.085),.014,BRASS)
    cyl('Cylinder_lock',(.36,-.12,face*.031),(.36,-.12,face*.043),.020,BRASS)
    box('Engraved_plaque',(0,.615,face*.034),(.53,.20,.009),BRASS,.006)
    box('Plaque_inlay',(0,.615,face*.040),(.50,.174,.004),TRIM,.002)
    for x in (-.228,.228):cyl('Plaque_screw',(x,.615,face*.043),(x,.615,face*.045),.004,CHROME,12)
for y in (-.80,0,.80):cyl('Door_hinge_pin',(-.50,y-.06,0),(-.50,y+.06,0),.013,CHROME)
box('Latch_face',(.503,-.025,0),(.004,.13,.030),BRASS,.001)
finish('SM_Office_Door')

# One tagged spare, including cut teeth, ring and rounded plastic tag.
torus('Brass_key_bow',(0,0,.045),.020,.005,BRASS)
box('Key_shaft',(0,0,-.015),(.012,.007,.080),BRASS,.002)
for z in (-.035,-.047):box('Cut_key_tooth',(.009,0,z),(.018,.007,.008),BRASS,.001)
torus('Steel_split_ring',(.035,.002,.057),.017,.0016,CHROME,major=32)
box('Ivory_key_tag',(.077,0,.045),(.064,.006,.035),TRIM,.008)
finish('SM_Office_SpareKey')

# Room surfaces, actual ceiling-grid construction and door jambs, no floating trim.
box('Floor_substrate',(.47,-.035,8.49),(2.7,.065,3.76),DARK,0)
for ix in range(6):
    for iz in range(8):
        x=-.88+(ix+.5)*.45;z=6.61+(iz+.5)*.47
        box('Individual_linoleum_tile',(x,.002,z),(.447,.008,.467),TILE,.001)
# The original records partition ends at x=-.80. Its old block texture must
# remain behind the new paint skin, with 5mm clearance rather than z fighting.
box('West_painted_lining',(-.845,1.39,8.49),(.10,2.78,3.76),WALL,.002)
box('East_painted_lining',(1.83,1.39,8.49),(.10,2.78,3.76),WALL,.002)
# Real rear window, not a blind attached to an opaque wall. The native helper
# opens the original Annex paint/concrete behind this lining at the same X/Y.
box('Rear_lining_left',(-.74,1.39,10.37),(.38,2.78,.10),WALL,.002)
box('Rear_lining_right',(1.585,1.39,10.37),(.57,2.78,.10),WALL,.002)
box('Rear_lining_below_window',(.375,.54,10.37),(1.85,1.08,.10),WALL,.002)
box('Rear_lining_above_window',(.375,2.59,10.37),(1.85,.38,.10),WALL,.002)
for x in (-.80,1.75):
    box('Baseboard',(x,.075,8.49),(.035,.15,3.70),WOOD,.004)
    box('Picture_rail',(x,2.31,8.49),(.029,.035,3.68),TRIM,.003)
box('Rear_baseboard',(.47,.075,10.30),(2.58,.15,.035),WOOD,.004)
for ix in range(5):
    for iz in range(6):box('Acoustic_ceiling_panel',(-.88+(ix+.5)*.54,2.735,6.61+(iz+.5)*.626),(.530,.035,.616),CEILING,.002)
for ix in range(6):box('Ceiling_T_bar',(-.88+ix*.54,2.712,8.49),(.012,.018,3.76),TRIM,.002)
for iz in range(7):box('Ceiling_cross_T',(.47,2.712,6.61+iz*.626),(2.7,.018,.012),TRIM,.002)
# Same rebated profile as records/lab, with the leaf inside the wall depth.
sys.path.insert(0,str(ROOT.parent/'DoorSet'))
import build_frames
frame_z=6.70+FRONT_OFFSET
def frame_point(p):return (p[0],p[1],2*frame_z-p[2])
def frame_box(name,at,size,material,bevel=.008):return box(name,frame_point(at),size,material,bevel)
def frame_cyl(name,a,b,r,material,segments=20):return cyl(name,frame_point(a),frame_point(b),r,material,segments)
# Operations swings into +Z, so its stop is on the corridor (-Z) side.
build_frames.append_frame(frame_box,frame_cyl,MATS,origin=(-.03,0,frame_z),
                          width=1.,height=2.07,style='service',prefix='Operations_frame')
# Fluorescent fixture housing and split diffusers, attached to actual ceiling.
box('Fluorescent_backplate',(.47,2.68,8.8),(.38,.07,1.22),STEEL,.012)
for x in (.37,.57):box('Fluorescent_opal_diffuser',(x,2.625,8.8),(.16,.045,1.15),LIGHT,.013)
sys.path.insert(0,str(ROOT))
import corridor_dressing
corridor_dressing.build(box,cyl,curve,MATS)
finish('SM_Office_Room')

# Tanker desk, rounded rolled lip, honest kneehole, drawers and slide hardware.
box('Desk_bullnose_top',(0,.765,0),(1.62,.055,.82),DESK,.025)
box('Desk_linoleum_inset',(0,.795,0),(1.54,.009,.74),DARK,.018)
box('Desk_modesty_panel',(0,.47,.30),(1.44,.50,.025),DESK,.009)
for x in (-.55,.55):
    box('Drawer_pedestal',(x,.405,.03),(.44,.66,.66),DESK,.018)
    for y,h in ((.63,.17),(.42,.22),(.17,.23)):
        box('Drawer_shadow',(x,y,-.309),(.40,h+.012,.014),DARK,.003)
        box('Tanker_drawer_face',(x,y,-.321),(.378,h,.025),DESK,.008)
        for xx in (x-.065,x+.065):cyl('Desk_pull_mount',(xx,y,-.338),(xx,y,-.365),.008,CHROME,16)
        cyl('Desk_drawer_pull',(x-.065,y,-.366),(x+.065,y,-.366),.009,CHROME,20)
        box('Desk_label_frame',(x,y+.046,-.339),(.095,.025,.005),CHROME,.002)
        box('Desk_label_card',(x,y+.046,-.343),(.078,.015,.002),PAPER,.001)
    for z in (-.22,.28):cyl('Desk_foot',(x,.015,z),(x,.09,z),.034,DARK)
box('Pencil_drawer',(0,.70,-.22),(.62,.07,.29),DESK,.009)
box('Pencil_drawer_pull',(0,.70,-.382),(.12,.015,.03),CHROME,.004)
finish('SM_Office_Desk')

# 1980s upholstered task chair, five wheeled legs, pressed fabric and separate armrests.
cyl('Gas_lift',(0,.14,0),(0,.43,0),.034,CHROME)
for i in range(5):
    a=i*math.tau/5;x=math.cos(a)*.30;z=math.sin(a)*.30
    curve('Five_star_leg',[(0,.17,0),(x*.55,.12,z*.55),(x,.09,z)],.022,CHROME)
    cyl('Caster_pin',(x,.07,z),(x,.11,z),.009,CHROME,16)
    for xx in (-.015,.015):cyl('Caster_wheel',(x+xx-.012,.047,z),(x+xx+.012,.047,z),.041,DARK,20)
box('Seat_pan',(0,.435,0),(.48,.055,.46),DARK,.045)
box('Upholstered_seat',(0,.480,-.015),(.47,.08,.45),CLOTH,.050)
box('Back_support',(0,.64,.23),(.09,.40,.028),CHROME,.014)
box('Back_shell',(0,.79,.25),(.46,.43,.055),DARK,.045)
box('Upholstered_back',(0,.79,.214),(.44,.40,.075),CLOTH,.040)
for x in (-.277,.277):
    curve('Curved_chair_arm',[(x,.46,.12),(x,.64,.15),(x,.67,-.19)],.014,CHROME)
    box('Bakelite_armrest',(x,.68,-.03),(.062,.042,.32),DARK,.018)
finish('SM_Office_Chair')

# Distinctive room dressing: shelves, tagged folders, wall clock, coat hooks, radiator.
# Components stay clear of the existing 0.95m walking corridor.
for y in (1.10,1.63,2.16):
    box('West_wall_book_shelf',(-.64,y,9.0),(.35,.035,1.42),WOOD,.007)
    for z in (8.44,9.56):box('Shelf_bracket',(-.75,y-.10,z),(.18,.20,.021),STEEL,.003)
# Shelf contents are individual original PBR models in Config/Props/dale-office.json.
# Keep the shelf boards/supports here; retire the repeated procedural binder run.
# A labelled kraft carton reads separately from the loose white paper groups.
box('Archive_storage_box',(-.63,1.78,8.56),(.29,.26,.46),TAN,.006)
box('Archive_box_lid',(-.63,1.925,8.548),(.315,.035,.48),TAN,.004)
box('Archive_carton_label',(-.483,1.79,8.57),(.003,.064,.17),PAPER,.001)
for y,z,w in ((1.807,8.57,.13),(1.788,8.595,.08),(1.771,8.58,.11)):
    box('Archive_label_type',(-.4808,y,z),(.0006,.0015,w),GRAY,.0001)
# Original Old West book stacks now occupy this upper shelf in the native layout.
# Physical research wall and incoming folders. Finished art is bound natively,
# just like the six independent live desk papers; blank bodies are not art acceptance.
sys.path.insert(0,str(ROOT))
import research_dressing
research_dressing.build(box,cyl,curve,MATS)
# shelf_dressing.py/shelf-dressing.json are retained source history. Their old
# Shelf_* solids are replaced by individually placed native paper/file models.
# The rough opening is X[-.55,1.30], height[1.08,2.40]. Deep jambs bridge the
# inner lining at Z10.32 to the original exterior at Z10.80. No opaque back or
# exported fake-lit pane: S47OfficeWindow supplies a passive translucent pane.
for x in (-.568,1.318):
    box('Window_deep_side_reveal',(x,1.74,10.56),(.036,1.32,.50),TRIM,.002)
    box('Window_inner_side_architrave',(x,1.74,10.292),(.065,1.39,.035),TRIM,.003)
    box('Window_outer_side_frame',(x,1.74,10.802),(.055,1.39,.035),TRIM,.003)
box('Window_deep_head',(.375,2.418,10.56),(1.922,.036,.50),TRIM,.002)
box('Window_sill',(.375,1.06,10.55),(1.98,.04,.56),TRIM,.004)
box('Window_inner_head_architrave',(.375,2.422,10.292),(1.99,.065,.035),TRIM,.003)
box('Window_outer_head_frame',(.375,2.422,10.802),(1.99,.055,.035),TRIM,.003)
# Painted 0.9mm metal slats, 25mm chord. Partly raised and tilted 28 degrees:
# the lower .39m stays open, while separated thin slats filter the upper light.
for i in range(35):
    sl=box('Venetian_blind_thin_slat',(.375,1.49+i*.025,10.267),(1.79,.0009,.025),TRIM,.00015)
    sl.rotation_euler.x=-math.radians(28)
for x in (-.17,.92):
    cyl('Blind_ladder_cord',(x,1.475,10.253),(x,2.39,10.253),.0008,PAPER,12)
cyl('Blind_pull_cord',(1.264,1.45,10.25),(1.264,2.39,10.25),.001,DARK,12)
box('Blind_bottom_rail',(.375,1.475,10.267),(1.80,.012,.025),TRIM,.002)
box('Blind_head_rail',(.375,2.388,10.263),(1.91,.036,.055),TRIM,.004)
# Water radiator under noticeboard.
for i in range(10):box('Radiator_section',(1.66,.43,8.53+i*.043),(.12,.49,.032),TRIM,.012)
for y in (.23,.64):cyl('Radiator_header',(1.66,y,8.50),(1.66,y,8.97),.022,TRIM)
# Desk accessories at precise tabletop height. Documents themselves remain live interactions.
cyl('Coffee_mug_body',(.91,.804,9.97),(.91,.895,9.97),.039,TRIM,32)
cyl('Coffee_dark_surface',(.91,.897,9.97),(.91,.899,9.97),.032,DARK,32)
torus('Coffee_mug_handle',(.957,.853,9.97),.030,.006,TRIM,axis='Z')
for x,z in ((.70,10.02),(.75,10.05)):
    cyl('Pencil_in_pot',(x,.80,z),(x+.018,.995,z+.01),.004,WOOD,8)
cyl('Pencil_cup',(.72,.803,10.04),(.72,.88,10.04),.033,STEEL,24)
box('Desk_blotter',(.32,.802,9.69),(.64,.003,.45),CLOTH,.006)
# The lamp cable drops behind the desk, with a little slack at the skirting.
# Its actual outlet is on the rear lining, clear of the new window opening.
box('Rear_outlet_plate',(-.38,.28,10.310),(.070,.115,.006),TRIM,.004)
for y in (.257,.300):
    for x in (-.395,-.380):
        box('Outlet_contact_slot',(x,y,10.305),(.003,.014,.0015),DARK,.0004)
box('Lamp_power_plug',(-.38,.28,10.285),(.024,.029,.033),DARK,.004)
curve('Desk_lamp_power_cable',[
    (-.24,.802,9.77),(-.33,.803,9.91),(-.39,.802,10.08),
    (-.42,.76,10.18),(-.43,.40,10.20),(-.43,.10,10.23),
    (-.42,.035,10.20),(-.30,.035,10.21),(-.30,.06,10.25),
    (-.38,.267,10.268)],.0022,DARK)
# VHS stand modeled as open steel trolley, casters and shelves instead of solid cube.
for x in (-.70,-.24):
    for z in (7.50,8.0):
        cyl('Deck_trolley_leg',(x,.06,z),(x,.62,z),.014,CHROME)
        cyl('Deck_trolley_wheel',(x-.015,.048,z),(x+.015,.048,z),.035,DARK)
for y in (.16,.605):box('Deck_trolley_shelf',(-.47,y,7.75),(.53,.027,.59),STEEL,.009)
# Original VCR fascia with cassette slot and transport buttons, not a playable film.
box('VHS_recorder_body',(-.47,.67,7.75),(.43,.10,.32),DARK,.012)
box('VHS_brushed_fascia',(-.47,.67,7.579),(.415,.08,.013),STEEL,.004)
box('VHS_cassette_slot',(-.52,.68,7.57),(.22,.025,.006),DARK,.002)
for i in range(4):box('VHS_transport_button',(-.35+i*.022,.650,7.565),(.015,.011,.008),TRIM,.002)
finish('SM_Office_Dressing')

# Save source with editable assemblies composed at runtime placements, export objects hidden.
placements={
'SM_Office_ServiceCabinet':(-.59,.525,6.32+FRONT_OFFSET),'SM_Office_ServiceDrawer':(-.59,.85,6.22+FRONT_OFFSET),
'SM_Office_Door':(.47,1.047,6.70+FRONT_OFFSET),'SM_Office_SpareKey':(-.59,.805,6.17+FRONT_OFFSET),
'SM_Office_Desk':(.37,0,9.73),'SM_Office_Chair':(1.19,0,8.73),
'SM_Office_Room':(0,0,0),'SM_Office_Dressing':(0,0,0)}
# All original components are grouped sequentially according to part component counts.
for name,row in PARTS.items():
    for o in ORIGINALS[name]:
        if name=='SM_Office_Door':o.scale.y*=.045/.055
        o.location+=xyz(placements[name])
    row['runtime_origin_three_m']=list(placements[name])
    if name=='SM_Office_Door':row['runtime_scale_three']=[1,1,.045/.055]
(ROOT/'materials.json').write_text(json.dumps(ROWS,indent=2)+'\n')
(ROOT/'model_contract.json').write_text(json.dumps({'units':'metres','mapping':'Blender (x,-z,y) to UE (100x,100z,100y)','authorship':'Original beveled furniture, joinery, hardware and procedural PBR, no downloaded geometry','parts':PARTS},indent=2)+'\n')
# Real light fixture render; no scene-only set dressing is mistaken for delivered meshes.
def light(name,at,energy,size,target):
    d=bpy.data.lights.new(name,'AREA');d.energy=energy;d.shape='DISK';d.size=size;o=bpy.data.objects.new(name,d);STUDIO.objects.link(o);o.location=xyz(at);o.rotation_euler=(xyz(target)-o.location).to_track_quat('-Z','Y').to_euler()
light('Ceiling_preview_light',(.47,2.55,8.8),105,1.0,(.47,.5,8.8))
light('Corridor_preview_fill',(.1,2.3,5.3),160,2.0,(.47,1.0,7.8))
light('Desk_lamp_preview',(-.24,1.15,9.7),26,.45,(.2,.8,9.5))
def camera(name,at,target):
    d=bpy.data.cameras.new(name);d.lens=24;o=bpy.data.objects.new(name,d);STUDIO.objects.link(o);o.location=xyz(at);o.rotation_euler=(xyz(target)-o.location).to_track_quat('-Z','Y').to_euler();return o
cam=camera('Office_interior_review',(.50,1.60,7.0),(.40,1.20,9.60));SC.camera=cam
for im in bpy.data.images:
    if im.source=='FILE':im.pack()
bpy.ops.file.make_paths_relative();bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'Signal47_Operations.blend'))
if '--render' in sys.argv:
    SC.render.filepath=str(PRE/'office-interior.png');bpy.ops.render.render(write_still=True)
    SC.camera=camera('Cabinet_review',(-1.60,1.6,4.5),(-.40,1.0,6.32));SC.render.filepath=str(PRE/'service-cabinet.png');bpy.ops.render.render(write_still=True)
    SC.camera=cam;bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'Signal47_Operations.blend'))
print('OFFICE SOURCE READY',json.dumps({k:v['triangles'] for k,v in PARTS.items()}))
