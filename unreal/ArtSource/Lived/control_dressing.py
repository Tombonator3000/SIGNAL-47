"""SIGNAL / 47: fifteen years of night shifts in the SARO control room. Own geometry, own textures.
blender -b -t 2 --python control_dressing.py
Everything is room-origin and NoCollision: nothing with a function is touched. Where it could be walked into, S47Lived.cpp adds a collider.
Writes Export/SM_Lived_ControlFloor.fbx, Export/SM_Lived_ControlDressing.fbx and lived-control-*.json beside this file.
"""
import json,math,random,sys
from pathlib import Path
import bpy
ROOT=Path(__file__).resolve().parent
sys.path.insert(0,str(ROOT))
import importlib,geo as G
importlib.reload(G)
bpy.ops.wm.read_factory_settings(use_empty=True)
(ROOT/'Export').mkdir(exist_ok=True)
R=random.Random(1986)
# ------------------------------------------------------------------ materials
ENAMEL={'base_color':'textures/T_Lived_Enamel_BC.png','roughness':'textures/T_Lived_Enamel_R.png'}
PLASTIC={'base_color':'textures/T_Lived_Plastic_BC.png','roughness':'textures/T_Lived_Plastic_R.png'}
CARD={'base_color':'textures/T_Lived_Card_BC.png','roughness':'textures/T_Lived_Card_R.png'}
CLOTH={'base_color':'textures/T_Lived_Cloth_BC.png','roughness':'textures/T_Lived_Cloth_R.png'}
def tex(name,t,tint,metallic=0,spec=.45):return {'name':name,'textures':t,'texture_tint_linear':list(tint),'metallic':metallic,'specular':spec,'roughness':.6,'base_color_linear':[1,1,1,1]}
def flat(name,col,rough=.6,metallic=0,spec=.45):return {'name':name,'base_color_linear':list(col)+[1],'roughness':rough,'metallic':metallic,'specular':spec}
def lit(name,col,glow,strength,rough=.35):return {'name':name,'base_color_linear':list(col)+[1],'roughness':rough,'metallic':0,'specular':.5,'emission':list(glow)+[1],'emission_strength':strength}
ROWS=[tex('M_Lived_RackSteel',ENAMEL,(.082,.096,.092)),tex('M_Lived_ShelfGreen',ENAMEL,(.115,.165,.125)),tex('M_Lived_PanelGrey',ENAMEL,(.285,.295,.27)),
 tex('M_Lived_TrayGalv',ENAMEL,(.46,.47,.45),metallic=.85),tex('M_Lived_Beige',PLASTIC,(.47,.42,.31)),tex('M_Lived_BlackPlastic',PLASTIC,(.022,.022,.024)),
 tex('M_Lived_WhiteCan',PLASTIC,(.55,.54,.48)),tex('M_Lived_BlueCan',PLASTIC,(.06,.11,.19)),tex('M_Lived_Ceramic',PLASTIC,(.66,.63,.55)),tex('M_Lived_Card',CARD,(.40,.28,.155)),tex('M_Lived_Cloth',CLOTH,(.145,.105,.065)),
 flat('M_Lived_CableBlack',(.016,.016,.017),.55),flat('M_Lived_CableGrey',(.17,.17,.165),.6),flat('M_Lived_CableBeige',(.42,.37,.26),.6),flat('M_Lived_AmberGlass',(.22,.10,.03),.16),flat('M_Lived_Butt',(.52,.40,.24),.85),flat('M_Lived_Ash',(.10,.10,.10),.95),flat('M_Lived_Foam',(.74,.72,.66),.8),flat('M_Lived_Tape',(.50,.40,.20),.62),
 flat('M_Lived_Chrome',(.62,.62,.60),.3,.95),flat('M_Lived_RedPaint',(.32,.025,.02),.45),flat('M_Lived_ReelWindow',(.012,.012,.014),.2),
 {'name':'M_Lived_Paper','textures':{'base_color':'textures/T_Lived_Paper_BC.png'},'texture_tint_linear':[.92,.92,.90],'roughness':.88,'metallic':0,'specular':.3,'base_color_linear':[1,1,1,1]},
 {'name':'M_Lived_Grime','textures':{'base_color':'textures/T_Lived_Grime_BC.png'},'texture_tint_linear':[1,1,1],'roughness':.82,'metallic':0,'specular':.25,'opacity_from_base_color':True,'alpha_mode':'dithered_masked','base_color_linear':[1,1,1,1]},
 lit('M_Lived_LampGreen',(.02,.05,.02),(.10,.95,.22),9.),lit('M_Lived_LampAmber',(.05,.03,.01),(1.,.55,.08),8.),lit('M_Lived_LampRed',(.05,.012,.01),(1.,.08,.04),8.),lit('M_Lived_LampOff',(.015,.015,.012),(.0,.0,.0),0.),lit('M_Lived_BulbWarm',(.6,.5,.35),(1.,.62,.28),14.)]
FLOOR_ROW={'name':'M_Lived_VCTFloor','textures':{'base_color':'textures/T_Lived_VCT_BC.png','roughness':'textures/T_Lived_VCT_R.png','normal':'textures/T_Lived_VCT_N.png'},'texture_tint_linear':[.76,.70,.60],'roughness':.45,'metallic':0,'specular':.45,'normal_strength':1.,'normal_convention':'DirectX','base_color_linear':[1,1,1,1]}
# Atlas windows (u0,v0,u1,v1 in the picture, v downward from the top), turned into Blender UVs.
def W(px0,py0,px1,py1,S=2048.):return (px0/S,1-py1/S,px1/S,1-py0/S)
def uvq(w):u0,v0,u1,v1=w;return [(u0,v0),(u1,v0),(u1,v1),(u0,v1)]
PAPER=dict(roster=W(0,0,820,1024),n1=W(830,0,1230,300),n2=W(1240,0,1640,300),n3=W(1650,0,2048,300),n4=W(830,310,1230,610),n5=W(1240,310,1640,610),n6=W(1650,310,2048,610),n7=W(830,620,1230,1024),n8=W(1240,620,1640,1024),
 greenbar=W(0,1024,1024,2048),label=W(1024,1024,1152,1280),tape=W(1024,1290,2048,1400),patch=W(1024,1410,2048,1640),transport=W(1024,1650,2048,2048))
GRIME=dict(halo=W(0,1024,512,1536),leak=W(512,1024,1024,1536),yellow=W(1024,1024,1536,1536),scuff=W(1536,1024,2048,1536),hand=W(0,1536,512,2048),dust=W(512,1536,1024,2048),ring=W(1024,1536,1536,2048),burn=W(1536,1536,2048,2048))
g=G.Geo()
# ------------------------------------------------------------------ the floor is its own mesh
fg=G.Geo();x0,x1,z0,z1=-6.,6.,-4.5,4.5;Y=.0018
fg.quad([(x0,Y,z1),(x1,Y,z1),(x1,Y,z0),(x0,Y,z0)],'M_Lived_VCTFloor',[(0,0),(1,0),(1,1),(0,1)])
# ------------------------------------------------------------------ helpers
def jitter(a,s):return a+R.uniform(-s,s)
def sheet(axis,fixed,c,w,h,name,roll=0.,flip=False,tapes=2,mat='M_Lived_Paper'):
    """A sheet of paper on a wall, taped at the top with masking tape (the bit people forget is that tape stays)."""
    g.plane_on(mat,axis,fixed,c,w,h,roll,uvq(PAPER[name]),flip)
    off=.0008 if not flip else -.0008
    cr,sr=math.cos(roll),math.sin(roll)
    for k in ([-1,1] if tapes==2 else [-1,1,-3,3] if tapes==4 else [0]):
        if tapes==4 and abs(k)==3:
            s=1 if k>0 else -1;a=s*w*.5;b=-h*.5
        else:a=k*w*.42;b=h*.5
        aa,bb=a*cr-b*sr,a*sr+b*cr
        if axis=='x':cc=(c[0]+(aa if flip else -aa),c[1]+bb)
        elif axis=='z':cc=(c[0]+(-aa if flip else aa),c[1]+bb)
        g.plane_on('M_Lived_Tape',axis,fixed+off,cc,.055,.022,roll+R.uniform(-.25,.25),None,flip)
def on_desk(x,z,y,name,w,h,roll=0.):
    g.plane_on('M_Lived_Paper','y',y,(x,z),w,h,roll,uvq(PAPER[name]))
def mug(x,y,z,mat='M_Lived_Ceramic',turn=0.,coffee=True):
    r0,r1,h=.038,.041,.092
    g.lathe(mat,(x,y,z),[(0,0),(r0,0),(r0+.001,.006),(r1,h),(r1-.004,h),(r1-.006,h*.82),(0,h*.80)],14)
    if coffee:g.lathe('M_Lived_Ash',(x,y,z),[(0,h*.78),(r1-.006,h*.79)],10)
    # the handle, a bent tube
    a=turn;s,c=math.sin(a),math.cos(a)
    pts=[(r1+.002,.07),(r1+.022,.068),(r1+.03,.048),(r1+.022,.028),(r1,.024)]
    g.tube(mat,[(x+c*p,y+q,z+s*p) for p,q in pts],.0055,5)
def foam_cup(x,y,z):
    g.lathe('M_Lived_Foam',(x,y,z),[(0,0),(.028,0),(.041,.098),(.043,.1),(.039,.1),(.037,.085),(0,.083)],12)
def ashtray(x,y,z,butts=5,mat='M_Lived_AmberGlass'):
    g.lathe(mat,(x,y,z),[(0,0),(.062,0),(.072,.006),(.074,.03),(.066,.032),(.058,.012),(0,.010)],14)
    g.lathe('M_Lived_Ash',(x,y,z),[(0,.0105),(.056,.0125)],12)
    for i in range(butts):
        a=R.random()*6.28;d=R.uniform(.012,.05);b=R.random()*6.28;L=R.uniform(.02,.035)
        cx,cz=x+math.cos(a)*d,z+math.sin(a)*d
        ey=y+R.uniform(.016,.034)
        g.tube('M_Lived_Butt',[(cx,y+.013,cz),(cx+math.cos(b)*L,ey,cz+math.sin(b)*L)],.0038,5)
def cig_pack(x,y,z,yaw=0.):
    g.box('M_Lived_WhiteCan',(x,y+.011,z),(.054,.022,.088),yaw)
    g.box('M_Lived_LampRed' if False else 'M_Lived_BlueCan',(x,y+.0225,z+.01),(.05,.0005,.05),yaw)
def spill_ring(x,y,z,r,):
    g.plane_on('M_Lived_Grime','y',y,(x,z),r*2,r*2,R.random()*6,uvq(GRIME['ring']))
def grime_wall(axis,fixed,c,w,h,name,flip=False,roll=0.):
    g.plane_on('M_Lived_Grime',axis,fixed,c,w,h,roll,uvq(GRIME[name]),flip)
BAND={0:(1-256/2048,1.),1:(1-512/2048,1-256/2048),2:(1-768/2048,1-512/2048)}
def band_strip(p0,p1,up,height,band,scale=4.,uoff=None):
    """A long strip in a band that wraps in U. p0,p1 the feet on the floor, up the height."""
    v0,v1=BAND[band];L=math.dist(p0,p1);u=R.random() if uoff is None else uoff
    a=(p0[0],p0[1],p0[2]);b=(p1[0],p1[1],p1[2])
    c=(p1[0]+up[0]*height,p1[1]+up[1]*height,p1[2]+up[2]*height);d=(p0[0]+up[0]*height,p0[1]+up[1]*height,p0[2]+up[2]*height)
    g.quad([a,b,c,d],'M_Lived_Grime',[(u,v0),(u+L/scale,v0),(u+L/scale,v1),(u,v1)])
# ------------------------------------------------------------------ ladder tray round three walls, on brackets
TY=2.60;TW=.26
def tray_run(x0,z0,x1,z1):
    L=math.hypot(x1-x0,z1-z0);dx,dz=(x1-x0)/L,(z1-z0)/L;nx,nz=-dz,dx
    mid=((x0+x1)/2,(z0+z1)/2);yaw=-math.atan2(dz,dx)
    for s in(-1,1):
        g.box('M_Lived_TrayGalv',(mid[0]+nx*s*TW/2,TY,mid[1]+nz*s*TW/2),(L,.05,.012),yaw)
    n=int(L/.24)
    for i in range(n+1):
        t=i/n*L
        g.box('M_Lived_TrayGalv',(x0+dx*t,TY-.022,z0+dz*t),(.022,.008,TW),yaw)
tray_run(-5.78,-4.42,-5.78,4.3);tray_run(-5.91,4.3,5.91,4.3);tray_run(5.78,-4.42,5.78,4.3)
# End stops, brackets to the wall, every 1.2 m.
for z in [(-4.2+i*1.2) for i in range(8)]:
    for x,s in((-5.78,-1),(5.78,1)):
        g.box('M_Lived_RackSteel',(x+s*.105,TY-.05,z),(.21,.02,.03))
        g.box('M_Lived_RackSteel',(x+s*.205,TY-.09,z),(.012,.11,.04))
for x in [(-5.4+i*1.2) for i in range(10)]:
    g.box('M_Lived_RackSteel',(x,TY-.05,4.39),(.03,.02,.2))
    g.box('M_Lived_RackSteel',(x,TY-.09,4.485),(.04,.11,.012))
# Cables along the tray: one bundle runs the whole U. Their heights wander, a few sag past the rungs.
def tray_path(off,y,seed,steps=.55):
    r=random.Random(seed);pts=[]
    corners=[(-5.78+off,-4.40),(-5.78+off,4.3-off),(5.78-off,4.3-off),(5.78-off,-4.40)]
    for (a,b),(c,d) in zip(corners,corners[1:]):
        L=math.hypot(c-a,d-b);n=max(2,int(L/steps))
        for i in range(n):
            t=i/n;pts.append((a+(c-a)*t+r.uniform(-.018,.018),y+r.uniform(-.005,.012),b+(d-b)*t+r.uniform(-.018,.018)))
    pts.append((corners[-1][0],y,corners[-1][1]))
    return pts
kinds=['M_Lived_CableBlack']*6+['M_Lived_CableGrey']*2+['M_Lived_CableBeige']*2
for i,m in enumerate(kinds):
    off=-.09+.018*i+R.uniform(-.006,.006)
    p=tray_path(off,TY-.006+.012*(i%3),i)
    g.tube(m,p,R.uniform(.0055,.009),5)
# A few that have slipped through and hang in a loop under the tray.
for x0,z0,x1,z1,sag in[(-5.70,.9,-5.70,1.6,.13),(-1.7,4.22,-.9,4.22,.16),(2.6,4.24,3.2,4.24,.09),(5.72,-.6,5.72,.3,.14)]:
    pts=[];n=10
    for i in range(n+1):
        t=i/n;y=TY-.02-sag*math.sin(t*math.pi)
        pts.append((x0+(x1-x0)*t,y,z0+(z1-z0)*t))
    g.tube('M_Lived_CableBlack',pts,.0065,5)
    # cable ties
    g.box('M_Lived_BlackPlastic',(x0,TY-.018,z0),(.012,.02,.012))
def drop(m,a,b,bow=.1,r=.006,n=10):
    pts=[]
    for i in range(n+1):
        t=i/n;s=math.sin(t*math.pi)
        pts.append((a[0]+(b[0]-a[0])*t,a[1]+(b[1]-a[1])*t**1.3,a[2]+(b[2]-a[2])*t+s*bow*.3))
    g.tube(m,pts,r,5)
# Down to the RX rack, the top of the room's old equipment.
for i in range(6):
    drop(R.choice(['M_Lived_CableBlack','M_Lived_CableBlack','M_Lived_CableGrey']),(5.70+R.uniform(-.05,.05),TY-.02,-2.4+R.uniform(-.1,.1)),(5.5+R.uniform(-.1,.1),1.94,-2.4+R.uniform(-.2,.2)),bow=R.uniform(-.2,.2),r=R.uniform(.005,.008))
# Surface raceway from the tray down to the ops desk, and a junction box.
g.box('M_Lived_PanelGrey',(-5.972,1.72,1.30),(.03,1.72,.055));g.box('M_Lived_RackSteel',(-5.965,.92,1.30),(.05,.12,.11))
for y in(1.2,1.65,2.1,2.5):g.box('M_Lived_Chrome',(-5.955,y,1.30),(.006,.018,.075))
g.box('M_Lived_PanelGrey',(-5.86,2.56,1.30),(.2,.03,.055))
# ------------------------------------------------------------------ the old equipment cabinet in the north-west corner
CX0,CX1,CZ0,CZ1=-5.985,-5.40,-4.44,-3.66;CH=1.96
cm=(CX0+CX1)/2;czm=(CZ0+CZ1)/2
g.box('M_Lived_RackSteel',(cm,CH/2,czm),(CX1-CX0,CH,CZ1-CZ0),bevel=.006)
g.box('M_Lived_PanelGrey',(cm,CH+.012,czm),(CX1-CX0+.012,.024,CZ1-CZ0+.012),bevel=.006)
F=CX1+.001 # the front face
def front(mat,y0,y1,z0,z1,depth=.006,uv=None):
    if uv:
        g.quad([(F+depth,y0,z1),(F+depth,y0,z0),(F+depth,y1,z0),(F+depth,y1,z1)],mat,uv)
    else:g.box(mat,(F+depth/2,(y0+y1)/2,(z0+z1)/2),(depth,y1-y0,z1-z0),bevel=.002)
# Top: lamp strip; two rows of lamps that go about their business.
front('M_Lived_BlackPlastic',1.76,1.92,CZ0+.03,CZ1-.03,.008)
def lamp_row(x,y,z0,z1,n,kinds,axis='x',r=.0075):
    for i in range(n):
        z=z0+(z1-z0)*(i+.5)/n
        kind=kinds[i%len(kinds)]
        if axis=='x':
            g.lathe('M_Lived_Chrome',(x,y,z),[(r+.003,0),(r+.003,.004),(r,.004)],8,'x')
            g.lathe(kind,(x+.004,y,z),[(r,0),(r*.6,.003),(0,.0035)],8,'x')
        else: # on a wall of constant z, facing -z
            g.lathe('M_Lived_Chrome',(z,y,x),[(r+.003,0),(r+.003,-.004),(r,-.004)],8,'z')
            g.lathe(kind,(z,y,x-.004),[(r,0),(r*.6,-.003),(0,-.0035)],8,'z')
lamp_row(F+.009,1.875,CZ0+.06,CZ1-.06,14,['M_Lived_LampGreen','M_Lived_LampGreen','M_Lived_LampRed','M_Lived_LampGreen','M_Lived_LampAmber','M_Lived_LampOff'])
lamp_row(F+.009,1.81,CZ0+.06,CZ1-.06,14,['M_Lived_LampAmber','M_Lived_LampOff','M_Lived_LampGreen','M_Lived_LampGreen','M_Lived_LampOff','M_Lived_LampRed','M_Lived_LampGreen'])
# Tape transport: grey panel with two reels behind a door, a window of dark glass.
front('M_Lived_PanelGrey',1.16,1.74,CZ0+.03,CZ1-.03,.01)
g.quad([(F+.0125,1.18,CZ1-.05),(F+.0125,1.18,CZ0+.05),(F+.0125,1.72,CZ0+.05),(F+.0125,1.72,CZ1-.05)],'M_Lived_ReelWindow')
for zz in(czm-.17,czm+.17):
    g.lathe('M_Lived_WhiteCan',(F+.015,1.50,zz),[(0,0),(.135,0),(.135,.008),(.04,.008),(.04,.018),(0,.018)],22,'x')
    g.lathe('M_Lived_BlackPlastic',(F+.017,1.50,zz),[(.11,.001),(.075,.0075)],18,'x')
    g.lathe('M_Lived_Chrome',(F+.02,1.50,zz),[(.028,.012),(.01,.02),(0,.021)],10,'x')
# Patch panel with patch cords that loop down and back.
front('M_Lived_RackSteel',.86,1.14,CZ0+.03,CZ1-.03,.008)
for y0,y1 in((.995,1.125),(.87,1.0)):
    g.quad([(F+.0095,y0,CZ1-.04),(F+.0095,y0,CZ0+.04),(F+.0095,y1,CZ0+.04),(F+.0095,y1,CZ1-.04)],'M_Lived_Paper',uvq(PAPER['patch']))
for i in range(11):
    z=R.uniform(CZ0+.06,CZ1-.06);y=R.choice([1.08,.95]);z2=z+R.uniform(-.25,.25);z2=max(CZ0+.06,min(CZ1-.06,z2));y2=R.choice([1.08,.95,.87]);drp=R.uniform(.05,.2);c=R.choice(['M_Lived_CableBlack','M_Lived_CableGrey','M_Lived_LampOff','M_Lived_CableBeige'])
    pts=[]
    for k in range(9):
        t=k/8;pts.append((F+.02+.02*math.sin(t*3.14),y+(y2-y)*t-drp*math.sin(t*3.14),z+(z2-z)*t))
    g.tube(c if c!='M_Lived_LampOff' else 'M_Lived_CableBlack',pts,.0032,4)
# Below: blank panel with vents, handles, kick plate with fifteen years of boots.
front('M_Lived_RackSteel',.14,.84,CZ0+.03,CZ1-.03,.006)
for y in [.32+i*.022 for i in range(9)]+[.60+i*.022 for i in range(6)]:
    g.box('M_Lived_BlackPlastic',(F+.0072,y,czm),(.0008,.009,.5))
for zz in(czm-.32,czm+.32):g.box('M_Lived_Chrome',(F+.018,.62,zz),(.02,.14,.018))
g.quad([(F+.0075,.03,CZ1-.03),(F+.0075,.03,CZ0+.03),(F+.0075,.34,CZ0+.03),(F+.0075,.34,CZ1-.03)],'M_Lived_Grime',uvq(GRIME['scuff']))
# Something's been left on top: a dead box of forms and a coil.
g.box('M_Lived_Card',(cm+.03,CH+.15,czm-.06),(.34,.28,.42),.08,bevel=.004)
g.box('M_Lived_Tape',(cm+.03,CH+.2915,czm-.06),(.34,.001,.05),.08)
g.lathe('M_Lived_CableBlack',(cm+.05,CH+.31,czm+.0),[(.09,0),(.115,.012),(.12,.03),(.105,.042),(.08,.03),(.085,.01)],14)
drop('M_Lived_CableBlack',(-5.72,TY-.02,-3.86),(-5.66,CH+.03,-3.82),.1,.008)
drop('M_Lived_CableGrey',(-5.72,TY-.02,-3.98),(-5.72,CH+.03,-3.98),.05,.007)
drop('M_Lived_CableBlack',(-5.70,TY-.02,-4.12),(-5.76,CH+.03,-4.1),-.1,.008)
# ------------------------------------------------------------------ shelves of tape on the west wall
SX0,SX1,SZ0,SZ1=-5.975,-5.50,-2.40,-1.56;SH=1.92
for zz in(SZ0+.015,SZ1-.015):
    for xx in(SX0+.015,SX1-.015):
        g.box('M_Lived_ShelfGreen',(xx,SH/2,zz),(.03,SH,.03),bevel=.003)
shelves=[.10,.46,.82,1.18,1.54,1.90]
for y in shelves:
    g.box('M_Lived_ShelfGreen',((SX0+SX1)/2,y,(SZ0+SZ1)/2),(SX1-SX0,.018,SZ1-SZ0),bevel=.002)
    g.box('M_Lived_ShelfGreen',(SX1+.001,y-.018,(SZ0+SZ1)/2),(.003,.035,SZ1-SZ0))
for y in shelves[:-1]:
    # a blank label strip here and there on the shelf's lip
    for k in range(3):
        z=R.uniform(SZ0+.1,SZ1-.1)
        g.quad([(SX1+.0032,y-.03,z+.04),(SX1+.0032,y-.03,z-.04),(SX1+.0032,y-.005,z-.04),(SX1+.0032,y-.005,z+.04)],'M_Lived_Paper',uvq(PAPER['label']))
# Reels on edge in their cans.
for si,y in enumerate(shelves[1:5]):
    z=SZ0+.05
    while z<SZ1-.05:
        w=R.choice([.026,.026,.026,.032]);rr=R.choice([.135,.135,.135,.115])
        mat=R.choice(['M_Lived_WhiteCan','M_Lived_WhiteCan','M_Lived_BlueCan','M_Lived_PanelGrey','M_Lived_Beige'])
        lean=R.uniform(-.004,.004)
        cx=(SX0+SX1)/2+R.uniform(-.01,.01)
        g.lathe(mat,(cx,y+.009+rr,z+lean),[(0,0),(rr,0),(rr,w),(0,w)],12,'z')
        if R.random()<.6:
            # a label on the spine
            g.quad([(cx+rr+.0005,y+.009+rr-.05,z+w*.9),(cx+rr+.0005,y+.009+rr-.05,z+w*.1),(cx+rr+.0005,y+.009+rr+.05,z+w*.1),(cx+rr+.0005,y+.009+rr+.05,z+w*.9)],'M_Lived_Paper',uvq(PAPER['label']))
        z+=w+R.uniform(.002,.009)
        if R.random()<.04:z+=R.uniform(.03,.08) # someone has one out
# Disk packs in their plastic cases on the bottom shelf, and a stack.
for z,n in((SZ0+.21,2),(SZ1-.21,1)):
    for k in range(n):
        yy=shelves[0]+.009+k*.15
        g.lathe('M_Lived_BlackPlastic',((SX0+SX1)/2,yy,z),[(0,0),(.185,0),(.19,.02),(.185,.03),(0,.03)],20)
        g.lathe('M_Lived_ReelWindow',((SX0+SX1)/2,yy,z),[(.185,.03),(.178,.13),(.03,.145),(0,.145)],20)
        g.lathe('M_Lived_Chrome',((SX0+SX1)/2,yy+.145,z),[(.03,0),(.03,.008),(0,.01)],10)
# Top of the shelf: printout and a box.
g.box('M_Lived_Card',((SX0+SX1)/2,SH+.14,SZ0+.28),(.40,.24,.34),.05,bevel=.004)
# ------------------------------------------------------------------ status wall on the south wall, over the telex
AX0,AX1,AY0,AY1=-5.30,-3.40,1.30,2.30;AZ=4.492
g.box('M_Lived_PanelGrey',((AX0+AX1)/2,(AY0+AY1)/2,AZ-.02),(AX1-AX0,AY1-AY0,.04),bevel=.004)
fz=AZ-.0405
g.box('M_Lived_BlackPlastic',((AX0+AX1)/2,AY1-.05,fz-.002),(AX1-AX0-.1,.04,.003)) # a blank head strip, no legend
# The 27 antennas, three rows of nine big lamps with a little card under each. Mostly green; one amber; two dead.
kinds=['M_Lived_LampGreen']*27
kinds[5]='M_Lived_LampAmber';kinds[16]='M_Lived_LampOff';kinds[22]='M_Lived_LampOff';kinds[13]='M_Lived_LampAmber'
for r in range(3):
    for c in range(9):
        x=AX0+.14+c*.112;y=AY1-.17-r*.135
        k=kinds[r*9+c]
        g.lathe('M_Lived_Chrome',(x,y,fz),[(.021,0),(.021,-.006),(.016,-.006)],10,'z')
        g.lathe(k,(x,y,fz-.006),[(.016,0),(.009,-.004),(0,-.005)],10,'z')
        g.quad([(x+.03,y-.056,fz-.0008),(x-.03,y-.056,fz-.0008),(x-.03,y-.034,fz-.0008),(x+.03,y-.034,fz-.0008)],'M_Lived_Paper',uvq(PAPER['label']))
# Right-hand third: four round meters and a block of small lamps that flicker.
for i,(mx,my) in enumerate(((-3.86,2.08),(-3.60,2.08),(-3.86,1.82),(-3.60,1.82))):
    g.box('M_Lived_BlackPlastic',(mx,my,fz-.006),(.18,.18,.012),bevel=.004)
    g.lathe('M_Lived_WhiteCan',(mx,my,fz-.012),[(.075,0),(0,-.0005)],16,'z')
    a=R.uniform(.6,2.4)
    g.box('M_Lived_BlackPlastic',(mx+math.cos(a)*.03,my+math.sin(a)*.03-.01,fz-.0135),(.002,.072,.0008),roll=a-1.5708)
for r in range(5):
    lamp_row(fz-.001,1.62-r*.052,-4.0,-3.46,14,['M_Lived_LampGreen','M_Lived_LampRed','M_Lived_LampOff','M_Lived_LampGreen','M_Lived_LampAmber','M_Lived_LampGreen','M_Lived_LampOff'],'z',.006)
for r in range(2):
    lamp_row(fz-.001,1.44-r*.052,-5.2,-4.14,18,['M_Lived_LampOff','M_Lived_LampGreen','M_Lived_LampGreen','M_Lived_LampAmber','M_Lived_LampOff'],'z',.006)
# Its feed, up into the tray.
for i in range(4):drop('M_Lived_CableBlack',(-4.32+i*.05,TY-.02,4.30),(-4.32+i*.05,2.30,4.455),.03,.006,8)
# A sheet of paper people have taped on the panel because nobody remembers why that lamp is amber.
sheet('z',fz-.0015,(-4.72,1.40-.11),.17,.13,'n1',R.uniform(-.06,.06),True,1) if False else None
# ------------------------------------------------------------------ paper on the walls: roster, notes, nothing that can be read
sheet('x',5.988,(-3.40,1.48),.42,.525,'roster',.015,True)
sheet('x',5.987,(-3.03,1.56),.16,.13,'n1',-.05,True)
sheet('x',5.987,(-3.02,1.34),.14,.14,'n4',.04,True)
sheet('x',5.987,(-3.83,1.40),.14,.105,'n2',.03,True)
# South wall, between the door and the board.
sheet('z',4.488,(-.62,1.62),.21,.27,'n7',.02,True)
sheet('z',4.4875,(-.29,1.72),.16,.12,'n5',-.04,True)
sheet('z',4.4875,(-.30,1.46),.2,.15,'n6',.03,True)
sheet('z',4.4875,(.1,1.62),.24,.17,'n2',-.015,True)
sheet('z',4.4875,(-.68,1.27),.14,.105,'n1',.06,True)
# West wall under the frames at the ops desk.
sheet('x',-5.988,(2.3,1.3),.2,.15,'n4',.03,False)
sheet('x',-5.988,(2.72,1.32),.16,.2,'n7',-.02,False)
sheet('x',-5.988,(3.14,1.28),.16,.12,'n1',.05,False)
# On the side of the RX rack.
# Bits taped on the monitors' cases, off the glass: the left and the right one.
sheet('x',-2.402,(-3.95,1.1),.09,.07,'n1',.04,True,1)
sheet('x',2.402,(-3.95,1.1),.09,.07,'n5',-.05,False,1)
g.plane_on('M_Lived_Tape','y',1.2505,(-.12,-3.92),.1,.02,.2)
g.plane_on('M_Lived_Paper','y',1.2505,(.1,-3.97),.1,.07,.1,uvq(PAPER['n4']))
g.plane_on('M_Lived_Tape','y',1.2512,(.1,-3.94),.05,.015,.1)
# ------------------------------------------------------------------ the console: coffee, ash, a lamp
Y=.78
mug(-1.42,Y,-3.63,turn=.7);mug(.92,Y,-3.62,turn=2.6);mug(-2.98,Y,-3.72,'M_Lived_BlueCan',turn=-.5,coffee=False);foam_cup(1.56,Y,-3.98);foam_cup(-.62,Y,-4.12)
spill_ring(-1.42+.08,Y+.0006,-3.76,.045);spill_ring(.84,Y+.0006,-3.71,.045);spill_ring(-.32,Y+.0006,-3.58,.045);spill_ring(1.28,Y+.0006,-3.52,.045);spill_ring(3.2,Y+.0006,-3.6,.045)
ashtray(-.78,Y,-3.94,7);cig_pack(-.56,Y,-3.72,.4);ashtray(3.34,Y,-3.72,4);cig_pack(3.2,Y,-3.95,-.2)
# Worn laminate along the front edge where forearms lie, in front of each chair.
for x in(-2.23,.05,2.07):
    v0,v1=BAND[1];u=R.random()
    g.quad([(x-.62,Y+.0007,-3.455),(x+.62,Y+.0007,-3.455),(x+.62,Y+.0007,-3.53),(x-.62,Y+.0007,-3.53)],'M_Lived_Grime',[(u,v0),(u+.3,v0),(u+.3,v1),(u,v1)])
    for k in range(3):
        bx=x+R.uniform(-.55,.55);g.plane_on('M_Lived_Grime','y',Y+.0009,(bx,-3.47-R.random()*.03),.03,.012,R.uniform(-.2,.2),uvq(GRIME['burn']))
# A gooseneck someone brought from home; the only warm thing on the console at night.
LX,LZ=-1.18,-4.14
g.lathe('M_Lived_BlackPlastic',(LX,Y,LZ),[(0,0),(.075,0),(.078,.012),(.05,.026),(.012,.03),(0,.03)],16)
neck=[(LX,Y+.03,LZ),(LX,Y+.14,LZ-.005),(LX+.01,Y+.27,LZ+.01),(LX+.03,Y+.36,LZ+.06),(LX+.05,Y+.40,LZ+.13),(LX+.055,Y+.395,LZ+.18)]
g.tube('M_Lived_Chrome',neck,.006,6)
hx,hy,hz=LX+.055,Y+.385,LZ+.2
g.lathe('M_Lived_ShelfGreen',(hx,hy-.075,hz),[(.082,0),(.075,.012),(.04,.07),(.02,.09),(0,.092)],16)
g.lathe('M_Lived_BulbWarm',(hx,hy-.075,hz),[(.03,.002),(.034,.03),(.018,.055),(0,.06)],10)
# A power strip and the lamp's lead along the back.
g.tube('M_Lived_CableBeige',[(LX,Y+.012,LZ-.06),(LX-.15,Y+.008,LZ-.14),(LX-.5,Y+.006,LZ-.17),(LX-.9,Y+.006,LZ-.16)],.004,5)
# From the real thing (NRAO's old VLA control room): a roll of masking tape left out, a cut-down box of pens, a lunch pail, long print-outs taped down a rack's side.
def tape_roll(x,y,z):
    g.lathe('M_Lived_Tape',(x,y,z),[(.026,0),(.05,0),(.05,.025),(.026,.025),(.026,0)],16)
def pen_box(x,y,z,yaw=0.):
    g.box('M_Lived_Card',(x,y+.004,z),(.24,.008,.13),yaw)
    c,sn=math.cos(yaw),math.sin(yaw)
    for dx,dz,w,d,h in((0,-.06,.24,.008,.09),(0,.06,.24,.008,.045),(-.116,0,.008,.13,.07),(.116,0,.008,.13,.07)):
        g.box('M_Lived_Card',(x+dx*c+dz*sn,y+h/2,z-dx*sn+dz*c),(w,h,d),yaw)
    for k in range(9):
        px=x+R.uniform(-.09,.09);pz=z+R.uniform(-.04,.04);m=R.choice(['M_Lived_BlueCan','M_Lived_LampOff','M_Lived_Beige','M_Lived_BlackPlastic'])
        g.tube(m,[(px,y+.01,pz),(px+R.uniform(-.03,.03),y+.135,pz-.035)],.004,5)
tape_roll(.62,Y,-3.5);tape_roll(3.0,Y,-3.52);pen_box(1.62,Y,-3.62,.12);pen_box(-2.92,Y,-3.98,-.2)
# A framed snapshot someone brought in, on the console's back; a dented lunch pail on the file cabinets; binders standing on the console's end.
g.box('M_Lived_BlackPlastic',(-.34,Y+.075,-4.12),(.11,.15,.012),.3,-.14)
g.box('M_Lived_ReelWindow',(-.335,Y+.075,-4.112),(.09,.125,.002),.3,-.14)
LX0,LY0,LZ0=-5.68,1.40,-.72
g.box('M_Lived_TrayGalv',(LX0,LY0+.08,LZ0),(.18,.16,.28),.15,bevel=.012)
g.lathe('M_Lived_TrayGalv',(LX0,LY0+.16,LZ0),[(.086,0),(.07,.05),(0,.06)],12)
g.tube('M_Lived_Chrome',[(LX0,LY0+.21,LZ0-.08),(LX0,LY0+.25,LZ0),(LX0,LY0+.21,LZ0+.08)],.006,5)
for k,(c,w) in enumerate((('M_Lived_BlueCan',.055),('M_Lived_BlackPlastic',.07),('M_Lived_ShelfGreen',.05),('M_Lived_BlueCan',.06))):
    g.box(c,(3.05+k*.075,Y+.15,-4.28),(w,.3,.24),.0,bevel=.004)
# Long fan-fold strips taped down the side of the rack, overlapping, curling a little at the foot.
for k,(z0,w,h,top) in enumerate(((5.42,.13,.86,1.86),(5.58,.13,1.2,1.84),(5.76,.12,.64,1.82))):
    g.plane_on('M_Lived_Paper','z',-2.7135-k*.0006,(z0,top-h/2),w,h,R.uniform(-.006,.006),uvq(W(40,1040,260,1880)),True)
    g.plane_on('M_Lived_Tape','z',-2.7146-k*.0006,(z0,top-.01),.06,.02,.1,None,True)
# And a dead strip of it on the floor by the printer, half under the table.
# A small status box at the console's east end, facing the room: what lets the console be read in the dark.
g.box('M_Lived_PanelGrey',(3.2,Y+.09,-4.24),(.36,.18,.16),bevel=.006)
for r_ in range(2):
    for c_ in range(6):
        k=['M_Lived_LampGreen','M_Lived_LampGreen','M_Lived_LampRed','M_Lived_LampGreen','M_Lived_LampAmber','M_Lived_LampGreen'][(c_+r_*2)%6]
        g.lathe(k,(3.07+c_*.052,Y+.13-r_*.05,-4.159),[(.007,0),(.004,.003),(0,.0035)],8,'z')
# ------------------------------------------------------------------ the supervisor's desk and the ops desk
ashtray(1.95,Y,2.28,6);cig_pack(2.0,Y,2.82,.5);spill_ring(2.2,Y+.0006,2.95,.045);spill_ring(3.38,Y+.0006,2.32,.045);
ashtray(-5.3,Y,1.97,3);mug(-5.72,Y,3.02,turn=1.9);spill_ring(-5.3,Y+.0006,2.95,.045)
# Coffee corner, on the cabinet by the machine.
K=.92
for i in range(3):
    pass
g.lathe('M_Lived_Foam',(4.14,K,4.3),[(0,0),(.028,0),(.041,.098),(.042,.1),(.042,.27),(.0395,.27),(0,.27)],12)
mug(4.5,K,4.33,turn=.2,coffee=False);mug(4.62,K,4.2,'M_Lived_Beige',turn=3.3,coffee=False)
g.lathe('M_Lived_Chrome',(4.3,K,4.33),[(0,0),(.05,0),(.05,.14),(.046,.145),(0,.145)],14) # a can of creamer, label worn off
g.lathe('M_Lived_BlueCan',(4.3,K+.01,4.33),[(.0505,.03),(.0505,.10)],14)
spill_ring(4.25,K+.0006,4.08,.05);
# ------------------------------------------------------------------ boxes and printout at the console's east end: out of the way, against the window wall
g.box('M_Lived_Card',(3.88,.17,-4.18),(.44,.34,.42),.12,bevel=.006)
g.box('M_Lived_Tape',(3.88,.3405,-4.18),(.44,.001,.055),.12)
g.box('M_Lived_Card',(3.86,.47,-4.20),(.36,.26,.3),-.1,bevel=.005)
g.box('M_Lived_PanelGrey',(4.38,.11,-4.2),(.4,.2,.34),.03,bevel=.003) # a dead modem case
# A fan-fold stack.
yy=.60
for i in range(16):
    g.box('M_Lived_Paper',(3.87+R.uniform(-.003,.003),yy+i*.004,-4.19+R.uniform(-.003,.003)),(.36,.0035,.27),-.1+R.uniform(-.01,.01))
g.plane_on('M_Lived_Paper','y',yy+.0655,(3.87,-4.19),.36,.27,.1,uvq(PAPER['greenbar']))
# And a loop of printout that has slid off the rack end.
g.box('M_Lived_Paper',(4.95,.12,-3.15),(.34,.0035,.25),.5)
# ------------------------------------------------------------------ grime
# The foot of every wall, in long strips, broken where things stand in front of them anyway.
def foot(ax,fixed,a,b,flipu=False):
    u=R.random()
    if ax=='x':
        p,q=(fixed,.002,b),(fixed,.002,a)
        if flipu:p,q=(fixed,.002,a),(fixed,.002,b)
    else:
        p,q=(a,.002,fixed),(b,.002,fixed)
        if flipu:p,q=(b,.002,fixed),(a,.002,fixed)
    band_strip(p,q,(0,1,0),.32,0)
foot('x',-5.974,-4.45,4.45);foot('x',5.974,-4.45,1.05,True);foot('x',5.974,2.15,4.45,True);foot('z',4.474,-5.95,-2.95,True);foot('z',4.474,-1.85,5.95,True);foot('z',-4.49,-5.95,5.95)
# Rub where chairs and shoulders touch the wall.
band_strip((-5.974,.72,-.9),(-5.974,.72,-1.45),(0,1,0),.24,2)
band_strip((-1.8,1.02,4.47),(-.3,1.02,4.47),(0,1,0),.22,2) if False else None
band_strip((-.3,.98,4.470),(-1.8,.98,4.470),(0,1,0),.26,2)
band_strip((5.974,.98,.95),(5.974,.98,-.9),(0,1,0),.2,2) if False else None
# Smears beside the doors at hand height.
# Cabinet fronts and the rack's foot: boots.
grime_wall('x',-5.392,(-1.05,.2),.34,.34,'scuff')
grime_wall('x',-5.392,(-.2,.17),.44,.3,'scuff')
grime_wall('x',5.298,(-2.4,.2),.5,.34,'scuff',True)
grime_wall('x',5.178,(-.75,.2),.34,.26,'scuff',True) if False else None
# The ceiling: nicotine round the lights, tiles gone yellow, three old leaks.
CY=2.9955
for x in(-3.5,0,3.5):
    for z in(-1.8,1.8):
        g.plane_on('M_Lived_Grime','y',CY,(x,z),2.5,1.5,0,uvq(GRIME['halo'])) if False else None
        # (faces down)
        w,h=2.6,1.6
        g.quad([(x-w/2,CY,z-h/2),(x+w/2,CY,z-h/2),(x+w/2,CY,z+h/2),(x-w/2,CY,z+h/2)],'M_Lived_Grime',uvq(GRIME['halo']))
# A second, wider and fainter cloud off-centre on each, so they are not boxes.
for x in(-3.5,0,3.5):
    for z in(-1.8,1.8):
        ox,oz=R.uniform(-.4,.4),R.uniform(-.25,.25);w,h=3.4,2.2
        g.quad([(x+ox-w/2,CY-.0002,z+oz-h/2),(x+ox+w/2,CY-.0002,z+oz-h/2),(x+ox+w/2,CY-.0002,z+oz+h/2),(x+ox-w/2,CY-.0002,z+oz+h/2)],'M_Lived_Grime',uvq(GRIME['halo']))
for x,z,s in((-4.8,-2.4,.55),(2.1,3.3,.7),(4.8,.3,.42),(-1.8,3.6,.5)):
    g.quad([(x-s/2,CY-.0004,z-s/2),(x+s/2,CY-.0004,z-s/2),(x+s/2,CY-.0004,z+s/2),(x-s/2,CY-.0004,z+s/2)],'M_Lived_Grime',uvq(GRIME['leak']))
# ------------------------------------------------------------------ a jacket left over a chair, by cloth, not by hand
def cloth_jacket(cx,cz,yaw,col_y):
    bpy.ops.object.select_all(action='DESELECT')
    # flat T of cloth, sewn in one: the body (with a slit at the neck) and the sleeves out to the sides
    nu,nv=30,44;bw,bl=.62,1.26;sw,sl=.21,.62
    verts=[];faces=[];idx={}
    du=(bw+2*sl)/ (nu+2*12);dv=bl/nv
    # grid on metres: i across (with sleeves), j along
    ni=nu+24;x0=-(bw/2+sl)
    for j in range(nv+1):
        for i in range(ni+1):
            x=x0+i*(bw+2*sl)/ni;y=-bl/2+j*dv
            inb=abs(x)<=bw/2+1e-4;insl=abs(y-(bl*.02))<=sw/2+1e-4
            if inb or insl:
                idx[(i,j)]=len(verts);verts.append((x,y,0))
    for (i,j),a in list(idx.items()):
        if (i+1,j) in idx and (i,j+1) in idx and (i+1,j+1) in idx:
            faces.append((a,idx[(i+1,j)],idx[(i+1,j+1)],idx[(i,j+1)]))
    me=bpy.data.meshes.new('jacket');me.from_pydata(verts,[],faces);me.update()
    ob=bpy.data.objects.new('jacket',me);bpy.context.scene.collection.objects.link(ob)
    # A proxy of the chair's back, as measured from the game's chair; the cloth starts above it.
    bpy.ops.mesh.primitive_cube_add(size=1,location=(0,0,col_y-.31));back=bpy.context.object;back.scale=(.50,.10,.62)
    bpy.ops.mesh.primitive_cube_add(size=1,location=(0,-.26,.47));seat=bpy.context.object;seat.scale=(.52,.5,.09)
    for o in(back,seat):
        o.modifiers.new('C','COLLISION');o.collision.thickness_outer=.012;o.collision.cloth_friction=30
    ob.rotation_euler=(0,0,0)
    ob.location=(0,.0,col_y+.09)
    bpy.context.view_layer.objects.active=ob;ob.select_set(True)
    # Rotate the flat cloth so it hangs in Y and comes over the top: bend it into an inverted V first.
    m=ob.data
    for v in m.vertices:
        y=v.co.y
        v.co=(v.co.x,y*.55,-abs(y)*.72+R.uniform(0,.004))
    c=ob.modifiers.new('Cloth','CLOTH');st=c.settings
    st.mass=.25;st.tension_stiffness=9;st.compression_stiffness=9;st.shear_stiffness=5;st.bending_stiffness=.9;st.quality=8
    st.effector_weights.gravity=1.;c.collision_settings.use_self_collision=False;c.collision_settings.distance_min=.006
    sc=bpy.context.scene;sc.frame_start=1;sc.frame_end=60
    for f in range(1,61):sc.frame_set(f)
    dg=bpy.context.evaluated_depsgraph_get();ev=ob.evaluated_get(dg);em=ev.to_mesh()
    out=[ob.matrix_world@v.co for v in em.vertices];fa=[tuple(p.vertices) for p in em.polygons]
    cs,sn=math.cos(yaw),math.sin(yaw)
    base=len(g.v)
    for v in out:
        # Blender of this little set: x across, y out behind the chair, z up. To the game: rotate by yaw about the chair.
        lx,ly,lz=v.x,v.y,v.z
        gx=cx+lx*cs+ly*sn;gz=cz-lx*sn+ly*cs
        g.vert((gx,lz,gz))
    for f in fa:g.face([base+i for i in f],'M_Lived_Cloth',True,None)
    for o in(ob,back,seat):bpy.data.objects.remove(o,do_unlink=True)
    print('jacket',len(out),'verts')
try:
    cloth_jacket(2.07,-2.95+.21,0.,1.045)
except Exception as e:
    print('JACKET FAILED',e);raise
# ------------------------------------------------------------------ the floor's middle: a rubber runner worn to the console from the corridor door, a cable cover taped across to the rack
def flat(mat,x0,x1,z0,z1,y=.0042,uv=None):g.quad([(x0,y,z1),(x1,y,z1),(x1,y,z0),(x0,y,z0)],mat,uv)
g.box('M_Lived_PanelGrey',(4.25,.012,-3.0),(.12,.024,1.9),-.52,bevel=.006)
for z_ in(-3.6,-3.0,-2.4):g.plane_on('M_Lived_Tape','y',.0245,(4.25+(z_+3.0)*-.5,z_),.13,.06,.5)
# a dropped curl of fan-fold and a paper bin by the supervisor's desk, against it
g.box('M_Lived_Paper',(1.1,.003,-1.4),(.36,.003,.26),.8)
g.lathe('M_Lived_RackSteel',(1.82,0,3.2),[(0,0),(.13,0),(.155,.33),(.15,.33),(.125,.012),(0,.012)],14)
for k in range(4):g.box('M_Lived_Paper',(1.82+R.uniform(-.05,.05),.3+k*.02,3.2+R.uniform(-.05,.05)),(.09,.06,.08),R.random()*3,bevel=.02)
# ------------------------------------------------------------------ the corridor, its own mesh (it hangs off the annex, not the room)
gc=G.Geo(11)
WN,WS=4.835,6.579
def cband(z,face,x0,x1,y0,h,band):
    v0,v1=BAND[band];u=R.random();L=x1-x0
    zz=z+face*.004
    q=[(x0,y0,zz),(x1,y0,zz),(x1,y0+h,zz),(x0,y0+h,zz)];uv=[(u,v0),(u+L/4,v0),(u+L/4,v1),(u,v1)]
    if face<0:q=[q[1],q[0],q[3],q[2]];uv=[uv[1],uv[0],uv[3],uv[2]]
    gc.quad(q,'M_Lived_Grime',uv)
for z,face,spans in((WN,1,[(-5.96,-2.96),(-1.84,1.96)]),(WS,-1,[(-5.96,-4.47),(-3.33,-.96)])):
    for x0,x1 in spans:
        cband(z,face,x0,x1,.002,.30,0)
def cpatch(z,face,x,y,w,h,name):
    zz=z+face*.0045;u0,v0,u1,v1=GRIME[name]
    q=[(x-w/2,y,zz),(x+w/2,y,zz),(x+w/2,y+h,zz),(x-w/2,y+h,zz)];uv=[(u0,v0),(u1,v0),(u1,v1),(u0,v1)]
    if face<0:q=[q[1],q[0],q[3],q[2]];uv=[uv[1],uv[0],uv[3],uv[2]]
    gc.quad(q,'M_Lived_Grime',uv)
# Notes on the walls, taped, illegible; a fire extinguisher on its bracket; a wall phone's old mounting plate.
def csheet(z,face,x,y,w,h,name,roll=0.):
    gc.plane_on('M_Lived_Paper','z',z+face*.003,(x,y),w,h,roll,uvq(PAPER[name]),face<0)
    gc.plane_on('M_Lived_Tape','z',z+face*.0038,(x,y+h/2),.055,.02,R.uniform(-.2,.2),None,face<0)
csheet(WN,1,-4.1,1.55,.21,.27,'n7',.02);csheet(WN,1,-3.75,1.62,.16,.12,'n5',-.04);csheet(WS,-1,-1.6,1.5,.2,.15,'n6',.03);csheet(WN,1,.3,1.45,.24,.17,'n2',-.015)
gc.box('M_Lived_RackSteel',(.9,1.1,WN+.025),(.12,.05,.05))
gc.lathe('M_Lived_LampRed',(.9,.62,WN+.11),[(0,0),(.075,0),(.078,.03),(.078,.46),(.06,.5),(.02,.53),(0,.53)],14) if False else gc.lathe('M_Lived_RedPaint',(.9,.62,WN+.11),[(0,0),(.075,0),(.078,.03),(.078,.46),(.06,.5),(.02,.53),(0,.53)],14)
gc.tube('M_Lived_BlackPlastic',[(.92,1.13,WN+.12),(.97,1.05,WN+.16),(.96,.8,WN+.18)],.008,5)
gc.box('M_Lived_PanelGrey',(-5.3,1.45,WN+.006),(.16,.24,.012),bevel=.002)
for x_ in(-5.34,-5.26):gc.box('M_Lived_Chrome',(x_,1.52,WN+.014),(.012,.012,.006))
# Ceiling: a couple of old leaks over the corridor.
for x,s_ in((-4.6,.5),(-.2,.42),(1.3,.36)):
    gc.quad([(x-s_/2,2.748,5.7-s_/2),(x+s_/2,2.748,5.7-s_/2),(x+s_/2,2.748,5.7+s_/2),(x-s_/2,2.748,5.7+s_/2)],'M_Lived_Grime',uvq(GRIME['leak']))
corr_ob,ct=G.build(gc,'SM_Lived_CorridorDressing')
ct=G.export_fbx(corr_ob,ROOT/'Export/SM_Lived_CorridorDressing.fbx')
# ------------------------------------------------------------------ write out
floor_ob,ft=G.build(fg,'SM_Lived_ControlFloor')
ob,tr=G.build(g,'SM_Lived_ControlDressing')
ft=G.export_fbx(floor_ob,ROOT/'Export/SM_Lived_ControlFloor.fbx')
tr=G.export_fbx(ob,ROOT/'Export/SM_Lived_ControlDressing.fbx')
assert tr<70000,tr
used={m.name for m in ob.data.materials};dress_used=sorted(used)
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'Lived_Control.blend'))
def bounds(o):
    vs=[v.co for v in o.data.vertices];return [[min(v[k] for v in vs) for k in range(3)],[max(v[k] for v in vs) for k in range(3)]]
used|={m.name for m in corr_ob.data.materials}
(ROOT/'lived-control-materials.json').write_text(json.dumps([r for r in ROWS if r['name'] in used]+[FLOOR_ROW],indent=1))
(ROOT/'lived-control-contract.json').write_text(json.dumps({'units':'metres','mapping':'Blender (x,-z,y) to UE (100x,100z,100y)','meshes':{'SM_Lived_ControlFloor':{'triangles':ft,'materials':['M_Lived_VCTFloor'],'bounds_local_m':bounds(floor_ob)},'SM_Lived_ControlDressing':{'triangles':tr,'materials':dress_used,'bounds_local_m':bounds(ob)},'SM_Lived_CorridorDressing':{'triangles':ct,'materials':sorted({m.name for m in corr_ob.data.materials}),'bounds_local_m':bounds(corr_ob)}}},indent=1))
print('DONE triangles',tr,'materials',len(used))
for r in sorted(used):print(' ',r)
