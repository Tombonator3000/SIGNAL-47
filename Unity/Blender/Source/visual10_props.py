"""Original SIGNAL / 47 visual pass 10 laboratory and field props, in metres.

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
def correct_dish_bowl():
    """Reverse original bowl winding; retain vertices, pivot and supports upstream.

    The prototype is y=(x*x+z*z)/12 in imported mesh-local coordinates.
    Its winding originally points away from its feed/concave side. Only this
    new bowl-only export is corrected; old FBX and feed/supports are untouched.
    """
    source=ROOT/'Blender/SIGNAL47_visual10_props.blend'
    bpy.ops.wm.open_mainfile(filepath=str(source))
    old_collection=bpy.data.collections.get('V10_DishBowlCorrected')
    if old_collection:
        for ob in list(old_collection.objects):bpy.data.objects.remove(ob,do_unlink=True)
        bpy.data.collections.remove(old_collection)
    bpy.ops.object.select_all(action='DESELECT')
    bpy.ops.import_scene.fbx(filepath=str(ROOT/'Assets/Signal47/Art/Refined/SM_RadioDish_Bowl_A.fbx'))
    imported=list(bpy.context.selected_objects)
    bowl=next(o for o in imported if o.name.endswith('__DishBowl'))
    for ob in imported:
        if ob!=bowl:bpy.data.objects.remove(ob,do_unlink=True)
    mesh=bowl.data
    before=[tuple(v.co) for v in mesh.vertices]
    old_normal_bounds=[[min(p.normal[i] for p in mesh.polygons),max(p.normal[i] for p in mesh.polygons)] for i in range(3)]
    mesh.flip_normals()
    mesh.update()
    # Analytic smooth normals face the concavity without weighted-normal bias.
    mesh.normals_split_custom_set_from_vertices([Vector((-v.co.x/6,1,-v.co.z/6)).normalized() for v in mesh.vertices])
    for face in mesh.polygons:face.use_smooth=True
    if not mesh.uv_layers:
        uv=mesh.uv_layers.new(name='DishPlanarUV')
        for poly in mesh.polygons:
            for li in poly.loop_indices:
                co=mesh.vertices[mesh.loops[li].vertex_index].co
                uv.data[li].uv=(co.x/8.4+.5,co.z/8.4+.5)
    coll=bpy.data.collections.new('V10_DishBowlCorrected');bpy.context.scene.collection.children.link(coll)
    for c in list(bowl.users_collection):c.objects.unlink(bowl)
    coll.objects.link(bowl)
    report={'source':'Assets/Signal47/Art/Refined/SM_RadioDish_Bowl_A.fbx',
            'geometry_equation_mesh_local':'y = (x*x + z*z) / 12',
            'radius_m':4.2,'depth_m':1.47,'vertices':len(mesh.vertices),'polygons':len(mesh.polygons),
            'original_polygon_normal_bounds':old_normal_bounds,
            'corrected_polygon_normal_bounds':[[min(p.normal[i] for p in mesh.polygons),max(p.normal[i] for p in mesh.polygons)] for i in range(3)],
            'analytic_custom_normals':'normalize(-x/6, 1, -z/6)',
            'vertex_positions_unchanged':before==[tuple(v.co) for v in mesh.vertices],
            'matrix_world':[list(row) for row in bowl.matrix_world],
            'support_meshes':'not exported; retain original Feed and FeedSupport_0/1/2 in Unity',
            'scope':'Bowl only, opposite winding. Runtime appearance and Unity import require verification.'}
    bpy.context.preferences.filepaths.save_version=0
    bpy.ops.wm.save_as_mainfile(filepath=str(source))
    bpy.ops.object.select_all(action='DESELECT');bowl.select_set(True);bpy.context.view_layer.objects.active=bowl
    dest=OUT/'V10_RadioDish_BowlCorrected.fbx'
    bpy.ops.export_scene.fbx(filepath=str(dest),use_selection=True,object_types={'MESH'},add_leaf_bones=False,bake_anim=False,axis_forward='-Z',axis_up='Y',apply_unit_scale=True,use_mesh_modifiers=True,mesh_smooth_type='FACE')
    original_world=[bowl.matrix_world@v.co for v in mesh.vertices]
    bpy.data.objects.remove(bowl,do_unlink=True)
    bpy.ops.import_scene.fbx(filepath=str(dest))
    restored=next(o for o in bpy.context.selected_objects if o.type=='MESH')
    restored_world=[restored.matrix_world@v.co for v in restored.data.vertices]
    # Vertices may be reordered by FBX; compare bounds, count and parabola.
    obounds=[[min(v[i] for v in original_world),max(v[i] for v in original_world)] for i in range(3)]
    rbounds=[[min(v[i] for v in restored_world),max(v[i] for v in restored_world)] for i in range(3)]
    report['fbx_roundtrip_bounds_pass']=all(abs(a-b)<.00001 for x,y in zip(obounds,rbounds) for a,b in zip(x,y))
    report['fbx_roundtrip_positive_concave_normal_pass']=all(p.normal.y>0 for p in restored.data.polygons)
    report['fbx_roundtrip_uv_pass']=bool(restored.data.uv_layers)
    assert report['vertex_positions_unchanged'] and report['fbx_roundtrip_bounds_pass'] and report['fbx_roundtrip_positive_concave_normal_pass']
    (OUT/'dish-normal-verification.json').write_text(json.dumps(report,indent=2)+'\n')
    notice=OUT/'SOURCE_AND_LICENSE.md'
    extra='\n## Corrected dish surface\n\nV10_RadioDish_BowlCorrected.fbx is derived from the existing project-owned Refined/SM_RadioDish_Bowl_A.fbx bowl. Only face winding and smooth normals are corrected; vertex positions, radius, depth and pivot are preserved. Feed/support meshes remain in the original asset and must be preserved during integration. Reproduce independently with visual10_props.py -- --dish-only. The original assets are untouched. See dish-normal-verification.json for measured geometry and FBX roundtrip checks.\n'
    if notice.exists() and '## Corrected dish surface' not in notice.read_text():
        notice.write_text(notice.read_text()+extra)
    print('VISUAL10_DISH_NORMALS '+json.dumps(report))

import sys
if '--dish-only' in sys.argv:
    correct_dish_bowl()
    sys.exit(0)

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


# Laboratory bench: origin floor centre, long axis Y, accessible drawer faces +X.
# 1.16 wide x 4.35 long x .96 tall. Back edge is -X; no deep backboard
# makes placement under existing props and against either room wall predictable.
group('V10_LabBench')
box('Continuous rolled linoleum worktop', (0,0,.935), (1.16,4.35,.05), 'Cream', .012)
box('Dark worktop edging', (0,0,.900), (1.145,4.335,.020), 'Rubber', .004)
for y in (-1.67,1.67):
    box('Drawer pedestal carcass',(-.07,y,.47),(.94,.76,.86),'Green',.008)
    box('Recessed pedestal toe space',(-.05,y,.055),(.86,.69,.10),'Rubber',.003)
    for z,h in ((.76,.19),(.53,.245),(.265,.245)):
        box('Separate enamel drawer front',(.415,y,z),(.035,.72,h),'Cream',.006)
        box('Drawer shadow seam',(.392,y,z),(.009,.738,h+.012),'Rubber',.001)
        # Pull mounted horizontally along the length direction of the bench.
        tube('Pressed drawer pull',[(.445,y-.12,z+.026),(.484,y-.12,z+.026),(.49,y+.12,z+.026),(.445,y+.12,z+.026)],.008,'Steel')
        box('Drawer label holder',(.440,y,z-.048),(.009,.112,.035),'Steel',.002)
        box('Drawer index card',(.446,y,z-.048),(.003,.096,.021),'Cream',.001)
for y in (-2.07,0,2.07):
    for x in (-.47,.47):
        box('Square section frame leg',(x,y,.44),(.038,.038,.87),'Steel',.003)
        box('Adjustable bench foot',(x,y,.018),(.065,.065,.036),'Rubber',.004)
for x in (-.47,.47):
    box('Long structural rail',(x,0,.84),(.038,4.20,.10),'Green',.004)
box('Rear foot rail',(-.47,0,.20),(.032,4.18,.032),'Steel',.003)
# A lower shelf in the open centre stays visually useful but does not fill knee space.
box('Centre low equipment shelf',(-.13,0,.21),(.65,2.41,.026),'Green',.005)

# Open lathed reflectors: the shell returns up its inner surface, so it is
# visible from below and does not incorrectly cap the light aperture.
def shade(name, rings, material):
    o=profile(name,rings,material,32)
    # Remove profile end caps: a reflector must have an open light aperture.
    mesh=o.data
    vertices=[tuple(v.co) for v in mesh.vertices]
    faces=[tuple(p.vertices) for p in list(mesh.polygons)[:-2]]
    mesh.clear_geometry();mesh.from_pydata(vertices,[],faces);mesh.update()
    for p in mesh.polygons:p.use_smooth=True
    return o

group('V10_WallLamp')
# origin at wall rose centre, wall plane Y=0; lamp extends toward Blender -Y.
cylinder('Wall mounting rose',(0,0,0),.085,.028,'Green',32,(math.pi/2,0,0))
tube('Gooseneck conduit',[(0,-.025,0),(0,-.08,0),(0,-.11,.04),(0,-.11,.14),(0,-.16,.20),(0,-.28,.20),(0,-.34,.15)],.014,'Steel')
lamp=shade('Open enamel bell reflector',[(0,.158),(.008,.163),(.018,.155),(.065,.131),(.12,.085),(.145,.044),(.16,.039),(.16,.029),(.135,.034),(.113,.075),(.056,.121),(.012,.145),(0,.148)],'Green')
lamp.location=(0,-.34,-.012)
inner=shade('Cream reflective bell interior',[(.016,.143),(.055,.120),(.108,.074),(.132,.032)],'Cream')
inner.location=lamp.location
cylinder('Bulb ceramic socket',(0,-.34,.104),.028,.069,'Cream',20)
bpy.ops.mesh.primitive_uv_sphere_add(segments=20,ring_count=12,radius=.036,location=(0,-.34,.047))
bulb=finish(bpy.context.object,'Incandescent bulb envelope','Cream')
for x in (-.052,.052):
    cylinder('Wall rose screw',(x,-.017,0),.006,.005,'Steel',12,(math.pi/2,0,0))

group('V10_FieldLampShade')
# origin at reflector aperture centre. Axis vertical; illumination points down.
shade('Field enamel reflector',[(0,.20),(.008,.208),(.017,.201),(.075,.173),(.15,.11),(.205,.048),(.223,.044),(.223,.032),(.20,.037),(.146,.10),(.07,.160),(.014,.19),(0,.19)],'Green')
shade('Pale reflective liner',[(.02,.187),(.069,.158),(.142,.098),(.195,.035)],'Cream')
cylinder('Weatherproof lamp neck',(0,0,.252),.046,.060,'Steel')
cylinder('Porcelain socket',(0,0,.16),.031,.08,'Cream',20)
bpy.ops.mesh.primitive_uv_sphere_add(segments=20,ring_count=12,radius=.047,location=(0,0,.101))
finish(bpy.context.object,'Field bulb envelope','Cream')
# Cast service cage leaves the warm bulb visible without increasing light count.
for a in (0,math.pi/2,math.pi,3*math.pi/2):
    ca,sa=math.cos(a),math.sin(a)
    tube('Protective curved cage',[(ca*.18,sa*.18,.008),(ca*.14,sa*.14,-.07),(ca*.065,sa*.065,-.10),(0,0,-.10)],.004,'Steel')

group('V10_MetalBin')
# Floor-centred galvanized laboratory bin with rolled rim, ribs and domed lid.
profile('Tapered galvanized bin',[(.012,.175),(.025,.185),(.43,.202),(.455,.211),(.469,.211),(.48,.203),(.48,.192),(.45,.190),(.04,.173)],'Steel',40)
for i in range(24):
    a=i*math.tau/24
    tube('Pressed vertical strengthening rib',[(math.cos(a)*.185,math.sin(a)*.185,.06),(math.cos(a)*.202,math.sin(a)*.202,.43)],.006,'Steel')
profile('Domed removable lid',[(.481,.212),(.491,.215),(.503,.199),(.525,.157),(.535,.07),(.535,0)],'Steel',40)
tube('Lid grip',[(-.053,0,.536),(-.053,0,.577),(.053,0,.577),(.053,0,.536)],.009,'Rubber')
for x in (-.211,.211):
    tube('Side carrying handle',[(x,-.045,.395),(x*1.12,-.045,.355),(x*1.12,.045,.355),(x,.045,.395)],.008,'Steel')

# Drape model built as a continuous folded ribbon across sink rim; origin
# is rim centre, X is cloth width, +Y back toward basin, -Y hanging outside.
group('V10_SinkCloth')
verts=[];faces=[]
rows=25;cols=15
for j in range(rows):
    t=j/(rows-1)
    # Back half lies over the rim; the outer section hangs down .32 m.
    if t<.36:
        y=.15-t/.36*.16; z=.012
    else:
        q=(t-.36)/.64; y=-.01-.06*math.sin(q*math.pi/2);z=.012-.32*q
    for i in range(cols):
        u=i/(cols-1)
        fold=math.sin(u*math.tau*3.5)*.009 + math.sin(u*math.tau*7)*.003
        verts.append(((u-.5)*.32,y+fold*t,z+fold*(1-t)+.008*math.sin(u*math.pi)*t))
for j in range(rows-1):
    for i in range(cols-1):
        k=j*cols+i;faces.append((k,k+1,k+1+cols,k+cols))
mesh=bpy.data.meshes.new('Woven draped cleaning cloth');mesh.from_pydata(verts,[],faces);mesh.update()
o=bpy.data.objects.new('Woven draped cleaning cloth',mesh);bpy.context.collection.objects.link(o)
finish(o,o.name,'Cream')
for p in o.data.polygons:p.use_smooth=True
solid=o.modifiers.new('Cloth thickness','SOLIDIFY');solid.thickness=.002
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

source = ROOT / 'Blender/SIGNAL47_visual10_props.blend'
bpy.context.preferences.filepaths.save_version = 0
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
(OUT/'SOURCE_AND_LICENSE.md').write_text('''# Visual 10 authored laboratory and field assets

Original geometry authored for SIGNAL / 47 with the reproducible Blender source
`Unity/Blender/Source/visual10_props.py` and editable
`Unity/Blender/SIGNAL47_visual10_props.blend`. No external models, textures or
paid services were used for these meshes. Project ownership/license applies.

The FBX exports consolidate copies by material, preserve UVs and real bevels,
and use metric dimensions with -Z forward / Y up. Their source collections
retain separate editable components. See `blender-verification.json` for
executed Blender version, triangle counts, UV presence and bounds. A successful
Blender export does not itself certify Unity import or runtime appearance.

Scene materials, colliders, lighting and placement are assigned by the Visual10 integration. Lamp bulbs use CH09_Cream as a remapping slot; actual emitted light is supplied in Unity. Bench drawers face Blender +X, long axis Blender Y, floor origin Z=0. Wall lamp extends Blender -Y from wall rose origin. Field shade illuminates Blender -Z from aperture origin. Cloth origin is sink rim; drape falls on Blender -Y side. Existing VT323 and
Poly Haven floor resources retain their original notices in the project.
''')
print('VISUAL10_BLENDER_EXPORT '+json.dumps(report))

# Round-trip FBX validation is separate from Unity runtime/import verification.
for c in list(bpy.data.objects): bpy.data.objects.remove(c,do_unlink=True)
for asset in report['assets']:
    for material in list(bpy.data.materials): bpy.data.materials.remove(material,do_unlink=True)
    bpy.ops.import_scene.fbx(filepath=str(OUT/asset['file']))
    imported=list(bpy.context.selected_objects)
    meshes=[o for o in imported if o.type=='MESH']
    bbox=[o.matrix_world@Vector(c) for o in meshes for c in o.bound_box]
    bounds=[max(v[i] for v in bbox)-min(v[i] for v in bbox) for i in range(3)]
    asset['fbx_roundtrip_dimensions_m']=bounds
    asset['fbx_roundtrip_scale_pass']=all(abs(a-b)<.0001 for a,b in zip(bounds,asset['blender_dimensions_m']))
    asset['fbx_roundtrip_uv_pass']=all(len(o.data.uv_layers)>0 for o in meshes)
    asset['fbx_roundtrip_material_names']=sorted(set(m.name for o in meshes for m in o.data.materials))
    for o in imported:bpy.data.objects.remove(o,do_unlink=True)
(OUT/'blender-verification.json').write_text(json.dumps(report,indent=2)+'\n')
assert all(a['fbx_roundtrip_scale_pass'] and a['fbx_roundtrip_uv_pass'] for a in report['assets'])
print('VISUAL10_ROUNDTRIP_PASS')

# Keep the authored source and corrected bowl reproducible in a full rebuild.
correct_dish_bowl()
