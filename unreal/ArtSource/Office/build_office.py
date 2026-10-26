"""Original SARO Operations office, Blender 5.2. Metric, editable, no downloaded geometry.
Run blender -b -t 2 --python build_office.py [-- --render]
All helpers take game coordinates (x, height, z); stored Blender coordinates are (x,-z,height).
Native measured FBX recipe therefore yields exact game-world positions.
"""
from pathlib import Path
import bpy, math, json, sys
from mathutils import Vector
import numpy as np
ROOT=Path(__file__).resolve().parent
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
rng=np.random.default_rng(47026);N=512
v,u=np.mgrid[0:N,0:N]/N
fine=rng.normal(0,1,(N,N));slow=np.sin(u*51)*np.sin(v*39)+np.sin(u*173+v*21)*.3

def imwrite(name,arr):
    im=bpy.data.images.new(name,N,N,alpha=True);im.pixels.foreach_set(arr.astype(np.float32).ravel());im.filepath_raw=str(TEX/(name+'.png'));im.file_format='PNG';im.save();bpy.data.images.remove(im)
    return bpy.data.images.load(str(TEX/(name+'.png')),check_existing=True)
def mat(name,color,rough=.7,metal=0,kind='paint'):
    name='M_Office_'+name;m=bpy.data.materials.new(name);m.use_nodes=True
    p=m.node_tree.nodes.get('Principled BSDF');p.inputs['Base Color'].default_value=(*color,1);p.inputs['Roughness'].default_value=rough;p.inputs['Metallic'].default_value=metal
    if kind=='wood':field=np.sin(u*127+np.sin(v*8)*2)*.32+np.sin(u*317+np.sin(v*17)*1.5)*.17+slow*.06+fine*.04
    elif kind=='fabric':field=(np.sin(u*1600)+np.sin(v*1600))*.12+fine*.10
    elif kind=='tile':field=slow*.025+fine*.03
    else:field=slow*.035+fine*.025
    rgba=np.ones((N,N,4));linear=np.clip(np.array(color)[None,None,:]*(1+field[:,:,None]),.001,1)
    rgba[:,:,:3]=np.where(linear<.0031308,linear*12.92,1.055*linear**(1/2.4)-.055)
    base=imwrite(name+'_base',rgba);base.colorspace_settings.name='sRGB'
    r=np.ones((N,N,4));r[:,:,:3]=np.clip(rough+field[:,:,None]*.10,.1,1);rim=imwrite(name+'_rough',r);rim.colorspace_settings.name='Non-Color'
    nodes=m.node_tree.nodes;links=m.node_tree.links
    for image,socket in ((base,'Base Color'),(rim,'Roughness')):
        n=nodes.new('ShaderNodeTexImage');n.image=image;links.new(n.outputs['Color'],p.inputs[socket])
    ROWS.append({'name':name,'base_color_linear':[1,1,1,1],'roughness':rough,'metallic':metal,'specular':.33,'textures':{'base_color':'textures/'+name+'_base.png','roughness':'textures/'+name+'_rough.png'}})
    MATS[name]=m;return m
WALL=mat('SageWall',(.27,.31,.25),.91)
TRIM=mat('CreamEnamel',(.49,.46,.36),.63)
WOOD=mat('Walnut',(.10,.053,.026),.54,kind='wood')
DESK=mat('TankerGreen',(.105,.15,.13),.49,.42)
STEEL=mat('CabinetEnamel',(.19,.23,.21),.57,.38)
DARK=mat('BlackBakelite',(.018,.023,.021),.49)
BRASS=mat('AgedBrass',(.32,.22,.075),.31,.82)
CHROME=mat('BrushedMetal',(.34,.37,.36),.29,.84)
CLOTH=mat('ChairFabric',(.12,.19,.16),.89,kind='fabric')
PAPER=mat('Paper',(.64,.59,.44),.96)
CORK=mat('Cork',(.21,.14,.075),.93,kind='wood')
TILE=mat('FloorLinoleum',(.25,.29,.25),.68,kind='tile')
CEILING=mat('CeilingTile',(.46,.47,.40),.98,kind='tile')
RED=mat('BinderBurgundy',(.16,.038,.025),.73)
BLUE=mat('BinderBlue',(.032,.069,.10),.76)
LIGHT=mat('Diffuser',(.65,.62,.48),.56)

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
box('West_painted_lining',(-.88,1.39,8.49),(.10,2.78,3.76),WALL,.002)
box('East_painted_lining',(1.83,1.39,8.49),(.10,2.78,3.76),WALL,.002)
box('Rear_painted_lining',(.47,1.39,10.37),(2.8,2.78,.10),WALL,.002)
for x in (-.80,1.75):
    box('Baseboard',(x,.075,8.49),(.035,.15,3.70),WOOD,.004)
    box('Picture_rail',(x,2.31,8.49),(.029,.035,3.68),TRIM,.003)
box('Rear_baseboard',(.47,.075,10.30),(2.58,.15,.035),WOOD,.004)
for ix in range(5):
    for iz in range(6):box('Acoustic_ceiling_panel',(-.88+(ix+.5)*.54,2.735,6.61+(iz+.5)*.626),(.530,.035,.616),CEILING,.002)
for ix in range(6):box('Ceiling_T_bar',(-.88+ix*.54,2.712,8.49),(.012,.018,3.76),TRIM,.002)
for iz in range(7):box('Ceiling_cross_T',(.47,2.712,6.61+iz*.626),(2.7,.018,.012),TRIM,.002)
for x in (-.062,1.032):
    box('Door_steel_jamb',(x,1.08,6.70),(.055,2.16,.23),TRIM,.005)
    box('Door_stop',(x+(.027 if x<0 else -.027),1.075,6.685),(.015,2.15,.025),WOOD,.002)
box('Door_frame_head',(.485,2.145,6.70),(1.15,.065,.23),TRIM,.005)
box('Threshold_brass',(.47,.014,6.69),(1.06,.018,.24),BRASS,.003)
for x in (.1,.84):cyl('Threshold_screw',(x,.024,6.69),(x,.025,6.69),.005,CHROME,12)
# Fluorescent fixture housing and split diffusers, attached to actual ceiling.
box('Fluorescent_backplate',(.47,2.68,8.8),(.38,.07,1.22),STEEL,.012)
for x in (.37,.57):box('Fluorescent_opal_diffuser',(x,2.625,8.8),(.16,.045,1.15),LIGHT,.013)
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
for i in range(15):
    z=8.37+i*.086;h=.27+(.04 if i%3 else 0)
    box('Service_binder',(-.60,1.12+h/2,z),(.28,h,.066),BLUE if i%3 else RED,.004)
    box('Binder_spine_label',(-.449,1.22+h*.2,z),(.004,.095,.039),PAPER,.001)
    cyl('Binder_finger_ring',(-.447,1.175,z),(-.444,1.175,z),.009,BRASS,16)
box('Archive_storage_box',(-.63,1.78,8.56),(.29,.26,.46),PAPER,.006)
box('Archive_box_lid',(-.63,1.925,8.56),(.315,.035,.48),CORK,.004)
box('Private_folder_stack',(-.63,2.205,8.73),(.27,.055,.48),CORK,.004)
# Notice board east wall: layered sheets with separate pinheads, tasteful irregularity.
box('Noticeboard_frame',(1.72,1.65,9.45),(.07,.72,.84),WOOD,.009)
box('Noticeboard_cork',(1.676,1.65,9.45),(.025,.66,.78),CORK,.004)
for i,(y,z) in enumerate(((1.78,9.23),(1.75,9.60),(1.46,9.38))):
    box('Pinned_shift_sheet',(1.66,y,z),(.004,.25,.20),PAPER,.001)
    cyl('Brass_drawing_pin',(1.651,y+.10,z),(1.644,y+.10,z),.007,BRASS,16)
    for j in range(5):box('Printed_invoice_line',(1.655,y+.055-j*.025,z),(.002,.002,.14-(j%2)*.035),DARK,0)
# Rear blind over existing opaque lining: a credible interior glazed panel, no fake exterior view.
box('Glazed_panel_frame',(.47,1.83,10.298),(1.37,.94,.07),TRIM,.009)
box('Glazed_panel_dark_back',(.47,1.83,10.254),(1.25,.83,.020),DARK,.004)
for i in range(19):
    sl=box('Venetian_blind_slat',(.47,1.44+i*.042,10.225),(1.23,.032,.045),TRIM,.002);sl.rotation_euler.x=-.25
for x in (.03,.91):cyl('Blind_ladder_cord',(x,1.41,10.195),(x,2.23,10.195),.0025,PAPER,12)
cyl('Blind_pull_cord',(1.12,1.40,10.21),(1.12,2.25,10.21),.002,DARK,12)
box('Blind_head_rail',(.47,2.275,10.24),(1.34,.065,.085),TRIM,.005)
# Water radiator under noticeboard.
for i in range(10):box('Radiator_section',(1.66,.43,8.53+i*.043),(.12,.49,.032),TRIM,.012)
for y in (.23,.64):cyl('Radiator_header',(1.66,y,8.50),(1.66,y,8.97),.022,TRIM)
# Desk accessories at precise tabletop height. Documents themselves remain live interactions.
cyl('Coffee_mug_body',(.91,.804,9.97),(.91,.895,9.97),.039,TRIM,32)
cyl('Coffee_dark_surface',(.91,.897,9.97),(.91,.899,9.97),.032,DARK,32)
torus('Coffee_mug_handle',(.957,.853,9.97),.030,.006,TRIM,axis='Z')
for x,z in ((.65,9.98),(.70,10.01)):
    cyl('Pencil_in_pot',(x,.80,z),(x+.018,.995,z+.01),.004,WOOD,8)
cyl('Pencil_cup',(.67,.803,10.0),(.67,.88,10.0),.033,STEEL,24)
box('Desk_blotter',(.32,.802,9.69),(.64,.003,.45),CLOTH,.006)
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
'SM_Office_ServiceCabinet':(-.59,.525,6.32),'SM_Office_ServiceDrawer':(-.59,.85,6.22),
'SM_Office_Door':(.47,1.035,6.57),'SM_Office_SpareKey':(-.59,.805,6.17),
'SM_Office_Desk':(.37,0,9.73),'SM_Office_Chair':(1.19,0,8.73),
'SM_Office_Room':(0,0,0),'SM_Office_Dressing':(0,0,0)}
# All original components are grouped sequentially according to part component counts.
for name,row in PARTS.items():
    for o in ORIGINALS[name]:o.location+=xyz(placements[name])
    row['runtime_origin_three_m']=list(placements[name])
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
