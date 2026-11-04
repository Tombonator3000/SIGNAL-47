"""EastWing authored shell layout in the existing three.js gameplay metre frame.

Pure data, safe to inspect without Blender. Primary wall/opening coordinates are
the 3 November native blockout; decoration never changes a gameplay anchor.
"""
X0, X1, Z0, Z1 = 20.6, 38.6, -5.5, 5.5
T, H, CEILING = .25, 3.6, 3.32
LAB_X, GARAGE_X, STORE_Z = 26., 31.6, .5
SOUTH_GAPS = [(21.3, 22.4, 0., 2.15), (33.2, 37.6, 0., 3.)]
EAST_GAPS = [(1.2, 2.3, 0., 2.15)]
NORTH_GAPS = [(21.4, 23., 1., 2.25), (23.6, 25.2, 1., 2.25),
              (27., 28.6, 1., 2.25), (29.2, 30.8, 1., 2.25),
              (33., 34.6, 1.6, 2.4), (35.6, 37.2, 1.6, 2.4)]
INTERIOR_GAPS = [('Lab', 'z', LAB_X, 2.4, 3.4),
                 ('GarageNorth', 'z', GARAGE_X, -3., -2.),
                 ('GarageSouth', 'z', GARAGE_X, 2.9, 3.9),
                 ('Storage', 'x', STORE_Z, 28.3, 29.3)]
ANCHORS = {
    'SM_EW_PersonnelDoor': [21.3, 0., 5.5],
    'SM_EW_RollUpDoor': [35.4, 3., 5.5],
    'SM_EW_ServiceDoor': [38.6, 0., 1.2],
}
ASSETS = ['SM_EW_Shell', 'SM_EW_Ceiling', 'SM_EW_Trim', 'SM_EW_WindowGlass',
          'SM_EW_SignBoard', 'SM_EW_PersonnelDoor', 'SM_EW_RollUpDoor', 'SM_EW_ServiceDoor']


def build(lod=0):
    """Editable primitives; authored LODs reduce bevel detail, not wall boundaries."""
    rows = []

    def box(asset, name, lo, hi, material, bevel=.004, collide=False, **extra):
        assert all(hi[k] > lo[k] for k in range(3)), (name, lo, hi)
        row = dict(asset=asset, name=name, kind='box', lo=list(lo), hi=list(hi),
                   material=material, bevel=bevel, collide=collide)
        row.update(extra)
        rows.append(row)

    def tube(name, start, end, radius=.012, material='Conduit'):
        rows.append(dict(asset='SM_EW_Trim', name=name, kind='tube', start=list(start),
                         end=list(end), radius=radius, material=material, collide=False))

    def wall(name, axis, fixed, a, b, bottom, top, gaps=(), thickness=T,
             inward=1, exterior=False):
        f0, f1 = sorted((fixed, fixed+inward*thickness))

        def piece(s0, s1, y0, y1, suffix):
            if s1-s0 < .001 or y1-y0 < .001:
                return
            lo, hi = ((s0, y0, f0), (s1, y1, f1)) if axis == 'x' else ((f0, y0, s0), (f1, y1, s1))
            box('SM_EW_Shell', name+suffix, lo, hi, 'Masonry', .003, True,
                exterior_axis=(2 if axis == 'x' else 0) if exterior else None,
                exterior_sign=-inward if exterior else None)
        current = a
        for index, (ga, gb, gy0, gy1) in enumerate(gaps):
            piece(current, ga, bottom, top, f'_solid{index}')
            piece(ga, gb, bottom, gy0, f'_below{index}')
            piece(ga, gb, gy1, top, f'_above{index}')
            current = gb
        piece(current, b, bottom, top, '_end')

    wall('South', 'x', Z1, X0, X1, 0, H, SOUTH_GAPS, inward=-1, exterior=True)
    wall('North', 'x', Z0, X0, X1, 0, H, NORTH_GAPS, exterior=True)
    wall('West', 'z', X0, Z0, Z1, 0, H, exterior=True)
    wall('East', 'z', X1, Z0, Z1, 0, H, EAST_GAPS, inward=-1, exterior=True)
    wall('LabPartition', 'z', LAB_X-.075, Z0+T, Z1-T, 0, CEILING,
         [(2.4, 3.4, 0, 2.15)], thickness=.15)
    wall('GaragePartition', 'z', GARAGE_X-.075, Z0+T, Z1-T, 0, CEILING,
         [(-3., -2., 0, 2.15), (2.9, 3.9, 0, 2.15)], thickness=.15)
    wall('StorePartition', 'x', STORE_Z-.075, LAB_X, GARAGE_X, 0, CEILING,
         [(28.3, 29.3, 0, 2.15)], thickness=.15)
    # Retain the established base and floor planes; no replacement roof above 3.6m.
    for name, lo, hi in [
        ('PlinthSouth', (X0,-.6,Z1-.02),(X1,0,Z1+.06)),
        ('PlinthNorth', (X0,-.6,Z0-.06),(X1,0,Z0+.02)),
        ('PlinthWest', (X0-.06,-.6,Z0),(X0+.02,0,Z1)),
        ('PlinthEast', (X1-.02,-.6,Z0),(X1+.06,0,Z1))]:
        box('SM_EW_Shell',name,lo,hi,'Plinth',.008,True)
    box('SM_EW_Shell','Floor',(X0+T,-.08,Z0+T),(GARAGE_X,0,Z1-T),'Concrete',.001,True)
    # Four slabs leave the 0.50 x 0.30 m grate aperture genuinely open.
    # Props places the existing grate at (35.4,-.0535,.20), with its top at zero.
    for name,xa,xb,za,zb in [
        ('West',GARAGE_X,35.15,Z0+T,Z1-T),
        ('East',35.65,X1-T,Z0+T,Z1-T),
        ('North',35.15,35.65,Z0+T,.05),
        ('South',35.15,35.65,.35,Z1-T)]:
        box('SM_EW_Shell','GarageFloor'+name,(xa,-.12,za),(xb,0,zb),'Concrete',0,True)
    box('SM_EW_Shell','DrainRecessBottom',(35.15,-.13,.05),(35.65,-.12,.35),'Scuff',0,True)
    # Thin paint geometry clears the floor depth buffer, with no collision lip.
    for x in (33.6,37.2):
        box('SM_EW_Trim',f'GarageParkingStripe{x}',(x-.05,.001,-4.5),
            (x+.05,.002,5.25),'Yellow',0)

    # Visible ramps follow AS47Props::AddFloor's exact interpolation, not a raised flat slab.
    for name, xa, xb, length in [('GarageRamp',33.2,37.6,4.)]:
        rows.append(dict(asset='SM_EW_Shell',name=name,kind='ramp',x0=xa,x1=xb,z0=Z1,z1=Z1+length,
                         y0=0.,y1=-.576,bottom=-.65,material='Concrete',collide=True))
    # Claude (Tom 4 Nov): poured concrete steps instead of ramps at the personnel and service doors.
    # 0.45 m landing at floor level, then three 0.192 m risers with 0.30 m treads down to the apron
    # (-0.576). AS47Props walking floors in S47EastWing::Walls follow the same landing + flight.
    STEP_LANDING, STEP_TREAD, STEP_RISE = .45, .30, .192
    def steps(name, axis, edge, a, b):
        # axis 'z': flight runs from z=edge toward +z (south door); axis 'x': from x=edge toward +x (east door).
        tops = [0., -STEP_RISE, -2 * STEP_RISE]
        ends = [STEP_LANDING, STEP_LANDING + STEP_TREAD, STEP_LANDING + 2 * STEP_TREAD]
        start = 0.
        for k, (top, end) in enumerate(zip(tops, ends)):
            if axis == 'z':
                box('SM_EW_Shell', f'{name}{k}', (a, -.65, edge + start), (b, top, edge + end), 'Concrete', .012, True)
            else:
                box('SM_EW_Shell', f'{name}{k}', (edge + start, -.65, a), (edge + end, top, b), 'Concrete', .012, True)
            start = end
    steps('PersonnelSteps', 'z', Z1, 21., 22.7)
    steps('ServiceSteps', 'x', X1, 1., 2.5)
    box('SM_EW_Ceiling','Ceiling',(X0+T,CEILING,Z0+T),(X1-T,CEILING+.06,Z1-T),'Ceiling',.002)
    for index, x in enumerate([21.,23.5,26.,28.5,31.6,34.,36.4,38.2]):
        box('SM_EW_Ceiling',f'RoofRib{index}',(x-.035,3.19,Z0+T),(x+.035,CEILING,Z1-T),'CeilingSteel',.003)

    def frame(name, axis, fixed, a, b, y0, y1, depth=.10, width=.055, sill=True):
        # Frame outside the nominal aperture: preserve the blockout's clear width.
        f0, f1 = fixed-depth*.5, fixed+depth*.5
        def part(suffix, s0,s1,h0,h1):
            lo,hi=((s0,h0,f0),(s1,h1,f1)) if axis=='x' else ((f0,h0,s0),(f1,h1,s1))
            box('SM_EW_Trim',name+suffix,lo,hi,'GreenTrim',.003)
        part('Left',a-width,a,y0,y1+width)
        part('Right',b,b+width,y0,y1+width)
        part('Head',a,b,y1,y1+width)
        if sill:part('Sill',a-width,b+width,y0-width,y0)

    for i,(a,b,y0,y1) in enumerate(NORTH_GAPS):
        frame(f'NorthWindow{i}','x',Z0+.10,a,b,y0,y1,depth=.19)
        mid=(a+b)*.5
        box('SM_EW_Trim',f'WindowMullion{i}',(mid-.022,y0,Z0+.025),(mid+.022,y1,Z0+.17),'GreenTrim',.002)
        box('SM_EW_Trim',f'WindowTransom{i}',(a,y0+.38,Z0+.03),(b,y0+.42,Z0+.16),'GreenTrim',.002)
        for side,(left,right) in enumerate([(a+.012,mid-.025),(mid+.025,b-.012)]):
            box('SM_EW_WindowGlass',f'NorthPane{i}_{side}',(left,y0+.012,Z0+.095),(right,y1-.012,Z0+.101),'Glass',0)
        box('SM_EW_Trim',f'WindowStoneSill{i}',(a-.08,y0-.055,Z0-.055),(b+.08,y0-.005,Z0+.33),'Plinth',.005)
        # Upper-half venetian blinds, with an open lower view as in the references.
        if i<4:
            box('SM_EW_Trim',f'BlindHead{i}',(a,y1-.025,Z0+.29),(b,y1+.025,Z0+.35),'BlindWood',.004)
            for slat in range(9):
                y=y1-.08-slat*.065
                box('SM_EW_Trim',f'Blind{i}_{slat}',(a+.015,y-.005,Z0+.29),(b-.015,y+.005,Z0+.34),'BlindWood',.001 if lod==0 else 0)
            for x in (a+.18,b-.18):
                tube(f'BlindCord{i}_{x}',(x,y1-.6,Z0+.335),(x,y1-.04,Z0+.335),.002,'BlindCord')
    frame('PersonnelFrame','x',Z1-.10,21.3,22.4,0,2.15,depth=.30,sill=False)
    frame('GarageFrame','x',Z1-.10,33.2,37.6,0,3.,depth=.30,width=.075,sill=False)
    frame('ServiceFrame','z',X1-.10,1.2,2.3,0,2.15,depth=.30,sill=False)
    for name,axis,fixed,a,b in INTERIOR_GAPS:
        frame(name+'Frame',axis,fixed,a,b,0,2.15,depth=.20,sill=False)

    # Lower wall scuff strips stop at every door opening.
    for name,axis,fixed,a,b,gaps in [
        ('SouthSkirt','x',Z1-T-.018,X0+T,X1-T,[(21.3,22.4),(33.2,37.6)]),
        ('NorthSkirt','x',Z0+T+.018,X0+T,X1-T,[]),
        ('WestSkirt','z',X0+T+.018,Z0+T,Z1-T,[]),
        ('EastSkirt','z',X1-T-.018,Z0+T,Z1-T,[(1.2,2.3)])]:
        at=a
        for index,(ga,gb) in enumerate(gaps+[(b,b)]):
            if ga>at:
                lo,hi=((at,.01,fixed-.018),(ga,.115,fixed+.018)) if axis=='x' else ((fixed-.018,.01,at),(fixed+.018,.115,ga))
                box('SM_EW_Trim',name+str(index),lo,hi,'Scuff',.002)
            at=gb
    # Industrial electrical distribution below the ceiling, with junction boxes/clamps.
    for z in (Z0+.32,Z1-.32):
        for y in (2.83,2.91):tube(f'Conduit_{z}_{y}',(X0+.32,y,z),(X1-.32,y,z))
        for x in (21.,24.,27.,30.,33.,36.,38.):
            box('SM_EW_Trim',f'Junction_{z}_{x}',(x-.06,2.79,z-.035),(x+.06,2.94,z+.035),'Conduit',.004)
    for x,z in [(21.02,5.16),(25.82,4.9),(31.78,4.9),(38.27,4.8),(26.2,-5.15)]:
        tube(f'ConduitDrop_{x}',(x,.95,z),(x,2.87,z),.010)
        box('SM_EW_Trim',f'Outlet_{x}',(x-.045,.89,z-.032),(x+.045,1.02,z+.032),'Conduit',.004)
        if lod<2:
            for y in (1.32,2.12,2.67):
                box('SM_EW_Trim',f'ConduitClamp_{x}_{y}',(x-.023,y-.012,z-.02),(x+.023,y+.012,z+.023),'Conduit',.002)
    # South awning and exposed brackets, keeping its original placement.
    box('SM_EW_Trim','Awning',(21.,2.35,Z1),(22.7,2.42,Z1+.9),'GreenSteel',.004)
    for x in (21.10,22.60):
        tube(f'AwningBrace_{x}',(x,2.0,Z1+.025),(x,2.35,Z1+.78),.016,'GreenTrim')
    for i in range(7):
        x=21.08+i*.25
        box('SM_EW_Trim',f'AwningSeam{i}',(x-.009,2.42,Z1),(x+.009,2.435,Z1+.90),'GreenTrim',.002)
    # Header coil/guide hardware is fixed; the moving curtain is exported separately.
    box('SM_EW_Trim','RollUpHeader',(33.10,3.,Z1-.02),(37.70,3.35,Z1+.25),'Galvanized',.008)
    for x in (33.13,37.67):
        box('SM_EW_Trim',f'RollTrack{x}',(x-.035,.02,Z1-.21),(x+.035,3.21,Z1-.11),'Galvanized',.003)
    for x in (32.7,38.1):
        rows.append(dict(asset='SM_EW_Trim',name=f'Bollard{x}',kind='tube',
                         start=[x,-.58,Z1+.21],end=[x,.55,Z1+.21],radius=.09,
                         material='Yellow',collide=True))

    box('SM_EW_SignBoard','SignBack',(23.4,1.05,Z1),(30.4,2.75,Z1+.08),'GreenTrim',.008)
    box('SM_EW_SignBoard','SignFace',(23.46,1.11,Z1+.08),(30.34,2.69,Z1+.084),'SignFace',0,sign_uv=True)
    for y in (1.075,2.725):box('SM_EW_SignBoard',f'SignBorder{y}',(23.42,y-.012,Z1+.08),(30.38,y+.012,Z1+.095),'Galvanized',.001)
    for x in (23.425,30.375):box('SM_EW_SignBoard',f'SignBorder{x}',(x-.012,1.075,Z1+.08),(x+.012,2.725,Z1+.095),'Galvanized',.001)
    # Separate green door leaf, with a real glazed aperture and kick plate.
    door='SM_EW_PersonnelDoor'
    for name,lo,hi in [
        ('DoorLower',(21.35,.02,5.44),(22.35,1.10,5.48)),
        ('DoorTop',(21.35,1.88,5.44),(22.35,2.12,5.48)),
        ('DoorLeft',(21.35,1.10,5.44),(21.65,1.88,5.48)),
        ('DoorRight',(22.05,1.10,5.44),(22.35,1.88,5.48))]:
        box(door,name,lo,hi,'GreenSteel',.003,True)
    box(door,'DoorGlass',(21.66,1.11,5.455),(22.04,1.87,5.465),'Glass',0)
    for z in (5.432,5.488):
        box(door,f'DoorKick{z}',(21.37,.04,z-.003),(22.33,.30,z+.003),'Galvanized',.002)
        box(door,f'DoorHandlePlate{z}',(22.18,.88,z-.005),(22.26,1.08,z+.005),'Galvanized',.003)
        box(door,f'DoorLever{z}',(22.07,.96,z-.035),(22.24,.985,z+.035),'Galvanized',.008)
    for y in (.25,1.02,1.82):
        rows.append(dict(asset=door,name=f'DoorHinge{y}',kind='tube',start=[21.34,y,5.465],
                         end=[21.34,y+.09,5.465],radius=.013,material='Galvanized',collide=False))
    # Keep the runtime anchor on the actual barrel axis. The closed leaf sits
    # just outside the facade so its +90-degree swing clears the inside jamb.
    for row in rows:
        if row['asset']!=door:continue
        if row['name'].startswith('DoorHinge'):
            for key in ('start','end'):
                row[key][0]=ANCHORS[door][0]
                row[key][2]=ANCHORS[door][2]
        else:
            for key in ('lo','hi'):
                if key in row:row[key][2]+=.065
    for y in (.25,1.02,1.82):
        box(door,f'DoorHingeLeaf{y}',(21.3,y,5.5),(21.39,y+.09,5.508),'Galvanized',.001)
    # The requested service entry is a new aperture, clear of the retained meter at z3.4.
    # Reuse the same detailed leaf at the east wall; its own hinge anchor is explicit.
    for original in list(rows):
        if original['asset']!=door:continue
        service=dict(original,asset='SM_EW_ServiceDoor',name='Service'+original['name'])
        def relocate(point):return [X1+point[2]-Z1,point[1],1.2+point[0]-21.3]
        for key in ('lo','hi','start','end'):
            if key in original:service[key]=relocate(original[key])
        rows.append(service)
    # Closed curtain retains every slat in the editable source for a later roll-up rig.
    for i in range(20):
        y=.012+i*.1485
        box('SM_EW_RollUpDoor',f'RollSlat{i:02}',(33.215,y,5.41),(37.585,y+.146,5.475),'Galvanized',.004,True)
        box('SM_EW_RollUpDoor',f'RollRib{i:02}',(33.23,y+.055,5.475),(37.57,y+.083,5.491),'Galvanized',.004)
    box('SM_EW_RollUpDoor','BottomSeal',(33.215,.002,5.405),(37.585,.025,5.48),'Scuff',.003)
    return rows


def validate():
    rows=build()
    assert set(r['asset'] for r in rows)==set(ASSETS)
    assert len({(r['asset'],r['name']) for r in rows})==len(rows)
    # Collision boxes must leave the exact native walk apertures unobstructed above floor level.
    probes=[(21.85,1.,5.38),(35.4,1.5,5.38),(26.,1.,2.9),
            (31.6,1.,-2.5),(31.6,1.,3.4),(28.8,1.,.5),(38.475,1.,1.75)]
    for point in probes:
        for r in rows:
            if r['asset']=='SM_EW_Shell' and r.get('collide') and r['kind']=='box':
                assert not all(r['lo'][i]<point[i]<r['hi'][i] for i in range(3)),(point,r['name'])
    clearances=[((21.3,0.,5.25),(22.4,2.15,5.5)),((33.2,0.,5.25),(37.6,3.,5.5)),
                ((25.925,0.,2.4),(26.075,2.15,3.4)),((31.525,0.,-3.),(31.675,2.15,-2.)),
                ((31.525,0.,2.9),(31.675,2.15,3.9)),((28.3,0.,.425),(29.3,2.15,.575)),
                ((38.35,0.,1.2),(38.6,2.15,2.3))]
    for lo,hi in clearances:
        for row in rows:
            if row['asset']=='SM_EW_Shell' and row.get('collide') and row['kind']=='box':
                assert not all(min(hi[i],row['hi'][i])-max(lo[i],row['lo'][i])>1e-8 for i in range(3)),row['name']
    return {'assets':len(ASSETS),'editable_parts':len(rows),'clear_aperture_probes':len(probes),
            'clear_aperture_volumes':len(clearances),'footprint_three_m':[X0,X1,Z0,Z1], 'ceiling_y_m':CEILING}


if __name__=='__main__':
    import json
    print(json.dumps(validate(),indent=2))
