"""Original, metre-scale archive furniture. Blender 4.5 CLI; no external assets.

Front is +Y in Blender (-Z after Unity's FBX conversion). Parts are merged by
material for rendering; the folio label remains a named mesh for text placement.
"""
import bpy, json, math
from pathlib import Path
from mathutils import Vector

root = Path(__file__).resolve().parents[2]
out = root / 'Assets/Signal47/Art/Archive17'
evidence = root.parent / 'Artifacts/Archive17'
out.mkdir(exist_ok=True); evidence.mkdir(exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.context.scene.unit_settings.system = 'METRIC'
bpy.context.preferences.filepaths.save_version = 0
palette = {'Green': (.12,.18,.13), 'Steel': (.35,.39,.37),
           'Rubber': (.025,.03,.025), 'Paper': (.80,.76,.63),
           'Card': (.37,.25,.12), 'Tab': (.40,.08,.035),
           'WarmDiffuser': (.95,.77,.43)}
mats = {}
for name, color in palette.items():
    m = bpy.data.materials.new('A17_' + name); m.use_nodes = True
    m.diffuse_color = (*color, 1)
    bsdf = m.node_tree.nodes['Principled BSDF']
    bsdf.inputs['Base Color'].default_value = (*color, 1)
    bsdf.inputs['Roughness'].default_value = .38 if name == 'Steel' else .68
    bsdf.inputs['Metallic'].default_value = .7 if name == 'Steel' else 0
    mats[name] = m
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

def box(name, loc, size, mat='Green', bevel=.004):
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

def handle(x,y,z,width=.22):
    for dx in (-width/2,width/2):
        box('Handle foot',(x+dx,y,z),(.035,.018,.040),'Steel',.004)
    tube('Bent drawer pull',[(x-width/2,y+.005,z),(x-width/2,y+.04,z),
                             (x+width/2,y+.04,z),(x+width/2,y+.005,z)],.009)

def label_pocket(x,y,z):
    box('Label pocket recess',(x,y,z),(.302,.008,.094),'Rubber',.001)
    box('Paper index',(x,y+.005,z),(.272,.004,.069),'Paper',.001)
    for dx in (-.151,.151): box('Folded label side',(x+dx,y+.007,z),(.009,.012,.094),'Steel',.002)
    for dz in (-.047,.047): box('Folded label lip',(x,y+.007,z+dz),(.304,.012,.008),'Steel',.001)

def desk():
    box('Rolled tabletop edge',(0,0,.758),(2.7,1.35,.084),'Steel',.017)
    box('Green writing surface',(0,0,.797),(2.67,1.32,.016),'Green',.009)
    # Separate drawer pedestals give a usable knee opening and realistic mass.
    for x in (-1.045,1.045):
        box('Pedestal case',(x,-.01,.41),(.48,1.10,.67),'Green',.014)
        box('Recessed toe plinth',(x,-.02,.058),(.40,.99,.10),'Rubber',.010)
        for z in (.235,.525):
            box('Drawer shadow gap',(x,.548,z),(.445,.016,.282),'Rubber',.005)
            box('Inset drawer front',(x,.559,z),(.427,.025,.262),'Green',.008)
            handle(x,.58,z+.063,.18)
        cylinder('Desk lock',(x,.59,.697),.017,.013,axis=(0,1,0))
    box('Rear modesty panel',(0,-.50,.50),(1.64,.026,.40),'Green',.004)
    box('Shallow central drawer',(0,.41,.702),(1.48,.34,.10),'Green',.007)
    handle(0,.59,.704,.25)

def cabinet():
    box('Recessed cabinet plinth',(0,0,.045),(1.22,.49,.09),'Rubber',.012)
    box('Cabinet folded case',(0,-.025,1.018),(1.3,.56,1.856),'Green',.015)
    box('Rolled cabinet crown',(0,-.025,1.945),(1.3,.56,.030),'Steel',.007)
    for i in range(4):
        z = .28+i*.46
        box('Drawer perimeter shadow',(0,.261,z),(1.216,.012,.432),'Rubber',.005)
        box('Drawer pressed panel',(0,.273,z),(1.192,.028,.407),'Green',.010)
        box('Drawer recessed center',(0,.288,z),(1.145,.007,.360),'Green',.007)
        handle(0,.299,z-.078)
        label_pocket(0,.299,z+.056)
        # Small press-in stops on each front, period industrial construction.
        for x in (-.548,.548): cylinder('Panel rivet',(x,.299,z),.006,.005,axis=(0,1,0),vertices=12)
    cylinder('Master lock',(.50,.29,1.90),.018,.015,axis=(0,1,0))

def lamp():
    box('Rubber underside',(0,0,.007),(.27,.205,.014),'Rubber',.005)
    box('Weighted rounded base',(0,0,.034),(.285,.22,.051),'Green',.014)
    box('Base inset',(0,0,.061),(.235,.176,.017),'Steel',.006)
    cylinder('Stem collar',(-.066,-.025,.088),.031,.045)
    tube('Curved support',[(-.066,-.025,.09),(-.066,-.025,.30),(-.05,-.025,.405),(.02,-.025,.419)],.012)
    cylinder('Shade pivot',(.02,-.025,.421),.027,.070,axis=(1,0,0))
    # Hollow curved metal shade, rather than a solid block; open underside.
    verts=[]; faces=[]; steps=20
    for x in (-.205,.245):
        for j in range(steps+1):
            a=math.pi*j/steps
            verts.append((x,.112*math.cos(a),.429+.088*math.sin(a)))
    for j in range(steps): faces.append((j,j+1,j+steps+2,j+steps+1))
    mesh=bpy.data.meshes.new('Curved shade shell');mesh.from_pydata(verts,[],faces);mesh.update()
    o=bpy.data.objects.new('Curved shade shell',mesh);bpy.context.collection.objects.link(o)
    bpy.ops.object.select_all(action='DESELECT');o.select_set(True);bpy.context.view_layer.objects.active=o
    mod=o.modifiers.new('Sheet metal thickness','SOLIDIFY');mod.thickness=.004
    bpy.ops.object.modifier_apply(modifier=mod.name);finish(o,'Curved green shade','Green',.001,True)
    for x in (-.205,.245):
        cap_verts=[(x,0,.429)]+[(x,.111*math.cos(math.pi*j/steps),.429+.087*math.sin(math.pi*j/steps)) for j in range(steps+1)]
        cap_faces=[(0,j+1,j+2) if x<0 else (0,j+2,j+1) for j in range(steps)]
        cap_mesh=bpy.data.meshes.new('Shade end');cap_mesh.from_pydata(cap_verts,[],cap_faces);cap_mesh.update()
        cap=bpy.data.objects.new('Closed shade end',cap_mesh);bpy.context.collection.objects.link(cap)
        bpy.ops.object.select_all(action='DESELECT');cap.select_set(True);bpy.context.view_layer.objects.active=cap
        mod=cap.modifiers.new('End cap thickness','SOLIDIFY');mod.thickness=.002;bpy.ops.object.modifier_apply(modifier=mod.name)
        finish(cap,'Closed shade end','Green')
        tube('Shade rolled end',[(x,.112*math.cos(math.pi*j/8),.429+.088*math.sin(math.pi*j/8)) for j in range(9)],.004)
    for y in (-.112,.112): tube('Shade rolled lip',[(-.205,y,.429),(.245,y,.429)],.004)
    cylinder('Warm tube',(.02,0,.449),.018,.38,'WarmDiffuser',axis=(1,0,0))
    cylinder('Base toggle socket',(.065,.04,.077),.019,.011,'Rubber')
    cylinder('Toggle',(.065,.04,.089),.007,.023)
    tube('Power cord',[(0,-.10,.024),(.10,-.17,.008),(.20,-.24,.008),(.25,-.32,.008)],.004,'Rubber')

def folio():
    box('Back cover',(0,0,.002),(.46,.34,.004),'Card',.0009)
    for i in range(7):
        o=box('Individual paper',(i*.0003-.001,0,.005+i*.0011),(.430,.31,.0008),'Paper',.0002)
        o.rotation_euler.z=(i-3)*.0018
    # Mesh cover lifts at the outer corner; the label stays planar/readable.
    verts=[(-.23,-.17,.014),(.23,-.17,.018),(.23,.17,.014),(-.23,.17,.014)]
    mesh=bpy.data.meshes.new('Cover');mesh.from_pydata(verts,[],[(0,1,2,3)]);mesh.update()
    o=bpy.data.objects.new('Soft cover',mesh);bpy.context.collection.objects.link(o)
    bpy.ops.object.select_all(action='DESELECT');o.select_set(True);bpy.context.view_layer.objects.active=o
    mod=o.modifiers.new('Card thickness','SOLIDIFY');mod.thickness=.0015;bpy.ops.object.modifier_apply(modifier=mod.name)
    finish(o,'Soft cover','Card')
    for x in (-.210,-.202): box('Binding crease',(x,0,.015),(.0015,.327,.001),'Card',.0002)
    box('Index tab',(.135,.181,.016),(.103,.032,.002),'Tab',.0005)
    box('Label field',(-.040,-.01,.018),(.272,.113,.001),'Paper',.0002)
    for x in (-.175,-.157):
        tube('Bent metal fastener',[(x,.088,.018),(x,.13,.021),(x+.009,.135,.021),(x+.01,.090,.020)],.0015)

def papers():
    for i in range(12):
        o=box('Stacked sheet',((i%3-1)*.002,(i%4)*.001,.0014+i*.0018),(.34,.25,.0014),'Paper',.00025)
        o.rotation_euler.z=(i%3-1)*.007
    box('Red archive slip',(.052,.07,.024),(.12,.052,.001),'Tab',.0003)

def bounds(objects):
    bpy.context.view_layer.update()
    coords=[o.matrix_world@Vector(c) for o in objects for c in o.bound_box]
    return [max(c[i] for c in coords)-min(c[i] for c in coords) for i in range(3)]

records=[]; assets=[]
for name, recipe, budget in [('Desk',desk,14000),('Cabinet',cabinet,16000),('Lamp',lamp,12000),('Folio',folio,5000),('PaperStack',papers,5000)]:
    parts=[];recipe()
    # Smart unwrap for the existing engine material textures, then batch meshes.
    for o in parts:
        bpy.ops.object.select_all(action='DESELECT');o.select_set(True);bpy.context.view_layer.objects.active=o
        bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT')
        bpy.ops.uv.smart_project(angle_limit=math.radians(66),island_margin=.015)
        bpy.ops.object.mode_set(mode='OBJECT')
    batched=[]
    groups={mat:[o for o in parts if o.data.materials[0]==mats[mat] and o.name!='Label field'] for mat in mats}
    for mat,group in groups.items():
        if not group: continue
        bpy.ops.object.select_all(action='DESELECT')
        for o in group:o.select_set(True)
        bpy.context.view_layer.objects.active=group[0];bpy.ops.object.join()
        o=bpy.context.object;o.name=name+'_'+mat;batched.append(o)
    labels=[o for o in bpy.context.scene.objects if o.type=='MESH' and o.name=='Label field'] if name=='Folio' else []
    batched+=labels
    for o in batched:
        assert len(o.data.uv_layers)>0
        assert all(p.area>1e-12 for p in o.data.polygons),o.name
    dims=bounds(batched);triangles=sum(len(p.vertices)-2 for o in batched for p in o.data.polygons)
    assert triangles<=budget,(name,triangles,budget)
    bpy.ops.object.select_all(action='DESELECT')
    for o in batched:o.select_set(True)
    path=out/('A17_'+name+'.fbx')
    bpy.ops.export_scene.fbx(filepath=str(path),use_selection=True,object_types={'MESH'},add_leaf_bones=False,bake_anim=False,axis_forward='-Z',axis_up='Y',apply_unit_scale=True)
    bpy.ops.object.select_all(action='DESELECT');bpy.ops.import_scene.fbx(filepath=str(path))
    copies=list(bpy.context.selected_objects);roundtrip=bounds(copies)
    assert all(abs(a-b)<1e-5 for a,b in zip(dims,roundtrip)),(dims,roundtrip)
    for o in copies:bpy.data.objects.remove(o,do_unlink=True)
    records.append({'name':name,'fbx':path.name,'dimensions_blender_xyz':dims,'triangles':triangles,'triangle_budget':budget,'render_meshes':len(batched),'uv_layers':'PASS','nonzero_face_area':'PASS','fbx_roundtrip_bounds':'PASS'})
    assets.append((name,batched))

# Save a usable studio arrangement without changing metre-scale exported meshes.
offsets={'Desk':(0,0,0),'Cabinet':(-1.85,-1.8,0),'Lamp':(-.83,.03,.806),'Folio':(0,.16,.806),'PaperStack':(.69,-.12,.806)}
for name,objects in assets:
    for o in objects:o.location+=Vector(offsets[name])
scene=bpy.context.scene;scene.world=bpy.data.worlds.new('Neutral archive studio');scene.world.color=(.18,.18,.18)
bpy.ops.object.camera_add(location=(3.7,4.7,3.4));cam=bpy.context.object
cam.rotation_euler=(Vector((-.3,-.5,.9))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.type='ORTHO';cam.data.ortho_scale=5.7;scene.camera=cam
for loc,energy,size in [((1,2,5),800,5),((-3,0,3),500,3)]:
    bpy.ops.object.light_add(type='AREA',location=loc);light=bpy.context.object;light.data.energy=energy;light.data.shape='DISK';light.data.size=size
    light.rotation_euler=(Vector((0,0,.7))-light.location).to_track_quat('-Z','Y').to_euler()
scene.render.engine='CYCLES';scene.cycles.samples=24
scene.render.resolution_x=1100;scene.render.resolution_y=850;scene.render.resolution_percentage=100
bpy.ops.wm.save_as_mainfile(filepath=str(root/'Blender/SIGNAL47_archive17_furniture.blend'))
(out/'source-manifest.json').write_text(json.dumps({'authored_original':True,'external_assets':[],'blender':bpy.app.version_string,'units':'metres','source':'Unity/Blender/SIGNAL47_archive17_furniture.blend','recipe':'Unity/Blender/Source/archive17_furniture.py','assets':records},indent=2)+'\n')
scene.render.filepath=str(evidence/'furniture-authoring.png');bpy.ops.render.render(write_still=True)
print('ARCHIVE17_ASSETS_OK',json.dumps(records))
