"""SIGNAL / 47: the one farmhouse. Plan v1 (Docs/Design/Concepts/Farmhouse), its walls and doors as S47FarmHouse.cpp says, in and out as Tom's concept pictures.
blender -b -t 2 --python build_house.py
Local frame as the other farm buildings': mesh (x,y,z) = plan (px-6, 4.5-py, up), placed at road (98, PadY, 220.5) with yaw pi. In geo (three) terms: (px-6, up, py-4.5).
"""
import json,math,random,sys
from pathlib import Path
import bpy
ROOT=Path(__file__).resolve().parent
sys.path.insert(0,str(ROOT.parent))
import geo as G
bpy.ops.wm.read_factory_settings(use_empty=True)
R=random.Random(1986)
g=G.Geo()
def p(px,up,py):return (px-6.,up,py-4.5)
STUCCO,WOOD,PAINT,TIN,GLASS,CONC,STEEL,LIT='M_Farm_Stucco','M_Farm_Wood','M_Farm_Paint','M_Farm_Tin','M_Farm_Glass','M_Farm_Concrete','M_Farm_Steel','M_Farm_WindowLit'
PLASTER,PLANK,BEAM,LINO,RUG,SOFA,TILE,CHINA,IRON,MATTRESS,CURTAIN,TV,KNOB,CAB,GRIME,PAPER,TAPEB='M_House_Plaster','M_House_Plank','M_House_Beam','M_House_Lino','M_House_Rug','M_House_Sofa','M_House_Tile','M_House_China','M_House_Iron','M_House_Mattress','M_House_Curtain','M_House_TV','M_House_Knob','M_House_CabPaint','M_Lived_Grime','M_Lived_Paper','M_House_Blind'
WH,DH,SILL,WH2,PH=2.6,2.05,.9,2.1,3.32
W_,D_=12.,9.
WALLS=[('north',0,0,12,.3,[(1.6,3.4,0),(5.5,6.5,1),(7.4,8.6,0),(10.5,11.3,0)]),('south',0,8.7,12,9,[(1.3,3.1,0),(5.5,6.5,1),(8,9.8,0)]),('west',0,.3,.3,8.7,[(5.8,7.1,0)]),('east',11.7,.3,12,8.7,[(6.2,7.5,0)]),
 ('hallW',5.1,.3,5.25,8.7,[(1.8,2.7,1),(6.1,7,1)]),('hallE',6.75,.3,6.9,8.7,[(1.8,2.7,1),(6.1,7,1)]),('midW',.3,4.4,5.1,4.55,[]),('midE',6.9,4.4,11.7,4.55,[]),('bath',9.8,.3,9.95,4.4,[(2.6,3.4,1)])]
ROOMS={'bed':(.3,5.1,.3,4.4),'kit':(.3,5.1,4.55,8.7),'hall':(5.25,6.75,.3,8.7),'back':(6.9,9.8,.3,4.4),'bath':(9.95,11.7,.3,4.4),'stue':(6.9,11.7,4.55,8.7)}
OUTER={'north','south','west','east'}
def box(mat,px0,px1,up0,up1,py0,py1,bevel=0.):
    a=p((px0+px1)/2,(up0+up1)/2,(py0+py1)/2)
    g.box(mat,a,(abs(px1-px0),abs(up1-up0),abs(py1-py0)),bevel=bevel)
# Wall skins: one face at a time, as rectangles round the openings. Outer stucco, inner plaster; each interior wall has a room each side.
def skin_x(px,y0,y1,top0,top1,mat,facing,bottom=None):
    # A face on constant plan x; facing +1 looks towards growing px.
    a,b=(y1,y0) if facing>0 else (y0,y1)
    g.quad([p(px,top0,a),p(px,top0,b),p(px,top1,b),p(px,top1,a)],mat)
def skin_y(py,x0,x1,top0,top1,mat,facing):
    a,b=(x0,x1) if facing>0 else (x1,x0)
    g.quad([p(a,top0,py),p(b,top0,py),p(b,top1,py),p(a,top1,py)],mat)
def wall_face(along,fixed,a0,a1,bottom,top,opens,mat,facing):
    at=a0
    def R_(f,t,lo,hi):
        if t-f<.002 or hi-lo<.002:return
        if along=='x':skin_y(fixed,f,t,lo,hi,mat,facing)
        else:skin_x(fixed,f,t,lo,hi,mat,facing)
    for o0,o1,door in opens:
        o0=max(o0,a0);o1=min(o1,a1)
        if o1<=o0:continue
        R_(at,o0,bottom,top)
        if not door:R_(o0,o1,bottom,SILL)
        R_(o0,o1,DH if door else WH2,top)
        at=o1
    R_(at,a1,bottom,top)
def reveal(along,c0,c1,o0,o1,door,mat):
    # the thickness of the wall round an opening
    lo=0. if door else SILL;hi=DH if door else WH2
    if along=='x':
        for x,f in((o0,1),(o1,-1)):skin_x(x,c0,c1,lo,hi,mat,f)
        g.quad([p(o0,hi,c0),p(o1,hi,c0),p(o1,hi,c1),p(o0,hi,c1)],mat)
        if not door:g.quad([p(o0,lo,c1),p(o1,lo,c1),p(o1,lo,c0),p(o0,lo,c0)],mat)
    else:
        for y,f in((o0,-1),(o1,1)):skin_y(y,c0,c1,lo,hi,mat,f)
        g.quad([p(c0,hi,o1),p(c1,hi,o1),p(c1,hi,o0),p(c0,hi,o0)],mat)
        if not door:g.quad([p(c0,lo,o0),p(c1,lo,o0),p(c1,lo,o1),p(c0,lo,o1)],mat)
for name,x0,y0,x1,y1,opens in WALLS:
    along='x' if x1-x0>y1-y0 else 'y'
    a0,a1=(x0,x1) if along=='x' else (y0,y1)
    c0,c1=(y0,y1) if along=='x' else (x0,x1)
    if name in OUTER:
        outside=(name in('north','west'))
        # Outer face goes well down: the ground under a level floor is not level. Plaster inside, and the reveal is plaster.
        fo=c0 if outside else c1;fi=c1 if outside else c0
        fac_o=-1 if outside else 1;fac_i=1 if outside else -1
        inner0,inner1=(.3,8.7) if along=='y' else (.3,11.7)
        wall_face(along,fo,a0-(0 if along=='x' else .3),a1+(0 if along=='x' else .3),-1.3,WH+.72,opens,STUCCO,fac_o)
        wall_face(along,fi,inner0,inner1,0,WH,opens,PLASTER,fac_i)
    else:
        wall_face(along,c0,a0,a1,0,WH,opens,PLASTER,-1)
        wall_face(along,c1,a0,a1,0,WH,opens,PLASTER,1)
    for o0,o1,door in opens:
        reveal(along,c0,c1,o0,o1,door,STUCCO if name in OUTER else PLASTER)
# ------- floors. Plank through the house; lino in the kitchen and the bath, on top of them. 2 mm apart so the kitchen's lies on the boards.
def floor(name,mat,up=0.,scale=None):
    x0,x1,y0,y1=ROOMS[name];g.quad([p(x0,up,y1),p(x1,up,y1),p(x1,up,y0),p(x0,up,y0)],mat)
for n in('bed','hall','back','stue'):floor(n,PLANK)
for n in('kit','bath'):floor(n,LINO,.001)
# door thresholds
for name,x0,y0,x1,y1,opens in WALLS:
    along='x' if x1-x0>y1-y0 else 'y'
    for o0,o1,door in opens:
        if not door:continue
        if along=='x':box(WOOD,o0,o1,0,.012,y0-.02,y1+.02)
        else:box(WOOD,x0-.02,x1+.02,0,.012,o0,o1)
# ------- ceilings: boards with vigas across each room, a soffit round them; the beams carry the concept's weight
for rname,(x0,x1,y0,y1) in ROOMS.items():
    g.quad([p(x0,WH,y0),p(x1,WH,y0),p(x1,WH,y1),p(x0,WH,y1)],PLASTER)
    # latillas: flat boards the other way
    long_x=(x1-x0)>=(y1-y0)
    beams=max(2,int((x1-x0)/.8))
    for i in range(beams):
        x=x0+(i+.5)*(x1-x0)/beams
        r=R.uniform(.075,.095);y=WH-r*.75
        g.lathe(BEAM,p(x,y,y0),[(r,0),(r*1.02,(y1-y0)*.3),(r*.97,(y1-y0)*.7),(r,y1-y0)],9,'z')
    for i in range(int((y1-y0)/.45)):
        y=y0+(i+.5)*(y1-y0)/int((y1-y0)/.45)
        box(BEAM,x0,x1,WH-.012,WH,y-.2,y+.2) if False else None
# ------- baseboards and door casings, dark; window trim and blinds
def base_run(along,fixed,a0,a1,opens,facing):
    at=a0
    def run(f,t):
        if t-f<.01:return
        if along=='x':box(WOOD,f,t,0,.13,fixed+(0 if facing>0 else -.016),fixed+(.016 if facing>0 else 0),.003)
        else:box(WOOD,fixed+(0 if facing>0 else -.016),fixed+(.016 if facing>0 else 0),0,.13,f,t,.003)
    for o0,o1,door in opens:
        if door:run(at,o0-.08);at=o1+.08
    run(at,a1)
def wainscot(along,fixed,a0,a1,opens,facing,top=.86):
    at=a0
    def run(f,t):
        if t-f<.05:return
        d0,d1=(fixed,fixed+.012) if facing>0 else (fixed-.012,fixed)
        e0,e1=(fixed,fixed+.03) if facing>0 else (fixed-.03,fixed)
        n=max(1,int((t-f)/.11))
        for k in range(n):
            u0=f+k*(t-f)/n;u1=f+(k+1)*(t-f)/n-.004
            if along=='x':box(CAB,u0,u1,.13,top,d0,d1)
            else:box(CAB,d0,d1,.13,top,u0,u1)
        if along=='x':box(WOOD,f,t,top,top+.045,e0,e1,.004)
        else:box(WOOD,e0,e1,top,top+.045,f,t,.004)
    for o0,o1,door in opens:
        if door:run(at,o0-.09);at=o1+.09
    run(at,a1)
def casing(along,fixed,o0,o1,door,facing,mat=WOOD,w=.085,dp=.02):
    hi=DH if door else WH2;lo=0 if door else SILL
    f0,f1=(fixed,fixed+dp) if facing>0 else (fixed-dp,fixed)
    if along=='x':
        box(mat,o0-w,o0,lo,hi+.002,f0,f1,.004);box(mat,o1,o1+w,lo,hi+.002,f0,f1,.004);box(mat,o0-w-.015,o1+w+.015,hi,hi+w+.02,f0,f1+.008,.004)
        if not door:box(mat,o0-w-.02,o1+w+.02,lo-.035,lo,f0,f1+.03,.004)
    else:
        box(mat,f0,f1,lo,hi+.002,o0-w,o0,.004);box(mat,f0,f1,lo,hi+.002,o1,o1+w,.004);box(mat,f0,f1+.008,hi,hi+w+.02,o0-w-.015,o1+w+.015,.004)
        if not door:box(mat,f0,f1+.03,lo-.035,lo,o0-w-.02,o1+w+.02,.004)
for name,x0,y0,x1,y1,opens in WALLS:
    along='x' if x1-x0>y1-y0 else 'y'
    a0,a1=(x0,x1) if along=='x' else (y0,y1)
    c0,c1=(y0,y1) if along=='x' else (x0,x1)
    outer=name in OUTER
    outside=name in('north','west')
    faces=[]
    if outer:
        faces=[(c1,1)] if outside else [(c0,-1)]
        if along=='x':a0,a1=.3,11.7
        else:a0,a1=.3,8.7
    else:faces=[(c0,-1),(c1,1)]
    for fx,fac in faces:
        base_run(along,fx,a0,a1,opens,fac)
        if name not in('bath',) and not (outer and name in('north',) and False):wainscot(along,fx,a0,a1,opens,fac)
        for o0,o1,door in opens:
            # Turquoise doors' frames as in the concepts, in the hall; the rest dark wood
            casing(along,fx,o0,o1,door,fac,PAINT if door and name.startswith('hall') and False else WOOD)
# Outside, turquoise frames and heavy lintels (as S47FarmHouse's cubes had them).
for name,x0,y0,x1,y1,opens in WALLS:
    if name not in OUTER:continue
    along='x' if x1-x0>y1-y0 else 'y'
    outside=name in('north','west')
    c0,c1=(y0,y1) if along=='x' else (x0,x1);fo=c0 if outside else c1;fac=-1 if outside else 1
    for o0,o1,door in opens:
        hi=DH if door else WH2;lo=0 if door else SILL
        casing(along,fo,o0,o1,door,fac,PAINT,.09,.035)
        if along=='x':box(WOOD,o0-.22,o1+.22,hi+.02,hi+.24,fo+(-.02 if fac>0 else -.16),fo+(.06 if fac>0 else .02),.012)
        else:box(WOOD,fo+(-.02 if fac>0 else -.16),fo+(.06 if fac>0 else .02),hi+.02,hi+.24,o0-.22,o1+.22,.012)
# ------- windows: sash two lights, mullion, glass with dirt; venetian blinds, some down
def window(along,c0,c1,o0,o1,facing_out,blind,lit=False,curtain=False):
    inner=c1 if facing_out<0 else c0
    mid=inner+facing_out*.07
    # Sash at the middle of the wall.
    def B(a0,a1,up0,up1,d0,d1,m=PAINT):
        if along=='x':box(m,a0,a1,up0,up1,d0,d1)
        else:box(m,d0,d1,up0,up1,a0,a1)
    fw=.05
    B(o0,o1,SILL,SILL+fw,mid-.025,mid+.025);B(o0,o1,WH2-fw,WH2,mid-.025,mid+.025);B(o0,o0+fw,SILL,WH2,mid-.025,mid+.025);B(o1-fw,o1,SILL,WH2,mid-.025,mid+.025)
    m=(o0+o1)/2;B(m-.018,m+.018,SILL,WH2,mid-.02,mid+.02);h=(SILL+WH2)/2;B(o0,o1,h-.018,h+.018,mid-.02,mid+.02)
    # glass: one sheet, seen both ways; the lit one a second sheet out, one-sided
    if along=='x':
        pts=[p(o0,SILL,mid),p(o1,SILL,mid),p(o1,WH2,mid),p(o0,WH2,mid)]
        # No glass: it went long ago, and a pane would stop the light from outside, which the scenes inside need.
    else:
        pts=[p(mid,SILL,o1),p(mid,SILL,o0),p(mid,WH2,o0),p(mid,WH2,o1)]
        pass
    ins=c1 if facing_out<0 else c0  # inner face plane
    insd=-facing_out
    if blind>0:
        n=int((WH2-SILL)*blind/.055)
        for i in range(n):
            up=WH2-.04-i*.055
            tilt=R.uniform(-.02,.02)
            B(o0+.03,o1-.03+tilt,up-.004,up,ins+insd*.022-.025,ins+insd*.022+.025,TAPEB)
        B(o0+.01,o1-.01,WH2-.035,WH2+.01,ins+insd*.02-.03,ins+insd*.02+.03,TAPEB)
        B(o0+.05,o0+.055,WH2-(n*.055)-.1,WH2,ins+insd*.022-.002,ins+insd*.022+.002,TAPEB)
    if curtain:
        for k,(a,b) in enumerate(((o0-.16,o0+.34),(o1-.34,o1+.16))):
            w=int((b-a)/.05)
            for j in range(w):
                x0=a+j*(b-a)/w;x1=a+(j+1)*(b-a)/w
                d0=ins+insd*(.055+.018*math.sin(j*1.9)*(1.0))
                d1=ins+insd*(.055+.018*math.sin((j+1)*1.9)*(1.0))
                for bot,top in((SILL-.32,WH2+.14),):
                    if along=='x':
                        q=[p(x0,bot,d0),p(x1,bot,d1),p(x1,top,d1),p(x0,top,d0)]
                    else:
                        q=[p(d0,bot,x0),p(d1,bot,x1),p(d1,top,x1),p(d0,top,x0)]
                    g.quad(q,CURTAIN,None,True);g.quad(q[::-1],CURTAIN,None,True)
        if along=='x':g.cyl(IRON,p(o0-.24,WH2+.15,ins+insd*.055),.009,(o1-o0)+.48,'x',6)
        else:g.cyl(IRON,p(ins+insd*.055,WH2+.15,o0-.24),.009,(o1-o0)+.48,'z',6) if False else box(IRON,ins+insd*.055-.008,ins+insd*.055+.008,WH2+.145,WH2+.16,o0-.24,o1+.24)
# Which face is outside: north wall outside is py<0 (-1), south +1; west px<0 (-1), east +1
window('x',0,.3,1.6,3.4,-1,.28);window('x',0,.3,7.4,8.6,-1,.6);window('x',0,.3,10.5,11.3,-1,.35)
window('x',8.7,9,1.3,3.1,1,.3);window('x',8.7,9,8,9.8,1,.5)
window('y',0,.3,5.8,7.1,-1,.5);window('y',11.7,12,6.2,7.5,1,.45)
# ------- the roof as this country builds it: parapet, cap, canales. Roof deck, tar and gravel.
g.quad([p(.3,WH+.3,8.7),p(11.7,WH+.3,8.7),p(11.7,WH+.3,.3),p(.3,WH+.3,.3)],CONC)
def ring(out,up0,up1,mat):
    box(mat,-out,W_+out,up0,up1,-out,.3+out);box(mat,-out,W_+out,up0,up1,D_-.3-out,D_+out);box(mat,-out,.3+out,up0,up1,.3+out,D_-.3-out);box(mat,W_-.3-out,W_+out,up0,up1,.3+out,D_-.3-out)
PT=WH+.72
for x0,x1,y0,y1 in((.3,W_-.3,.3,.3),(.3,W_-.3,D_-.3,D_-.3)):
    skin_y(y0,x0,x1,WH+.3,PT,STUCCO,1 if y0<4 else -1)
for x0 in(.3,W_-.3):skin_x(x0,.3,D_-.3,WH+.3,PT,STUCCO,1 if x0<4 else -1)
ring(.035,PT,PT+.07,WOOD)
def canal(x0,y0,x1,y1):
    box(WOOD,x0,x1,WH+.32,WH+.46,y0,y1,.008)
for x in(2.2,9.6):canal(x-.09,-.62,x+.09,.3)
for y in(2.,6.5):canal(-.62,y-.09,.3,y+.09);canal(W_-.3,y-.09,W_+.62,y+.09)
box(STUCCO,8.5,9.25,WH+.3,WH+1.6,3.,3.7,.015);box(WOOD,8.42,9.33,WH+1.6,WH+1.68,2.92,3.78,.01)
g.cyl(IRON,p(4.1,WH+.3,2.2),.05,.9);g.lathe(IRON,p(4.1,WH+1.2,2.2),[(.09,0),(.03,.08)],8)
# a dead swamp cooler
box(STEEL,10.1,11,WH+.3,WH+1.05,5.7,6.6,.01);box(STEEL,10.15,10.95,WH+1.05,WH+1.09,5.75,6.55)
# ------- the veranda on thick beams
FR=D_+2.;BU=WH-.34
g.quad([p(0,-.04,FR),p(W_,-.04,FR),p(W_,-.04,D_),p(0,-.04,D_)],PLANK)
for a,b,c in((0,W_,0),):pass
box(WOOD,0,W_,-.5,-.05,FR-.03,FR,.005);box(WOOD,0,.03,-.5,-.05,D_,FR);box(WOOD,W_-.03,W_,-.5,-.05,D_,FR)
for s0,s1,up,f in((FR,FR+.32,-.2,1),(FR+.32,FR+.64,-.36,1)):box(PLANK,4.8,7.2,up-.05,up,s0,s1,.004);box(WOOD,4.8,7.2,-1,up-.05,s1-.03,s1)
box(TIN,-.06,W_+.06,WH-.1,WH+.04,D_,FR+.14)
box(WOOD,-.09,W_+.09,WH-.14,WH+.08,FR+.14,FR+.2,.006)
box(WOOD,-.02,W_+.02,BU-.2,BU,FR-.24,FR-.02,.01)
for k,x in enumerate([.35+i*1.1 for i in range(11)]):
    r=R.uniform(.078,.098);g.lathe(WOOD,p(x,BU+.09,D_-.02),[(r,0),(r,1.1),(r*.96,2.36),(0,2.37)],9,'z')
for x in(.3,4.,8.,11.7):
    box(WOOD,x-.11,x+.11,-.04,BU-.34,FR-.24,FR-.02,.012)
    box(WOOD,x-.34,x+.34,BU-.34,BU-.2,FR-.22,FR-.04,.012)
# ------- rooms' things. Keep the hall clear; keep every door's swing clear; all against walls. Solids in S47FarmHouse.cpp follow these.
def leg_table(x0,x1,y0,y1,h,mat=WOOD,top=.04):
    box(mat,x0,x1,h-top,h,y0,y1,.006)
    for x in(x0+.04,x1-.04):
        for y in(y0+.04,y1-.04):box(mat,x-.022,x+.022,0,h-top,y-.022,y+.022)
def chair(x,y,turn,mat=WOOD):
    c,s=math.cos(turn),math.sin(turn)
    def T(a,b):return x+a*c-b*s,y+a*s+b*c
    a,b=T(0,0);g.box(mat,p(a,.45,b),(.40,.03,.40),turn*-1 if False else -turn,bevel=.004)
    for u in(-.17,.17):
        for v in(-.17,.17):
            a,b=T(u,v);box(mat,a-.016,a+.016,0,.44,b-.016,b+.016)
    for u in(-.17,.17):
        a,b=T(u,-.18);box(mat,a-.016,a+.016,.45,.9,b-.016,b+.016)
    for k in range(4):
        a,b=T(0,-.18);g.box(mat,p(a,.56+k*.1,b),(.34,.04,.012),-turn)
# --- Soverom: iron bed, nightstand with a drawer out, wardrobe, dresser with a mirror, a rag rug.
BX0,BX1,BY0,BY1=.5,1.95,.42,2.42
for x in(BX0,BX1):
    for y,h in((BY0,1.15),(BY1,.82)):g.cyl(IRON,p(x,0,y),.016,h,'y',8)
for y,h in((BY0,1.13),(BY1,.8)):
    box(IRON,BX0,BX1,h-.018,h,y-.012,y+.012);box(IRON,BX0,BX1,h-.3,h-.285,y-.008,y+.008)
    for k in range(9):g.cyl(IRON,p(BX0+(k+.5)*(BX1-BX0)/9,h-.3,y),.006,.29,'y',5)
for x in(BX0,BX1):box(IRON,x-.012,x+.012,.3,.33,BY0,BY1)
box(MATTRESS,BX0+.03,BX1-.03,.33,.5,BY0+.04,BY1-.04,.04)
# a blanket thrown back, thin folds
for j in range(14):
    y=BY0+1.0+j*.07;h=.515+.012*math.sin(j*1.3)
    box(CURTAIN,BX0-.0,BX1+.0,h,h+.016,y,y+.075,.004)
for s,x in((1,BX0-.012),(-1,BX1-.012)):
    box(CURTAIN,x,x+.024,.14,.52,BY0+1.0,BY1-.02,.004)
box(MATTRESS,BX0+.12,BX1-.12,.5,.6,BY0+.08,BY0+.5,.05)
leg_table(2.05,2.5,.4,.82,.58);box(WOOD,2.08,2.47,.36,.5,.82,1.02,.004) # nightstand with its drawer out
g.lathe(CHINA,p(2.25,.58,.62),[(0,0),(.038,0),(.042,.09),(.038,.09),(0,.08)],10)
# wardrobe by the wall to the hall, door ajar
WX0,WX1=3.9,5.08
box(WOOD,WX0,WX1,0,2.02,.32,.92,.008);box(WOOD,WX0+.03,WX1-.03,.05,1.98,.92,.925)
box(WOOD,WX0+.03,WX0+.6,.05,1.98,.925,.95)
box(WOOD,WX1-.42,WX1-.39,.05,1.98,.93,1.42,.004)
# dresser & mirror on the wall to the kitchen
box(WOOD,.5,1.75,0,.86,3.86,4.38,.01)
for k in range(3):box(WOOD,.54,1.71,.08+k*.26,.31+k*.26,3.84,3.86,.004);box(STEEL,1.0,1.26,.2+k*.26,.215+k*.26,3.825,3.84)
box(WOOD,.72,1.53,1.05,1.82,4.36,4.385,.006)
g.quad([p(.77,1.1,4.358),p(1.48,1.1,4.358),p(1.48,1.77,4.358),p(.77,1.77,4.358)][::-1],GLASS)
g.lathe(CHINA,p(1.45,.86,4.1),[(0,0),(.05,0),(.05,.02),(.13,.09),(.14,.1),(0,.03)],12)
# rag rug
g.quad([p(1.95,.004,1.15),p(3.65,.004,1.15),p(3.65,.004,3.05),p(1.95,.004,3.05)][::-1],RUG,[(0,0),(0,1),(1,1),(1,0)])
# --- Kjøkken: counter with sink under the west window, stove, fridge, wall cupboards; table and two chairs; lino.
box(CAB,.32,.95,0,.86,4.58,7.6,.008);box(WOOD,.3,.98,.86,.9,4.56,7.62,.004)
for y in [4.7+i*.56 for i in range(5)]:
    box(WOOD,.95,.957,.1,.8,y,y+.5,.003);box(STEEL,.957,.965,.62,.72,y+.42,y+.44)
box(STEEL,.42,.86,.885,.905,6.0,6.75,.002) # sink rim
box(IRON,.47,.81,.902,.906,6.05,6.7)
g.tube(STEEL,[p(.4,.9,6.38),p(.4,1.08,6.38),p(.55,1.15,6.38),p(.63,1.08,6.38)],.012,6)
for y in(6.2,6.56):g.cyl(STEEL,p(.36,.9,y),.018,.06,'y',6)
# stove
box(CHINA,.34,.98,0,.92,7.65,8.3,.012);box(IRON,.36,.96,.92,.93,7.67,8.28)
for x,y in((.5,7.82),(.8,7.82),(.5,8.12),(.8,8.12)):g.lathe(IRON,p(x,.93,y),[(.09,0),(.09,.012),(.03,.016),(0,.016)],12)
box(CHINA,.34,.98,.92,1.14,8.26,8.3);box(IRON,.98,.985,.2,.68,7.72,8.22,.002);box(STEEL,.985,1.005,.72,.74,7.75,8.2)
for y in(7.8,7.95,8.1,8.25):g.cyl(IRON,p(.98,.84,y),.016,.03,'x',8)
# fridge, the round-shouldered kind
box(CHINA,3.72,4.45,0,1.55,4.58,5.25,.06);box(CHINA,3.75,4.42,1.55,1.62,4.6,5.22,.03)
box(STEEL,4.22,4.3,.98,1.28,5.25,5.285,.01)
box(WOOD,3.74,4.43,.28,.285,5.25,5.255)
# wall cupboards above the counter on the wall to the bedroom
box(CAB,.32,3.3,1.5,2.2,4.57,4.87,.01)
for x in [.36+i*.72 for i in range(4)]:box(CAB,x,x+.68,1.53,2.17,4.87,4.885,.004);box(STEEL,x+.6,x+.62,1.6,1.68,4.885,4.895)
box(CAB,.32,3.3,0,.86,4.57,5.05,.01) ;box(WOOD,.3,3.32,.86,.9,4.55,5.08,.004)
# table and chairs by the wall
leg_table(1.55,2.55,6.75,7.75,.76)
chair(1.35,7.2,1.57);chair(2.05,6.45,3.1)
g.lathe(CHINA,p(1.9,.76,7.1),[(0,0),(.04,0),(.045,.095),(.04,.095),(0,.08)],10)
g.plane_on(PAPER,'y',.7608,(2.2,-7.3+4.5-4.5+0.0-0.0+(7.3-4.5)*2+0.0) if False else (2.2-6,7.45-4.5),.3,.22,.2)
# --- Stue: sofa on the wall to the bedroom store, coffee table, TV on a chest, wood stove with its pipe, a rug, a rocking chair by the window.
g.quad([p(8.0,.004,5.6),p(11.0,.004,5.6),p(11.0,.004,7.8),p(8.0,.004,7.8)][::-1],RUG,[(0,0),(0,1),(1,1),(1,0)])
SX0,SX1=8.0,10.2
box(SOFA,SX0,SX1,.08,.44,4.58,5.42,.05);box(SOFA,SX0,SX1,.44,.9,4.58,4.8,.05)
for x in(SX0,SX1-.18):box(SOFA,x,x+.18,.3,.68,4.58,5.42,.06)
for k in range(3):box(SOFA,SX0+.18+k*.613,SX0+.18+(k+1)*.613-.01,.44,.54,4.82,5.4,.05)
g.box(SOFA,p(SX0+.5,.66,5.0),(.38,.3,.12),.3,bevel=.05)
for x in(SX0+.03,SX1-.09):box(WOOD,x,x+.06,0,.08,4.62,4.68);box(WOOD,x,x+.06,0,.08,5.34,5.4)
leg_table(8.5,9.6,6.25,6.85,.42,WOOD,.05)
g.lathe(CHINA,p(9.25,.42,6.5),[(0,0),(.04,0),(.044,.09),(.04,.09),(0,.075)],10);g.lathe(WOOD,p(8.85,.42,6.55),[(0,0),(.06,0),(.11,.05),(.1,.05),(0,.012)],14)
box(BEAM,8.95,9.2,.42,.46,6.35,6.7) # two old books
# chest against the south wall, and the TV on it
box(WOOD,10.1,11.55,0,.72,8.08,8.68,.012)
for x in(10.14,10.84):box(WOOD,x,x+.67,.08,.66,8.06,8.08,.004);g.cyl(STEEL,p(x+.6,.38,8.05),.012,.02,'z',8) if False else None
box(TV,10.4,11.05,.72,1.2,8.2,8.66,.012)
box(IRON,10.46,10.86,.78,1.14,8.185,8.2,.012)
box(CHINA,10.88,11.02,.78,1.14,8.185,8.2,.002) if False else None
for y in(1.05,.95):g.cyl(KNOB,p(10.95,y,8.2),.016,.015,'z',10)
g.lathe(GLASS,p(10.66,.96,8.185),[(.16,0),(.14,-.003)],14,'z')
g.tube(STEEL,[p(10.7,1.2,8.4),p(10.5,1.55,8.4)],.003,4);g.tube(STEEL,[p(10.75,1.2,8.4),p(11.0,1.52,8.4)],.003,4)
# the wood stove in the corner, pipe through the ceiling
SVX,SVY=11.1,4.95
g.lathe(IRON,p(SVX,.16,SVY),[(0,0),(.26,0),(.3,.05),(.3,.55),(.26,.6),(.2,.66),(.09,.7)],16)
for a in(0,2.09,4.19):
    box(IRON,SVX+.2*math.cos(a)-.02,SVX+.2*math.cos(a)+.02,0,.17,SVY+.2*math.sin(a)-.02,SVY+.2*math.sin(a)+.02)
box(IRON,SVX-.12,SVX+.12,.3,.56,SVY+.27,SVY+.31,.008)
g.cyl(IRON,p(SVX,.85,SVY),.066,WH-.85,'y',12)
box(WOOD,10.2,10.7,0,.26,4.6,4.92,.01) if False else None
for i in range(5):g.lathe(WOOD,p(10.62,.04+.05*(i%2)+.0,4.7+i*.09),[(.04,0),(.04,.32)],7,'x')
# rocking chair by the east window
chair(11.05,6.95,-1.57)
for s in(-1,1):box(WOOD,11.05-.25,11.05+.25,.0,.02,6.95+s*.2,6.95+s*.2+.02)
# --- Bakrom: steel shelves, boxes, dust sheets, a leaning door.
for x0,x1 in((7.0,7.45),(9.2,9.72)):
    for y0,y1 in((.42,2.6),):
        for u in(.1,.55,1.0,1.45,1.9):box(STEEL,x0,x1,u,u+.02,y0,y1)
        for xx in(x0+.01,x1-.03):
            for yy in(y0,(y0+y1)/2,y1-.02):box(STEEL,xx,xx+.02,0,1.95,yy,yy+.02)
    for u in(.12,.57,1.02,1.47):
        y=.46
        while y<2.5:
            d=R.uniform(.3,.55)
            if R.random()<.75:box(R.choice(['M_Lived_Card','M_Lived_Card',CAB,STEEL]),x0+.03,x1-.03,u,u+R.uniform(.18,.36),y,y+d-.03,.005)
            y+=d
box('M_Lived_Card',7.55,8.05,0,.34,3.6,4.3,.01,);box('M_Lived_Card',7.6,8.0,.34,.56,3.7,4.2,.01)
box(WOOD,9.1,9.14,0,1.95,3.0,3.75,.004)
# --- Bad: tub, basin, toilet, tile to the shoulder.
for fx,y0,y1,fa in((9.95,.3,4.4,1),(11.7,.3,4.4,-1)):
    wall_face('y',fx+fa*.003,y0,y1,0,1.25,[(2.6,3.4,1)] if fx==9.95 else [],TILE,fa)
wall_face('x',.303,9.95,11.7,0,1.25,[(10.5,11.3,0)],TILE,1)
wall_face('x',4.397,9.95,11.7,0,1.25,[],TILE,-1)
box(CHINA,10.0,11.65,0,.54,.33,1.1,.05);g.quad([p(10.08,.545,1.03),p(11.57,.545,1.03),p(11.57,.545,.4),p(10.08,.545,.4)],IRON)
g.tube(STEEL,[p(11.4,.54,.36),p(11.4,.72,.36),p(11.28,.74,.36)],.012,6)
box(CHINA,11.35,11.68,.78,.92,2.0,2.5,.03);g.cyl(CHINA,p(11.5,0,2.25),.05,.78,'y',10)
box(GLASS,11.68,11.695,1.25,1.75,2.0,2.5);box(WOOD,11.66,11.68,1.22,1.78,1.97,2.53,.004)
g.lathe(CHINA,p(11.35,.1,3.55),[(.14,0),(.18,.26),(.16,.3),(.13,.3),(0,.12)],12)
box(CHINA,11.5,11.68,.3,.72,3.4,3.7,.012)
# --- The hall: a runner, a hook board, a picture.
g.quad([p(5.65,.004,1.0),p(6.35,.004,1.0),p(6.35,.004,7.8),p(5.65,.004,7.8)][::-1],RUG,[(0,0),(0,1),(1,1),(1,0)])
box(WOOD,5.25,5.29,1.55,1.66,3.4,4.3,.004)
for y in(3.6,3.85,4.1):g.cyl(IRON,p(5.29,1.6,y),.008,.05,'x',6)
g.plane_on(PAPER,'x',6.75-6-.002,(.0,1.55),.3,.24,0,None,True) if False else None
box(WOOD,6.72,6.75,1.45,1.8,3.2,3.62,.004)
box(BEAM,6.71,6.72,1.49,1.76,3.24,3.58)
g.lathe(CHINA,p(6,WH-.14,2.8),[(0,.14),(.12,.14),(.1,.03),(0,0)],14)
g.lathe(CHINA,p(9.3,WH-.14,6.6),[(0,.14),(.12,.14),(.1,.03),(0,0)],14)
g.lathe(CHINA,p(2.7,WH-.14,2.3),[(0,.14),(.12,.14),(.1,.03),(0,0)],14)
# Grime overlays, from the control room's atlas: foot of every interior wall, dark round light fittings, some damp.
BAND={0:(1-256/2048,1.),2:(1-768/2048,1-512/2048)}
def band_y(fixedx,y0,y1,face,up0=.13,h=.42,band=0):
    v0,v1=BAND[band];L=abs(y1-y0);u=R.random()
    a,b=(y1,y0) if face>0 else (y0,y1)
    fx=fixedx+face*.004
    g.quad([p(fx,up0,a),p(fx,up0,b),p(fx,up0+h,b),p(fx,up0+h,a)],GRIME,[(u,v0),(u+L/4,v0),(u+L/4,v1),(u,v1)])
def band_x(fixedy,x0,x1,face,up0=.13,h=.42,band=0):
    v0,v1=BAND[band];L=abs(x1-x0);u=R.random()
    a,b=(x0,x1) if face>0 else (x1,x0)
    fy=fixedy+face*.004
    g.quad([p(a,up0,fy),p(b,up0,fy),p(b,up0+h,fy),p(a,up0+h,fy)],GRIME,[(u,v0),(u+L/4,v0),(u+L/4,v1),(u,v1)])
for n,(x0,x1,y0,y1) in ROOMS.items():
    band_x(y0,x0,x1,1);band_x(y1,x0,x1,-1);band_y(x0,y0,y1,1);band_y(x1,y0,y1,-1)
# Out: mud up the foot of the walls, a heavy band, and rain down from the parapet under each canal.
for fy,a,b,f in((0,0,12,-1),(9,0,12,1)):band_x(fy,a,b,f,-.3,1.0)

for fx,f in((0,-1),(12,1)):band_y(fx,0,9,f,-.3,1.0)
# Lime gone back to the adobe: few, small, light, low by the panel, at the frames' feet and in the corners, where knocks and damp do it.
LOSS='M_House_Loss'
def loss(along,fixed,face,c,up,w,h,k):
    u0,v0=(k%2)*.5,1-(k//2)*.5-.5;uv=[(u0,v0),(u0+.5,v0),(u0+.5,v0+.5),(u0,v0+.5)]
    f=fixed+face*.006
    if along=='x':
        q=[p(c-w/2,up,f),p(c+w/2,up,f),p(c+w/2,up+h,f),p(c-w/2,up+h,f)]
        if face<0:q=[q[1],q[0],q[3],q[2]];uv=[uv[1],uv[0],uv[3],uv[2]]
    else:
        q=[p(f,up,c+w/2),p(f,up,c-w/2),p(f,up+h,c-w/2),p(f,up+h,c+w/2)]
        if face<0:q=[q[1],q[0],q[3],q[2]];uv=[uv[1],uv[0],uv[3],uv[2]]
    g.quad(q,LOSS,uv)
R2=random.Random(7)
spots=[('x',.3,1,1.25,.93),('x',.3,1,4.2,.95),('y',5.1,-1,1.6,.98),('y',5.25,1,6.0,.94),('y',6.75,-1,2.85,.96),('x',4.4,-1,8.0,.92),('x',8.7,-1,10.3,.95),('y',11.7,-1,5.0,.97),
       ('x',.3,1,6.0,2.35),('y',.3,1,8.3,.15),('x',4.55,1,11.6,.2),('y',6.9,1,7.15,.95)]
for k,(al,fx,fc,c,up) in enumerate(spots):
    w=R2.uniform(.16,.32);loss(al,fx,fc,c,up,w,w*R2.uniform(.7,1.1),k%4)
# ---- outside, after Tom: a small portal over the back door, rain under the canales, adobe showing low and at the corners, dark soil at the foot.
BP=-1.6
g.quad([p(4.4,-.04,0),p(7.6,-.04,0),p(7.6,-.04,BP),p(4.4,-.04,BP)],PLANK)
box(TIN,4.3,7.7,2.36,2.44,BP-.12,0)
box(WOOD,4.28,7.72,2.24,2.36,BP-.2,BP-.12,.008)
for x in(4.5,5.6,6.4,7.5):
    box(WOOD,x-.1,x+.1,-.04,2.06,BP+.02,BP+.22,.012)
    box(WOOD,x-.28,x+.28,2.06,2.2,BP+.04,BP+.2,.01)
for x in(4.45,5.15,5.85,6.55,7.25):
    r=R.uniform(.07,.09);g.lathe(WOOD,p(x,2.3,BP-.32),[(r,0),(r,1.94),(r*.95,1.98),(0,1.99)],9,'z')
OG='M_Lived_OutGrime'
def outband(along,fixed,face,a0,a1,y0,y1,which,scale=1.):
    r0,r1=[(0,512),(512,1024),(1024,1536),(1536,2048)][which];vt,vb=1-r0/2048,1-r1/2048
    if which==0:vt,vb=1-r1/2048,1-r0/2048
    u=R.random();L=abs(a1-a0)/scale;f=fixed+face*.012
    if along=='x':q=[p(a0,y0,f),p(a1,y0,f),p(a1,y1,f),p(a0,y1,f)]
    else:q=[p(f,y0,a1),p(f,y0,a0),p(f,y1,a0),p(f,y1,a1)]
    uv=[(u,vb),(u+L,vb),(u+L,vt),(u,vt)]
    if face>0 and along=='x' or face<0 and along=='y':q=q[::-1];uv=uv[::-1]
    g.quad(q,OG,uv)
# rain from every canal: a dark streak down the wall under it
for x in(2.2,9.6):outband('x',0,-1,x-.25,x+.25,.2,WH+.5,3,.5)
for y in(2.,6.5):outband('y',0,-1,y-.25,y+.25,.2,WH+.5,3,.5);outband('y',12,1,y-.25,y+.25,.2,WH+.5,3,.5)
# adobe showing through, outside: bigger, low, and at the corners
def outloss(along,fixed,face,c,up,w,h,k):loss(along,fixed,-face if False else face,c,up,w,h,k)
for k,(al,fx,fc,c,up,w) in enumerate((('x',0,-1,.5,-.1,.9),('x',0,-1,11.4,.0,1.1),('x',9,1,.6,-.05,1.),('x',9,1,11.5,.2,.8),('y',0,-1,.6,.1,.9),('y',0,-1,8.3,-.1,1.2),('y',12,1,.7,.05,1.),('y',12,1,8.2,.3,.7),('x',0,-1,4.2,1.9,.5),('x',9,1,7.3,.0,.9))):
    loss(al,fx+fc*.004,fc,c,up,w,w*R2.uniform(.6,.9),k%4)
# the foot: dark soil and gravel, sloping out and down into the ground, so it meets the land whatever the land does
SOIL='M_House_Soil'
def apron(ax,ay,bx_,by_,ox,oy,w=.9,drop=.7):
    L=math.hypot(bx_-ax,by_-ay);u=R.random()
    q=[p(ax,-.1,ay),p(bx_,-.1,by_),p(bx_+ox*w,-.1-drop,by_+oy*w),p(ax+ox*w,-.1-drop,ay+oy*w)]
    uv=[(u,1),(u+L/2,1),(u+L/2,0),(u,0)]
    g.quad(q,SOIL,uv);g.quad(q[::-1],SOIL,uv[::-1])
apron(0,0,12,0,0,-1);apron(0,0,0,9,-1,0);apron(12,0,12,9,1,0);apron(0,11,12,11,0,1)
ob,tr=G.build(g,'SM_Lived_FarmHouse',uvscale={'M_House_Soil':1.,'M_Lived_OutGrime':1.,'M_House_Loss':1.,STUCCO:3.,WOOD:3.,PAINT:3.4,TIN:3.6,CONC:4.,STEEL:3.,PLASTER:3.,PLANK:2.,BEAM:2.,LINO:1.2,SOFA:1.,TILE:.9,CAB:1.5,'M_Lived_Card':1.})
Export=ROOT/'Export'
tr=G.export_fbx(ob,Export/'SM_Lived_FarmHouse.fbx')
def bounds(o):
    vs=[v.co for v in o.data.vertices];return [[min(v[k] for v in vs) for k in range(3)],[max(v[k] for v in vs) for k in range(3)]]
meshes={'SM_Lived_FarmHouse':{'triangles':tr,'materials':[m.name for m in ob.data.materials],'bounds_local_m':bounds(ob),'collision':'complex'}}
# ------- the door leaf: a unit cube, X across, Z up, Y through: S47FarmHouse scales it to each door. Panels, flaking turquoise.
for o in list(bpy.data.objects):bpy.data.objects.remove(o,do_unlink=True)
d=G.Geo()
# In geo terms (x across, y up, z through), all within +-.5
def dbx(mat,x0,x1,y0,y1,z0,z1):d.box(mat,((x0+x1)/2,(y0+y1)/2,(z0+z1)/2),(x1-x0,y1-y0,z1-z0))
dbx('M_House_DoorPaint',-.5,.5,-.5,.5,-.5,.5)
for sz in(-1,1):
    for x0,x1 in((-.34,-.04),(.04,.34)):
        for y0,y1 in((-.42,-.08),(.02,.42)):
            z=sz*.52
            q=[(x0,y0,z),(x1,y0,z),(x1,y1,z),(x0,y1,z)]
            d.quad(q if sz>0 else q[::-1],'M_House_DoorPanel')
    for s in(-1,):
        d.lathe('M_Farm_Steel',(.42,-.02,sz*.5),[(.05,0),(.05,sz*.45),(0,sz*.5)],8,'z')
do,tt=G.build(d,'SM_Lived_HouseDoor',uvscale={'M_House_DoorPaint':2.,'M_House_DoorPanel':2.})
tt=G.export_fbx(do,Export/'SM_Lived_HouseDoor.fbx')
meshes['SM_Lived_HouseDoor']={'triangles':tt,'materials':[m.name for m in do.data.materials],'bounds_local_m':bounds(do)}
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'Lived_House.blend'))
TEX='house/textures/'
def t(name,bc,rough=None,normal=None,tint=(1,1,1),r=.8,spec=.3,ns=.8):
    tex={'base_color':bc}
    if rough:tex['roughness']=rough
    if normal:tex['normal']=normal
    row={'name':name,'textures':tex,'texture_tint_linear':list(tint),'roughness':r,'metallic':0,'specular':spec,'base_color_linear':[1,1,1,1],'normal_strength':ns,'normal_convention':'DirectX'}
    return row
def f(name,col,r=.7,m=0,spec=.35):return {'name':name,'base_color_linear':list(col)+[1],'roughness':r,'metallic':m,'specular':spec}
rows=[{'name':n,'reuse':'/Game/FarmBuildings/Materials/'+n} for n in(STUCCO,WOOD,PAINT,TIN,GLASS,CONC,STEEL,LIT)]+[
 t(PLASTER,TEX+'T_House_Plaster_BC.png',TEX+'T_House_Plaster_R.png',TEX+'T_House_Plaster_N.png'),
 t(PLANK,TEX+'T_House_Plank_BC.png',TEX+'T_House_Plank_R.png',TEX+'T_House_Plank_N.png'),
 t(BEAM,TEX+'T_House_Beam_BC.png',TEX+'T_House_Beam_R.png',TEX+'T_House_Beam_N.png'),
 {'name':'M_House_Soil','textures':{'base_color':TEX+'T_House_Soil_BC.png'},'texture_tint_linear':[1,1,1],'roughness':.95,'metallic':0,'specular':.2,'opacity_from_base_color':True,'alpha_mode':'dithered_masked','base_color_linear':[1,1,1,1]},
 {'name':'M_Lived_OutGrime','reuse':'/Game/Lived/Saro/Materials/M_Lived_OutGrime'},
 {'name':'M_House_Loss','textures':{'base_color':TEX+'T_House_Loss_BC.png'},'texture_tint_linear':[1,1,1],'roughness':.92,'metallic':0,'specular':.25,'opacity_from_base_color':True,'alpha_mode':'dithered_masked','base_color_linear':[1,1,1,1]},
 t(LINO,TEX+'T_House_Lino_BC.png',TEX+'T_House_Lino_R.png',None),t(RUG,TEX+'T_House_Rug_BC.png',None,None,r=.95),t(SOFA,TEX+'T_House_Sofa_BC.png',None,None,r=.95),
 t(CAB,'textures/T_Lived_Enamel_BC.png','textures/T_Lived_Enamel_R.png',None,(.50,.52,.40)),t(TILE,'textures/T_Lived_Plastic_BC.png','textures/T_Lived_Plastic_R.png',None,(.50,.56,.50)),
 f(CHINA,(.62,.59,.50),.32),f(IRON,(.03,.03,.03),.7,.5),f(MATTRESS,(.42,.37,.30),.9),f(CURTAIN,(.48,.44,.36),.9),f(TV,(.07,.045,.025),.45),f(KNOB,(.12,.12,.12),.4,.8),f(TAPEB,(.40,.36,.28),.55),
 f('M_House_DoorPaint',(.24,.40,.37),.72),f('M_House_DoorPanel',(.18,.30,.28),.8),
 {'name':'M_Lived_Card','textures':{'base_color':'textures/T_Lived_Card_BC.png','roughness':'textures/T_Lived_Card_R.png'},'texture_tint_linear':[.40,.28,.155],'metallic':0,'specular':.45,'roughness':.6,'base_color_linear':[1,1,1,1]},
 {'name':GRIME,'reuse':'/Game/Lived/Control/Materials/M_Lived_Grime'},{'name':PAPER,'reuse':'/Game/Lived/Control/Materials/M_Lived_Paper'}]
used=set(meshes['SM_Lived_FarmHouse']['materials'])|set(meshes['SM_Lived_HouseDoor']['materials'])
(ROOT.parent/'lived-farm-materials.json').write_text(json.dumps([r for r in rows if r['name'] in used],indent=1))
(ROOT.parent/'lived-farm-contract.json').write_text(json.dumps({'export_dir':'house/Export','meshes':meshes},indent=1))
print('DONE',{k:v['triangles'] for k,v in meshes.items()},sorted(used))
