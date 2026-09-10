"""Original SIGNAL / 47 photographic laboratory props, in metres.

Run with Blender 4.5 LTS --background --python this_file.py. The editable
collection-based .blend is saved before export copies are consolidated by
material. FBX uses -Z forward / Y up, applied metric scale, real bevels,
recalculated normals, and generated UVs. No downloaded resources are used.
"""
import bpy
import json
import math
from pathlib import Path
from mathutils import Vector

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'Assets/Signal47/Art/Chapter09'
OUT.mkdir(parents=True, exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.context.scene.unit_settings.system = 'METRIC'
bpy.context.scene.unit_settings.scale_length = 1

MATERIALS = {
    'Cream': ((.64, .61, .49, 1), .52, .02),
    'Green': ((.20, .27, .23, 1), .58, .05),
    'Steel': ((.27, .31, .30, 1), .31, .72),
    'Rubber': ((.025, .030, .026, 1), .86, .0),
    'Paper': ((.85, .82, .68, 1), .90, .0),
    'Amber': ((.22, .10, .025, 1), .25, .1),
    'Red': ((.30, .055, .025, 1), .48, .0),
    'Water': ((.025, .058, .054, 1), .16, .1),
}
mats = {}
for key, (color, rough, metal) in MATERIALS.items():
    m = bpy.data.materials.new('CH09_' + key)
    m.diffuse_color = color
    m.use_nodes = True
    bsdf = m.node_tree.nodes.get('Principled BSDF')
    bsdf.inputs['Base Color'].default_value = color
    bsdf.inputs['Roughness'].default_value = rough
    bsdf.inputs['Metallic'].default_value = metal
    mats[key] = m

current = None
collections = []

def group(name):
    global current
    current = bpy.data.collections.new(name)
    bpy.context.scene.collection.children.link(current)
    collections.append(current)

def finish(o, name, material, bevel=0):
    o.name = name
    for c in list(o.users_collection):
        c.objects.unlink(o)
    current.objects.link(o)
    o.data.materials.clear()
    o.data.materials.append(mats[material])
    if bevel:
        mod = o.modifiers.new('Manufactured edge radius', 'BEVEL')
        mod.width = bevel
        mod.segments = 3
        o.modifiers.new('Weighted corner normals', 'WEIGHTED_NORMAL')
    return o

def box(name, loc, size, material, bevel=.006):
    bpy.ops.mesh.primitive_cube_add(size=1, location=loc)
    o = bpy.context.object
    o.scale = size
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    return finish(o, name, material, bevel)

def cylinder(name, loc, radius, depth, material, vertices=24, rotation=None):
    bpy.ops.mesh.primitive_cylinder_add(vertices=vertices, radius=radius, depth=depth, location=loc)
    o = bpy.context.object
    if rotation:
        o.rotation_euler = rotation
    for p in o.data.polygons:
        p.use_smooth = len(p.vertices) == 4
    return finish(o, name, material, .002)

def tube(name, points, radius, material):
    curve = bpy.data.curves.new(name, 'CURVE')
    curve.dimensions = '3D'
    curve.bevel_depth = radius
    curve.bevel_resolution = 2
    spline = curve.splines.new('POLY')
    spline.points.add(len(points)-1)
    for point, co in zip(spline.points, points):
        point.co = (*co, 1)
    o = bpy.data.objects.new(name, curve)
    bpy.context.collection.objects.link(o)
    bpy.context.view_layer.objects.active = o
    o.select_set(True)
    bpy.ops.object.convert(target='MESH')
    o.select_set(False)
    return finish(o, name, material)

def profile(name, rings, material, count=32):
    verts = []
    for z, radius in rings:
        verts.extend([(math.cos(i*math.tau/count)*radius, math.sin(i*math.tau/count)*radius, z)
                      for i in range(count)])
    faces = []
    for k in range(len(rings)-1):
        for i in range(count):
            j = (i+1)%count
            faces.append((k*count+i, k*count+j, (k+1)*count+j, (k+1)*count+i))
    faces.extend([tuple(reversed(range(count))), tuple((len(rings)-1)*count+i for i in range(count))])
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(verts, [], faces)
    mesh.update()
    o = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(o)
    for p in mesh.polygons:
        p.use_smooth = len(p.vertices) == 4
    return finish(o, name, material, .002)

group('CH09_Enlarger')
box('Weighted enamel baseboard', (0, 0, .028), (.76, .70, .055), 'Cream', .012)
for x in (-.29, .29):
    for y in (-.24, .24):
        cylinder('Isolated rubber foot', (x, y, .010), .03, .020, 'Rubber')
box('Focusing column', (0, .25, .59), (.10, .09, 1.08), 'Green', .006)
box('Column stainless scale', (-.062, .202, .61), (.018, .012, 1.02), 'Steel', .001)
for i in range(21):
    box('Engraved height graduation', (-.063, .194, .16+i*.043), (.026 if i%5==0 else .014, .004, .003), 'Rubber', 0)
box('Head carriage', (0, .218, .815), (.17, .13, .20), 'Steel', .009)
box('Projector cantilever', (0, .065, .84), (.10, .29, .072), 'Green', .008)
cylinder('Focus handwheel', (.116, .205, .79), .070, .026, 'Rubber', rotation=(0, math.pi/2, 0))
cylinder('Focus hub', (.133, .205, .79), .021, .01, 'Steel', rotation=(0, math.pi/2, 0))
box('Condenser housing', (0, -.135, .89), (.33, .34, .23), 'Cream', .022)
box('Housing lower seam', (0, -.135, .780), (.34, .35, .019), 'Rubber', .004)
box('Negative carrier slot', (0, -.145, .763), (.29, .30, .026), 'Steel', .002)
box('Negative carrier grip', (.19, -.145, .764), (.10, .17, .02), 'Rubber', .003)
for i in range(7):
    v = .024 if i%2==0 else .002
    box('Accordion bellows fold', (0, -.135, .742-i*.022), (.225+v, .235+v, .018), 'Rubber', .002)
cylinder('Lens focusing barrel', (0, -.135, .556), .068, .070, 'Steel')
cylinder('Lens knurled grip', (0, -.135, .535), .077, .025, 'Rubber')
cylinder('Recessed optical glass', (0, -.135, .519), .054, .006, 'Water')
cylinder('Housing top vent cap', (0, -.135, 1.028), .09, .045, 'Green')
for i in range(7):
    box('Head ventilation slot', (-.125+i*.042, -.307, .925), (.020, .007, .032), 'Rubber', .002)
for x in (-.135, .135):
    for z in (.82, .965):
        cylinder('Enamel case screw', (x, -.311, z), .007, .005, 'Steel', 12, (math.pi/2, 0, 0))
box('Print easel backing', (0, -.12, .07), (.49, .39, .025), 'Rubber', .006)
box('Unexposed photographic paper', (0, -.12, .086), (.435, .33, .003), 'Paper', .001)
box('Easel masking blade left', (-.223, -.12, .093), (.025, .38, .013), 'Steel', .001)
box('Easel masking blade back', (0, .043, .093), (.47, .025, .013), 'Steel', .001)
tube('Power cable', [(.15,.005,.89),(.24,.075,.75),(.27,.23,.43),(.20,.31,.20),(.34,.31,.065)], .007, 'Rubber')

group('CH09_ProcessingTrays')
for i, material in enumerate(('Cream', 'Red', 'Green')):
    x = (i-1)*.51
    box('Tray basin '+str(i), (x,0,.019), (.465,.37,.035), material, .010)
    for side in (-1,1):
        wall = box('Long rolled lip', (x, side*.183, .062), (.48,.026,.072), material, .009)
        wall.rotation_euler.x=side*math.radians(-8)
        wall = box('Short rolled lip', (x+side*.229,0,.062), (.026,.37,.072), material, .009)
        wall.rotation_euler.y=side*math.radians(8)
    for ridge in range(6):
        box('Drain ridge', (x-.16+ridge*.064,0,.039), (.010,.30,.006), material, .002)
    box('Chemical bath surface', (x,0,.046), (.41,.31,.004), 'Water', .004)
    # Tongs rest across a tray; bent end and hinge make their use legible.
    if i == 1:
        for side in (-1,1):
            tong=box('Print tong arm', (x+side*.020,-.03,.104), (.012,.28,.012), 'Steel', .002)
            tong.rotation_euler.z=side*math.radians(7)
        cylinder('Print tong hinge', (x,.11,.105), .023, .012, 'Steel', 16)
        for side in (-1,1):
            box('Rubber tong tip', (x+side*.038,-.155,.105), (.025,.042,.015), 'Rubber', .003)

group('CH09_Sink')
box('Sink folded backboard', (0,.265,.19), (1.55,.06,.37), 'Steel', .016)
box('Sink basin floor', (0,0,.035), (1.45,.52,.07), 'Steel', .022)
for side in (-1,1):
    box('Sink sidewall', (side*.73,0,.125), (.04,.52,.20), 'Steel', .015)
    box('Sink longwall', (0,side*.265,.125), (1.52,.04,.20), 'Steel', .015)
    box('Rolled basin rim', (0,side*.276,.226), (1.59,.035,.025), 'Steel', .012)
cylinder('Drain strainer', (.25,-.02,.078), .053, .01, 'Rubber')
for i in range(4):
    box('Drain strainer slot', (.224+i*.017,-.02,.085), (.006,.066,.005), 'Steel', 0)
tube('Swan-neck faucet',[(0,.23,.28),(0,.23,.48),(0,.22,.53),(0,.16,.56),(0,.05,.56),(0,.00,.53),(0,.00,.48)], .017,'Steel')
for x in (-.18,.18):
    cylinder('Faucet valve body',(x,.22,.27),.032,.12,'Steel')
    box('Cross tap handle',(x,.22,.342),(.10,.022,.015),'Cream',.007)
    box('Cross tap handle',(x,.22,.342),(.022,.10,.015),'Cream',.007)
tube('Visible waste pipe',[(.25,0,0),(.25,0,-.31),(.25,.09,-.38),(.25,.21,-.38),(.25,.27,-.28),(.25,.28,-.15)], .025,'Steel')

group('CH09_ChemicalBottle')
bottle=profile('Amber chemical storage bottle', [(0,.062),(.012,.072),(.18,.072),(.215,.050),(.225,.027),(.255,.027)],'Amber')
cylinder('Threaded black bottle cap',(0,0,.269),.033,.035,'Rubber')
for i in range(16):
    angle=i*math.tau/16
    cylinder('Cap grip flute',(math.cos(angle)*.032,math.sin(angle)*.032,.269),.003,.029,'Rubber',8)
box('Editable label backing',(0,-.071,.123),(.104,.005,.094),'Paper',.002)

group('CH09_LabStool')
cylinder('Upholstered round seat',(0,0,.60),.22,.080,'Rubber',32)
cylinder('Seat pressed steel underside',(0,0,.559),.195,.019,'Steel',32)
for i in range(4):
    angle=math.tau*i/4+math.pi/4
    top=(math.cos(angle)*.13,math.sin(angle)*.13,.55)
    bottom=(math.cos(angle)*.235,math.sin(angle)*.235,.035)
    tube('Splayed steel stool leg',[bottom,top],.018,'Steel')
    cylinder('Stool rubber foot',(bottom[0],bottom[1],.023),.029,.037,'Rubber',16)
ring=[(math.cos(i*math.tau/48)*.19,math.sin(i*math.tau/48)*.19,.23) for i in range(49)]
tube('Circular footrest',ring,.012,'Steel')

group('CH09_FieldCabinet')
box('Folded galvanized cabinet shell', (0,0,.585), (.70,.46,1.15), 'Green', .019)
box('Pressed weather door', (0,-.246,.595), (.645,.035,1.065), 'Green', .015)
box('Door shadow gasket', (0,-.228,.595), (.672,.009,1.09), 'Rubber', .003)
box('Rain lip', (0,-.038,1.168), (.756,.56,.046), 'Green', .008)
for x in (-.255,.255):
    box('Cabinet mounting foot', (x,0,.025), (.095,.55,.05), 'Steel', .006)
for z in (.28,.91):
    cylinder('Door barrel hinge',(-.326,-.26,z),.021,.12,'Steel',16)
box('Analog voltmeter bevel',(.15,-.277,.875),(.235,.035,.19),'Rubber',.013)
box('Analog voltmeter face',(.15,-.298,.875),(.188,.006,.137),'Paper',.002)
pointer=box('Voltmeter needle',(.16,-.305,.867),(.006,.005,.091),'Rubber',0)
pointer.rotation_euler.y=math.radians(-27)
cylinder('Reference rotary switch',(0,-.285,.505),.058,.045,'Rubber',24,(math.pi/2,0,0))
box('Rotary switch indicator',(0,-.313,.525),(.012,.01,.055),'Cream',.001)
box('Door pull',(.24,-.30,.29),(.032,.07,.18),'Steel',.009)
for z in (.19,.37):
    box('Door pull mount',(.24,-.273,z),(.059,.032,.025),'Steel',.003)
for i in range(7):
    box('Lower ventilation louver',(-.03,-.27,.15+i*.020),(.20,.018,.008),'Rubber',.002)
tube('Conduit tail',[(.34,.08,.17),(.44,.08,.17),(.46,.08,.10),(.46,.08,0)],.022,'Steel')

# All source origins and dimensions remain in metres; arrange collections only
# for an authoring overview without changing their exported local coordinates.
for c in collections:
    for o in c.objects:
        bpy.context.view_layer.objects.active=o
        bpy.ops.object.select_all(action='DESELECT')
        o.select_set(True)
        if o.type=='MESH':
            bpy.ops.object.mode_set(mode='EDIT')
            bpy.ops.mesh.select_all(action='SELECT')
            bpy.ops.mesh.normals_make_consistent(inside=False)
            bpy.ops.uv.smart_project(angle_limit=math.radians(66), island_margin=.015)
            bpy.ops.object.mode_set(mode='OBJECT')

source = ROOT / 'Blender/SIGNAL47_chapter09_lab.blend'
bpy.ops.wm.save_as_mainfile(filepath=str(source))
report={'blender':bpy.app.version_string,'units':'metres','axis_forward':'-Z','axis_up':'Y',
        'source':str(source.relative_to(ROOT)), 'original_authored_resources':True,'assets':[]}
for c in collections:
    bpy.ops.object.select_all(action='DESELECT')
    copies=[]
    for original in c.objects:
        o=original.copy()
        o.data=original.data.copy()
        bpy.context.scene.collection.objects.link(o)
        bpy.context.view_layer.objects.active=o
        o.select_set(True)
        for mod in list(o.modifiers):
            bpy.ops.object.modifier_apply(modifier=mod.name)
        copies.append(o)
        o.select_set(False)
    groups={}
    for o in copies:
        groups.setdefault(o.data.materials[0].name,[]).append(o)
    exported=[]
    for material,parts in groups.items():
        bpy.ops.object.select_all(action='DESELECT')
        for o in parts:o.select_set(True)
        bpy.context.view_layer.objects.active=parts[0]
        if len(parts)>1:
            bpy.ops.object.join()
        joined=bpy.context.object
        joined.name=c.name+'_'+material
        exported.append(joined)
    bpy.ops.object.select_all(action='DESELECT')
    for o in exported:o.select_set(True)
    dest=OUT/(c.name+'.fbx')
    bpy.ops.export_scene.fbx(filepath=str(dest),use_selection=True,object_types={'MESH'},
        add_leaf_bones=False,bake_anim=False,axis_forward='-Z',axis_up='Y',
        apply_unit_scale=True,use_mesh_modifiers=True,mesh_smooth_type='FACE')
    bbox=[o.matrix_world@Vector(corner) for o in exported for corner in o.bound_box]
    dimensions=[max(v[i] for v in bbox)-min(v[i] for v in bbox) for i in range(3)]
    triangles=sum(sum(len(p.vertices)-2 for p in o.data.polygons) for o in exported)
    report['assets'].append({'file':dest.name,'source_parts':len(c.objects),'export_renderers':len(exported),
                             'triangles':triangles,'blender_dimensions_m':dimensions,
                             'uv_layers':all(len(o.data.uv_layers)>0 for o in exported)})
    for o in exported:bpy.data.objects.remove(o,do_unlink=True)

(OUT/'blender-verification.json').write_text(json.dumps(report,indent=2)+'\n')
(OUT/'SOURCE_AND_LICENSE.md').write_text('''# Chapter 09 authored laboratory assets

Original geometry authored for SIGNAL / 47 with the reproducible Blender source
`Unity/Blender/Source/chapter09_lab_assets.py` and editable
`Unity/Blender/SIGNAL47_chapter09_lab.blend`. No external models, textures or
paid services were used for these meshes. Project ownership/license applies.

The FBX exports consolidate copies by material, preserve UVs and real bevels,
and use metric dimensions with -Z forward / Y up. Their source collections
retain separate editable components. See `blender-verification.json` for
executed Blender version, triangle counts, UV presence and bounds. A successful
Blender export does not itself certify Unity import or runtime appearance.

Scene materials, colliders, editable English labels and game interactions are
assigned by `Unity/Assets/Signal47/Editor/Chapter09Pass.cs`. Existing VT323 and
Poly Haven floor resources retain their original notices in the project.
''')
print('CHAPTER09_BLENDER_EXPORT '+json.dumps(report))
