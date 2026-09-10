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
OUT = ROOT / 'Assets/Signal47/Art/Visual10/Models'
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
    'WarmDiffuser': ((.88, .85, .73, 1), .28, .0),
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


group('V10_TwinFluorescent')
# Lower tube surface at Z=0, ceiling backplate at Z=.10; long axis Blender Y.
box('Folded enamel back tray',(0,0,.092),(.25,.95,.016),'Cream',.004)
for x in (-.1175,.1175):
    box('Rolled side flange',(x,0,.063),(.015,.95,.070),'Steel',.003)
    # Angled bright reflector separates metal housing from luminous tubes.
    b=box('Enamel angled reflector',(x*.78,0,.058),(.072,.858,.008),'Cream',.003)
    b.rotation_euler.y=math.copysign(math.radians(24),x)
for y in (-.45,.45):
    box('Stamped end cap',(0,y,.057),(.22,.05,.078),'Cream',.006)
    for x in (-.06,.06):
        box('Porcelain tube holder',(x,y*.947,.033),(.042,.027,.048),'Cream',.005)
        cylinder('Metal pin ferrule',(x,y*.928,.020),.018,.020,'Steel',20,(math.pi/2,0,0))
        for offset in (-.005,.005):
            cylinder('Tube contact pin',(x+offset,y*.89,.020),.0015,.01,'Steel',8,(math.pi/2,0,0))
for x in (-.06,.06):
    cylinder('Separate fluorescent glass tube',(x,0,.016),.016,.816,'WarmDiffuser',32,(math.pi/2,0,0))
box('Central ballast cover',(0,0,.074),(.044,.73,.033),'Cream',.004)
for y in (-.39,.39):
    cylinder('Service cover screw',(0,y,.052),.005,.006,'Steel',12)

# Open plastic measuring jug, one litre scale, worktop centred at Z=0.
group('V10_MeasuringJug')
jug=profile('Open moulded measuring jug',[(0,.061),(.007,.067),(.145,.079),(.16,.08),(.163,.076),(.153,.073),(.018,.061)],'Cream',40)
# Local front rim forms an actual pouring spout, not a painted triangle.
for v in jug.data.vertices:
    if v.co.y<-.052 and v.co.z>.135:
        strength=max(0,(-v.co.y-.052)/.028)
        v.co.y-=.021*strength;v.co.z+=.006*strength
jug.data.update()
tube('Jug moulded loop handle',[(0,.072,.137),(0,.112,.144),(0,.127,.128),(0,.125,.047),(0,.101,.031),(0,.067,.040)],.010,'Cream')
for i in range(7):
    z=.034+i*.017
    box('Raised measuring graduation',(.070+i*.0015,-.024,z),(.002,.018 if i%2 else .027,.0025),'Rubber',.0005)

group('V10_ChemicalFunnel')
funnel=profile('Open chemical transfer funnel',[(0,.008),(.044,.009),(.111,.059),(.128,.066),(.134,.066),(.134,.060),(.118,.056),(.043,.0045),(0,.0045)],'Cream',32)
# Remove end discs so both funnel mouth and outlet are open.
m=funnel.data;v=[tuple(v.co) for v in m.vertices];f=[tuple(p.vertices) for p in list(m.polygons)[:-2]]
m.clear_geometry();m.from_pydata(v,[],f);m.update()
for p in m.polygons:p.use_smooth=True
box('Funnel hanging tab',(.072,0,.124),(.025,.016,.010),'Cream',.004)

# Flat tongs with spring hinge and rubber gripping tips. Worktop floor origin.
group('V10_PrintTongs')
for sign in (-1,1):
    tube('Stainless sprung tong arm',[(sign*.007,.115,.015),(sign*.009,.08,.013),(sign*.025,-.05,.020),(sign*.033,-.109,.011)],.004,'Steel')
    tip=box('Soft photo gripping pad',(sign*.033,-.112,.012),(.022,.04,.020),'Rubber',.003)
    tip.rotation_euler.z=-sign*.1
cylinder('Rolled spring hinge',(0,.112,.015),.012,.017,'Steel',20,(0,math.pi/2,0))
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

source = ROOT / 'Blender/SIGNAL47_visual10_lab_details.blend'
bpy.context.preferences.filepaths.save_version=0
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

(OUT/'lab-details-verification.json').write_text(json.dumps(report,indent=2)+'\n')

for o in list(bpy.data.objects):bpy.data.objects.remove(o,do_unlink=True)
for a in report['assets']:
    bpy.ops.import_scene.fbx(filepath=str(OUT/a['file']))
    imported=list(bpy.context.selected_objects)
    points=[o.matrix_world@Vector(c) for o in imported if o.type=='MESH' for c in o.bound_box]
    dims=[max(v[i] for v in points)-min(v[i] for v in points) for i in range(3)]
    a['roundtrip_dimensions_m']=dims
    a['roundtrip_bounds_pass']=all(abs(x-y)<.00001 for x,y in zip(dims,a['blender_dimensions_m']))
    a['roundtrip_uv_pass']=all(o.data.uv_layers for o in imported if o.type=='MESH')
    a['roundtrip_unit_normals_pass']=all(abs(p.normal.length-1)<1e-4 for o in imported if o.type=='MESH' for p in o.data.polygons)
    assert a['roundtrip_bounds_pass'] and a['roundtrip_uv_pass'] and a['roundtrip_unit_normals_pass']
    for o in imported:bpy.data.objects.remove(o,do_unlink=True)
(OUT/'lab-details-verification.json').write_text(json.dumps(report,indent=2)+'\n')
(OUT/'LAB_DETAILS_SOURCE_AND_LICENSE.md').write_text("""# Visual 10 laboratory detail meshes

Original geometry authored for SIGNAL / 47. No external assets or paid services.
Rebuild with Unity/Blender/Source/visual10_lab_details.py. Editable source:
Unity/Blender/SIGNAL47_visual10_lab_details.blend. Project ownership applies.

V10_TwinFluorescent has two individual glass tubes, separate sockets, enamel
reflectors and metal frame. Map CH09_WarmDiffuser only to low warm emission;
do not make housing glow. Fixture nominal bounds .25 x .95 x .10 metres in
Blender XYZ; long Y becomes Unity Z. Tube underside origin is at Blender Z=0.
Use a separate Unity light source; the FBX itself contains no realtime light.
Other objects have worktop origin, Blender Z-up, -Z forward/Y up FBX export.
Material slots use CH09_Cream, CH09_Steel, CH09_Rubber, CH09_WarmDiffuser.
Measuring jug/funnel contain actual open interiors; photo tongs have two
spring arms and separate gripping pads. See lab-details-verification.json
for measured exported dimensions, triangles, UV and reimport checks.
Unity integration, final materials and runtime appearance remain to verify.
""")
print('V10_LAB_DETAILS '+json.dumps(report))
