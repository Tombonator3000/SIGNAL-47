"""Original 1986 SARO photo-lab detail assemblies, Blender 5.2.
Authoring only: run under the project's heavy-job guard. No Unreal edits.
Helpers accept game (x,height,z); Blender stores (x,-z,height).
The ten exports are additive. Existing story objects and physical lights remain.
"""
from pathlib import Path
import json
import math
import sys
import bpy
import bmesh
import numpy as np
from mathutils import Vector

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT))
from surface_fields import surface_channels, normal_map
from detail_pass import append_detail
OUT, TEX, PRE = (ROOT / p for p in ('Export', 'textures', 'Preview'))
for p in (OUT, TEX, PRE):
    p.mkdir(parents=True, exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
SC = bpy.context.scene
SC.unit_settings.system = 'METRIC'
SC.unit_settings.scale_length = 1
SC.render.engine = 'CYCLES'
SC.cycles.device = 'CPU'
SC.cycles.samples = 32
SC.render.threads_mode = 'FIXED'
SC.render.threads = 2
SC.render.resolution_x, SC.render.resolution_y = 1600, 1000
SC.render.resolution_percentage = 100
SC.view_settings.view_transform = 'AgX'
SC.world = bpy.data.worlds.new('PhotoLab_ReviewWorld')
SC.world.color = (.025, .025, .025)

def collection(name):
    c = bpy.data.collections.new(name)
    SC.collection.children.link(c)
    return c

EDIT = collection('01_EDITABLE_ORIGINAL_DETAIL')
EXPORT = collection('02_NATIVE_EXPORTS_HIDDEN')
CONTEXT = collection('03_EXISTING_LAYOUT_PREVIEW_ONLY')
STUDIO = collection('04_REVIEW_CAMERAS_LIGHTS_ONLY')
CURRENT, PARTS, MATS, ROWS = [], {}, {}, []
N = 1024
rng = np.random.default_rng(471027)
v, u = np.mgrid[0:N, 0:N] / N
noise = rng.normal(0, 1, (N, N))

# Original procedural PBR, following the existing Operations office recipe.
def image(name, pixels):
    im = bpy.data.images.new(name, N, N, alpha=True)
    # Albedo is already sRGB-encoded; data maps are already linear. Saving the
    # arrays as Non-Color prevents an unintended second colour transform.
    im.colorspace_settings.name = 'Non-Color'
    im.pixels.foreach_set(pixels.astype(np.float32).ravel())
    im.filepath_raw = str(TEX / (name + '.png'))
    im.file_format = 'PNG'
    im.save()
    bpy.data.images.remove(im)
    return bpy.data.images.load(str(TEX / (name + '.png')), check_existing=True)

def material(name, color, rough=.65, metal=0, kind='paint'):
    name = 'M_PhotoLab_' + name
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    bsdf = m.node_tree.nodes.get('Principled BSDF')
    color_field, roughness, height = surface_channels(kind, u, v, noise, rough)
    rgba = np.ones((N, N, 4))
    linear = np.clip(np.array(color)[None, None, :] * color_field[:, :, None], .001, 1)
    rgba[:, :, :3] = np.where(linear < .0031308, linear * 12.92, 1.055 * linear ** (1 / 2.4) - .055)
    base = image(name + '_base', rgba)
    base.colorspace_settings.name = 'sRGB'
    rgba[:, :, :3] = roughness[:, :, None]
    rim = image(name + '_rough', rgba)
    rim.colorspace_settings.name = 'Non-Color'
    nim = image(name + '_normal_dx', normal_map(height))
    nim.colorspace_settings.name = 'Non-Color'
    bsdf.inputs['Metallic'].default_value = metal
    bsdf.inputs['Roughness'].default_value = rough
    for im, socket in ((base, 'Base Color'), (rim, 'Roughness')):
        tex = m.node_tree.nodes.new('ShaderNodeTexImage')
        tex.image = im
        m.node_tree.links.new(tex.outputs['Color'], bsdf.inputs[socket])
    # Native delivery uses DirectX normals. Blender previews invert G only in
    # their node graph, following the existing Office material/export recipe.
    nodes, links = m.node_tree.nodes, m.node_tree.links
    tex = nodes.new('ShaderNodeTexImage'); tex.image = nim
    separate = nodes.new('ShaderNodeSeparateColor'); links.new(tex.outputs['Color'], separate.inputs['Color'])
    inv = nodes.new('ShaderNodeMath'); inv.operation = 'SUBTRACT'; inv.inputs[0].default_value = 1
    links.new(separate.outputs['Green'], inv.inputs[1])
    combine = nodes.new('ShaderNodeCombineColor')
    links.new(separate.outputs['Red'], combine.inputs['Red']); links.new(inv.outputs[0], combine.inputs['Green'])
    links.new(separate.outputs['Blue'], combine.inputs['Blue'])
    normal = nodes.new('ShaderNodeNormalMap'); links.new(combine.outputs[0], normal.inputs['Color'])
    links.new(normal.outputs['Normal'], bsdf.inputs['Normal'])
    ROWS.append({'name': name, 'base_color_linear': [1, 1, 1, 1], 'roughness': rough,
                 'metallic': metal, 'specular': .33, 'normal_strength': 1.,
                 'normal_convention': 'DirectX', 'surface_kind': kind, 'uv_span_m': 1.,
                 'textures': {'base_color': 'textures/' + name + '_base.png',
                              'roughness': 'textures/' + name + '_rough.png',
                              'normal': 'textures/' + name + '_normal_dx.png'}})
    MATS[name] = m
    return m

ENAMEL = material('AgedGreyGreen', (.18, .23, .21), .58)
CREAM = material('CreamEnamel', (.51, .49, .39), .59)
STEEL = material('BrushedStainless', (.38, .41, .39), .32, .86, 'metal')
DARK = material('Bakelite', (.019, .023, .021), .48, kind='bakelite')
RUBBER = material('Rubber', (.012, .014, .012), .86, kind='rubber')
PAPER = material('Manila', (.57, .49, .33), .94, kind='paper')
WHITE = material('DialIvory', (.69, .66, .54), .83)
BROWN = material('AmberPlastic', (.22, .072, .018), .34, kind='plastic')
BLUE = material('BlueCap', (.023, .061, .13), .49, kind='plastic')
RED = material('OxideHandle', (.24, .045, .025), .53, kind='plastic')

def xyz(p):
    return Vector((p[0], -p[2], p[1]))

def active(o):
    bpy.ops.object.select_all(action='DESELECT')
    o.select_set(True)
    bpy.context.view_layer.objects.active = o

def put(o, name, mat):
    o.name = name
    for c in list(o.users_collection):
        c.objects.unlink(o)
    EDIT.objects.link(o)
    o.data.materials.append(mat)
    CURRENT.append(o)
    return o

def apply(o, mod):
    active(o)
    bpy.ops.object.modifier_apply(modifier=mod.name)

def box(name, at, size, mat, bevel=.004):
    bpy.ops.mesh.primitive_cube_add(size=1, location=xyz(at))
    o = put(bpy.context.object, name, mat)
    o.dimensions = (size[0], size[2], size[1])
    active(o)
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    if bevel:
        mod = o.modifiers.new('Folded_and_rounded_edges', 'BEVEL')
        mod.width = min(bevel, min(size) * .22)
        mod.segments = 2
        apply(o, mod)
        mod = o.modifiers.new('Weighted_normals', 'WEIGHTED_NORMAL')
        mod.keep_sharp = True
        apply(o, mod)
    return o

def cyl(name, a, b, r, mat, sides=16):
    a, b = xyz(a), xyz(b)
    d = b - a
    bpy.ops.mesh.primitive_cylinder_add(vertices=sides, radius=r, depth=d.length, location=(a + b) / 2)
    o = put(bpy.context.object, name, mat)
    o.rotation_euler = d.to_track_quat('Z', 'Y').to_euler()
    active(o)
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    for poly in o.data.polygons:
        poly.use_smooth = len(poly.vertices) == 4
    return o

def pipe(name, points, radius, mat):
    c = bpy.data.curves.new(name, 'CURVE')
    c.dimensions = '3D'
    c.resolution_u = 5
    c.bevel_depth = radius
    c.bevel_resolution = 1
    sp = c.splines.new('BEZIER')
    sp.bezier_points.add(len(points) - 1)
    for bp, p in zip(sp.bezier_points, points):
        bp.co = xyz(p)
        bp.handle_left_type = bp.handle_right_type = 'AUTO'
    o = bpy.data.objects.new(name, c)
    EDIT.objects.link(o)
    o.data.materials.append(mat)
    active(o)
    bpy.ops.object.convert(target='MESH')
    CURRENT.append(o)
    return o

def ring(name, center, ro, ri, depth, mat, axis='Y', sides=32):
    # A genuine annulus with an open center, never a cylinder cap across a light aperture.
    at = Vector(center)
    verts, faces = [], []
    for axial, radius in ((-depth / 2, ro), (depth / 2, ro), (-depth / 2, ri), (depth / 2, ri)):
        for i in range(sides):
            a = i * math.tau / sides
            q = {'Y': (radius * math.cos(a), axial, radius * math.sin(a)),
                 'Z': (radius * math.cos(a), radius * math.sin(a), axial),
                 'X': (axial, radius * math.cos(a), radius * math.sin(a))}[axis]
            verts.append(xyz(at + Vector(q)))
    for i in range(sides):
        j = (i + 1) % sides
        faces.extend([(i, j, sides + j, sides + i), (2*sides+i, 3*sides+i, 3*sides+j, 2*sides+j),
                      (i, 2*sides+i, 2*sides+j, j), (sides+i, sides+j, 3*sides+j, 3*sides+i)])
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(verts, [], faces)
    mesh.update()
    return put(bpy.data.objects.new(name, mesh), name, mat)

def screw(name, x, y, z):
    cyl(name, (x, y, z), (x, y, z+.003), .005, STEEL, 12)
    box(name+'_slot', (x, y, z+.004), (.006, .0012, .001), DARK, 0)

def finish(name, purpose, collision='existing', protect=()):
    append_detail(name, box, cyl, ring, pipe,
                  {key.removeprefix('M_PhotoLab_'): value for key, value in MATS.items()})
    objs = list(CURRENT)
    CURRENT.clear()
    assert objs, name
    for o in objs:
        active(o)
        bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
        o['detail_group'] = name
    bpy.ops.object.select_all(action='DESELECT')
    for o in objs:
        o.select_set(True)
    bpy.context.view_layer.objects.active = objs[0]
    bpy.ops.object.duplicate()
    copies = list(bpy.context.selected_objects)
    bpy.context.view_layer.objects.active = copies[0]
    bpy.ops.object.join()
    o = bpy.context.object
    o.name = name
    for c in list(o.users_collection):
        c.objects.unlink(o)
    EXPORT.objects.link(o)
    active(o)
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.cube_project(cube_size=1)
    bpy.ops.object.mode_set(mode='OBJECT')
    bm = bmesh.new()
    bm.from_mesh(o.data)
    bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
    bm.to_mesh(o.data)
    bm.free()
    mod = o.modifiers.new('Triangulated_delivery', 'TRIANGULATE')
    apply(o, mod)
    bpy.ops.export_scene.fbx(filepath=str(OUT/(name+'.fbx')), use_selection=True,
        object_types={'MESH'}, apply_unit_scale=True, apply_scale_options='FBX_SCALE_UNITS',
        axis_forward='-Y', axis_up='Z', bake_anim=False, mesh_smooth_type='FACE')
    bounds = [[float(fn(v.co[k] for v in o.data.vertices)) for k in range(3)] for fn in (min, max)]
    PARTS[name] = {'triangles':len(o.data.polygons), 'bounds_local_m':bounds,
        'runtime_origin_three_m':[0,0,0], 'materials':[m.name for m in o.data.materials],
        'original_components':len(objs), 'purpose':purpose, 'collision':collision,
        'protected_originals':list(protect), 'integration':'additive; never hide original parent nodes'}
    o.hide_render = True
    o.hide_set(True)

# 1. Cabinet fronts overlay the original solid wet cabinet, below the existing towel.
for i in range(6):
    x = 13.45 + i * .70
    box('Cabinet_reveal', (x,.43,-5.125), (.69,.75,.018), DARK)
    box('Recessed_enamel_door', (x,.43,-5.111), (.668,.728,.017), ENAMEL)
    box('Inset_door_field', (x,.42,-5.099), (.60,.63,.008), ENAMEL, .008)
    for y in (.17,.69):
        cyl('Hinge', (x-.318,y-.025,-5.085), (x-.318,y+.025,-5.085), .007, STEEL)
    for hx in (x-.045,x+.045):
        cyl('Pull_post', (hx,.73,-5.098), (hx,.73,-5.068), .007, STEEL)
    cyl('Rounded_pull', (x-.045,.73,-5.068), (x+.045,.73,-5.068), .007, STEEL)
box('Toe_kick', (15.2,.055,-5.107), (4.18,.10,.014), RUBBER)
box('Worktop_rolled_front', (15.2,.905,-5.091), (4.22,.065,.024), STEEL)
finish('SM_PhotoLab_WetCabinetDetail', 'Six recessed cabinet doors, folded edge, pulls and hinges.', protect=['wetPrint','wetBench','existing towel'])

# 2. New freestanding rinse sink occupies the already-collided chemistry corner.
# Existing jugs stay underneath. Basin is four sloped walls + a low base, with no top cap.
x, z = 17.78, -5.44
box('Sink_bottom', (x,.715,z), (.47,.025,.32), STEEL, .018)
for sx in (-1,1):
    o=box('Sink_side', (x+sx*.265,.81,z), (.027,.195,.40), STEEL)
    o.rotation_euler.y = -sx*.15
for sz in (-1,1):
    o=box('Sink_end', (x,.81,z+sz*.19), (.55,.195,.027), STEEL)
    o.rotation_euler.x = sz*.14
for sx in (-1,1):
    box('Sink_rolled_side', (x+sx*.30,.916,z), (.05,.035,.49), STEEL, .012)
for sz in (-1,1):
    box('Sink_rolled_end', (x,.916,z+sz*.22), (.56,.035,.05), STEEL, .012)
box('Splashback', (x,1.005,-5.735), (.66,.21,.022), STEEL)
for sx in (-1,1):
    for sz in (-1,1):
        at=(x+sx*.32,z+sz*.22)
        cyl('Sink_leg',(at[0],.035,at[1]),(at[0],.89,at[1]),.018,STEEL)
        cyl('Adjustable_foot',(at[0],.003,at[1]),(at[0],.047,at[1]),.024,RUBBER)
ring('Drain_rim',(x,.732,z),.035,.022,.007,STEEL)
for off in (-.015,0,.015):box('Drain_grate',(x+off,.733,z),(.004,.004,.042),DARK,0)
pipe('Drain_trap',[(x,.70,z),(x,.49,z),(x+.11,.43,z),(x+.17,.53,z),(x+.17,.58,-5.76)],.022,STEEL)
pipe('Gooseneck',[(x,.935,-5.69),(x,1.15,-5.69),(x,1.205,-5.54),(x,1.14,-5.46)],.013,STEEL)
for dx,mat in ((-.13,RED),(.13,BLUE)):
    cyl('Valve_stem',(x+dx,.932,-5.68),(x+dx,.99,-5.68),.016,STEEL)
    cyl('Cross_handle',(x+dx-.036,.986,-5.68),(x+dx+.036,.986,-5.68),.007,mat)
pipe('Visible_water_feed',[(17.91,.94,-5.70),(17.91,.45,-5.75),(18.50,.45,-5.76)],.009,STEEL)
finish('SM_PhotoLab_RinseSink','Open stainless sink with drain, trap, gooseneck, valves and grounded legs.', collision='Existing dressLab corner: x17.35..19,z-5.85..-5.15; confirm bounds after import', protect=['existing chemistry jugs'])

# 3. An additive enlarger cover: old light gate mesh and lens remain untouched.
box('Enlarger_cover',(17,1.762,-5.47),(.245,.279,.262),ENAMEL,.018)
box('Negative_carrier',(17,1.60,-5.467),(.30,.020,.245),DARK)
box('Carrier_pull',(16.83,1.60,-5.46),(.062,.018,.054),DARK)
for i in range(7):box('Cooling_slot',(17.126,1.69+i*.022,-5.47),(.003,.007,.135),DARK,.001)
for dx in (-.085,.085):screw('Head_cover_screw',17+dx,1.855,-5.335)
for yy in (1.56,1.58):ring('Open_lens_knurl',(17,yy,-5.45),.064,.051,.012,DARK)
cyl('Focus_shaft',(17.025,1.53,-5.60),(17.15,1.53,-5.60),.010,STEEL)
cyl('Focus_wheel',(17.15,1.53,-5.60),(17.175,1.53,-5.60),.039,DARK,24)
for i in range(12):
    a=i*math.tau/12
    cyl('Focus_grip',(17.15,1.53+.037*math.sin(a),-5.60+.037*math.cos(a)),(17.177,1.53+.037*math.sin(a),-5.60+.037*math.cos(a)),.003,DARK,8)
for yy in (1.03,1.37,1.70):
    box('Column_clamp',(17,yy,-5.60),(.09,.027,.085),STEEL)
box('Scale_strip',(17.032,1.36,-5.585),(.006,.68,.016),WHITE,.001)
for i in range(17):box('Column_scale_tick',(17.038,1.045+i*.038,-5.582),(.002,.0015,.012 if i%4==0 else .007),DARK,0)
# Four easel stops surround the existing print exposure base without covering it.
for sx in (-1,1):box('Easel_side',(17+sx*.228,.991,-5.4),(.018,.018,.46),CREAM)
for sz in (-1,1):box('Easel_end',(17,.991,-5.4+sz*.221),(.44,.018,.018),CREAM)
finish('SM_PhotoLab_EnlargerDetail','Vented cover, carrier, focus control, scale and open optical ring.',protect=['native light gate mesh','lens at (17,1.5,-5.45)'])

# 4. Timer and tray tools keep the old labels and wet print fully exposed.
box('Timer_bezel',(16.85,1.0,-5.512),(.173,.13,.012),CREAM)
cyl('Timer_face',(16.825,1.007,-5.505),(16.825,1.007,-5.499),.048,WHITE,32)
for i in range(12):
    a=i*math.tau/12
    o=box('Timer_tick',(16.825+math.sin(a)*.039,1.007+math.cos(a)*.039,-5.495),(.002,.008,.001),DARK,0)
    o.rotation_euler.y=a
pipe('Timer_hand',[(16.825,1.007,-5.493),(16.825,1.033,-5.493)],.0018,DARK)
cyl('Timer_switch',(16.904,.976,-5.506),(16.904,.976,-5.491),.011,DARK)
for tx in (14.0,14.7,15.4):
    # Only outer rim corners are detailed. No panel across the top of the FIX tray.
    for dx in (-.248,.248):box('Tray_grip_edge',(tx+dx,.985,-5.4),(.012,.017,.405),RED)
# Graduated cylinder sits in unused back-right wet-bench gap; opaque ivory, no fake glass.
cyl('Measuring_cylinder_foot',(16.08,.941,-5.70),(16.08,.953,-5.70),.045,CREAM)
cyl('Measuring_cylinder',(16.08,.953,-5.70),(16.08,1.13,-5.70),.024,CREAM,24)
for i in range(7):box('Graduation',(16.08,.98+i*.02,-5.674),(.018 if i%2==0 else .011,.0015,.001),DARK,0)
finish('SM_PhotoLab_ProcessDetails','Analogue timer face, tray grips and graduated measuring cylinder.',protect=['DEV/STOP/FIX labels','wetPrint'])

# 5. Storage detail in the empty middle section of the existing western shelving.
for yy in (.5,1.1,1.7):
    box('Folded_shelf_front',(13.183,yy-.015,-4.3),(.018,.055,2.40),ENAMEL)
    for zz in (-5.3,-4.3,-3.3):
        box('Shelf_wall_bracket',(12.85,yy-.10,zz),(.025,.20,.03),STEEL)
        box('Shelf_bracket_arm',(13.0,yy-.03,zz),(.30,.022,.03),STEEL)
for zz,yy in ((-4.20,1.12),(-4.43,1.12),(-4.18,1.72)):
    box('Film_carton',(13.0,yy+.065,zz),(.18,.13,.17),PAPER)
    box('Carton_seam',(13.093,yy+.065,zz),(.002,.13,.012),WHITE,0)
    box('Film_carton_label',(13.095,yy+.078,zz),(.003,.054,.095),WHITE,.001)
# Chemistry bottles on unused lower shelf span: simple shouldered solids with ridged caps.
for zz,mat in ((-4.13,BROWN),(-4.36,CREAM)):
    cyl('Bottle_body',(13.0,.516,zz),(13.0,.68,zz),.052,mat,24)
    bpy.ops.mesh.primitive_cone_add(vertices=24,radius1=.052,radius2=.021,depth=.05,location=xyz((13.0,.705,zz)))
    put(bpy.context.object,'Bottle_shoulder',mat)
    cyl('Bottle_neck',(13.0,.73,zz),(13.0,.755,zz),.020,mat)
    cyl('Ribbed_screw_cap',(13.0,.750,zz),(13.0,.782,zz),.025,DARK,24)
    box('Bottle_inventory_label',(13.053,.61,zz),(.003,.075,.065),WHITE,.001)
finish('SM_PhotoLab_StorageDetail','Shelf brackets, folded edges, film cartons and sealed chemistry.',protect=['existing bottles/film boxes'])

# 6. Archive bench receives drawers/legs underneath the live reference document.
for zz in (-4.1,-1.90):
    for xx in (18.29,18.90):cyl('Archive_leg',(xx,.03,zz),(xx,.825,zz),.017,STEEL)
for zz in (-4.10,-1.90):
    box('Archive_drawer',(18.60,.70,zz),(.70,.19,.50),ENAMEL)
    box('Archive_reveal',(18.235,.70,zz),(.012,.165,.46),DARK)
    box('Archive_front',(18.223,.70,zz),(.017,.15,.44),ENAMEL)
    for dz in (-.055,.055):cyl('Archive_pull_post',(18.215,.72,zz+dz),(18.184,.72,zz+dz),.007,STEEL)
    cyl('Archive_pull',(18.184,.72,zz-.055),(18.184,.72,zz+.055),.007,STEEL)
# Negative sleeve rack at north end, well away from reference (z-3.2) and task lamp(z-3.9).
box('Sleeve_rack_base',(18.70,.902,-4.44),(.38,.025,.20),ENAMEL)
for i in range(5):box('Negative_sleeve',(18.56+i*.064,1.015,-4.44),(.012,.22,.17),PAPER)
finish('SM_PhotoLab_ArchiveDetail','Steel supports, drawers and negative-sleeve rack.',protect=['referenceSheet','loupe','archive proxy','task lamp'])

# 7. Records desk joinery and file sorter, leaving night report and pencil unchanged.
box('Records_apron',(17.0,.674,.065),(2.47,.12,.022),ENAMEL)
for xx in (15.73,18.27):
    box('Desk_foot_pad',(xx,.016,.42),(.10,.030,.71),RUBBER)
    for zz in (.13,.70):box('Desk_leg_edge',(xx,.38,zz),(.07,.72,.025),ENAMEL)
box('Report_sorter_base',(17.75,.802,.58),(.30,.025,.25),ENAMEL)
for i in range(4):box('Report_sorter_divider',(17.63+i*.08,.875,.58),(.010,.14,.25),ENAMEL)
for i in range(3):box('Film_envelope',(17.67+i*.08,.91,.58),(.016,.21,.22),PAPER)
finish('SM_PhotoLab_RecordsDetail','Folded desk apron, grounded feet and small film-envelope sorter.',protect=['records clipboard at (17.2,.79,.35)','mug','stool','pencil'])

# 8. Safelight housings ring the existing glass. No new emissive panels or lights.
for xx in (14.826,15.174):box('Safelight_side_cheek',(xx,2.45,-5.65),(.018,.18,.20),ENAMEL)
box('Safelight_top',(15,2.538,-5.65),(.365,.018,.20),ENAMEL)
box('Safelight_bottom',(15,2.362,-5.66),(.365,.018,.18),ENAMEL)
for xx in (14.86,15.14):
    cyl('Safelight_wall_standoff',(xx,2.45,-5.77),(xx,2.45,-5.72),.012,STEEL)
    screw('Safelight_front_screw',xx,2.51,-5.543)
# Ceiling trim outside glass x15.79..16.01,z-2.51..-2.29 at y2.645.
for xx in (15.765,16.035):box('Ceiling_housing_side',(xx,2.70,-2.4),(.020,.115,.285),CREAM)
for zz in (-2.535,-2.265):box('Ceiling_housing_end',(15.9,2.70,zz),(.25,.115,.020),CREAM)
# Conduit rises to existing ceiling; the lower optical openings remain unobstructed.
pipe('Safelight_feed',[(15,2.55,-5.70),(15,2.70,-5.76),(15.9,2.73,-5.76),(15.9,2.73,-2.56)],.008,ENAMEL)
finish('SM_PhotoLab_SafelightHousing','Wall standoffs and removable lamp cases around original diffusers.',protect=['yard.redGlass[0]','yard.redGlass[1]','normal/safe/enlarger light controls'])

# 9. Wall extractor at the existing back corner, physically mounted above the sink.
box('Extractor_backplate',(18.55,2.24,-5.775),(.49,.48,.025),CREAM)
box('Extractor_recess',(18.55,2.24,-5.741),(.41,.40,.048),DARK)
for i in range(9):
    slat=box('Extractor_louvre',(18.55,2.075+i*.041,-5.703),(.425,.027,.042),CREAM,.002)
    slat.rotation_euler.x=.25
for xx in (18.335,18.765):
    for yy in (2.035,2.445):screw('Extractor_mount',xx,yy,-5.752)
pipe('Extractor_surface_conduit',[(18.80,2.24,-5.76),(18.84,2.24,-5.76),(18.84,1.36,-5.76)],.009,ENAMEL)
box('Extractor_switch_box',(18.84,1.33,-5.735),(.105,.14,.06),CREAM)
box('Extractor_switch',(18.84,1.33,-5.700),(.025,.055,.009),DARK)
finish('SM_PhotoLab_Ventilation','Screwed extractor grille and surface-mounted switch/conduit.',protect=['existing room tone and power-driven hum'])

# 10. Junctions, pipe clips and corner seals explain the fittings' attachments to the room.
for xx in (15.0,15.9,18.84):
    box('Conduit_junction',(xx,2.72,-5.757),(.09,.07,.035),CREAM)
    screw('Junction_screw',xx,2.72,-5.736)
for xx in (17.91,18.20,18.48):
    box('Water_pipe_clip',(xx,.45,-5.775),(.028,.047,.015),STEEL)
for xx in (13.3,14.7,16.1,17.5,18.7):
    box('Wall_base_seal',(xx,.055,-5.785),(1.20,.09,.014),RUBBER,.001)
# End grain and fascia around existing shelves, not an unrelated new furniture silhouette.
for yy in (.5,1.1,1.7):
    for zz in (-5.515,-3.085):box('Shelf_folded_end',(13.0,yy-.014,zz),(.36,.055,.015),ENAMEL)
finish('SM_PhotoLab_UtilityDetail','Pipe clips, junctions, cabinet zone seals and shelf end folds.')

assert len(PARTS) == 10
triangles = sum(p['triangles'] for p in PARTS.values())
if triangles >= 80000:
    raise RuntimeError('Detail budget exceeded: %d triangles' % triangles)
(ROOT/'materials.json').write_text(json.dumps(ROWS, indent=2)+'\n')
contract = {'schema':1,'status':'GENERATED_REQUIRES_BLENDER_AND_NATIVE_REVIEW',
    'units':'metres','mapping':'game(x,height,z) -> Blender(x,-z,height) -> UE(100x,100z,100height)',
    'authorship':'Original photo-lab detail geometry and procedural PBR; existing Office export recipe',
    'parts':PARTS,'triangles_lod0':triangles,'legacy_removal_bounds':[],
    'lod_plan':'native import 1.0/0.50/0.20; verify geometric openings and measured ray-tracing footprint',
    'collision_plan':'reuse existing lab bench, shelf, archive, records and chemistry-corner colliders; all new components query/physics off',
    'light_plan':'No exported lights or new emissive surfaces. Existing S47LabLighting gates and glass retained.',
    'protected_positions_three_m':{'wetPrint':[15.4,1.003,-5.4],'referenceSheet':[18.55,.895,-3.2],
       'records':[17.2,.79,.35],'labDoorPivot':[12.8,0,-.73],'enlarger_lens':[17,1.5,-5.45]},
    'validation':'Generator budget assertion only. Source reopen, visual fit, native import and gameplay not yet checked.'}
(ROOT/'model_contract.json').write_text(json.dumps(contract,indent=2)+'\n')

# Neutral context for visual fitting only, never exported or counted as new work.
def context_box(name, at, size):
    o=box(name,at,size,ENAMEL,0)
    CURRENT.remove(o)
    EDIT.objects.unlink(o)
    CONTEXT.objects.link(o)
    return o
context_box('Existing_floor_NOT_EXPORT',(15.9,-.04,-2.5),(6.2,.08,6.6))
context_box('Existing_back_wall_NOT_EXPORT',(15.9,1.5,-5.88),(6.6,3,.16))
context_box('Existing_wet_bench_NOT_EXPORT',(15.2,.9,-5.45),(4.2,.08,.7))
context_box('Existing_archive_top_NOT_EXPORT',(18.6,.86,-3),(.8,.06,3.2))
context_box('Existing_report_top_NOT_EXPORT',(17,.76,.42),(2.6,.06,.7))
for yy in (.5,1.1,1.7):context_box('Existing_shelf_NOT_EXPORT',(13,yy,-4.3),(.35,.03,2.4))

def area(name,at,target,energy,size):
    d=bpy.data.lights.new(name,'AREA');d.energy=energy;d.shape='DISK';d.size=size
    o=bpy.data.objects.new(name,d);STUDIO.objects.link(o);o.location=xyz(at)
    o.rotation_euler=(xyz(target)-o.location).to_track_quat('-Z','Y').to_euler()
area('Review_softbox_ONLY',(15.8,2.6,-1.3),(16,1,-5.2),260,3)
area('Review_sink_fill_ONLY',(18.2,2.4,-4.1),(17.8,.9,-5.4),70,1)

def camera(name,at,target,lens=32):
    d=bpy.data.cameras.new(name);d.lens=lens
    o=bpy.data.objects.new(name,d);STUDIO.objects.link(o);o.location=xyz(at)
    o.rotation_euler=(xyz(target)-o.location).to_track_quat('-Z','Y').to_euler()
    return o
cameras=[camera('Photolab_overview',(15.7,1.65,-.85),(16.0,1.35,-5.35),25),
         camera('Photolab_enlarger',(16.2,1.65,-4.1),(17,1.4,-5.45),45),
         camera('Photolab_sink',(17.0,1.5,-4.2),(17.95,.9,-5.45),42)]
SC.camera=cameras[0]
for im in bpy.data.images:
    if im.source=='FILE':im.pack()
bpy.ops.file.make_paths_relative()
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'Signal47_PhotoLab_Detail.blend'))
if '--render' in sys.argv:
    for cam in cameras:
        SC.camera=cam;SC.render.filepath=str(PRE/(cam.name+'.png'))
        bpy.ops.render.render(write_still=True)
    SC.camera=cameras[0]
    bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'Signal47_PhotoLab_Detail.blend'))
print('PHOTOLAB DETAIL GENERATED',json.dumps({k:v['triangles'] for k,v in PARTS.items()}))
