"""EASTWING-01 lab props. Original geometry plus licensed RM565 reuse.
Run only under root's require-idle PASS/production lease, Blender background.
Coordinates: metres, Z up, equipment front -Y, grounded local origin.
"""
from pathlib import Path
import math
import json
import sys
import zipfile
import shutil
import bpy
from mathutils import Vector

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent))
from prop_common import (material, box, cyl, sphere, rod, text, start_asset,
                         finish_asset, render_catalog)
OUT = HERE
PRIVATE = Path.home() / 'Assets/signal47-private/EastWing/Props/Lab'
SCOPE_ZIP = Path.home() / 'Assets/signal47-free/sketchfab/oscilloscope_rm565.zip'
SOURCE = 'unreal/ArtSource/EastWing/Props/Lab/build_lab.py'

cream = material('EW_Lab_CreamEnamel', (.69,.66,.53), .3)
green = material('EW_Lab_GreenEnamel', (.16,.24,.20), .33, texture='EW_GreenSteel')
steel = material('EW_Lab_Stainless', (.48,.51,.5), .24, .85, texture='EW_Galvanized')
chrome = material('EW_Lab_Chrome', (.7,.73,.74), .17, 1)
dark = material('EW_Lab_Bakelite', (.017,.02,.019), .39)
rubber = material('EW_Lab_Rubber', (.022,.025,.023), .7)
paper = material('EW_Lab_LabelPaper', (.81,.78,.63), .8)
ink = material('EW_Lab_LabelInk', (.025,.032,.028), .7)
red = material('EW_Lab_Red', (.38,.025,.012), .38)
glass = material('EW_Lab_Glass', (.55,.74,.68), .12)
bs = glass.node_tree.nodes.get('Principled BSDF')
if bs:
    bs.inputs['Transmission Weight'].default_value = .85
    bs.inputs['IOR'].default_value = 1.46
amber = material('EW_Lab_AmberGlass', (.24,.08,.016), .18)
bs = amber.node_tree.nodes.get('Principled BSDF')
if bs:
    bs.inputs['Transmission Weight'].default_value = .48
    bs.inputs['IOR'].default_value = 1.46


def front_label(name, body, x, y, z, width, height, size=None):
    box(name+'_plate',(x,y,z),(width,.0015,height),paper,.001)
    text(name+'_ink',body,(x,y-.0015,z),size or height*.55,ink)



def curved_label(name,body,r,z,width,height,size):
    # A thin paper patch and glyphs conform to the bottle rather than float as a board.
    verts=[]
    for zz in (z-height/2,z+height/2):
        for i in range(25):
            a=(-width/2+width*i/24)/r
            verts.append((r*math.sin(a),-r*math.cos(a),zz))
    faces=[(i,i+1,i+26,i+25) for i in range(24)]
    mesh=bpy.data.meshes.new(name+'_paper');mesh.from_pydata(verts,[],faces);mesh.update()
    ob=bpy.data.objects.new(name+'_paper',mesh);bpy.context.collection.objects.link(ob)
    mesh.materials.append(paper)
    glyph=text(name+'_ink',body,(0,-r-.0003,z),size,ink)
    bpy.ops.object.select_all(action='DESELECT');glyph.select_set(True)
    bpy.context.view_layer.objects.active=glyph;bpy.ops.object.convert(target='MESH')
    glyph=bpy.context.object;matrix=glyph.matrix_world.copy()
    for vert in glyph.data.vertices:
        p=matrix@vert.co;a=p.x/r
        vert.co=(math.sin(a)*(r+.0003),-math.cos(a)*(r+.0003),p.z)
    glyph.matrix_world.identity()


def screw(name,x,y,z,r=.003):
    cyl(name,(x,y,z),r,.0018,steel,vertices=16,rotation=(math.pi/2,0,0))
    box(name+'_slot',(x,y-.0011,z),(r*1.25,.0005,.0007),dark,0)


def feet(w,d,z=.012):
    for x in (-w*.36,w*.36):
        for y in (-d*.36,d*.36):
            cyl('RubberFoot',(x,y,z),.014,z*2,rubber,vertices=24)


def knob(name,x,y,z,r=.015,depth=.018,mat=dark):
    cyl(name,(x,y,z),r,depth,mat,vertices=48,rotation=(math.pi/2,0,0))
    for i in range(24):
        a=i*math.tau/24
        rod(name+'_knurl',(x+math.cos(a)*r,y-depth*.35,z+math.sin(a)*r),
            (x+math.cos(a)*r,y+depth*.35,z+math.sin(a)*r),.0007,mat)
    box(name+'_index',(x,y-depth*.53,z+r*.48),(.0015,.0007,r*.65),paper,.0002)


def torus(name,loc,major,minor,mat,rotation=(0,0,0)):
    bpy.ops.mesh.primitive_torus_add(major_segments=48,minor_segments=10,
        location=loc,major_radius=major,minor_radius=minor,rotation=rotation)
    ob=bpy.context.object;ob.name=name;ob.data.materials.append(mat)
    for p in ob.data.polygons:p.use_smooth=True
    return ob


def lathe(name, profile, mat, x=0,y=0,n=64):
    """Revolved closed wall profile including inner return: physically hollow glass."""
    verts=[];rings=[]
    for r,z in profile:
        ring=[]
        for i in range(1 if r==0 else n):
            ring.append(len(verts))
            verts.append((x+r*math.cos(i*math.tau/n),y+r*math.sin(i*math.tau/n),z))
        rings.append(ring)
    faces=[]
    for lower,upper in zip(rings,rings[1:]):
        if len(lower)==1:
            for i in range(n):faces.append((lower[0],upper[(i+1)%n],upper[i]))
        elif len(upper)==1:
            for i in range(n):faces.append((lower[i],lower[(i+1)%n],upper[0]))
        else:
            for i in range(n):faces.append((lower[i],lower[(i+1)%n],upper[(i+1)%n],upper[i]))
    mesh=bpy.data.meshes.new(name);mesh.from_pydata(verts,[],faces);mesh.update()
    ob=bpy.data.objects.new(name,mesh);bpy.context.collection.objects.link(ob)
    ob.data.materials.append(mat)
    for p in mesh.polygons:p.use_smooth=True
    return ob


def finish(name, description, sources=None):
    return finish_asset(name,OUT,description,sources or [SOURCE])


# Compound microscope: cast base/arm, actual stage aperture, binocular head,
# rotating nosepiece, objective lenses, coaxial focus knobs, condenser and lamp.
start_asset('SM_EW_Microscope')
feet(.21,.29,.008)
box('CastFoot',(0,0,.029),(.21,.29,.042),cream,.018)
box('BaseInset',(0,-.065,.051),(.135,.105,.006),dark,.008)
cyl('LampHousing',(0,-.055,.068),.041,.032,dark)
cyl('LampLens',(0,-.055,.085),.031,.004,glass)
box('RearArmFoot',(0,.092,.12),(.085,.09,.16),cream,.018)
rod('CurvedCastArmLower',(0,.09,.10),(0,.105,.28),.042,cream)
rod('CurvedCastArmUpper',(0,.105,.28),(0,.027,.365),.037,cream)
# Four ledges make a real opening below objectives.
for x in (-.064,.064):box('StageSide',(x,-.035,.194),(.035,.145,.014),dark,.002)
for y in (-.098,.028):box('StageEnd',(0,y,.194),(.094,.019,.014),dark,.002)
box('GlassSlide',(0,-.035,.203),(.075,.025,.0015),glass,.0005)
for x in (-.045,.045):
    rod('SlideClip',(x,.018,.207),(x,-.049,.207),.002,chrome)
    screw('ClipScrew',x,.018,.21,.003)
cyl('Condenser',(0,-.035,.17),.027,.023,steel)
torus('CondenserIris',(0,-.035,.158),.022,.003,dark)
rod('CondenserLever',(0,-.055,.157),(.032,-.075,.157),.002,chrome)
# Nosepiece pitched toward stage; objectives individually focused at sample.
cyl('Turret',(0,-.025,.296),.039,.015,dark)
for i,l in enumerate((.039,.048,.059)):
    a=i*math.tau/3
    x,y=.024*math.cos(a),-.025+.024*math.sin(a)
    cyl('ObjectiveBarrel',(x,y,.288-l/2),.009,l,chrome,vertices=40)
    cyl('ObjectiveBand',(x,y,.288-l*.7),.0095,.003,red if i==0 else dark,vertices=32)
    cyl('ObjectiveLens',(x,y,.288-l),.006,.003,glass,vertices=32)
rod('HeadSupport',(0,.026,.36),(0,-.016,.345),.026,cream)
for x in (-.026,.026):
    rod('EyepieceTube',(x,-.008,.35),(x,-.06,.425),.0135,dark)
    a=Vector((x,-.008,.35));b=Vector((x,-.06,.425));v=(b-a).normalized()
    rod('Eyecup',b-v*.005,b+v*.011,.018,rubber)
for x in (-.067,.067):
    cyl('FocusWheel',(x,.08,.232),.026,.019,dark,rotation=(0,math.pi/2,0))
    cyl('FineFocus',(x*1.2,.08,.232),.011,.014,chrome,rotation=(0,math.pi/2,0))
front_label('MicroscopePlate','SARO / OPTICAL',0,-.147,.031,.12,.016)
finish('SM_EW_Microscope','1986 binocular compound microscope; open stage, three objectives, condenser, focus wheels and illumination lens. Local base pivot.')

# Table centrifuge with separate lid mesh that can be split/animated by integration.
start_asset('SM_EW_Centrifuge')
feet(.30,.34)
box('EnamelLowerBody',(0,0,.087),(.30,.34,.15),cream,.028)
box('TopGasket',(0,.017,.166),(.278,.284,.009),rubber,.026)
box('HingedLid',(0,.017,.18),(.279,.285,.021),cream,.03)
for x in (-.085,.085):
    cyl('LidHinge',(x,.16,.174),.008,.044,steel,rotation=(0,math.pi/2,0))
box('LidCatch',(0,-.127,.178),(.055,.026,.012),dark,.004)
box('ControlStrip',(0,-.174,.08),(.255,.006,.091),green,.005)
knob('Timer',-.072,-.187,.083,.027,.021)
knob('Speed',.055,-.187,.083,.027,.021)
front_label('TimerLegend','MIN',-.072,-.18,.128,.048,.013)
front_label('SpeedLegend','RPM',.055,-.18,.128,.048,.013)
for x in (-.122,.122):
    for z in (.046,.115):screw('PanelScrew',x,-.18,z)
for i in range(11):box('SideCoolingSlot',(.151,-.08+i*.015,.075),(.0015,.007,.048),dark,.001)
front_label('CentrifugePlate','BENCH CENTRIFUGE',0,-.176,.028,.17,.018)
finish('SM_EW_Centrifuge','Bench centrifuge with sealed separate lid, latch, hinges, timer/speed dials, vent slots and feet. No implied gameplay animation.')

start_asset('SM_EW_LabScale')
feet(.23,.28,.008)
box('ScaleBase',(0,0,.048),(.23,.28,.071),cream,.012)
box('DialBezel',(0,-.143,.051),(.18,.012,.052),dark,.004)
box('Readout',(0,-.15,.055),(.136,.002,.031),paper,.001)
text('ScaleDigits','000.0 g',(0,-.152,.055),.022,ink)
knob('ZeroAdjust',.091,-.15,.049,.009,.012)
cyl('PanStalk',(0,.025,.10),.024,.038,steel)
cyl('WeighingPan',(0,.025,.123),.104,.01,steel,vertices=64)
torus('PanRolledLip',(0,.025,.128),.099,.003,chrome)
front_label('ScaleLabel','PRECISION BALANCE',0,-.143,.023,.13,.012)
finish('SM_EW_LabScale','Period laboratory top-pan precision balance with metal pan, zero knob and non-electronic mechanical readout face.')

# Two distinct reusable local assets: specimen jar and Erlenmeyer flask.
for capped in (True,False):
    name='SM_EW_SampleJar' if capped else 'SM_EW_ErlenmeyerFlask'
    start_asset(name)
    if capped:
        profile=[(0,.002),(.038,.002),(.041,.009),(.041,.103),(.038,.113),(.026,.122),(.026,.142),(.023,.142),(.023,.123),(.035,.111),(.038,.101),(.038,.012),(0,.012)]
        lathe('AmberBottle',profile,amber)
        cyl('ScrewCap',(0,0,.147),.029,.019,dark)
        for i in range(32):
            a=i*math.tau/32
            rod('CapRib',(.029*math.cos(a),.029*math.sin(a),.139),(.029*math.cos(a),.029*math.sin(a),.155),.0008,dark)
        curved_label('SampleLabel','SAMPLE 047',.0415,.064,.064,.03,.007)
        curved_label('SampleDate','DATE:  /  /86',.0415,.044,.057,.012,.005)
    else:
        profile=[(0,.002),(.056,.002),(.06,.012),(.057,.045),(.019,.135),(.018,.175),(.021,.179),(.021,.184),(.016,.184),(.014,.176),(.015,.135),(.053,.044),(.056,.014),(0,.014)]
        lathe('HollowBorosilicateFlask',profile,glass)
        for i in range(4):
            z=.035+i*.019; r=.06-(z-.02)*.4
            box('Graduation',(0,-r-.001,z),(.018,.001,.001),paper,0)
        front_label('FlaskMark','250 mL',0,-.056,.035,.036,.014,.007)
    finish(name,'Reusable sealed amber sample jar with ribbed cap and inventory label.' if capped else 'Hollow borosilicate Erlenmeyer flask, real inner wall and rolled rim; simple graduated markings.',[SOURCE,'unreal/ArtSource/PhotoLab/build_photolab.py:351-359 (original project bottle shoulder/cap/label construction adapted)'])

start_asset('SM_EW_LabFridge')
# Rear coil, condenser base and split seals stop a plain cuboid appearance.
for x in (-.23,.23):
    for y in (-.21,.21):cyl('AdjustableFoot',(x,y,.025),.027,.05,dark)
box('FridgeCabinet',(0,0,.84),(.62,.62,1.60),cream,.024)
box('LowerDoorSeal',(0,-.319,.653),(.585,.024,1.11),rubber,.009)
box('FreezerSeal',(0,-.319,1.414),(.585,.024,.37),rubber,.009)
box('LowerDoor',(0,-.344,.653),(.581,.043,1.103),cream,.018)
box('FreezerDoor',(0,-.344,1.414),(.581,.043,.363),cream,.017)
for z,h in ((.96,.27),(1.41,.17)):
    for zz in (z-h/2,z+h/2):rod('HandleStandoff',(.222,-.365,zz),(.222,-.408,zz),.008,chrome)
    rod('Handle',(.222,-.408,z-h/2),(.222,-.408,z+h/2),.011,chrome)
for z in (.17,1.15,1.28,1.56):box('DoorHinge',(-.295,-.319,z),(.028,.04,.048),steel,.004)
front_label('LabFridgeLabel','LAB SAMPLES ONLY',-.03,-.37,1.012,.30,.065,.018)
front_label('FridgeNumber','SARO  /  COLD 01',0,-.37,1.49,.25,.038,.011)
for i in range(10):box('LowerVent',(0,-.316,.075+i*.004),(.45,.005,.0018),dark,0)
for i in range(9):rod('CondenserCoil',(-.235,.319,.19+i*.13),(.235,.319,.19+i*.13),.005,dark)
for x in (-.235,.235):rod('CoilUpright',(x,.319,.19),(x,.319,1.36),.005,dark)
finish('SM_EW_LabFridge','1980s separate freezer/lower-door laboratory refrigerator with seals, chrome handles, rear condenser and sample-only label.')

start_asset('SM_EW_Multimeter')
box('BakeliteMeterBody',(0,0,.045),(.12,.17,.09),dark,.011)
# Bench-facing instrument front vertical for readable gameplay views.
box('CreamFrontPanel',(0,-.087,.047),(.108,.007,.078),cream,.005)
box('MeterWindow',(0,-.093,.064),(.088,.003,.036),paper,.003)
# Arc marks and a physical needle on front Y plane.
for i in range(13):
    a=math.pi*.18+i*math.pi*.64/12
    x,z=.034*math.cos(a),.043+.028*math.sin(a)
    rod('MeterTick',(x,-.096,z),(x*.92,-.096,.043+(z-.043)*.92),.0004,ink)
rod('MeterNeedle',(0,-.097,.043),(.012,-.097,.071),.0006,red)
knob('RangeSelector',0,-.099,.022,.014,.014)
for x,mat in ((-.037,dark),(.037,red)):
    cyl('LeadSocket',(x,-.094,.022),.005,.004,mat,rotation=(math.pi/2,0,0))
# Coiled insulated leads and pointed probes alongside meter.
for side,mat in ((-1,dark),(1,red)):
    for i in range(3):torus('TestLeadCoil',(side*.087,.02,.006+i*.003),.028,.0016,mat)
    rod('ProbeHandle',(side*.085,-.061,.008),(side*.085,.002,.008),.004,mat)
    rod('ProbeTip',(side*.085,-.082,.008),(side*.085,-.061,.008),.0012,chrome)
finish('SM_EW_Multimeter','Analog portable multimeter with scale ticks, real needle/range selector, sockets, two coiled leads and probe tips.')

start_asset('SM_EW_SolderingIron')
box('StandBase',(0,0,.008),(.15,.18,.016),dark,.008)
box('SpongeTray',(0,-.055,.02),(.115,.055,.014),steel,.005)
box('CleaningSponge',(0,-.055,.029),(.095,.038,.007),paper,.003)
# Spring holder inclined, continuous helical mesh via narrow rods.
a=Vector((0,.04,.03));b=Vector((0,.09,.12));direction=(b-a).normalized()
u=Vector((1,0,0));v=direction.cross(u).normalized()
curve=bpy.data.curves.new('HolderSpring','CURVE');curve.dimensions='3D'
curve.bevel_depth=.0016;curve.bevel_resolution=2;curve.resolution_u=1
spline=curve.splines.new('POLY');spline.points.add(240)
for i in range(241):
    f=i/240;ang=f*math.tau*9
    p=a+(b-a)*f+(u*math.cos(ang)+v*math.sin(ang))*.014
    spline.points[i].co=(*p,1)
ob=bpy.data.objects.new('HolderSpring',curve);bpy.context.collection.objects.link(ob)
curve.materials.append(steel)
rod('IronShaft',(0,.045,.045),(0,.12,.18),.0055,steel)
rod('IronGrip',(0,.10,.144),(0,.145,.225),.012,dark)
rod('IronTip',(0,.025,.008),(0,.045,.045),.002,chrome)
for i in range(4):torus('PowerCordCoil',(.1,.035,.003+i*.003),.036,.002,dark)
box('Plug',(.1,-.021,.007),(.019,.029,.014),dark,.003)
for x in (.095,.105):box('PlugPin',(x,-.04,.007),(.002,.012,.004),steel,.0005)
finish('SM_EW_SolderingIron','Soldering iron in formed spring holder, cleaning sponge tray, coiled cord and two-pin plug.')


def import_reused_scope():
    """Keep CC-BY scope source/derived binaries private, manifest in delivery."""
    name='SM_EW_Oscilloscope_RM565'
    PRIVATE.mkdir(parents=True,exist_ok=True)
    extraction=PRIVATE/'source_rm565'
    extraction.mkdir(exist_ok=True)
    with zipfile.ZipFile(SCOPE_ZIP) as z:
        for item in z.infolist():
            dest=(extraction/item.filename).resolve()
            if not dest.is_relative_to(extraction.resolve()):raise ValueError('Unsafe archive path')
        z.extractall(extraction)
    start_asset(name)
    before=set(bpy.data.objects)
    bpy.ops.import_scene.gltf(filepath=str(extraction/'scene.gltf'))
    imported=[o for o in bpy.data.objects if o not in before and o.type=='MESH']
    if not imported:raise RuntimeError('Scope archive contains no mesh')
    # Flatten exporter parents to world mesh positions before measured scale.
    for o in imported:
        world=o.matrix_world.copy();o.parent=None;o.matrix_world=world
    points=[o.matrix_world@Vector(c) for o in imported for c in o.bound_box]
    lo=Vector(tuple(min(p[k] for p in points) for k in range(3)))
    hi=Vector(tuple(max(p[k] for p in points) for k in range(3)))
    width=hi.x-lo.x
    factor=.60/width
    center=Vector(((lo.x+hi.x)/2,(lo.y+hi.y)/2,lo.z))
    for o in imported:
        # Convert mesh world transform into grounded local metric positions.
        m=o.matrix_world.copy()
        for vert in o.data.vertices:vert.co=(m@vert.co-center)*factor
        o.matrix_world.identity()
    # Keep original PBR channels and UVs, copy all external texture paths private.
    tex=PRIVATE/'textures';tex.mkdir(exist_ok=True)
    for im in bpy.data.images:
        if not im.filepath:continue
        p=Path(bpy.path.abspath(im.filepath))
        if p.is_file() and p.is_relative_to(extraction):
            target=tex/(p.stem+'.png')
            if tuple(im.size)!=(2048,2048):im.scale(2048,2048)
            im.filepath_raw=str(target);im.file_format='PNG';im.save()
            im.filepath=str(target)
    desc='Reused Tektronix RM565 oscilloscope by Thusko; CC-BY-4.0. Original UV/PBR channel roles retained, textures resized to 2K, geometry resized to 0.60m width and grounded. Private licensed binaries.'
    finish_asset(name,PRIVATE,desc,[str(SCOPE_ZIP),
        'https://sketchfab.com/3d-models/tektronix-rm565-oscilloscope-b5b047d3ffec4fe2b98659d056c0f414'],license_id='CC-BY-4.0')
    shutil.copy2(extraction/'license.txt',PRIVATE/'RM565-LICENSE.txt')
    manifest={'asset':name,'license':'CC-BY-4.0','author':'Thusko',
      'source':'https://sketchfab.com/3d-models/tektronix-rm565-oscilloscope-b5b047d3ffec4fe2b98659d056c0f414',
      'source_archive':str(SCOPE_ZIP),'license_evidence':str(PRIVATE/'RM565-LICENSE.txt'),
      'private_delivery_directory':str(PRIVATE),'public_binary':False,
      'changes':['Metric scale 0.60m width','Base-centred pivot','FBX export; original UV preserved; texture copies reduced to 2048x2048 PNG'],
      'attribution':'Tektronix RM565 Oscilloscope by Thusko, CC-BY-4.0, https://sketchfab.com/3d-models/tektronix-rm565-oscilloscope-b5b047d3ffec4fe2b98659d056c0f414',
      'integration_note':'Copy/import from private delivery; do not stage licensed derived FBX or textures in Git.'}
    (HERE/(name+'.license.json')).write_text(json.dumps(manifest,indent=2)+'\n')

import_reused_scope()
render_catalog(OUT,'EASTWING / LAB EQUIPMENT')
print('EASTWING LAB EXPORT COMPLETE',flush=True)
