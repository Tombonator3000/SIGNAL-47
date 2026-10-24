"""Original Caliche geology design. Pure Python, metre units, no Blender needed.

Road bearing/path is reproduced from web/src/drive/roadShape.ts and
oldRoadLayout.ts (4m midpoint integration), including the crossing-relative
backward construction and translation to the diner. No imported terrain data.
"""
import math
from functools import lru_cache
ANCHOR=[8000,0,2060]
NAME='SM_Caliche_Mesas'
MATERIALS=['M_Caliche_Mesa_'+str(i) for i in range(6)]
ROAD_BUFFER=180.0
MAIN_BUFFER=180.0
TOWN_BOX=[-180,180,-500,450]
BUDGET=125000

def smooth(a,b,x):
    t=max(0.0,min(1.0,(x-a)/(b-a)));return t*t*(3-2*t)
def hash1(x,seed):
    v=math.sin(x*127.1+seed*311.7)*43758.5453123;return v-math.floor(v)
def noise1(x,seed):
    i=math.floor(x);t=x-i;t=t*t*(3-2*t)
    return hash1(i,seed)*(1-t)+hash1(i+1,seed)*t

def bearing(mi):
    if mi<5.6:
        lead=63+27*(1-smooth(.02,.3,mi));m=mi-.3
        return lead+(8*math.sin(m*2.3)+4*math.sin(m*5.1+1))*smooth(.3,1,mi)*(1-smooth(4.6,5.5,mi))
    if mi<6.75:return 63+(45-63)*smooth(6.25,6.75,mi)
    if mi<7.75:return 45+(35-45)*smooth(7.25,7.75,mi)
    return 35+(27-35)*smooth(8.5,8.9,mi)

def old_road():
    mile=1609.34;cross=8.15;s0=-cross*mile;x=z=0.;back=[];s=-4.
    while s>=s0:
        a=math.radians(bearing(cross+(s+2)/mile));x-=math.sin(a)*4;z+=math.cos(a)*4
        back.append((x,z));s-=4
    path=list(reversed(back))+[(0.,0.)];jx,jz=path[0]
    # Only first4.5km can approach this backdrop's projected footprint.
    return [(x-jx,z-jz+1999.95-2060) for x,z in path[:1127]]
OLD_ROAD=old_road()
ROAD_BINS={};CELL=220.
for i,(a,b) in enumerate(zip(OLD_ROAD,OLD_ROAD[1:])):
    for ix in range(math.floor(min(a[0],b[0])/CELL)-2,math.floor(max(a[0],b[0])/CELL)+3):
        for iz in range(math.floor(min(a[1],b[1])/CELL)-2,math.floor(max(a[1],b[1])/CELL)+3):
            ROAD_BINS.setdefault((ix,iz),[]).append(i)

@lru_cache(maxsize=160000)
def old_road_distance(x,z):
    best=math.inf
    for i in ROAD_BINS.get((math.floor(x/CELL),math.floor(z/CELL)),[]):
        a=OLD_ROAD[i];b=OLD_ROAD[i+1];dx=b[0]-a[0];dz=b[1]-a[1]
        t=max(0,min(1,((x-a[0])*dx+(z-a[1])*dz)/(dx*dx+dz*dz)))
        best=min(best,math.hypot(x-a[0]-t*dx,z-a[1]-t*dz))
    # Far bins need no exact query: distance is conservatively>=440m.
    return min(best,440.)

def protection(x,z):
    # Begin the visible rise well outside the exact180m exclusion, so the
    # conservative whole-polygon filter never exposes a cut-open hillside.
    main=smooth(MAIN_BUFFER+100,MAIN_BUFFER+360,abs(x))
    old=smooth(ROAD_BUFFER+100,ROAD_BUFFER+240,old_road_distance(x,z))
    return main*old

# CN4: one coherent rock headwall, with only three narrow unequal ledges.
# Benches are1–2m deep rather than10–14m repeated pale roads. Thick, unequal
# rock units carry restrained mineral bedding; cap is no longer a white stripe.
PROFILE=[(0,0,0),(55,.012,0),(110,.045,4),(160,.105,4),(195,.155,4),
 (220,.22,4),(235,.24,1),(244,.48,2),(245,.485,3),(248,.51,1),
 (258,.63,2),(260,.634,1),(275,.79,3),(279,.81,1),(287,.93,2),
 (288,.934,1),(300,1,2),(360,1,2),(610,.995,2),(830,.96,2),
 (1080,.83,0),(1270,.20,4),(1340,0,0)]
DISTANT_PROFILE=[(0,0,5),(100,.025,5),(205,.13,5),(270,.22,5),
 (283,.48,5),(305,.51,5),(320,.76,5),(345,.79,5),(360,.99,5),
 (430,1,5),(650,.96,5),(830,.10,5),(900,0,5)]
BELTS=[
 {'id':'western_escarpment','axis':'z','side':-1,'along':[-2300,1200],'toe':740,'height':117,'seed':81,'step':8,'profile':PROFILE,'layer':'middle','main_cliff_min_m':700},
 {'id':'eastern_north_escarpment','axis':'z','side':1,'along':[-2700,1450],'toe':620,'height':227,'seed':97,'step':8,'profile':PROFILE,'layer':'middle','main_cliff_min_m':700},
 {'id':'far_northern_ridges','axis':'x','side':-1,'along':[-2250,2250],'toe':2600,'height':151,'seed':137,'step':27,'profile':DISTANT_PROFILE,'layer':'far'},
 {'id':'far_western_ridges','axis':'z','side':-1,'along':[-2850,1850],'toe':1950,'height':171,'seed':173,'step':28,'profile':DISTANT_PROFILE,'layer':'far'},
]

def angular_noise(x,seed):
    # Piecewise linear erosion preserves angular drainage/headland shoulders.
    i=math.floor(x);t=x-i
    return hash1(i,seed)*(1-t)+hash1(i+1,seed)*t

def oldroad_erosion(belt,s,setback,fraction,x,z):
    """CN5 localized relief/material envelope, outside the unchanged road mask.

    Only the raised east-rim transition is treated. The exact road corridor
    and the original protection(x,z) function are deliberately untouched.
    """
    if belt['id']!='eastern_north_escarpment':return 0.,0.,0.
    distance=old_road_distance(x,z)
    zone=smooth(286,315,distance)*(1-smooth(397,435,distance))*smooth(.48,.85,fraction)
    # Angular31m drainage cuts and broader90m cross-ridges, not periodic sine
    # waves or independent random vertex spikes. Setback phase creates broken
    # branching ribs instead of one unbroken pale plateau plane.
    phase=.70*noise1(setback/170,belt['seed']+44)
    ridge=abs(2*angular_noise(s/31+phase,belt['seed']+45)-1)
    cross=angular_noise(setback/90+noise1(s/150,belt['seed']+49),belt['seed']+50)
    relief=zone*((ridge-.50)*22+(cross-.5)*6)
    return zone,relief,ridge

def surface_point(belt,s,setback,fraction):
    seed=belt['seed'];a0,a1=belt['along']
    end=smooth(0,145,s-a0)*smooth(0,145,a1-s)
    broad=noise1(s/580,seed)
    headland=angular_noise(s/240,seed+2)
    gully=1-abs(2*angular_noise(s/63,seed+11)-1)
    # Deep coves retreat into the plateau rather than bobbing all beds together.
    cove=220*max(0,(headland-.40)/.60)**1.5
    embay=150*(broad-.35)+62*(angular_noise(s/155,seed+3)-.5)
    # Coves stay recessed behind the rim as well: a height-based weight
    # unwound the220m setback on the descending backslope and folded it.
    lateral=belt['toe']+setback+embay+cove*smooth(30,220,setback)
    # Erosion offsets settle before the headwall. All narrow rock ledges
    # inherit the same lateral gully displacement, so even1m benches remain
    # strictly ordered; no fraction-dependent offset unwinds behind the rim.
    lateral+=28*gully*smooth(30,210,setback)
    lateral+=8*(angular_noise(s/23,seed+23)-.5)*smooth(80,210,setback)
    # Geologically broad stepped height families: low western rim, taller east,
    # lower distant skylines. A cove erodes the rim independently of the talus.
    height=belt['height']*(.74+.34*noise1(s/650,seed+5))
    height*=1-.19*smooth(.50,.92,headland)
    h=height*fraction
    # Subtle, nonperiodic2m bedding departures, with different phases per bed.
    # Large strata remain geologically coherent rather than parallel stairways.
    h+=2.0*(noise1(s/120+fraction*3.7,seed+31)-.5)*smooth(.20,.32,fraction)
    if .01<fraction<.25:
        fan=angular_noise(s/95,seed+17)-.5
        h+=height*(fan*.055-gully*.043)*math.sin(math.pi*fraction/.25)
    elif .25<=fraction<.985:
        h-=height*.035*gully
    elif fraction>=.985:
        h-=height*.008*gully
    x,z=(belt['side']*lateral,s) if belt['axis']=='z' else (s,belt['side']*lateral)
    zone,relief,ridge=oldroad_erosion(belt,s,setback,fraction,x,z)
    return x,z,max(0,h)*end*protection(x,z)-4+relief*end

def split_profile(profile):
    result=[]
    for (d,h,m),(d1,h1,m1) in zip(profile,profile[1:]):
        n=math.ceil((d1-d)/45)
        for i in range(n):
            t=i/n;result.append((d+(d1-d)*t,h+(h1-h)*t,m))
    result.append(profile[-1]);return result

def build_geometry():
    verts=[];faces=[];slots=[];uv=[];bands=[];filtered=0
    for belt in BELTS:
        first_v=len(verts);first_f=len(faces);cliff_axis=[];a0,a1=belt['along'];n=math.ceil((a1-a0)/belt['step']);profile=split_profile(belt['profile']);rows=[]
        for i in range(n+1):
            s=a0+(a1-a0)*i/n;row=[]
            for j,(setback,fraction,material) in enumerate(profile):
                x,z,h=surface_point(belt,s,setback,fraction)
                if row:
                    previous=verts[row[-1]]
                    old_lateral=belt['side']*(previous[0] if belt['axis']=='z' else -previous[1])
                    new_lateral=belt['side']*(x if belt['axis']=='z' else z)
                    assert new_lateral-old_lateral>1e-4,('Folded profile',belt['id'],i,j,new_lateral-old_lateral)
                row.append(len(verts));verts.append((x,-z,h))
                if belt['axis']=='z' and belt['layer']=='middle' and .24<=fraction<=.985 and h>35:cliff_axis.append(abs(x))
            rows.append(row)
        for i in range(n):
            for j in range(len(profile)-1):
                ids=[rows[i][j],rows[i+1][j],rows[i+1][j+1],rows[i][j+1]]
                ps=[verts[k] for k in ids];xz=[(p[0],-p[1]) for p in ps]
                maxedge=max(math.dist(a,b) for a in xz for b in xz)
                # Distance is1-Lipschitz. This conservative proof protects the
                # whole projected polygon, not only vertex/centroid samples.
                if max(p[2] for p in ps)>0:
                    mind=min(old_road_distance(x,z) for x,z in xz)
                    if mind-maxedge<ROAD_BUFFER or min(abs(x) for x,z in xz)-maxedge<MAIN_BUFFER:
                        filtered+=1;continue
                # Cross products set winding consistently for either belt axis.
                e=[ps[1][k]-ps[0][k] for k in range(3)];f=[ps[2][k]-ps[0][k] for k in range(3)]
                if e[0]*f[1]-e[1]*f[0]<0:ids.reverse();ps.reverse()
                mat=profile[j][2]
                # A plateau tilted down into the protected OldRoad opening is
                # eroded rubble/exposed rock, not a pale intact cap roof.
                cx=sum(p[0] for p in ps)/4;cz=-sum(p[1] for p in ps)/4
                along=(a0+(a1-a0)*(i+.5)/n)
                setback=(profile[j][0]+profile[j+1][0])*.5
                fraction=(profile[j][1]+profile[j+1][1])*.5
                eroded,relief,ridge=oldroad_erosion(belt,along,setback,fraction,cx,cz)
                patch=hash1(math.floor(along/29)+math.floor(setback/43)*251,belt['seed']+61)
                if eroded>0 and patch<eroded:
                    mat=1 if ridge>.72 else 3 if ridge<.16 else 4
                normal=(e[1]*f[2]-e[2]*f[1],e[2]*f[0]-e[0]*f[2],e[0]*f[1]-e[1]*f[0])
                steep=abs(normal[2])<.78*math.sqrt(sum(t*t for t in normal))
                # Follow the face's dominant horizontal normal, not the belt's
                # nominal axis. End slopes can be steep along the belt while
                # flat across the plateau; a fixed along/height projection
                # collapses those triangles. Test both possible quad diagonals
                # before selecting the wall projection; planarXY is the robust
                # heightfield fallback and keeps horizontal bedding readable.
                def projected_area_min(coords):
                    return min(abs((coords[b][0]-coords[a][0])*(coords[c][1]-coords[a][1])-
                                   (coords[b][1]-coords[a][1])*(coords[c][0]-coords[a][0]))*.5
                               for a,b,c in [(0,1,2),(0,2,3),(0,1,3),(1,2,3)])
                planar=[(p[0]/18,p[1]/18) for p in ps]
                uvs=planar
                if steep and mat in [1,3,5]:
                    horizontal_axis=1 if abs(normal[0])>=abs(normal[1]) else 0
                    wall=[(p[horizontal_axis]/38,p[2]/26) for p in ps]
                    if projected_area_min(wall)>1e-7:uvs=wall
                assert projected_area_min(uvs)>1e-10,('Source collapsed UV',belt['id'],i,j)
                # Integer tile offsets preserve the repeated image exactly while
                # keeping UE half-precision UV coordinates close to zero.
                origin=[math.floor(sum(t[k] for t in uvs)/4) for k in range(2)]
                uvs=[(t[0]-origin[0],t[1]-origin[1]) for t in uvs]
                faces.append(ids);slots.append(mat);uv.append(uvs)
        if cliff_axis:assert min(cliff_axis)>=700,(belt['id'],'vertical cliff closer than700m',min(cliff_axis))
        bands.append({k:v for k,v in belt.items() if k!='profile'}|{'actual_main_cliff_min_axis_distance_m':min(cliff_axis) if cliff_axis else None,'profile':profile,'vertices':len(verts)-first_v,'quads':len(faces)-first_f,'triangles':(len(faces)-first_f)*2,'bounds_local_m':[[f(p[k] for p in verts[first_v:]) for k in range(3)] for f in [min,max]]})
    return verts,faces,slots,uv,bands,filtered
