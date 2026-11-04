"""Original 150x120x45mm sample carrier and 420mm checkout desk sign.
Run make_sample_station_prints.py first, then existing Props/run_guarded.py.
Only these two assets are generated; delivery manifests/import/runtime are root-owned.
Default remains the original two assets. S47_SAMPLE_STATION_ONLY selects a comma-
separated subset, including CalibrationSign, SampleAnalysisSign and ReceivingSign
(use full SM_EW_ names). Set the same filter for make_sample_station_prints.py.
Holder pivot is its exact bounding centre, matching the former runtime box.
Model() places mesh bottom on the bench: At=SampleAt+(.03,.9325,.05), Width=.15.
Desk sign is base-centred; Model At=DepotAt+(0,.9245,0), Width=.42.
Blender metres Z-up, front -Y (= game +Z), existing prop_common FBX/LOD/UCX.
"""
from pathlib import Path
import math,os,sys,json
import bpy
HERE=Path(__file__).resolve().parent
sys.path.insert(0,str(HERE.parent))
from prop_common import material,box,cyl,rod,start_asset,finish_asset,render_catalog
OUT=Path(os.environ.get('S47_SAMPLE_STATION_OUT',str(HERE))).resolve()
PRINTS=OUT/'textures'
steel=material('EW_X_Stainless',(.48,.51,.5),.24,.85,texture='EW_Galvanized')
green=material('EW_X_GreenEnamel',(.16,.24,.20),.33,texture='EW_GreenSteel')
dark=material('EW_X_Bakelite',(.017,.02,.019),.39)
rubber=material('EW_X_Rubber',(.022,.025,.023),.7)
stone=material('EW_SampleStation_Fragment',(.28,.24,.19),.93)


def printed(name):
    mat=bpy.data.materials.new(name);mat.use_nodes=True
    shader=mat.node_tree.nodes.get('Principled BSDF');shader.inputs['Roughness'].default_value=.82
    image=bpy.data.images.load(str(PRINTS/(name+'_BaseColor.png')),check_existing=True);image.colorspace_settings.name='sRGB'
    node=mat.node_tree.nodes.new('ShaderNodeTexImage');node.image=image
    mat.node_tree.links.new(node.outputs['Color'],shader.inputs['Base Color'])
    return mat


def quad(name,points,mat):
    mesh=bpy.data.meshes.new(name);mesh.from_pydata(points,[],[(0,1,2,3)]);mesh.update()
    uv=mesh.uv_layers.new(name='UVMap')
    for i,p in enumerate(((0,0),(1,0),(1,1),(0,1))):uv.data[i].uv=p
    obj=bpy.data.objects.new(name,mesh);bpy.context.collection.objects.link(obj);mesh.materials.append(mat)
    return obj


def holder():
    name='SM_EW_SampleHolder';start_asset(name)
    # Full-size base fixes exact footprint and lower pivot boundary.
    box('CarrierBase',(0,0,-.0185),(.15,.12,.008),green,.001)
    # Real open recess: four raised sides, no box or dark decal across the cavity.
    box('RecessFloor',(0,0,-.0135),(.136,.103,.002),steel,.0005)
    for side in (-1,1):
        box(f'TraySide{side}',(side*.071,0,-.006),(.008,.112,.017),steel,.001)
        box(f'TrayEnd{side}',(0,side*.054,-.006),(.136,.008,.017),steel,.001)
    # Recessed removable sample puck, thin elastomer pad and three retaining tabs.
    cyl('Puck',(0,.012,-.007),.025,.010,steel,vertices=32)
    cyl('PuckPad',(0,.012,-.001),.021,.002,rubber,vertices=32)
    for angle in (0,2*math.pi/3,4*math.pi/3):
        x,y=.026*math.cos(angle),.012+.026*math.sin(angle)
        tab=box('RetainingTab',(x,y,.001),(.017,.006,.004),steel,.0006);tab.rotation_euler.z=angle
        cyl('RetainingScrew',(x,y,.0035),.0025,.001,steel,vertices=12)
        box('ScrewSlot',(x,y,.00405),(.0036,.00055,.00015),dark,0)
    # Authored irregular specimen; the entire holder follows existing fragment visibility.
    vertices=[(-.013,-.001,0),(.014,-.002,0),(.018,.014,.002),(.004,.029,.001),
              (-.014,.021,.001),(-.010,.007,.015),(.006,.005,.0225),(.010,.019,.013),(-.006,.024,.011)]
    faces=[(0,4,3,2,1),(0,1,6,5),(1,2,7,6),(2,3,8,7),(3,4,8),(4,0,5,8),(5,6,7),(5,7,8)]
    mesh=bpy.data.meshes.new('FieldFragment');mesh.from_pydata(vertices,[],faces);mesh.update()
    obj=bpy.data.objects.new('RetainedFieldFragment',mesh);bpy.context.collection.objects.link(obj);mesh.materials.append(stone)
    # Top-mounted paper index lies inside the tray and never overlaps the puck.
    box('IndexBacking',(0,-.035,-.009),(.104,.021,.004),dark,.0005)
    quad('SampleIndex',[(-.05,-.044,-.0069),(.05,-.044,-.0069),(.05,-.026,-.0069),(-.05,-.026,-.0069)],printed('EW_Print_SampleIndex'))
    for x in (-.060,.060):
        for y in (-.044,.044):
            cyl('TrayFastener',(x,y,-.012),.0022,.001,steel,vertices=12)
    finish_asset(name,OUT,'150 x 120 x 45 mm field sample carrier; centred pivot; open steel tray, removable puck, retaining tabs and original fragment. Inventory-controlled visibility.')
    row=json.loads((OUT/(name+'.asset.json')).read_text())
    expected=[[-.075,-.06,-.0225],[.075,.06,.0225]]
    assert max(abs(a-b) for p,q in zip(row['bounds_m'],expected) for a,b in zip(p,q))<.00001,row['bounds_m']


SIGNS={
    'SM_EW_DepotCheckoutSign':('EW_Print_DepotCheckout','PARTS STORAGE / EQUIPMENT CHECKOUT'),
    'SM_EW_CalibrationSign':('EW_Print_Calibration','CALIBRATION / REPAIR / DONOR / LOGGER TEST'),
    'SM_EW_SampleAnalysisSign':('EW_Print_SampleAnalysis','GENERAL TEST LAB / FIELD SAMPLE ANALYSIS'),
    'SM_EW_ReceivingSign':('EW_Print_Receiving','GARAGE RECEIVING / UNPACK CARRIED SUPPLIES'),
}


def desk_sign(name):
    print_name,description=SIGNS[name]
    start_asset(name)
    box('DeskFoot',(0,0,.004),(.42,.10,.008),green,.002)
    angle=-math.atan(.035/.13)
    panel=box('InclinedEnamelPanel',(0,-.0125,.09),(.408,.004,.146),green,.0015)
    panel.rotation_euler.x=angle
    # Face runs from y=-.030,z=.025 to y=.005,z=.155, leaned back on its stand.
    quad('CheckoutPrint',[(-.196,-.032,.025),(.196,-.032,.025),(.196,.003,.155),(-.196,.003,.155)],printed(print_name))
    for x in (-.16,.16):
        rod('RearBrace',(x,.035,.008),(x,.012,.14),.004,steel)
        cyl('FootScrew',(x,.032,.009),.0035,.002,steel,vertices=16)
    finish_asset(name,OUT,'420 mm counter sign: '+description+'. Inclined enamel panel, printed face, metal foot and braces. Base-centred origin; front -Y.')


selection=os.environ.get('S47_SAMPLE_STATION_ONLY','SM_EW_SampleHolder,SM_EW_DepotCheckoutSign')
selected={name.strip() for name in selection.split(',') if name.strip()}
assert selected and selected<=set(SIGNS)|{'SM_EW_SampleHolder'},('Unknown/empty sample station asset filter',selection)
if 'SM_EW_SampleHolder' in selected:holder()
for name in SIGNS:
    if name in selected:desk_sign(name)
if os.environ.get('S47_SAMPLE_STATION_PREVIEWS')=='1':render_catalog(OUT,'SARO sample carrier / equipment checkout')
print('SAMPLE_STATION_EXPORT_COMPLETE',OUT,flush=True)
