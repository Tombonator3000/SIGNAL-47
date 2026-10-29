"""Caliche's hanging four-way amber flasher, original 1986 span-wire signal. Blender 5.2, -b -t 2.

Two delivery meshes, three authored LODs each, metric FBX and original procedural PBR maps:
  SM_Caliche_Signal_Head  the part that swings: span clamp, hanger, entrance fitting, drip loop,
                          cast four-way section with doors, tunnel visors, lenses, bottom eye.
                          Origin = the pivot where the clamp grips the span wire.
  SM_Caliche_Signal_Span  fixed: sagging messenger wire, lashed feed cable, pole bands, eye bolts,
                          dead-end grips, the feed down the west pole, flasher cabinet and conduit.
                          Same origin, so both meshes are placed at one point.
Local contract: Three metres relative to the pivot (x east, y up, z south), Blender=(x,-z,y),
measured UE=(x,z,y)*100. The pivot is the old web crossing's wire midpoint, town-local
(0.5, 11.459, 2000): Town.ts crossing(), w0=(-8.6,2019), w1=(9.6,1981), tops 7.6 m, sag 0.35 m.
No downloaded mesh, generated concept image or third-party texture is used.
"""
from pathlib import Path
import json, math
import bpy, bmesh, numpy as np
from mathutils import Vector

HERE=Path(__file__).resolve().parent
OUT=HERE/'Export'; TEX=HERE/'textures'
OUT.mkdir(parents=True,exist_ok=True); TEX.mkdir(exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
SC=bpy.context.scene; SC.unit_settings.system='METRIC'; SC.unit_settings.scale_length=1
SC.render.threads_mode='FIXED'; SC.render.threads=2
MATS={}; ROWS=[]; LOD=0; COL=None; PART=[]
N=512

# ---------- original procedural weathering maps ----------
def noise(rng,cells,n=N):
    """Tileable value noise: a random grid of `cells` cells, bilinear, wrapped."""
    g=rng.random((cells,cells)); x=np.arange(n)*cells/n; i=np.floor(x).astype(int); f=x-i; f=f*f*(3-2*f)
    a=g[np.ix_(i%cells,i%cells)]; b=g[np.ix_(i%cells,(i+1)%cells)]; c=g[np.ix_((i+1)%cells,i%cells)]; d=g[np.ix_((i+1)%cells,(i+1)%cells)]
    fy=f[:,None]; fx=f[None,:]
    return (a*(1-fx)+b*fx)*(1-fy)+(c*(1-fx)+d*fx)*fy
def fbm(rng,base,octaves=5):
    s=np.zeros((N,N)); amp=1; tot=0
    for o in range(octaves): s+=amp*noise(rng,base*2**o); tot+=amp; amp*=.5
    return s/tot
def srgb(lin): lin=np.clip(lin,0,1); return np.where(lin<.0031308,lin*12.92,1.055*lin**(1/2.4)-.055)
def image(name,data,linear=False):
    im=bpy.data.images.new(name,width=data.shape[1],height=data.shape[0],alpha=True)
    if linear: im.colorspace_settings.name='Non-Color'
    im.pixels.foreach_set(np.asarray(data,dtype=np.float32).ravel())
    im.filepath_raw=str(TEX/(name+'.png')); im.file_format='PNG'; im.save(); im.pack()
    return im

def maps(kind,seed):
    rng=np.random.default_rng(seed); v,u=np.mgrid[:N,:N]/N
    broad=fbm(rng,4); mid=fbm(rng,16,4); fine=rng.normal(0,1,(N,N))
    if kind=='paint':
        # Traffic yellow enamel on cast aluminium, forty Mexico summers: chalked by UV,
        # dusty grime in the low spots, sparse chips to bare metal, bolt rust bleeding.
        yellow=np.array([.72,.40,.016]); chalk=np.array([.66,.52,.25]); grime=np.array([.20,.15,.08]); metal=np.array([.21,.21,.195])
        fade=np.clip((broad-.35)*1.6,0,1)[...,None]*.55
        col=yellow*(1-fade)+chalk*fade
        dirt=np.clip((mid-.52)*2.4,0,1)[...,None]*.45
        col=col*(1-dirt)+grime*dirt
        chips=(fbm(rng,64,3)>.80).astype(float)
        col=np.where(chips[...,None]>0,metal,col)
        col=col*(1+fine[...,None]*.018)
        rough=np.clip(.58+broad*.18+dirt[...,0]*.2-chips*.2+fine*.02,.2,.95)
        height=-chips*.0006+mid*.0004+fine*.00008
    elif kind=='dark':
        col=np.array([.022,.022,.021])*(1+fine[...,None]*.04)
        dust=np.clip((broad-.45)*2,0,1)[...,None]*.5
        col=col*(1-dust)+np.array([.11,.09,.065])*dust
        rough=np.clip(.82+dust[...,0]*.1+fine*.02,.5,.98); height=mid*.0003+fine*.00005
    elif kind=='galv':
        # Hot-dip galvanising: spangle cells, dull grey bloom and a few rust pinholes.
        sp=noise(rng,96); sp=np.abs(np.sin(sp*31))*.12
        col=np.array([.34,.35,.34])*(1+sp[...,None]-.06)
        bloom=np.clip((broad-.4)*1.8,0,1)[...,None]*.45
        col=col*(1-bloom)+np.array([.46,.46,.43])*bloom
        rust=np.clip((fbm(rng,40,3)-.70)*5,0,1)[...,None]
        col=col*(1-rust)+np.array([.26,.10,.035])*rust
        rough=np.clip(.42+bloom[...,0]*.3+rust[...,0]*.4+fine*.02,.15,.95); height=sp*.0002+rust[...,0]*.0003+fine*.00005
    elif kind=='cable':
        col=np.array([.028,.028,.026])*(1+fine[...,None]*.05)
        chalk=np.clip((broad-.5)*2.2,0,1)[...,None]*.6
        col=col*(1-chalk)+np.array([.10,.10,.095])*chalk
        rough=np.clip(.78+chalk[...,0]*.12+fine*.02,.5,.98); height=np.sin(v*math.tau*220)*.00015+fine*.00005
    else: # lens: moulded amber glass, faint dust haze and surface scuffs
        col=np.array([.92,.42,.018])*(1-np.clip((broad-.55)*1.5,0,1)[...,None]*.18)
        haze=np.clip((mid-.45)*1.8,0,1)
        rough=np.clip(.10+haze*.22+fine*.01,.05,.6); height=fine*.00004
    base=np.ones((N,N,4),dtype=np.float32); base[:,:,:3]=srgb(col)
    ri=np.ones_like(base); ri[:,:,:3]=rough[...,None]
    dy,dx=np.gradient(height); nrm=np.stack((-dx*N,-dy*N,np.ones_like(dx)),axis=2); nrm/=np.linalg.norm(nrm,axis=2)[...,None]
    nm=np.ones_like(base); nm[:,:,:3]=nrm*.5+.5; nm[:,:,1]=1-nm[:,:,1] # DirectX
    return base,ri,nm

def material(short,kind,metal,rough,strength=.2,seed=0):
    name='M_TownSignal_'+short
    b,r,n=maps(kind,1986+seed)
    bi=image(name+'_base',b); rr=image(name+'_rough',r,True); ni=image(name+'_normal',n,True)
    m=bpy.data.materials.new(name); m.use_nodes=True; nodes=m.node_tree.nodes; links=m.node_tree.links
    p=nodes.get('Principled BSDF'); p.inputs['Metallic'].default_value=metal; p.inputs['Roughness'].default_value=rough
    for im,socket in ((bi,'Base Color'),(rr,'Roughness')):
        t=nodes.new('ShaderNodeTexImage'); t.image=im; links.new(t.outputs['Color'],p.inputs[socket])
    t=nodes.new('ShaderNodeTexImage'); t.image=ni
    split=nodes.new('ShaderNodeSeparateColor'); combine=nodes.new('ShaderNodeCombineColor'); inv=nodes.new('ShaderNodeMath'); inv.operation='SUBTRACT'; inv.inputs[0].default_value=1
    links.new(t.outputs['Color'],split.inputs['Color']); links.new(split.outputs['Red'],combine.inputs['Red']); links.new(split.outputs['Blue'],combine.inputs['Blue'])
    links.new(split.outputs['Green'],inv.inputs[1]); links.new(inv.outputs[0],combine.inputs['Green'])
    no=nodes.new('ShaderNodeNormalMap'); no.inputs['Strength'].default_value=strength
    links.new(combine.outputs['Color'],no.inputs['Color']); links.new(no.outputs['Normal'],p.inputs['Normal'])
    MATS[short]=m
    ROWS.append({'name':name,'base_color_linear':[1,1,1,1],'metallic':metal,'roughness':rough,'specular':.5 if kind=='lens' else .35,
                 'emission':[0,0,0,1],'emission_strength':0,'normal_convention':'DirectX','normal_strength':strength,
                 'texture_tint_linear':[1,1,1],'textures':{'base_color':'textures/'+bi.name+'.png','roughness':'textures/'+rr.name+'.png','normal':'textures/'+ni.name+'.png'}})
    return m

material('Paint','paint',.0,.6,.25,1)
material('Dark','dark',.0,.82,.15,2)
material('Galv','galv',.85,.45,.18,3)
material('Cable','cable',.0,.8,.2,4)
material('Lens','lens',.0,.12,.1,5)

# ---------- geometry helpers, Three metres ----------
def xyz(p): return Vector((p[0],-p[2],p[1]))
def V3(*a): return np.array(a,dtype=float)
def unit(a): a=np.asarray(a,float); return a/np.linalg.norm(a)
def active(o):
    bpy.ops.object.select_all(action='DESELECT'); o.select_set(True); bpy.context.view_layer.objects.active=o
def add_mats(o,shorts):
    for s in shorts: o.data.materials.append(MATS[s])
def bevel(o,r):
    if r<=0: return o
    active(o); m=o.modifiers.new('Cast_edge_radius','BEVEL'); m.width=r; m.segments=(3,2,1)[LOD]
    bpy.ops.object.modifier_apply(modifier=m.name)
    m=o.modifiers.new('Face_weighted_normals','WEIGHTED_NORMAL'); m.keep_sharp=True
    bpy.ops.object.modifier_apply(modifier=m.name); return o
def mesh(name,points,faces,mats,face_mat=None,smooth=None,r=0):
    me=bpy.data.meshes.new(name); me.from_pydata([xyz(p) for p in points],[],faces); me.update()
    o=bpy.data.objects.new(name,me); COL.objects.link(o); add_mats(o,mats)
    if face_mat:
        for i,p in enumerate(me.polygons): p.material_index=face_mat[i]
    if smooth:
        for i,p in enumerate(me.polygons): p.use_smooth=smooth[i]
    bm=bmesh.new(); bm.from_mesh(me); bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces)); bm.to_mesh(me); bm.free()
    PART.append(o); return bevel(o,r)
def obox(name,c,e1,e2,size,mat,r=.002):
    """Box centred at c, its first axis along e1, second along e2 (third is their cross)."""
    e1=unit(e1); e2=unit(np.asarray(e2,float)-e1*np.dot(e2,e1)); e3=np.cross(e1,e2); c=np.asarray(c,float)
    pts=[c+e1*sx*size[0]/2+e2*sy*size[1]/2+e3*sz*size[2]/2 for sx in (-1,1) for sy in (-1,1) for sz in (-1,1)]
    f=[(0,1,3,2),(4,6,7,5),(0,4,5,1),(2,3,7,6),(0,2,6,4),(1,5,7,3)]
    return mesh(name,pts,f,[mat],r=min(r,min(size)*.24))
def box(name,c,size,mat,r=.002): return obox(name,c,(1,0,0),(0,1,0),size,mat,r)
def lathe(name,c,n,up,prof,mats,segs,a0=0.,a1=math.tau,closed=True,cap='Paint',smooth=True):
    """Revolve profile (r,t) about axis n through c. prof is a list or a function of the angle.
    mats[k] is the material of profile segment k. Angle 0 points along `up`."""
    n=unit(n); e1=unit(np.asarray(up,float)-n*np.dot(up,n)); e2=np.cross(n,e1); c=np.asarray(c,float)
    full=abs((a1-a0)-math.tau)<1e-9; steps=segs if full else max(2,segs)
    angles=[a0+(a1-a0)*k/(steps if full else steps-1) for k in range(steps)] if not full else [a0+math.tau*k/steps for k in range(steps)]
    P=prof if callable(prof) else (lambda a,p=prof:p)
    m=len(P(0.)); pts=[]; ring=[]
    axis_single={}
    for k,a in enumerate(angles):
        row=[]
        for j,(r,t) in enumerate(P(a)):
            if r<1e-9:
                if j not in axis_single: axis_single[j]=len(pts); pts.append(c+n*t)
                row.append(axis_single[j]); continue
            row.append(len(pts)); pts.append(c+n*t+r*(math.cos(a)*e1+math.sin(a)*e2))
        ring.append(row)
    faces=[]; fm=[]; sm=[]
    segcount=m if closed else m-1
    allm=sorted(set(mats)|{cap}); idx={s:i for i,s in enumerate(allm)}
    kr=range(steps) if full else range(steps-1)
    for k in kr:
        k2=(k+1)%steps
        for j in range(segcount):
            j2=(j+1)%m
            q=[ring[k][j],ring[k2][j],ring[k2][j2],ring[k][j2]]
            uq=[]
            for x in q:
                if x not in uq: uq.append(x)
            if len(uq)<3: continue
            pj=P(angles[k]); flat=abs(pj[j][1]-pj[j2][1])<1e-9 and smooth!='all'
            faces.append(tuple(uq)); fm.append(idx[mats[j%len(mats)]]); sm.append(bool(smooth) and not flat)
    if not full and closed:
        for k in (0,steps-1):
            loop=[]
            for x in ring[k]:
                if x not in loop: loop.append(x)
            if len(loop)>=3: faces.append(tuple(loop)); fm.append(idx[cap]); sm.append(False)
    return mesh(name,[tuple(p) for p in pts],faces,allm,fm,sm)
def cyl(name,a,b,r,mat,sides=None):
    a=np.asarray(a,float); b=np.asarray(b,float); d=b-a; L=np.linalg.norm(d)
    up=(0,1,0) if abs(unit(d)[1])<.9 else (1,0,0)
    return lathe(name,a,d,up,[(0,0),(r,0),(r,L),(0,L)],['Galv'],sides or (16,10,8)[LOD],cap='Galv')
def cylm(name,a,b,r,mat,sides=None):
    o=cyl(name,a,b,r,mat,sides); o.data.materials[0]=MATS[mat]; return o
def ring(name,c,n,up,ri,ro,t0,t1,mat,segs=None,a0=0.,a1=math.tau):
    return lathe(name,c,n,up,[(ri,t0),(ro,t0),(ro,t1),(ri,t1)],[mat],segs or (32,20,12)[LOD],a0,a1,cap=mat)
def torus(name,c,axis,up,R,r,mat,a0=0.,a1=math.tau,segs=None):
    """A ring of tube radius r on a circle of radius R about axis (a U-bolt or eye when partial)."""
    k=(6,5,4)[LOD]; prof=[(R+r*math.cos(t),r*math.sin(t)) for t in [math.tau*i/k for i in range(k)]]
    return lathe(name,c,axis,up,prof,[mat],segs or (20,12,8)[LOD],a0,a1,cap=mat,smooth='all')
def catmull(P,per=6):
    P=[np.asarray(p,float) for p in P]; out=[]
    for i in range(len(P)-1):
        p0=P[max(i-1,0)]; p1=P[i]; p2=P[i+1]; p3=P[min(i+2,len(P)-1)]
        for s in range(per):
            t=s/per; t2=t*t; t3=t2*t
            out.append(.5*((2*p1)+(-p0+p2)*t+(2*p0-5*p1+4*p2-p3)*t2+(-p0+3*p1-3*p2+p3)*t3))
    out.append(P[-1]); return out
def tube(name,pts,r,mat):
    """A cable/wire along a polyline: a bevelled curve converted to a closed mesh."""
    cu=bpy.data.curves.new(name,'CURVE'); cu.dimensions='3D'; cu.bevel_depth=r; cu.bevel_resolution=(2,1,0)[LOD]; cu.use_fill_caps=True
    sp=cu.splines.new('POLY'); sp.points.add(len(pts)-1)
    for i,p in enumerate(pts): q=xyz(p); sp.points[i].co=(q.x,q.y,q.z,1)
    o=bpy.data.objects.new(name,cu); COL.objects.link(o); active(o); bpy.ops.object.convert(target='MESH')
    o=bpy.context.object; o.name=name; o.data.materials.clear(); o.data.materials.append(MATS[mat])
    for p in o.data.polygons: p.use_smooth=True
    PART.append(o); return o
def hexnut(name,c,n,s=.008,h=.007,mat='Galv'):
    n=unit(n); up=(0,1,0) if abs(n[1])<.9 else (1,0,0)
    return lathe(name,c,n,up,[(0,0),(s,0),(s,h),(0,h)],[mat],6,cap=mat,smooth=False)

# ---------- the site, measured from the exported crossing ----------
PIVOT_TOWN=(0.5,11.459,2000.0)
POLE_W=V3(-8.6,0,2019)-V3(*PIVOT_TOWN)*V3(1,0,1); POLE_E=V3(9.6,0,1981)-V3(*PIVOT_TOWN)*V3(1,0,1)
EYE_Y=11.809-11.459; GROUND_Y=4.21-11.459
U=unit((POLE_E-POLE_W)*V3(1,0,1)); WS=np.cross(U,V3(0,1,0)) # along the wire, across it (horizontal)
def pole_r(y): return .17-.04*((y+11.459)-4.21)/8
UP=V3(0,1,0)
HOUSE_TOP=-.41; HOUSE_BOT=-.85; HALF=.205; LENS_Y=-.63
FACES=[V3(1,0,0),V3(-1,0,0),V3(0,0,1),V3(0,0,-1)]
DROP=(-.07*U+V3(0,-.026,0)) # where the lashed feed leaves the span for the head

def head():
    seg=(40,24,14)[LOD]
    # Span-wire clamp: grooved cast body, two U-bolts over the messenger, nuts below.
    obox('Span_clamp_body',(0,-.022,0),U,UP,(.13,.03,.045),'Galv',.006)
    for s in (-.042,.042):
        torus('Clamp_U_bolt',s*U,U,UP,.011,.0032,'Galv',-math.pi/2,math.pi/2)
        for w in (-.011,.011):
            cyl('U_bolt_leg',s*U+w*WS+V3(0,-.002,0),s*U+w*WS+V3(0,-.046,0),.0032,'Galv',8)
            if LOD<2: hexnut('U_bolt_nut',s*U+w*WS+V3(0,-.044,0),(0,-1,0),.0065,.006)
    # Clevis and pin, the true hinge of the swing.
    for w in (-.016,.016): obox('Clevis_plate',w*WS+V3(0,-.058,0),U,UP,(.05,.05,.006),'Galv',.002)
    cyl('Clevis_pin',-.024*WS+V3(0,-.066,0),.024*WS+V3(0,-.066,0),.006,'Galv',10)
    # Hanger pipe with balance adjuster and wire entrance fitting.
    cyl('Hanger_pipe',(0,-.07,0),(0,HOUSE_TOP+.03,0),.019,'Galv',(14,10,8)[LOD])
    box('Balance_adjuster',(0,-.215,0),(.062,.07,.062),'Galv',.006)
    if LOD<2:
        for z in (-.02,.02): cyl('Adjuster_bolt',(-.034,-.215,z),(.04,-.215,z),.0045,'Galv',8)
    lathe('Wire_entrance_fitting',(0,-.115,0),(0,1,0),(1,0,0),[(0,0),(.03,0),(.03,.03),(.012,.04),(0,.04)],['Galv'],(18,12,8)[LOD],cap='Galv')
    # Feed drip loop, from the lashed span cable down and up into the fitting.
    loop=[DROP,DROP+V3(0,-.03,0)-.02*U,(-.11*U)+V3(0,-.12,0)+.02*WS,(-.07*U)+V3(0,-.175,0)+.03*WS,(-.03*U)+V3(0,-.14,0)+.012*WS,V3(0,-.10,0)-.028*U]
    tube('Feed_drip_loop',catmull(loop,(8,5,3)[LOD]),.0085,'Cable')
    # Top bracket: cast hub on a square top plate with corner bolts.
    lathe('Top_bracket_hub',(0,HOUSE_TOP,0),(0,1,0),(1,0,0),[(0,0),(.075,0),(.075,.012),(.03,.03),(0,.03)],['Paint'],(24,14,8)[LOD])
    box('Top_plate',(0,HOUSE_TOP-.004,0),(.36,.012,.36),'Paint',.004)
    # Cast four-way section body with heavier edge radius and top/bottom flanges.
    box('Cast_section_body',(0,(HOUSE_TOP+HOUSE_BOT)/2,0),(2*HALF,HOUSE_TOP-HOUSE_BOT,2*HALF),'Paint',.022)
    for y in (HOUSE_TOP-.012,HOUSE_BOT+.012): box('Section_flange',(0,y,0),(2*HALF+.012,.022,2*HALF+.012),'Paint',.006)
    box('Bottom_plate',(0,HOUSE_BOT-.006,0),(.34,.012,.34),'Paint',.004)
    cylm('Drain_plug',(0,HOUSE_BOT-.012,0),(0,HOUSE_BOT-.024,0),.012,'Galv',10)
    torus('Tether_eye',(0,HOUSE_BOT-.055,0),WS,UP,.021,.0055,'Galv')
    cyl('Tether_eye_shank',(0,HOUSE_BOT-.012,0),(0,HOUSE_BOT-.034,0),.006,'Galv',8)
    if LOD<2:
        for x in (-.15,.15):
            for z in (-.15,.15):
                hexnut('Top_plate_bolt',(x,HOUSE_TOP+.002,z),(0,1,0),.008,.007,'Galv')
                hexnut('Bottom_plate_bolt',(x,HOUSE_BOT-.012,z),(0,-1,0),.008,.007,'Galv')
    for n in FACES:
        side=np.cross(UP,n); c=n*HALF+V3(0,LENS_Y,0)
        obox('Hinged_door',c+n*.008,side,UP,(.355,.40,.016),'Paint',.008)
        ring('Lens_door_ring',c,n,UP,.150,.170,.016,.032,'Paint',seg)
        lathe('Door_ring_lip',c+n*.032,n,UP,[(.150,0),(.170,0),(.162,.006),(.152,.006)],['Paint'],seg)
        ring('Lens_gasket',c,n,UP,.144,.152,.014,.034,'Dark',seg)
        # Moulded 12-inch amber lens: shallow dome with concentric Fresnel steps.
        steps=(6,3,0)[LOD]; prof=[(0,.050)]
        for i in range(steps):
            r0=.150*(i+1)/(steps+1); prof+= [(r0,.050-.010*(i+1)/(steps+1)-.004),(r0+.0006,.050-.010*(i+1)/(steps+1))]
        prof+=[(.150,.036),(.150,.030),(0,.030)]
        lathe('Fresnel_amber_lens',c,n,UP,prof,['Lens'],seg,cap='Lens')
        # Tunnel visor, longer at the crown, open at the bottom; dark inside.
        def vis(a):
            L=.235*(.42+.58*(1+math.cos(a))/2)
            return [(.170,.03),(.177,.03),(.177,.03+L),(.171,.03+L),(.171,.04)]
        lathe('Tunnel_visor',c,n,UP,vis,['Paint','Paint','Paint','Dark','Paint'],(36,22,12)[LOD],-math.radians(148),math.radians(148))
        # Hinge lugs and pin on one side, wing-nut latch on the other.
        for y in (-.13,.13):
            cylm('Door_hinge_knuckle',c-side*.19+n*.016+V3(0,y-.025,0),c-side*.19+n*.016+V3(0,y+.025,0),.011,'Paint',(10,8,6)[LOD])
        if LOD<2:
            cyl('Latch_eye_bolt',c+side*.188+n*.004,c+side*.188+n*.036,.005,'Galv',8)
            obox('Latch_wing_nut',c+side*.188+n*.036,n,UP,(.008,.03,.007),'Galv',.002)
            for a in (45,135,225,315):
                t=math.radians(a); p=c+n*.032+(.180)*(math.cos(t)*UP+math.sin(t)*side)
                hexnut('Visor_screw',p,n,.0055,.004,'Galv')

def span():
    seg=(28,18,10)[LOD]
    feed=[]
    for P,toward in ((POLE_W,1),(POLE_E,-1)):
        d=unit(-P*V3(1,0,1)); r=pole_r(EYE_Y); c=V3(P[0],EYE_Y,P[2])
        eye=c+d*(r+.055)
        # Galvanised pole band with forged eye lug, through-bolt and square nut.
        ring('Pole_band',c-V3(0,.035,0),UP,d,r+.001,r+.010,0,.07,'Galv',(24,14,8)[LOD])
        obox('Band_eye_lug',c+d*(r+.03),d,UP,(.05,.05,.014),'Galv',.004)
        torus('Lug_eye',eye,np.cross(d,UP),UP,.016,.005,'Galv')
        cyl('Through_bolt',c+d*(r+.02),c-d*(r+.05),.008,'Galv',8)
        if LOD<2: obox('Bolt_square_nut',c-d*(r+.03),d,UP,(.014,.03,.03),'Galv',.002)
        # Preformed dead-end grip wrapped back on the messenger.
        grip=[eye+d*.012]+[eye+d*(.02+k*.09)+V3(0,-.0006*k,0) for k in range(1,7)]
        tube('Dead_end_grip',grip,.0085,'Galv')
        torus('Wire_thimble',eye+d*.014,np.cross(d,UP),UP,.018,.0045,'Galv')
    # Messenger wire: a slight catenary in each half, meeting the clamp at the pivot.
    def wire_at(s):
        a=V3(POLE_W[0],EYE_Y,POLE_W[2])+unit(-POLE_W*V3(1,0,1))*(pole_r(EYE_Y)+.07)
        b=V3(POLE_E[0],EYE_Y,POLE_E[2])+unit(-POLE_E*V3(1,0,1))*(pole_r(EYE_Y)+.07)
        if s<.5: p=a+(V3(0,0,0)-a)*(s/.5); k=s/.5
        else: p=V3(0,0,0)+(b-V3(0,0,0))*((s-.5)/.5); k=(s-.5)/.5
        return p+V3(0,-.045*4*k*(1-k),0)
    n=(72,36,16)[LOD]
    tube('Messenger_span_wire',[wire_at(i/n) for i in range(n+1)],.0055,'Galv')
    # Lashed feed cable under the west half, with lashing clamps.
    fpts=[wire_at(i/n)+V3(0,-.026,0) for i in range(n//2+1)]
    fpts=[p for p in fpts if np.dot(p,U)<np.dot(DROP,U)-1e-6]+[DROP]
    W=V3(POLE_W[0],EYE_Y,POLE_W[2]); dW=unit(-POLE_W*V3(1,0,1))
    side=unit(dW*math.cos(math.radians(50))+np.cross(UP,dW)*math.sin(math.radians(50)))
    start=W+side*(pole_r(EYE_Y)+.016)+V3(0,-.12,0)
    lead=[start+V3(0,.07,0)+side*.02,W+dW*(pole_r(EYE_Y)+.11)+V3(0,-.10,0)]+[p for p in fpts if np.linalg.norm((p-W)*V3(1,0,1))>.32]
    tube('Lashed_feed_cable',catmull([start]+lead,(4,3,2)[LOD]),.0085,'Cable')
    if LOD<2:
        for k in range(1,int(np.linalg.norm(W*V3(1,0,1))/.9)):
            s=k*.9/np.linalg.norm(W*V3(1,0,1))*.5; c=wire_at(s)+V3(0,-.013,0)
            obox('Lashing_clamp',c,U,UP,(.016,.04,.02),'Galv',.003)
    # Feed down the west pole into the flasher cabinet, then rigid conduit to grade.
    cab_d=V3(1,0,0); cab_y=GROUND_Y+2.85; cab=V3(POLE_W[0],cab_y,POLE_W[2])+cab_d*(pole_r(cab_y)+.012+.10)
    down=[start+V3(0,-k*.5,0)*1.0+side*(pole_r(EYE_Y-k*.5)-pole_r(EYE_Y)) for k in range(int((EYE_Y-.12-(cab_y+.36))/.5)+1)]
    top=cab+V3(0,.29,0)
    cylm('Cabinet_cable_fitting',cab+V3(0,.255,0)-cab_d*.03,cab+V3(0,.292,0)-cab_d*.03,.014,'Galv',10)
    down+= [V3(POLE_W[0],cab_y+.55,POLE_W[2])+side*(pole_r(cab_y+.55)+.016), top+V3(0,.05,0)-cab_d*.03, top-cab_d*.03]
    tube('Pole_feed_cable',catmull(down,(2,2,1)[LOD]),.0085,'Cable')
    if LOD<2:
        for y in np.arange(cab_y+.8,EYE_Y-.3,1.1):
            obox('Cable_strap',V3(POLE_W[0],y,POLE_W[2])+side*(pole_r(y)+.016),side,UP,(.012,.03,.034),'Galv',.003)
    # 1986 pole-mounted flasher cabinet: galvanised box, door seam, hasp, rain hood, bands.
    box_size=(.30,.46,.20)
    obox('Flasher_cabinet',cab,np.cross(UP,cab_d),UP,box_size,'Galv',.008)
    obox('Cabinet_rain_hood',cab+V3(0,.245,0)+cab_d*.01,np.cross(UP,cab_d),UP,(.33,.025,.23),'Galv',.004)
    obox('Cabinet_door_seam',cab+cab_d*.1005,np.cross(UP,cab_d),UP,(.27,.43,.002),'Dark',0)
    obox('Cabinet_door',cab+cab_d*.103,np.cross(UP,cab_d),UP,(.262,.42,.006),'Galv',.003)
    if LOD<2:
        obox('Cabinet_hasp',cab+cab_d*.11+np.cross(UP,cab_d)*.11,cab_d,UP,(.012,.05,.02),'Galv',.002)
        for y in (-.15,.15): cyl('Cabinet_hinge',cab+cab_d*.104-np.cross(UP,cab_d)*.135+V3(0,y-.03,0),cab+cab_d*.104-np.cross(UP,cab_d)*.135+V3(0,y+.03,0),.007,'Galv',8)
    for y in (cab_y-.17,cab_y+.17):
        c=V3(POLE_W[0],y,POLE_W[2]); ring('Cabinet_pole_band',c-V3(0,.02,0),UP,cab_d,pole_r(y)+.001,pole_r(y)+.008,0,.04,'Galv',(24,14,8)[LOD])
    cylm('Cabinet_bottom_hub',cab+V3(0,-.23,0),cab+V3(0,-.26,0),.022,'Galv',10)
    conduit=[cab+V3(0,-.26,0),cab+V3(0,-.40,0)-cab_d*.04,V3(POLE_W[0],cab_y-.55,POLE_W[2])+cab_d*(pole_r(cab_y-.55)+.02),V3(POLE_W[0],GROUND_Y+.05,POLE_W[2])+cab_d*(pole_r(GROUND_Y)+.02),V3(POLE_W[0],GROUND_Y-.15,POLE_W[2])+cab_d*(pole_r(GROUND_Y)+.02)]
    tube('Rigid_conduit',catmull(conduit,(4,3,2)[LOD]),.016,'Galv')
    if LOD<2:
        for y in (GROUND_Y+.6,GROUND_Y+1.5):
            obox('Conduit_strap',V3(POLE_W[0],y,POLE_W[2])+cab_d*(pole_r(y)+.02),np.cross(UP,cab_d),UP,(.05,.018,.008),'Galv',.002)

BUILDERS={'SM_Caliche_Signal_Head':head,'SM_Caliche_Signal_Span':span}
LENSES=[{'center_three_m':list(n*HALF+V3(0,LENS_Y,0)+n*.050),'normal_three':list(n)} for n in FACES]
CONTRACT={'status':'SOURCE_GENERATED_NATIVE_UNVERIFIED','units':'metres','mapping':'Three (x,y,z) -> Blender (x,-z,y) -> UE centimetres (x,z,y)*100',
 'destination':'/Game/TownSignal','originality':'Original metric bpy geometry and procedural PBR textures. No third-party imagery or meshes.',
 'pivot_town_three_m':PIVOT_TOWN,'pivot_world_three_m':[PIVOT_TOWN[0]+8000,PIVOT_TOWN[1],PIVOT_TOWN[2]],
 'wire_axis_three':list(U),'swing_note':'Head rotates about the pivot; span is fixed. Light pool (0,-1.05,0) and lens glows (0,-0.63,0) of the old crossing keep their pivot-relative places.',
 'lens_material':'M_TownSignal_Lens','lens_runtime_parameters':{'EmissiveFactor':'linear RGB, zero when off, multiplies the amber texture'},
 'lenses':LENSES,'light_pool_three_m':[0,-1.05,0],'glow_three_m':[0,-.63,0],'parts':{}}
EXPORT=bpy.data.collections.new('DELIVERY_FBX'); SC.collection.children.link(EXPORT)
for name,builder in BUILDERS.items():
    counts=[]; data=None; components=[]
    for LOD in range(3):
        COL=bpy.data.collections.new(name+'_EDIT_LOD'+str(LOD)); SC.collection.children.link(COL); PART=[]
        builder(); components.append(len(PART))
        for o in PART:
            active(o); bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
        bpy.ops.object.select_all(action='DESELECT')
        for o in PART: o.select_set(True)
        bpy.context.view_layer.objects.active=PART[0]; bpy.ops.object.duplicate(); copies=list(bpy.context.selected_objects)
        bpy.context.view_layer.objects.active=copies[0]; bpy.ops.object.join(); ob=bpy.context.object
        ob.name=name if LOD==0 else name+'_LOD'+str(LOD)
        for collection in list(ob.users_collection): collection.objects.unlink(ob)
        EXPORT.objects.link(ob); active(ob)
        SC.cursor.location=(0,0,0); bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
        bpy.ops.object.mode_set(mode='EDIT'); bpy.ops.mesh.select_all(action='SELECT'); bpy.ops.mesh.remove_doubles(threshold=1e-6); bpy.ops.mesh.select_all(action='SELECT'); bpy.ops.uv.cube_project(cube_size=1); bpy.ops.object.mode_set(mode='OBJECT')
        t=ob.modifiers.new('Explicit_delivery_triangles','TRIANGULATE'); bpy.ops.object.modifier_apply(modifier=t.name)
        bm=bmesh.new(); bm.from_mesh(ob.data); bmesh.ops.dissolve_degenerate(bm,dist=1e-7,edges=bm.edges[:]); bmesh.ops.triangulate(bm,faces=bm.faces[:]); bm.to_mesh(ob.data); bm.free()
        bad=[p.index for p in ob.data.polygons if p.area<=1e-12]
        if bad:
            bm=bmesh.new(); bm.from_mesh(ob.data); bm.faces.ensure_lookup_table(); bmesh.ops.delete(bm,geom=[bm.faces[i] for i in bad],context='FACES'); bm.to_mesh(ob.data); bm.free()
        points=[ob.matrix_world@v.co for v in ob.data.vertices]
        bounds=[[float(fn(p[k] for p in points)) for k in range(3)] for fn in (min,max)]
        count=len(ob.data.polygons); counts.append(count)
        assert all(len(p.vertices)==3 and p.area>1e-12 for p in ob.data.polygons),name+' degenerate triangle'
        assert all(math.isfinite(f) for p in points for f in p),name+' nonfinite vertex'
        if LOD==0:
            three=[[bounds[0][0],bounds[0][2],-bounds[1][1]],[bounds[1][0],bounds[1][2],-bounds[0][1]]]
            data={'bounds_local_m':bounds,'bounds_three_m':three,'materials':[m.name for m in ob.data.materials],'origin':[0,0,0]}
        bpy.ops.export_scene.fbx(filepath=str(OUT/(ob.name+'.fbx')),use_selection=True,object_types={'MESH'},apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',axis_forward='-Y',axis_up='Z',bake_anim=False,add_leaf_bones=False,mesh_smooth_type='FACE',use_tspace=True)
        ob.hide_render=True; ob.hide_set(True)
        COL.hide_viewport=LOD>0; COL.hide_render=True
    assert counts[0]>counts[1]>counts[2]>0,(name,counts)
    data['lod_triangles']=counts; data['editable_components']=components; CONTRACT['parts'][name]=data
CONTRACT['total_LOD0_triangles']=sum(r['lod_triangles'][0] for r in CONTRACT['parts'].values())
assert CONTRACT['total_LOD0_triangles']<30000,CONTRACT['total_LOD0_triangles']
(HERE/'materials.json').write_text(json.dumps(ROWS,indent=2)+'\n')
(HERE/'model_contract.json').write_text(json.dumps(CONTRACT,indent=2)+'\n')
SC.world=bpy.data.worlds.new('Neutral_signal_world'); SC.world.color=(.18,.18,.18)
SC.render.engine='CYCLES'; SC.cycles.device='CPU'; SC.cycles.samples=20
SC.view_settings.view_transform='AgX'
bpy.ops.wm.save_as_mainfile(filepath=str(HERE/'Caliche_TrafficSignal.blend'))
print('TRAFFIC SIGNAL SOURCE PASS',json.dumps({k:v['lod_triangles'] for k,v in CONTRACT['parts'].items()}))
