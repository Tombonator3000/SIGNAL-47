"""Original metric SARO entrance fittings and human-scale visitor signs in Blender.

Source Three offset (x,y,z) maps to Blender (x,-z,y). The measured FBX import
recipe maps Blender to Unreal centimetres (x,-y,z)*100. No source mesh download.
"""
from pathlib import Path
import bpy, math, json, sys
from mathutils import Vector

HERE=Path(__file__).resolve().parent;OUT=HERE/'Export';OUT.mkdir(exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
SCENE=bpy.context.scene;SCENE.unit_settings.system='METRIC';SCENE.unit_settings.scale_length=1
ROWS=json.loads((HERE/'materials.json').read_text());MATS={}
for row in ROWS:
    mat=bpy.data.materials.new(row['name']);mat.use_nodes=True
    principled=mat.node_tree.nodes.get('Principled BSDF')
    principled.inputs['Roughness'].default_value=row['roughness'];principled.inputs['Metallic'].default_value=row['metallic']
    for role,file in row.get('textures',{}).items():
        image=bpy.data.images.load(str(HERE/file),check_existing=True);image.pack()
        if role!='base_color':image.colorspace_settings.name='Non-Color'
        node=mat.node_tree.nodes.new('ShaderNodeTexImage');node.image=image
        if role=='normal':
            normal=mat.node_tree.nodes.new('ShaderNodeNormalMap');normal.inputs['Strength'].default_value=.18
            mat.node_tree.links.new(node.outputs['Color'],normal.inputs['Color']);mat.node_tree.links.new(normal.outputs['Normal'],principled.inputs['Normal'])
        else:mat.node_tree.links.new(node.outputs['Color'],principled.inputs['Base Color' if role=='base_color' else 'Roughness'])
    MATS[row['name'].removeprefix('M_Arrival_')]=mat

LOD=0;PART=[];COL=None
def xyz(v):return Vector((v[0],-v[2],v[1]))
def active(obj):
    bpy.ops.object.select_all(action='DESELECT');obj.select_set(True);bpy.context.view_layer.objects.active=obj
def finish(obj,name,mat):
    obj.name=name;obj.data.materials.append(MATS[mat])
    for col in list(obj.users_collection):col.objects.unlink(obj)
    COL.objects.link(obj);PART.append(obj);return obj
def box(name,at,size,mat,bevel=.004):
    bpy.ops.mesh.primitive_cube_add(size=1,location=xyz(at));o=bpy.context.object
    o.dimensions=(size[0],size[2],size[1]);bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    if bevel:
        mod=o.modifiers.new('manufactured_edge_radius','BEVEL');mod.width=min(bevel,min(size)*.38);mod.segments=(3,2,1)[LOD]
        bpy.ops.object.modifier_apply(modifier=mod.name)
        mod=o.modifiers.new('weighted_corner_normals','WEIGHTED_NORMAL');mod.keep_sharp=True
        bpy.ops.object.modifier_apply(modifier=mod.name)
    return finish(o,name,mat)
def tube(name,a,b,r,mat,segments=None):
    aa,bb=xyz(a),xyz(b);d=bb-aa
    bpy.ops.mesh.primitive_cylinder_add(vertices=segments or (20,12,8)[LOD],radius=r,depth=d.length,location=(aa+bb)/2)
    o=bpy.context.object;o.rotation_euler=d.to_track_quat('Z','Y').to_euler()
    for p in o.data.polygons:p.use_smooth=len(p.vertices)==4
    return finish(o,name,mat)
def face(name,at,u,v,w,h,mat):
    at,u,v=xyz(at),xyz(u),xyz(v)
    verts=[at+u*(-w/2)+v*(-h/2),at+u*(w/2)+v*(-h/2),at+u*(w/2)+v*(h/2),at+u*(-w/2)+v*(h/2)]
    mesh=bpy.data.meshes.new(name);mesh.from_pydata(verts,[],[(0,1,2,3)]);mesh.update()
    uv=mesh.uv_layers.new(name='UVMap')
    for item,coord in zip(uv.data,[(0,0),(1,0),(1,1),(0,1)]):item.uv=coord
    o=bpy.data.objects.new(name,mesh);COL.objects.link(o);PART.append(o);o.data.materials.append(MATS[mat]);return o

def entrance():
    # Root = existing west exit hinge. All geometry stays outside the aperture.
    # Thin smooth sleeves follow the original five-sided handrails exactly.
    for z in (5.05-5.22,6.35-5.22):
        tops=[]
        for world_x in (-10.5,-9.1,-7.6,-6.45):
            x=world_x+6.27;floor=-.6+(min(world_x,-7.6)+10.6)*.2
            tube('existing_ramp_post_sleeve',(x,floor,z),(x,floor+.90,z),.027,'WeatheredSteel')
            tops.append((x,floor+.90,z))
        for a,b in zip(tops,tops[1:]):tube('existing_ramp_handrail_sleeve',a,b,.027,'WeatheredSteel')
    for z in (-.055,1.015):
        box('jamb_extrusion',(-.064,1.07,z),(.085,2.14,.072),'Aluminium',.007)
        box('recessed_weather_strip',(-.031,1.045,z+(.036 if z<0 else -.036)),(.022,2.09,.012),'Rubber',.003)
    box('door_head_extrusion',(-.064,2.14,.48),(.085,.085,1.14),'Aluminium',.007)
    # Slim folded pan canopy with a boxed leading edge and raised side flashings.
    box('canopy_underside',(-.835,2.445,.48),(1.63,.09,2.10),'Enamel',.009)
    box('standing_seam_roof_pan',(-.835,2.505,.48),(1.66,.03,2.14),'WeatheredSteel',.008)
    box('canopy_front_fascia',(-1.665,2.455,.48),(.055,.19,2.14),'PaintedMetal',.008)
    for z in (-.59,1.55):
        box('canopy_side_fold',(-.835,2.49,z),(1.68,.10,.035),'PaintedMetal',.008)
    box('wall_flashing',(-.017,2.56,.48),(.036,.16,2.12),'Aluminium',.006)
    for z in (-.38,1.34):
        box('canopy_wall_bracket',(-.042,2.19,z),(.07,.39,.085),'PaintedMetal',.006)
        tube('diagonal_canopy_strut',(-.07,2.06,z),(-.89,2.405,z),.022,'PaintedMetal')
        if LOD<2:
            for y in (2.08,2.31):tube('bracket_anchor',(-.085,y,z),(-.10,y,z),.013,'Aluminium',6)
    for z in (-.48,-.20,.08,.36,.64,.92,1.20,1.48):
        box('roof_standing_seam',(-.835,2.53,z),(1.61,.018,.014),'WeatheredSteel',.003)
    box('entrance_sign_back',(-1.699,2.455,.48),(.012,.163,1.18),'Enamel',.008)
    face('entrance_legible_enamel',(-1.706,2.455,.48),(0,0,1),(0,1,0),1.155,.146,'EntrancePrint')
    box('reception_plaque',(-.070,1.50,1.83),(.026,.24,.44),'Enamel',.009)
    face('reception_legible_print',(-.084,1.50,1.83),(0,0,1),(0,1,0),.414,.214,'ReceptionPrint')
    # 5 mm surface detailing follows existing landing; no raised gameplay floor.
    box('threshold_aluminium',(-.085,.009,.48),(.16,.018,1.0),'Aluminium',.004)
    for z in [i*.075+.08 for i in range(12 if LOD<2 else 6)]:
        if LOD==2:z=z*2-.08
        box('threshold_water_groove',(-.085,.0188,z),(.13,.0016,.009),'Rubber',0)
    # Scraper strips on the existing landing, with narrow side rails.
    for z in (-.10,1.06):box('landing_mat_edge',(-.73,.007,z),(.75,.014,.024),'Aluminium',.004)
    for i in range(14 if LOD<2 else 8):
        z=-.075+i*(1.10/(13 if LOD<2 else 7))
        box('landing_scraper',(-.73,.006,z),(.72,.012,.035),'Rubber',.003)

def door():
    # No new door leaf: this family follows the original animated hinge.
    for z in (.045,.915):box('leaf_stile_trim',(-.031,1.04,z),(.012,2.015,.028),'Aluminium',.004)
    for y in (.035,2.045):box('leaf_rail_trim',(-.031,y,.48),(.012,.028,.89),'Aluminium',.004)
    box('brushed_kick_plate',(-.043,.19,.48),(.012,.29,.82),'Aluminium',.008)
    for y in (.079,.301):
        for z in (.10,.86):
            if LOD<2:tube('kickplate_screw',(-.050,y,z),(-.054,y,z),.004,'WeatheredSteel',8)
    box('pull_backplate',(-.045,1.08,.80),(.018,.34,.078),'Aluminium',.012)
    tube('pull_lower_return',(-.058,.95,.80),(-.13,.95,.80),.014,'Aluminium')
    tube('pull_upper_return',(-.058,1.23,.80),(-.13,1.23,.80),.014,'Aluminium')
    tube('round_vertical_pull',(-.13,.95,.80),(-.13,1.23,.80),.016,'Aluminium')
    tube('key_escutcheon',(-.045,.85,.80),(-.058,.85,.80),.024,'Aluminium')
    box('keyway',(-.059,.85,.80),(.001,.018,.004),'Rubber',.001)
    for y in (.24,1.05,1.85):
        tube('hinge_barrel',(-.026,y-.052,.019),(-.026,y+.052,.019),.018,'Aluminium')
    box('weather_sweep',(-.047,.018,.48),(.018,.026,.86),'Rubber',.003)
    box('closer_body',(-.07,1.94,.24),(.075,.07,.26),'Aluminium',.012)
    tube('closer_arm',(-.115,1.97,.24),(-.155,2.025,.53),.008,'WeatheredSteel')

def sign():
    # Human-scale 46x60 cm enamel panel centered 1.45 m above grade.
    box('visitor_enamel_panel',(0,1.45,0),(.46,.60,.012),'Enamel',.022)
    face('visitor_legible_print',(0,1.45,.0068),(1,0,0),(0,1,0),.431,.571,'VisitorPrint')
    # Actual U-channel rather than a solid square post.
    box('post_web',(0,.85,-.023),(.046,1.70,.005),'WeatheredSteel',.002)
    for x in (-.0205,.0205):box('post_channel_return',(x,.85,-.035),(.005,1.70,.024),'WeatheredSteel',.002)
    box('post_cap',(0,1.704,-.033),(.047,.008,.030),'WeatheredSteel',.002)
    for y in (1.20,1.70):
        tube('sign_fixing_bolt',(0,y,.0075),(0,y,.018),.007,'Aluminium',6)
        tube('sign_fixing_washer',(0,y,.007),(0,y,.010),.013,'Aluminium',(16,10,8)[LOD])
    if LOD<2:
        for y in (.15,.28,.41,.54,.67,.80,.93,1.06):
            box('post_punched_slot',(0,y,-.0199),(.008,.019,.0005),'Rubber',.002)

BUILDERS={'SM_Arrival_Entrance':entrance,'SM_Arrival_DoorDetails':door,'SM_Arrival_VisitorSign':sign}
CONTRACT={'status':'SOURCE_GENERATED_NATIVE_UNVERIFIED','mapping':'Three (x,y,z) -> Blender (x,-z,y) -> UE centimetres (x,z,y)*100','units':'metres','parts':{},'originality':'Original bpy geometry and procedural PBR/enamel artwork; no third-party imagery or meshes.'}
for name,builder in BUILDERS.items():
    lod_counts=[];info=None
    for LOD in range(3):
        COL=bpy.data.collections.new(name+'_LOD'+str(LOD));SCENE.collection.children.link(COL);PART=[]
        builder();pieces=list(PART)
        bpy.ops.object.select_all(action='DESELECT')
        for ob in pieces:ob.select_set(True)
        bpy.context.view_layer.objects.active=pieces[0];bpy.ops.object.join();ob=bpy.context.object
        ob.name=name if LOD==0 else name+'_LOD'+str(LOD)
        bpy.context.scene.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
        bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
        tri=ob.modifiers.new('explicit_export_triangles','TRIANGULATE');bpy.ops.object.modifier_apply(modifier=tri.name)
        points=[ob.matrix_world@v.co for v in ob.data.vertices]
        bounds=[[min(p[k] for p in points) for k in range(3)],[max(p[k] for p in points) for k in range(3)]]
        count=len(ob.data.polygons);lod_counts.append(count)
        if LOD==0:info={'bounds_local_m':bounds,'materials':[m.name for m in ob.data.materials],'origin':[0,0,0]}
        active(ob)
        bpy.ops.export_scene.fbx(filepath=str(OUT/(ob.name+'.fbx')),use_selection=True,object_types={'MESH'},apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',axis_forward='-Y',axis_up='Z',bake_anim=False,add_leaf_bones=False,mesh_smooth_type='FACE')
        if LOD>0:COL.hide_viewport=True;COL.hide_render=True
    info['lod_triangles']=lod_counts;CONTRACT['parts'][name]=info
CONTRACT['total_LOD0_triangles']=sum(row['lod_triangles'][0] for row in CONTRACT['parts'].values())
assert CONTRACT['total_LOD0_triangles']<15000,CONTRACT['total_LOD0_triangles']
(HERE/'model_contract.json').write_text(json.dumps(CONTRACT,indent=2)+'\n')
SCENE.world=bpy.data.worlds.new('Arrival_neutral_world');SCENE.world.color=(.22,.22,.22)
SCENE.render.engine='CYCLES';SCENE.cycles.device='CPU';SCENE.cycles.samples=24
SCENE.render.threads_mode='FIXED';SCENE.render.threads=2
SCENE.render.resolution_x=1400;SCENE.render.resolution_y=1000;SCENE.render.resolution_percentage=100
SCENE.view_settings.view_transform='AgX'
bpy.ops.wm.save_as_mainfile(filepath=str(HERE/'SARO_ArrivalEntrance.blend'))
print('ARRIVAL SOURCE PASS',json.dumps({name:row['lod_triangles'] for name,row in CONTRACT['parts'].items()}))
