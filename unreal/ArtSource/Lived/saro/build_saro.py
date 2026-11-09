"""SIGNAL / 47: SARO from outside: painted concrete block with real joints, metal coping on every parapet, galvanised downspouts with straps, a trunk tray to the array, rusted cooling on the roof, dust up the foot of every wall.
blender -b -t 2 --python build_saro.py
A skin 6 mm in front of the walls the game already has; all NoCollision, room-origin in the ext group's terms (road-area metres). Nothing of the doors, signs or lamps is touched.
"""
import json,math,random,sys
from pathlib import Path
import bpy
ROOT=Path(__file__).resolve().parent
sys.path.insert(0,str(ROOT.parent))
import geo as G
bpy.ops.wm.read_factory_settings(use_empty=True)
R=random.Random(47)
g=G.Geo()
BLOCK,WING,DIRT,COP,GALV,RUST,CAB,CABG='M_Lived_BlockSaro','M_Lived_BlockWing','M_Lived_OutGrime','M_Lived_Coping','M_Lived_Downspout','M_Lived_RustSteel','M_Lived_CableBlack','M_Lived_CableGrey'
OFF=.006
def wall(axis,at,face,a0,a1,y0,y1,openings=(),mat=BLOCK):
    """axis 'z': wall on constant z, along x, facing +z (face 1) or -z. axis 'x': wall on constant x, along z, facing +x or -x."""
    plane=at+face*OFF
    def Q(u0,u1,v0,v1):
        if u1-u0<.002 or v1-v0<.002:return
        if axis=='z':
            q=[(u0,v0,plane),(u1,v0,plane),(u1,v1,plane),(u0,v1,plane)]
            g.quad(q if face>0 else q[::-1],mat)
        else:
            q=[(plane,v0,u1),(plane,v0,u0),(plane,v1,u0),(plane,v1,u1)]
            g.quad(q if face>0 else q[::-1],mat)
    at_=a0
    for o0,o1,oy0,oy1 in sorted(openings):
        Q(at_,o0,y0,y1);Q(o0,o1,y0,oy0);Q(o0,o1,oy1,y1);at_=o1
    Q(at_,a1,y0,y1)
def band(axis,at,face,a0,a1,y0,y1,which,off=.012,scale=3.):
    """one of the grime bands, wrapping in U. which: 0 dust from the foot, 1 rain under a coping, 2 rust, 3 damp."""
    plane=at+face*off;u=R.random()
    r0,r1=[(0,512),(512,1024),(1024,1536),(1536,2048)][which]
    vt,vb=1-r0/2048,1-r1/2048   # the picture's row r0 is the band's top
    if which==0:vt,vb=vb,vt # dust: its strong end (row 511) sits at the ground
    if axis=='z':
        q=[(a0,y0,plane),(a1,y0,plane),(a1,y1,plane),(a0,y1,plane)]
    else:
        q=[(plane,y0,a1),(plane,y0,a0),(plane,y1,a0),(plane,y1,a1)]
    L=(a1-a0)/scale
    uv=[(u,vb),(u+L,vb),(u+L,vt),(u,vt)]
    if face<0:q=q[::-1];uv=uv[::-1]
    if which==0:uv=[(a,1-r0/2048 if abs(b-(1-r0/2048))>1e-6 else 1-r1/2048) for a,b in uv]
    g.quad(q,DIRT,uv)
def coping(path,y,w=.34,t=.055,lip=.05):
    (x0,z0),(x1,z1)=path
    mx,mz=(x0+x1)/2,(z0+z1)/2;L=math.hypot(x1-x0,z1-z0);yaw=-math.atan2(z1-z0,x1-x0)
    g.box(COP,(mx,y+t/2,mz),(L,t,w),yaw,bevel=.006)
    nx,nz=-(z1-z0)/L,(x1-x0)/L
    for s in(-1,1):g.box(COP,(mx+nx*s*(w/2+.006),y-lip/2,mz+nz*s*(w/2+.006)),(L,lip,.012),yaw)
    # joints every 2.4 m: a small dark line, a seam strap
    n=int(L/2.4)
    for i in range(1,n+1):
        t_=i*2.4/L
        px,pz=x0+(x1-x0)*t_,z0+(z1-z0)*t_
        g.box(GALV,(px,y+t+.002,pz),(.03,.006,w+.02),yaw)
def downspout(x,z,top,nx,nz,bottom=-.45):
    # On the wall, facing (nx,nz): a rect pipe, straps, swan neck at the top from a scupper, a shoe at the foot.
    px,pz=x+nx*.055,z+nz*.055
    g.box(GALV,(px,(top+bottom)/2-.05,pz),(.075,top-bottom-.1,.075),bevel=.006)
    for y in [bottom+.5+i*.95 for i in range(4)]:
        if y<top-.2:g.box(RUST,(px,y,pz),(.095,.022,.095))
    g.box(GALV,(px-nx*.0,top-.04,pz-nz*.0),(.1,.09,.1))
    g.box(GALV,(x+nx*.11,bottom+.02,z+nz*.11),(.09,.06,.22) if abs(nz)>abs(nx) else (.22,.06,.09))
    band('z' if abs(nz)>abs(nx) else 'x',z if abs(nz)>abs(nx) else x,int(math.copysign(1,nz if abs(nz)>abs(nx) else nx)),(x if abs(nz)>abs(nx) else z)-.28,(x if abs(nz)>abs(nx) else z)+.28,bottom,top,3,.012,.6)
# ---- the control room: north wall with its window bays and the trunk tray over them
bays=[(-5.4+k*1.8+.05,-5.4+(k+1)*1.8-.05,.95,2.75) for k in range(6)]
wall('z',-4.8,-1,-6.3,6.3,-.6,3.6,bays)
wall('x',-6.3,-1,-4.8,4.8,-.6,3.6)
wall('x',6.3,1,-4.8,4.8,-.6,3.6,[(1.1,2.1,-.6,2.12)])
wall('z',4.8,1,2.3,6.3,-.6,3.6)
# ---- the annex: the fire exit's door in its west wall, and Dale's window in the south wall (DALE-WINDOW-20261108: the opening
# S47OfficeWindow.cpp cuts in the old walls, Left -.55 Right 1.30 Bottom 1.08 Top 2.40; the skin was solid over it from outside)
DALE_WIN=(-.55,1.30,1.08,2.40)
wall('z',10.8,1,-6.3,2.3,-.6,3.2,[DALE_WIN])
wall('x',-6.3,-1,4.8,10.8,-.6,3.2,[(5.22,6.18,-.6,2.12)])
wall('x',2.3,1,4.8,10.8,-.6,3.2)
# ---- the lab
wall('z',1.0,1,12.6,19.2,-.6,3.0);wall('x',19.2,1,-6.,1.,-.6,3.0);wall('z',-6.,-1,12.6,19.2,-.6,3.0);wall('x',12.6,-1,-6.,1.,-.6,3.0,[(-1.7,-.7,-.6,2.1)])
# ---- the east wing, by its own gaps (S47EastWing.cpp SouthGaps, NorthGaps): door, roll-up, windows
wall('z',5.5,1,20.6,38.6,-.6,3.6,[(21.3,22.4,-.6,2.15),(33.2,37.6,-.6,3.0)],WING)
wall('z',-5.5,-1,20.6,38.6,-.6,3.6,[(21.4,23.0,1.0,2.25),(23.6,25.2,1.0,2.25),(27.0,28.6,1.0,2.25),(29.2,30.8,1.0,2.25),(33.0,34.6,1.6,2.4),(35.6,37.2,1.6,2.4)],WING)
wall('x',20.6,-1,-5.5,5.5,-.6,3.6,(),WING);wall('x',38.6,1,-5.5,5.5,-.6,3.6,(),WING)
# ---- dust up the foot of everything
# Dale's window: the dust stops under its sill, the rain streaks start over its head.
band('z',10.8,1,DALE_WIN[0],DALE_WIN[1],-.5,DALE_WIN[2]-.04,0,.014)
band('z',10.8,1,DALE_WIN[0],DALE_WIN[1],DALE_WIN[3]+.04,3.18,1,.016)
for axis,at,face,a0,a1,top in(('z',-4.8,-1,-6.3,6.3,3.6),('x',-6.3,-1,-4.8,4.8,3.6),('x',6.3,1,-4.8,4.8,3.6),('z',4.8,1,2.3,6.3,3.6),('z',10.8,1,-6.3,DALE_WIN[0],3.2),('z',10.8,1,DALE_WIN[1],2.3,3.2),('x',-6.3,-1,4.8,10.8,3.2),('x',2.3,1,4.8,10.8,3.2),('z',1.,1,12.6,19.2,3.),('x',19.2,1,-6.,1.,3.),('z',-6.,-1,12.6,19.2,3.),('x',12.6,-1,-6.,1.,3.),('z',5.5,1,20.6,33.2,3.6),('z',5.5,1,37.6,38.6,3.6),('z',-5.5,-1,20.6,38.6,3.6),('x',20.6,-1,-5.5,5.5,3.6),('x',38.6,1,-5.5,5.5,3.6)):
    band(axis,at,face,a0,a1,-.5,1.15,0,.014)
    band(axis,at,face,a0,a1,top-1.0,top-.02,1,.016)
    # rust below each fixing of the coping, patchy
    for k in range(int((a1-a0)/2.4)):
        u=a0+k*2.4+R.uniform(0,1.)
        if R.random()<.55:band(axis,at,face,u,u+R.uniform(.25,.6),top-.8,top-.02,2,.018,.8)
# ---- copings
for p in(((-6.42,-4.92),(6.42,-4.92)),((-6.42,4.92),(6.42,4.92)) ,((-6.42,-4.92),(-6.42,4.92)),((6.42,-4.92),(6.42,4.92))):coping(p,3.6)
for p in(((-6.42,10.92),(2.42,10.92)),((-6.42,4.92),(-6.42,10.92)),((2.42,4.92),(2.42,10.92))):coping(p,3.2)
for p in(((12.48,1.12),(19.32,1.12)),((12.48,-6.12),(19.32,-6.12)),((12.48,-6.12),(12.48,1.12)),((19.32,-6.12),(19.32,1.12))):coping(p,3.0)
for p in(((20.48,5.62),(38.72,5.62)),((20.48,-5.62),(38.72,-5.62)),((20.48,-5.62),(20.48,5.62)),((38.72,-5.62),(38.72,5.62))):coping(p,3.66,.26,.05)
# ---- downspouts (the game's old square ones stand at the same corners and go under these)
for x,z,top,nx,nz in((23.,5.5,3.6,0,1),(32.4,5.5,3.6,0,1),(38.2,5.5,3.6,0,1),(22.,-5.5,3.6,0,-1),(32.,-5.5,3.6,0,-1),(-6.3,-4.5,3.55,-1,0),(6.3,-4.5,3.55,1,0),(-6.3,4.5,3.55,-1,0),(-6.3,10.5,3.15,-1,0),(2.3,10.5,3.15,1,0),(19.2,.6,2.95,1,0),(12.6,-5.6,2.95,-1,0)):
    downspout(x,z,top,nx,nz)
# ---- the trunk tray: out of the north wall over the windows and down the north-east corner, to a pull box at the foot. The array's coax.
TY=2.92;TZ=-5.03
for s in(-1,1):g.box(GALV,(-.2,TY+.0,TZ+s*.11),(11.2,.05,.012))
for i in range(47):
    x=-5.8+i*.24;g.box(GALV,(x,TY-.02,TZ),(.022,.008,.22))
for i in range(9):
    x=-5.4+i*1.35;g.box(RUST,(x,TY-.045,TZ+.1),(.03,.02,.26));g.box(RUST,(x,TY-.1,TZ+.22),(.04,.11,.012))
for i in range(8):
    z=TZ+.0;pts=[(-5.76,TY+.005,TZ-.09+.026*i+R.uniform(-.008,.008))]
    x=-5.76
    while x<5.4:
        x+=R.uniform(.4,.7);pts.append((min(x,5.4),TY+.005+R.uniform(0,.016),TZ-.09+.026*i+R.uniform(-.01,.01)))
    # round the corner and down
    pts+=[(5.72,TY-.01,TZ+.0+.01*i),(5.9,TY-.3,TZ+.02+.014*i),(5.94,TY-1.2,TZ+.05+.012*i),(5.94,.7,TZ+.08+.012*i),(5.94,.42,TZ+.08+.012*i)]
    g.tube(CAB if i<6 else CABG,pts,.009,5)
g.box(RUST,(5.94,.25,TZ+.12),(.34,.5,.2),bevel=.012)
g.box(GALV,(5.94,1.6,TZ+.1),(.12,1.0,.18))
# Through the wall at the tray's west end.
g.box(RUST,(-5.85,TY,-4.9),(.34,.2,.22))
# ---- rusted cooling on the control room's roof: a condenser on legs, above the parapet where it can be seen
CX,CZ,CY=2.5,-1.5,3.6
for dx in(-.95,.95):
    for dz in(-.55,.55):g.box(RUST,(CX+dx,CY+.2,CZ+dz),(.07,.4,.07))
g.box(RUST,(CX,CY+.82,CZ),(2.1,.86,1.3),bevel=.015)
for i in range(13):g.box(GALV,(CX-.95+i*.158,CY+.82,CZ-.656),(.1,.7,.008))
for i in range(13):g.box(GALV,(CX-.95+i*.158,CY+.82,CZ+.656),(.1,.7,.008))
for dx in(-.55,.55):
    g.lathe(RUST,(CX+dx,CY+1.25,CZ),[(.46,0),(.46,.1),(.42,.12)],16)
    g.lathe('M_Lived_BlackPlastic',(CX+dx,CY+1.3,CZ),[(.41,0),(.04,.02),(0,.03)],12)
    for k in range(5):
        a=k/5*6.283;g.box(GALV,(CX+dx+math.cos(a)*.22,CY+1.33,CZ+math.sin(a)*.22),(.34,.006,.09),-a)
g.tube(GALV,[(CX+1.0,CY+.5,CZ-.3),(CX+1.45,CY+.32,CZ-.3),(CX+1.8,CY+.12,CZ-.6)],.022,6)
g.tube(GALV,[(CX+1.0,CY+.5,CZ+.1),(CX+1.45,CY+.3,CZ+.1),(CX+1.9,CY+.1,CZ-.3)],.016,6)
band('z',CZ-.665,-1,CX-1.05,CX+1.05,CY+.4,CY+1.25,2,.002,1.)
ob,tr=G.build(g,'SM_Lived_SaroExterior',uvscale={BLOCK:1.6,WING:1.6,COP:2.,GALV:1.5,RUST:1.5})
Export=ROOT/'Export'
tr=G.export_fbx(ob,Export/'SM_Lived_SaroExterior.fbx')
vs=[v.co for v in ob.data.vertices];bd=[[min(v[k] for v in vs) for k in range(3)],[max(v[k] for v in vs) for k in range(3)]]
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'Lived_Saro.blend'))
BL={'base_color':'textures/T_Lived_Block_BC.png','roughness':'textures/T_Lived_Block_R.png','normal':'textures/T_Lived_Block_N.png'}
EN={'base_color':'textures/T_Lived_Enamel_BC.png','roughness':'textures/T_Lived_Enamel_R.png'}
rows=[{'name':WING,'textures':BL,'texture_tint_linear':[.74,.71,.65],'roughness':.85,'metallic':0,'specular':.3,'normal_strength':.9,'normal_convention':'DirectX','base_color_linear':[1,1,1,1]},{'name':BLOCK,'textures':BL,'texture_tint_linear':[.80,.74,.62],'roughness':.85,'metallic':0,'specular':.3,'normal_strength':.9,'normal_convention':'DirectX','base_color_linear':[1,1,1,1]},
 {'name':DIRT,'textures':{'base_color':'textures/T_Lived_OutGrime_BC.png'},'texture_tint_linear':[1,1,1],'roughness':.9,'metallic':0,'specular':.2,'opacity_from_base_color':True,'alpha_mode':'dithered_masked','base_color_linear':[1,1,1,1]},
 {'name':COP,'textures':EN,'texture_tint_linear':[.30,.30,.28],'metallic':.7,'specular':.5,'roughness':.55,'base_color_linear':[1,1,1,1]},
 {'name':GALV,'textures':EN,'texture_tint_linear':[.40,.40,.38],'metallic':.75,'specular':.5,'roughness':.55,'base_color_linear':[1,1,1,1]},
 {'name':RUST,'textures':EN,'texture_tint_linear':[.26,.115,.05],'metallic':.1,'specular':.35,'roughness':.85,'base_color_linear':[1,1,1,1]},
 {'name':CAB,'reuse':'/Game/Lived/Control/Materials/M_Lived_CableBlack'},{'name':CABG,'reuse':'/Game/Lived/Control/Materials/M_Lived_CableGrey'},{'name':'M_Lived_BlackPlastic','reuse':'/Game/Lived/Control/Materials/M_Lived_BlackPlastic'}]
used={m.name for m in ob.data.materials}
(ROOT.parent/'lived-saro-materials.json').write_text(json.dumps([r for r in rows if r['name'] in used],indent=1))
(ROOT.parent/'lived-saro-contract.json').write_text(json.dumps({'export_dir':'saro/Export','meshes':{'SM_Lived_SaroExterior':{'triangles':tr,'materials':sorted(used),'bounds_local_m':bd}}},indent=1))
print('DONE',tr,sorted(used))
