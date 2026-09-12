"""Original SARO workstation models, metre scale, Blender +Y front / Unity -Z.
Cosmetic replacements preserve the existing Unity display and interaction anchors.
"""
import bpy, json, math
from pathlib import Path
from mathutils import Vector
root=Path(__file__).resolve().parents[2]
out=root/'Assets/Signal47/Art/Workstation18';out.mkdir(parents=True,exist_ok=True)
evidence=root.parent/'Artifacts/Workstation18';evidence.mkdir(parents=True,exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.context.scene.unit_settings.system='METRIC'
bpy.context.preferences.filepaths.save_version=0
palette={'Cream':(.52,.49,.39),'Dark':(.028,.038,.034),'Steel':(.25,.29,.27),'Fabric':(.10,.15,.13),'Key':(.62,.59,.47),'Amber':(.56,.24,.06)}
mats={}
for name,color in palette.items():
 m=bpy.data.materials.new('W18_'+name);m.diffuse_color=(*color,1);m.use_nodes=True
 b=m.node_tree.nodes['Principled BSDF'];b.inputs['Base Color'].default_value=(*color,1);b.inputs['Roughness'].default_value=.35 if name=='Steel' else .65;b.inputs['Metallic'].default_value=.7 if name=='Steel' else 0
 mats[name]=m
parts = []

def finish(o, name, mat, bevel=0, smooth=False):
    o.name = name; o.data.materials.append(mats[mat])
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    if bevel:
        mod = o.modifiers.new('Manufactured edge radius', 'BEVEL')
        mod.width = bevel; mod.segments = 3
        bpy.ops.object.modifier_apply(modifier=mod.name)
    if smooth:
        for p in o.data.polygons: p.use_smooth = True
    mod = o.modifiers.new('Face weighted normals', 'WEIGHTED_NORMAL')
    mod.keep_sharp = True
    bpy.ops.object.modifier_apply(modifier=mod.name)
    parts.append(o)
    return o

def box(name, loc, size, mat='Cream', bevel=.004):
    bpy.ops.mesh.primitive_cube_add(size=1, location=loc)
    o = bpy.context.object; o.scale = size
    return finish(o, name, mat, min(bevel, min(size)*.3))

def cylinder(name, loc, radius, depth, mat='Steel', axis=(0,0,1), vertices=24):
    bpy.ops.mesh.primitive_cylinder_add(vertices=vertices, radius=radius, depth=depth, location=loc)
    o = bpy.context.object
    o.rotation_euler = Vector(axis).to_track_quat('Z','Y').to_euler()
    return finish(o, name, mat, min(.002, radius*.15), True)

def tube(name, points, radius=.006, mat='Steel'):
    c = bpy.data.curves.new(name, 'CURVE'); c.dimensions = '3D'
    c.resolution_u = 8; c.bevel_depth = radius; c.bevel_resolution = 2
    s = c.splines.new('BEZIER'); s.bezier_points.add(len(points)-1)
    for p, co in zip(s.bezier_points, points):
        p.co = co; p.handle_left_type = 'AUTO'; p.handle_right_type = 'AUTO'
    o = bpy.data.objects.new(name,c); bpy.context.collection.objects.link(o)
    bpy.ops.object.select_all(action='DESELECT'); o.select_set(True)
    bpy.context.view_layer.objects.active = o; bpy.ops.object.convert(target='MESH')
    return finish(bpy.context.object,name,mat, smooth=True)

def crt():
    # The live Unity quad remains at front=.435, height=.60, size=.82 x .49.
    # Tapered rear cabinet gives depth without covering that retained surface.
    v=[(-.55,.385,.19),(.55,.385,.19),(.55,.385,1.015),(-.55,.385,1.015),
       (-.39,-.33,.28),(.39,-.33,.28),(.39,-.33,.88),(-.39,-.33,.88)]
    faces=[(0,1,2,3),(7,6,5,4),(0,4,5,1),(1,5,6,2),(2,6,7,3),(3,7,4,0)]
    me=bpy.data.meshes.new('Tapered cabinet');me.from_pydata(v,[],faces);me.update()
    o=bpy.data.objects.new('Tapered cabinet',me);bpy.context.collection.objects.link(o)
    bpy.context.view_layer.objects.active=o;o.select_set(True)
    bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT');bpy.ops.mesh.normals_make_consistent(inside=False);bpy.ops.object.mode_set(mode='OBJECT')
    finish(o,'Tapered cabinet','Cream',.018)
    for x in (-.486,.486):box('Rounded bezel side',(x,.434,.60),(.112,.12,.69),'Cream',.022)
    for z in (.296,.904):box('Rounded bezel rail',(0,.434,z),(.91,.12,.078),'Cream',.018)
    for x in (-.421,.421):box('Inner screen lip',(x,.451,.60),(.018,.028,.523),'Dark',.005)
    for z in (.338,.862):box('Inner screen lip',(0,.451,z),(.85,.028,.018),'Dark',.005)
    box('Control strip',(0,.411,.211),(.93,.036,.075),'Dark',.009)
    for x in (.245,.338,.431):
        cylinder('Brightness contrast power',(x,.456,.211),.025,.034,'Steel',axis=(0,1,0),vertices=20)
        box('Knob index',(x,.475,.220),(.003,.003,.018),'Key',.0008)
    box('Power indicator',(-.41,.434,.211),(.016,.007,.010),'Amber',.002)
    box('Pedestal foot',(0,0,.027),(.69,.47,.054),'Dark',.016)
    box('Tilt pedestal',(0,-.01,.115),(.25,.23,.135),'Cream',.022)
    for i in range(11):
        # Vents sit on a rear service panel rather than floating over a slope.
        box('Rear vent slot',(-.27+i*.054,-.337,.62),(.024,.014,.19),'Dark',.004)
    box('Rear connector recess',(0,-.340,.35),(.27,.018,.075),'Dark',.007)
    cylinder('Rear cable socket',(.07,-.355,.35),.019,.023,'Steel',axis=(0,1,0))
    for x in (-.34,.34):
        for z in (.32,.82):cylinder('Case screw',(x,-.339,z),.008,.012,'Steel',axis=(0,1,0),vertices=12)

def keyboard():
    # A visible supported tray fixes the old keyboard's unsupported overhang.
    box('Sliding keyboard tray',(0,.745,-.018),(1.12,.73,.044),'Steel',.010)
    for x in (-.515,.515):box('Tray support runner',(x,.51,-.051),(.029,.62,.035),'Dark',.005)
    box('Tray front folded edge',(0,1.102,-.002),(1.12,.022,.066),'Dark',.005)
    box('Keyboard lower case',(0,.80,.042),(1.015,.44,.076),'Cream',.018)
    box('Recessed key well',(0,.80,.080),(.967,.398,.014),'Dark',.009)
    # Low bevelled keys, staggered rows and separate number block.
    for row in range(4):
        for col in range(11):
            x=-.417+col*.061+(row%2)*.010;y=.676+row*.065
            box('Alphanumeric key',(x,y,.098),(.052,.052,.030),'Key',.007)
    for row in range(4):
        for col in range(3):box('Numeric key',(.285+col*.061,.676+row*.065,.098),(.052,.052,.030),'Cream',.007)
    box('Space bar',(-.135,.950,.099),(.32,.048,.033),'Key',.007)
    for x in (-.415,-.352,.105,.171):box('Modifier key',(x,.950,.098),(.052,.048,.030),'Cream',.007)
    for x in (.296,.359,.422):box('Status LED',(x,.624,.084),(.012,.014,.005),'Amber',.002)
    tube('Keyboard cable',[(.40,.595,.055),(.44,.49,.055),(.37,.40,.067),(.27,.36,.13)],.008,'Dark')

def chair():
    cylinder('Height column',(0,0,.29),.031,.40,'Steel')
    cylinder('Column sleeve',(0,0,.22),.047,.24,'Dark')
    cylinder('Star hub',(0,0,.095),.095,.09,'Dark')
    for i in range(5):
        a=i*math.tau/5;x,y=math.cos(a),math.sin(a)
        tube('Curved star leg',[(x*.06,y*.06,.12),(x*.20,y*.20,.10),(x*.36,y*.36,.065)],.022,'Steel')
        cylinder('Caster fork',(x*.36,y*.36,.055),.026,.065,'Dark')
        for s in (-1,1):
            cylinder('Twin caster',(x*.36-y*s*.028,y*.36+x*s*.028,.044),.044,.022,'Dark',axis=(-y,x,0),vertices=20)
    box('Pressed seat pan',(0,0,.49),(.60,.57,.058),'Dark',.025)
    box('Upholstered seat',(0,.018,.557),(.62,.59,.090),'Fabric',.035)
    for x in (-.245,.245):tube('Back support',[(x,-.20,.46),(x,-.29,.68),(x,-.32,1.05)],.018,'Steel')
    box('Backrest shell',(0,-.34,.98),(.61,.078,.54),'Dark',.028)
    box('Backrest cushion',(0,-.288,.98),(.575,.064,.50),'Fabric',.025)
    for z in (.80,1.13):tube('Upholstery seam',[(-.254,-.252,z),(0,-.25,z),(.254,-.252,z)],.0025,'Dark')
    for x in (-.36,.36):
        tube('Tubular arm support',[(x*.8,-.15,.49),(x,-.17,.73),(x,.17,.73),(x*.8,.17,.49)],.016,'Steel')
        box('Armrest pad',(x,.015,.775),(.105,.39,.048),'Dark',.018)
    tube('Height lever',[(.12,0,.475),(.30,.02,.46),(.38,.06,.46)],.007,'Steel')
    box('Lever paddle',(.38,.08,.46),(.065,.09,.022),'Dark',.008)

def bounds(objects):
    bpy.context.view_layer.update();c=[o.matrix_world@Vector(v) for o in objects for v in o.bound_box]
    return [max(v[i] for v in c)-min(v[i] for v in c) for i in range(3)]

records=[];assets=[]
for name,recipe,budget in [('CRT',crt,11000),('Keyboard',keyboard,17000),('Chair',chair,16000)]:
    parts=[];recipe()
    if name=='Keyboard':
        # Viewed from +Y, screen-right is -X. Keep the numeric block on the right
        # after the terminal's retained 180-degree Unity placement.
        for o in parts:
            o.location.x=-o.location.x;o.scale.x=-o.scale.x
            bpy.ops.object.select_all(action='DESELECT');o.select_set(True);bpy.context.view_layer.objects.active=o
            bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
            bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT');bpy.ops.mesh.normals_make_consistent(inside=False);bpy.ops.object.mode_set(mode='OBJECT')
    for o in parts:
        bpy.ops.object.select_all(action='DESELECT');o.select_set(True);bpy.context.view_layer.objects.active=o
        bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT');bpy.ops.uv.smart_project(angle_limit=math.radians(66),island_margin=.015);bpy.ops.object.mode_set(mode='OBJECT')
    batched=[]
    groups={mat:[o for o in parts if o.data.materials[0]==mats[mat]] for mat in mats}
    for mat,group in groups.items():
        if not group:continue
        bpy.ops.object.select_all(action='DESELECT')
        for o in group:o.select_set(True)
        bpy.context.view_layer.objects.active=group[0];bpy.ops.object.join();o=bpy.context.object;o.name=name+'_'+mat;batched.append(o)
    dims=bounds(batched);triangles=sum(len(p.vertices)-2 for o in batched for p in o.data.polygons)
    assert triangles<=budget,(name,triangles,budget)
    assert all(len(o.data.uv_layers)>0 and all(p.area>1e-12 for p in o.data.polygons) for o in batched)
    bpy.ops.object.select_all(action='DESELECT')
    for o in batched:o.select_set(True)
    path=out/('W18_'+name+'.fbx');bpy.ops.export_scene.fbx(filepath=str(path),use_selection=True,object_types={'MESH'},add_leaf_bones=False,bake_anim=False,axis_forward='-Z',axis_up='Y',apply_unit_scale=True)
    bpy.ops.object.select_all(action='DESELECT');bpy.ops.import_scene.fbx(filepath=str(path));copies=list(bpy.context.selected_objects);roundtrip=bounds(copies)
    assert all(abs(a-b)<1e-5 for a,b in zip(dims,roundtrip)),(dims,roundtrip)
    for o in copies:bpy.data.objects.remove(o,do_unlink=True)
    records.append({'name':name,'fbx':path.name,'dimensions_blender_xyz':dims,'triangles':triangles,'triangle_budget':budget,'render_meshes':len(batched),'uv_layers':'PASS','nonzero_face_area':'PASS','fbx_roundtrip_bounds':'PASS'})
    assets.append((name,batched))
for name,objects in assets:
    for o in objects:o.location+=Vector({'CRT':(0,0,.82),'Keyboard':(0,0,.82),'Chair':(-1.2,.6,0)}[name])
scene=bpy.context.scene;scene.world=bpy.data.worlds.new('Neutral studio');scene.world.color=(.18,.18,.18)
bpy.ops.object.camera_add(location=(3.1,4.0,2.8));cam=bpy.context.object;scene.camera=cam;cam.data.type='ORTHO';cam.data.ortho_scale=3.55
def aim(target):cam.rotation_euler=(Vector(target)-cam.location).to_track_quat('-Z','Y').to_euler()
aim((-.35,.15,.85))
for loc,energy,size in [((2,3,5),800,4),((-3,0,3),550,3)]:
    bpy.ops.object.light_add(type='AREA',location=loc);o=bpy.context.object;o.data.energy=energy;o.data.shape='DISK';o.data.size=size;o.rotation_euler=(Vector((0,0,.7))-o.location).to_track_quat('-Z','Y').to_euler()
scene.render.engine='CYCLES';scene.cycles.samples=24;scene.render.resolution_x=1100;scene.render.resolution_y=850;scene.render.resolution_percentage=100
bpy.ops.wm.save_as_mainfile(filepath=str(root/'Blender/SIGNAL47_workstation18.blend'))
(out/'source-manifest.json').write_text(json.dumps({'authored_original':True,'external_assets':[],'blender':bpy.app.version_string,'units':'metres','source':'Unity/Blender/SIGNAL47_workstation18.blend','recipe':'Unity/Blender/Source/workstation18.py','assets':records},indent=2)+'\n')
scene.render.filepath=str(evidence/'front-authoring.png');bpy.ops.render.render(write_still=True)
cam.location=(2.4,-3.8,2.6);aim((-.35,.10,.9));scene.render.filepath=str(evidence/'rear-authoring.png');bpy.ops.render.render(write_still=True)
print('WORKSTATION18_ASSETS_OK',json.dumps(records))
