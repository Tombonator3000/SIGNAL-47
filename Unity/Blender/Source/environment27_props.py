"""Original SARO field survey / maintenance kit for SIGNAL / 47 (1986).

Authoring is metre scale, Blender Z-up.  The FBX exports use axis_forward=-Z,
axis_up=Y for Unity's established import convention.  Each recipe builds at
the origin, with a ground contact at local Z=0, then is exported separately.
"""
import bpy, json, math
from pathlib import Path
from mathutils import Vector

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'Assets/Signal47/Art/Environment27/Models'
OUT.mkdir(parents=True, exist_ok=True)
BLEND = ROOT / 'Blender/SIGNAL47_environment27_props.blend'

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.context.scene.unit_settings.system = 'METRIC'
bpy.context.scene.unit_settings.length_unit = 'METERS'
bpy.context.preferences.filepaths.save_version = 0

PALETTE = {
    'PaintedMetal': ((.18, .22, .20, 1), .46, .72),
    'Steel': ((.34, .37, .34, 1), .29, .82),
    'Rubber': ((.035, .042, .038, 1), .83, .04),
    'Wood': ((.27, .16, .085, 1), .68, .02),
    'Glass': ((.055, .13, .14, 1), .18, .05),
    'Lamp': ((.88, .47, .12, 1), .32, .12),
    'Sandstone': ((.48, .27, .12, 1), .9, .0),
}
mats = {}
for key, (color, rough, metallic) in PALETTE.items():
    m = bpy.data.materials.new('E27_' + key)
    m.diffuse_color = color
    m.use_nodes = True
    bsdf = m.node_tree.nodes.get('Principled BSDF')
    bsdf.inputs['Base Color'].default_value = color
    bsdf.inputs['Roughness'].default_value = rough
    bsdf.inputs['Metallic'].default_value = metallic
    mats[key] = m
ORIGINAL_MATERIALS = set(mats.values())

parts = []

def finish(obj, name, mat, bevel=0.0, smooth=False):
    obj.name = name
    if obj.type == 'MESH':
        obj.data.materials.append(mats[mat])
        bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
        if bevel:
            mod = obj.modifiers.new('Authored edge radius', 'BEVEL')
            mod.width = min(bevel, min(obj.dimensions) * .28)
            mod.segments = 2
            mod.limit_method = 'ANGLE'
            bpy.context.view_layer.objects.active = obj
            bpy.ops.object.modifier_apply(modifier=mod.name)
        if smooth:
            for poly in obj.data.polygons:
                poly.use_smooth = True
        try:
            mod = obj.modifiers.new('Weighted corner normals', 'WEIGHTED_NORMAL')
            bpy.context.view_layer.objects.active = obj
            bpy.ops.object.modifier_apply(modifier=mod.name)
        except RuntimeError:
            pass
    else:
        obj.data.materials.append(mats[mat])
    parts.append(obj)
    return obj

def box(name, loc, size, mat='PaintedMetal', bevel=.008, rot=None):
    bpy.ops.mesh.primitive_cube_add(size=1, location=loc)
    obj = bpy.context.object
    obj.scale = size
    if rot:
        obj.rotation_euler = rot
    return finish(obj, name, mat, bevel)

def cyl(name, loc, radius, depth, mat='Steel', axis=(0, 0, 1), vertices=16, bevel=.003):
    bpy.ops.mesh.primitive_cylinder_add(vertices=vertices, radius=radius, depth=depth, location=loc)
    obj = bpy.context.object
    obj.rotation_euler = Vector(axis).to_track_quat('Z', 'Y').to_euler()
    return finish(obj, name, mat, min(bevel, radius * .2), True)

def sphere(name, loc, scale, mat='Rubber', segments=16, rings=8):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=segments, ring_count=rings, location=loc)
    obj = bpy.context.object
    obj.scale = scale
    return finish(obj, name, mat, 0, True)

def tube(name, points, radius=.008, mat='Steel', resolution=1):
    curve = bpy.data.curves.new(name, 'CURVE')
    curve.dimensions = '3D'; curve.resolution_u = 3
    curve.bevel_depth = radius; curve.bevel_resolution = resolution
    spline = curve.splines.new('POLY')
    spline.points.add(len(points) - 1)
    for point, co in zip(spline.points, points):
        point.co = (*co, 1)
    obj = bpy.data.objects.new(name, curve)
    bpy.context.collection.objects.link(obj)
    bpy.context.view_layer.objects.active = obj; obj.select_set(True)
    bpy.ops.object.convert(target='MESH')
    return finish(bpy.context.object, name, mat, 0, True)

def ring(name, loc, major, minor, mat='Steel', axis=(0, 0, 1), segments=20, minor_segments=8):
    bpy.ops.mesh.primitive_torus_add(major_radius=major, minor_radius=minor, major_segments=segments, minor_segments=minor_segments, location=loc)
    obj = bpy.context.object
    obj.rotation_euler = Vector(axis).to_track_quat('Z', 'Y').to_euler()
    return finish(obj, name, mat, 0, True)

def custom_mesh(name, verts, faces, mat='PaintedMetal', bevel=0.0, smooth=False):
    mesh = bpy.data.meshes.new(name + '_Mesh')
    mesh.from_pydata(verts, [], faces); mesh.update()
    obj = bpy.data.objects.new(name, mesh); bpy.context.collection.objects.link(obj)
    return finish(obj, name, mat, bevel, smooth)

def screw(name, loc, axis=(0, 0, 1)):
    return cyl(name, loc, .009, .008, 'Steel', axis, 12, .001)

def survey_transit():
    # 1980s optical survey transit with level base and tripod. Ground contact z=0.
    for a in (0, math.tau/3, math.tau*2/3):
        x, y = .23 * math.cos(a), .23 * math.sin(a)
        tube('Tripod wooden leg', [(0, 0, 1.04), (x*.72, y*.72, .48), (x, y, .035)], .027, 'Wood', 2)
        cyl('Tripod rubber foot', (x, y, .025), .045, .05, 'Rubber', vertices=12, bevel=.002)
    box('Transit tripod spreader', (0, 0, .48), (.07, .07, .07), 'Steel', .008)
    cyl('Transit center column', (0, 0, .98), .052, .27, 'Steel', vertices=20)
    cyl('Transit leveling base', (0, 0, 1.10), .18, .07, 'PaintedMetal', vertices=24, bevel=.012)
    for a in (0, math.tau/3, math.tau*2/3):
        x, y = .105*math.cos(a), .105*math.sin(a)
        cyl('Leveling screw', (x, y, 1.05), .014, .09, 'Steel', vertices=12)
        sphere('Leveling bubble', (x, y, 1.115), (.023, .023, .010), 'Glass')
    # telescope body along X, with objective on +X and eyepiece on -X
    cyl('Transit telescope barrel', (0, 0, 1.28), .105, .48, 'PaintedMetal', axis=(1,0,0), vertices=20, bevel=.012)
    cyl('Objective rim', (.255, 0, 1.28), .125, .035, 'Steel', axis=(1,0,0), vertices=24, bevel=.006)
    cyl('Objective glass', (.276, 0, 1.28), .094, .008, 'Glass', axis=(1,0,0), vertices=24, bevel=.002)
    cyl('Eyepiece collar', (-.255, 0, 1.28), .078, .06, 'Steel', axis=(1,0,0), vertices=20)
    cyl('Eyepiece rubber cup', (-.30, 0, 1.28), .055, .07, 'Rubber', axis=(1,0,0), vertices=16)
    box('Transit side plate', (0, -.095, 1.18), (.29, .018, .15), 'Steel', .005)
    for x in (-.12, .12): screw('Transit plate screw', (x, -.108, 1.18), axis=(0,1,0))
    cyl('Horizontal tangent knob', (0, -.14, 1.28), .034, .055, 'PaintedMetal', axis=(0,1,0), vertices=16)
    cyl('Vertical tangent knob', (-.02, -.11, 1.40), .028, .05, 'PaintedMetal', axis=(0,1,0), vertices=16)
    ring('Horizontal graduated circle', (0, 0, 1.105), .145, .009, 'Steel', segments=24)
    for i in range(12):
        a = i * math.tau / 12
        tube('Transit scale mark', [(.145*math.cos(a), .145*math.sin(a), 1.114), (.132*math.cos(a), .132*math.sin(a), 1.114)], .002, 'PaintedMetal')
    box('Transit sunshade', (.29, 0, 1.28), (.09, .19, .18), 'PaintedMetal', .018)

def analog_receiver():
    # 0.48 x 0.30 x 0.25 m rugged field receiver, front faces +Y.
    box('Receiver main case', (0, 0, .145), (.48, .30, .25), 'PaintedMetal', .018)
    box('Receiver front inset', (0, .153, .145), (.415, .018, .19), 'Steel', .005)
    box('Receiver label plate', (-.10, .166, .225), (.17, .006, .035), 'PaintedMetal', .002)
    box('Receiver tuning window', (.07, .167, .205), (.16, .008, .064), 'Glass', .003)
    cyl('Receiver tuning dial', (.07, .175, .205), .028, .012, 'Steel', axis=(0,1,0), vertices=24)
    cyl('Receiver dial pointer', (.07, .183, .220), .003, .020, 'Lamp', axis=(0,1,0), vertices=8, bevel=.001)
    for i in range(9):
        a = math.radians(-65 + i*16.25)
        x=.07 + .043*math.sin(a); z=.205 + .043*math.cos(a)
        tube('Receiver dial tick', [(x,.184,z), (x,.184,z+.008)], .0018, 'PaintedMetal')
    for x, r in [(-.13,.024), (-.06,.018), (.16,.021)]:
        cyl('Receiver control knob', (x, .178, .105), r, .035, 'PaintedMetal', axis=(0,1,0), vertices=20, bevel=.004)
        tube('Receiver knob index', [(x,.197,.105), (x,.197,.105+r*.7)], .002, 'Lamp')
    for x in (-.17,-.13,-.09,-.05):
        box('Receiver lower vent', (x, .164, .055), (.018, .008, .032), 'Rubber', .002)
    for x in (-.20,.20):
        for z in (.055,.235): screw('Receiver corner screw', (x,.165,z), axis=(0,1,0))
    box('Receiver carry handle left', (-.19, 0, .325), (.045, .13, .045), 'Steel', .008)
    box('Receiver carry handle right', (.19, 0, .325), (.045, .13, .045), 'Steel', .008)
    tube('Receiver handle grip', [(-.19,.0,.34),(-.19,.0,.39),(.19,.0,.39),(.19,.0,.34)], .018, 'Rubber', 2)
    for x in (-.185,.185):
        for y in (-.105,.105): cyl('Receiver rubber bumper', (x,y,.025), .018, .03, 'Rubber', vertices=12)
    box('Receiver rear connector block', (0,-.157,.14), (.20,.024,.07), 'Steel', .004)
    for x in (-.07,0,.07): cyl('Receiver connector', (x,-.175,.14), .012, .012, 'Rubber', axis=(0,1,0), vertices=12)

def lamp_pole():
    # Rugged 1.7 m portable mast with guarded amber service lamp.
    box('Lamp weighted base', (0,0,.055), (.43,.34,.11), 'PaintedMetal', .022)
    for x in (-.16,.16):
        for y in (-.11,.11): cyl('Lamp base foot',(x,y,.02),.032,.04,'Rubber',vertices=14)
    cyl('Lamp mast', (0,0,.80), .036, 1.40, 'Steel', vertices=20, bevel=.004)
    box('Lamp lower collar', (0,0,.20), (.12,.12,.08), 'PaintedMetal', .012)
    box('Lamp control box', (.16,.0,.60), (.24,.12,.34), 'PaintedMetal', .012)
    box('Lamp control face', (.16,.067,.60), (.17,.012,.24), 'Steel', .003)
    cyl('Lamp toggle', (.12,.078,.67), .012, .018, 'Steel', axis=(0,1,0), vertices=12)
    cyl('Lamp status', (.20,.078,.67), .012, .015, 'Lamp', axis=(0,1,0), vertices=12)
    for z in (.53,.58,.63): box('Lamp control slot', (.16,.078,z), (.09,.008,.012), 'Rubber', .002)
    tube('Lamp cable', [(.20,.08,.43),(.20,.10,.28),(.10,.12,.12)], .010, 'Rubber', 2)
    cyl('Lamp head neck', (0,0,1.53), .055, .14, 'Steel', vertices=20)
    box('Lamp head housing', (0,.005,1.60), (.34,.26,.14), 'PaintedMetal', .025)
    box('Lamp lens guard', (0,.142,1.60), (.23,.018,.08), 'Steel', .006)
    box('Lamp amber lens', (0,.153,1.60), (.19,.009,.055), 'Lamp', .004)
    for x in (-.14,-.07,0,.07,.14): tube('Lamp guard rib', [(x,.13,1.54),(x,.16,1.66)], .008, 'Steel')
    box('Lamp head top handle', (0,-.02,1.695), (.19,.08,.025), 'Steel', .007)

def diesel_generator():
    # Compact service generator, 0.72 x 0.47 x 0.58 m, with canopy and pull start.
    box('Generator lower skid', (0,0,.055), (.72,.47,.11), 'Steel', .018)
    for x in (-.27,.27):
        cyl('Generator wheel', (x,.26,.12), .095, .045, 'Rubber', axis=(0,1,0), vertices=20)
        cyl('Generator wheel hub', (x,.29,.12), .025, .05, 'Steel', axis=(0,1,0), vertices=16)
    box('Generator canopy', (0,0,.38), (.64,.40,.52), 'PaintedMetal', .025)
    box('Generator front panel', (0,.21,.37), (.48,.018,.27), 'Steel', .006)
    box('Generator vent grille', (-.12,.225,.45), (.25,.012,.13), 'Rubber', .004)
    for x in (-.21,-.16,-.11,-.06,-.01): box('Generator vent rib',(x,.235,.45),(.018,.008,.11),'Steel',.002)
    cyl('Generator start switch', (.19,.235,.44), .018, .018, 'PaintedMetal', axis=(0,1,0), vertices=16)
    cyl('Generator fuel cap', (.19,-.13,.66), .04, .018, 'Steel', axis=(0,0,1), vertices=20)
    box('Generator exhaust guard', (-.20,-.13,.72), (.12,.12,.15), 'Steel', .012)
    cyl('Generator exhaust pipe', (-.20,-.13,.81), .028, .20, 'Steel', vertices=16)
    tube('Generator pull cord', [(.23,.23,.32),(.36,.29,.22),(.43,.33,.18)], .007, 'Rubber', 2)
    sphere('Generator pull handle', (.44,.34,.18), (.035,.018,.018), 'Rubber')
    for x in (-.29,.29): screw('Generator canopy screw',(x,.215,.67),axis=(0,1,0))

def distribution_cabinet():
    # Freestanding electrical distribution cabinet with openable-looking door.
    box('Distribution cabinet body',(0,0,.76),(.52,.28,1.52),'PaintedMetal',.022)
    box('Distribution cabinet door',(0,.148,.78),(.45,.018,1.38),'Steel',.008)
    box('Distribution door gasket',(0,.162,.78),(.40,.012,1.28),'Rubber',.004)
    box('Distribution label field',(-.06,.174,1.30),(.19,.008,.055),'PaintedMetal',.002)
    for x in (-.13,.13):
        cyl('Distribution breaker', (x,.177,.92), .027, .020, 'PaintedMetal', axis=(0,1,0), vertices=16)
        box('Distribution switch', (x,.191,.97), (.025,.010,.075), 'Steel', .003)
    for z in (.65,.73,.81): box('Distribution fuse window',(.0,.175,z),(.22,.008,.028),'Glass',.002)
    box('Distribution latch',(.18,.18,1.12),(.035,.018,.08),'Steel',.004)
    for x in (-.20,.20):
        for z in (.10,1.42): screw('Distribution corner bolt',(x,.174,z),axis=(0,1,0))
    box('Distribution cable gland', (0,-.16,.10), (.24,.05,.10), 'Rubber', .012)
    for x in (-.07,0,.07): cyl('Distribution cable port',(x,-.196,.11),.018,.014,'Steel',axis=(0,1,0),vertices=12)
    box('Distribution plinth',(0,0,.045),(.62,.36,.09),'Steel',.014)

def sandstone(shape='A'):
    # Deformed icosphere boulder: triangulated facets give natural broken
    # faces without repeated horizontal ring/cake silhouettes. A is a long,
    # low fractured ledge; B is a chunkier outcrop with a split crest.
    target=(1.025509, .729440, .547625) if shape == 'A' else (1.091841, .795901, .612712)
    phase=.45 if shape == 'A' else 1.25
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=4, radius=1, location=(0,0,0))
    obj=bpy.context.object; obj.name='Angular stratified sandstone '+shape
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    verts=[list(v.co) for v in obj.data.vertices]
    for v in verts:
        x,y,z=v; z01=(z+1.0)*.5
        foot=1.0 + .16*(1.0-z01) + .045*math.sin(4*math.atan2(y,x)+phase)
        v[0]=x*foot; v[1]=y*foot
        if shape == 'A':
            v[0]+=0.12*z01
            # One broad front-side break creates a long, low ledge.
            if z01>.34 and y>.02: v[2]-=.52*(z01-.34)
            if z01>.63 and x<-.18: v[2]-=.28*(z01-.63)
            if z01>.50 and x>.28: v[1]-=.18*(z01-.50)
        else:
            v[0]-=.08*z01
            # A single deep side fracture and a split high crest make B chunky.
            if z01>.25 and x<-.12 and y>.02: v[0]-=.42*(z01-.25)
            if z01>.56 and x>.12: v[2]-=.38*(z01-.56)
            if z01>.68 and y<-.18: v[2]-=.25*(z01-.68)
        if z01<.23: v[2]=max(v[2],-.76)
        v[0]+=.055*math.sin(5*y+phase)*z01
        v[1]+=.045*math.cos(4*x-phase)*z01
    for vertex,co in zip(obj.data.vertices,verts): vertex.co=co
    decimate=obj.modifiers.new('Broken face reduction','DECIMATE'); decimate.ratio=.55
    bpy.context.view_layer.objects.active=obj; bpy.ops.object.modifier_apply(modifier=decimate.name)
    verts=[list(v.co) for v in obj.data.vertices]
    mins=[min(v[i] for v in verts) for i in range(3)]; maxs=[max(v[i] for v in verts) for i in range(3)]
    for i in range(3):
        span=maxs[i]-mins[i]; target_center=0.0 if i<2 else target[i]*.5
        for v in verts: v[i]=(v[i]-mins[i])*target[i]/span + target_center-target[i]*.5
    for vertex,co in zip(obj.data.vertices,verts): vertex.co=co
    for poly in obj.data.polygons: poly.use_smooth=False
    bpy.context.view_layer.objects.active=obj; obj.select_set(True)
    return finish(obj, 'Angular stratified sandstone '+shape, 'Sandstone', 0.0, False)

def service_gate():
    # A short rugged gate panel for the station maintenance approach.
    for x in (-.72,.72):
        box('Gate post',(x,0,.66),(.14,.16,1.32),'PaintedMetal',.018)
        box('Gate post cap',(x,0,1.36),(.18,.20,.08),'Steel',.012)
        cyl('Gate hinge upper',(x,.10,1.05),.035,.05,'Steel',axis=(0,1,0),vertices=16)
        cyl('Gate hinge lower',(x,.10,.40),.035,.05,'Steel',axis=(0,1,0),vertices=16)
    box('Gate top rail',(0,0,1.28),(1.50,.12,.10),'Steel',.012)
    box('Gate bottom rail',(0,0,.18),(1.50,.12,.10),'Steel',.012)
    for x in (-.57,-.38,-.19,0,.19,.38,.57):
        box('Gate vertical slat',(x,0,.73),(.065,.08,1.04),'PaintedMetal',.008,rot=(0,0,math.radians(-4 if x<0 else 4)))
    box('Gate diagonal brace',(0,-.055,.73),(1.22,.055,.065),'Steel',.006,rot=(0,math.radians(0),math.radians(27)))
    box('Gate latch plate',(.72,.10,.74),(.11,.035,.15),'Steel',.005)
    cyl('Gate latch handle',(.66,.14,.76),.018,.12,'Rubber',axis=(0,1,0),vertices=12)

RECIPES = [
    ('SurveyTransit', survey_transit, 6200),
    ('AnalogReceiver', analog_receiver, 5200),
    ('LampPole', lamp_pole, 5200),
    ('DieselGenerator', diesel_generator, 5600),
    ('DistributionCabinet', distribution_cabinet, 4200),
    ('Sandstone_A', lambda: sandstone('A'), 1400),
    ('Sandstone_B', lambda: sandstone('B'), 1400),
    ('ServiceGate', service_gate, 5200),
]

def mesh_bounds(objects):
    bpy.context.view_layer.update()
    coords=[obj.matrix_world @ Vector(corner) for obj in objects for corner in obj.bound_box]
    lo=[min(c[i] for c in coords) for i in range(3)]
    hi=[max(c[i] for c in coords) for i in range(3)]
    return [round(hi[i]-lo[i],6) for i in range(3)], [round(lo[i],6) for i in range(3)], [round(hi[i],6) for i in range(3)]

def uv_and_join(name, budget):
    for obj in parts:
        if obj.type != 'MESH': continue
        bpy.ops.object.select_all(action='DESELECT'); obj.select_set(True); bpy.context.view_layer.objects.active=obj
        bpy.ops.object.mode_set(mode='EDIT'); bpy.ops.mesh.select_all(action='SELECT')
        bpy.ops.uv.smart_project(angle_limit=math.radians(70), island_margin=.015)
        bpy.ops.object.mode_set(mode='OBJECT')
    groups={key:[o for o in parts if o.type=='MESH' and o.data.materials and o.data.materials[0]==mats[key]] for key in mats}
    joined=[]
    for key, group in groups.items():
        if not group: continue
        bpy.ops.object.select_all(action='DESELECT')
        for obj in group: obj.select_set(True)
        bpy.context.view_layer.objects.active=group[0]
        bpy.ops.object.join(); obj=bpy.context.object; obj.name='E27_'+name+'_'+key; joined.append(obj)
    dims,lo,hi=mesh_bounds(joined)
    tris=sum(max(0,len(poly.vertices)-2) for obj in joined for poly in obj.data.polygons)
    assert tris <= budget, (name, tris, budget)
    assert all(obj.data.uv_layers and all(poly.area > 1e-12 for poly in obj.data.polygons) for obj in joined)
    return joined,dims,lo,hi,tris

records=[]; assembled=[]
for name, recipe, budget in RECIPES:
    parts=[]; recipe()
    joined,dims,lo,hi,tris=uv_and_join(name,budget)
    bpy.ops.object.select_all(action='DESELECT')
    for obj in joined: obj.select_set(True)
    path=OUT / ('E27_'+name+'.fbx')
    bpy.ops.export_scene.fbx(filepath=str(path), use_selection=True, object_types={'MESH'}, add_leaf_bones=False, bake_anim=False, axis_forward='-Z', axis_up='Y', apply_unit_scale=True)
    # Reimport into a temporary collection to prove the scale/axis roundtrip.
    bpy.ops.object.select_all(action='DESELECT')
    bpy.ops.import_scene.fbx(filepath=str(path))
    copies=list(bpy.context.selected_objects)
    rdims,_,_=mesh_bounds(copies)
    assert all(abs(a-b)<1e-5 for a,b in zip(dims,rdims)), (name,dims,rdims)
    for obj in copies:
        mesh = obj.data if obj.type == 'MESH' else None
        bpy.data.objects.remove(obj, do_unlink=True)
        if mesh and mesh.users == 0:
            bpy.data.meshes.remove(mesh)
    # FBX reimport creates temporary suffixed materials; discard only those
    # with no users so the saved authoring source keeps canonical E27 names.
    for material in list(bpy.data.materials):
        if material not in ORIGINAL_MATERIALS and material.users == 0:
            bpy.data.materials.remove(material)
    records.append({'name':name,'fbx':path.name,'dimensions_blender_xyz':dims,'bounds_min':lo,'bounds_max':hi,'triangles':tris,'triangle_budget':budget,'mesh_parts':len(joined),'uv_layers':'PASS','nonzero_face_area':'PASS','fbx_roundtrip_bounds':'PASS'})
    assembled.append((name,joined))

# Put a spaced display set in the source scene for inspection.  Each export
# remains rooted at ground z=0; these are only authoring offsets.
display_offsets={'SurveyTransit':(-1.45,0,0),'AnalogReceiver':(0,0,0),'LampPole':(1.25,0,0),'DieselGenerator':(-1.40,1.25,0),'DistributionCabinet':(0,1.25,0),'Sandstone_A':(1.0,1.22,0),'Sandstone_B':(1.65,1.35,0),'ServiceGate':(0,2.10,0)}
for name,objects in assembled:
    for obj in objects: obj.location += Vector(display_offsets[name])

scene=bpy.context.scene
scene.world=bpy.data.worlds.new('Environment27 neutral studio')
scene.world.color=(.035,.045,.05)
bpy.ops.mesh.primitive_plane_add(size=7, location=(0,1.0,-.012)); floor=bpy.context.object; finish(floor,'Environment27 display floor','Rubber',.0)
bpy.ops.object.camera_add(location=(4.7,-6.4,3.55)); cam=bpy.context.object; scene.camera=cam; cam.data.type='ORTHO'; cam.data.ortho_scale=5.1
def aim(target): cam.rotation_euler=(Vector(target)-cam.location).to_track_quat('-Z','Y').to_euler()
aim((0,1.0,.72))
for loc,energy,size in [((3,-3,5),950,4),((-4,-1,2.8),650,3),((0,4,4),500,2)]:
    bpy.ops.object.light_add(type='AREA', location=loc); light=bpy.context.object; light.data.energy=energy; light.data.shape='DISK'; light.data.size=size; light.rotation_euler=(Vector((0,1,.6))-light.location).to_track_quat('-Z','Y').to_euler()
scene.render.engine='BLENDER_EEVEE_NEXT'
scene.render.resolution_x=1280; scene.render.resolution_y=760; scene.render.resolution_percentage=100
scene.render.image_settings.file_format='PNG'; scene.render.film_transparent=False
evidence=ROOT.parent/'Artifacts/Environment27'; evidence.mkdir(parents=True,exist_ok=True)
scene.render.filepath=str(evidence/'environment27-contact-sheet.png')
bpy.ops.wm.save_as_mainfile(filepath=str(BLEND))
bpy.ops.render.render(write_still=True)
# Isolated front inspection: the receiver's +Y control face and both rocks.
for name,objects in assembled:
    for obj in objects:
        obj.hide_render = name not in ('AnalogReceiver','Sandstone_A','Sandstone_B')
cam.location=(2.8,5.8,2.25); cam.data.ortho_scale=2.85; aim((.46,.54,.38))
scene.render.filepath=str(evidence/'receiver-rocks-front.png')
bpy.ops.render.render(write_still=True)
for name,objects in assembled:
    for obj in objects: obj.hide_render=False
manifest={'authored_original':True,'external_assets':[],'blender':bpy.app.version_string,'units':'metres','axis':'Blender Z-up; FBX axis_forward=-Z axis_up=Y; Unity metres','source':'Unity/Blender/SIGNAL47_environment27_props.blend','recipe':'Unity/Blender/Source/environment27_props.py','materials':['E27_PaintedMetal','E27_Steel','E27_Rubber','E27_Wood','E27_Glass','E27_Lamp','E27_Sandstone'],'mounting_offsets':{'SurveyTransit':'ground z=0; optical center z=1.28 m','AnalogReceiver':'ground z=0; case bounds 0.48 x 0.30 x 0.25 m','LampPole':'ground z=0; overall height approx. 1.69 m including handle','DieselGenerator':'ground z=0; wheel/skid contact','DistributionCabinet':'ground z=0; plinth contact; body height 1.52 m','Sandstone_A':'ground z=0; irregular rock contact','Sandstone_B':'ground z=0; irregular rock contact','ServiceGate':'ground z=0; post center spacing 1.44 m'},'assets':records}
(OUT/'source-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
print('ENVIRONMENT27_ASSETS_OK',json.dumps(records))
