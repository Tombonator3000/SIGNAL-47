#!/usr/bin/env python3
"""SIGNAL / 47: the control room's floor, 12 x 9 m of flecked 30 cm VCT that has had fifteen years of night shifts.

Own work, nothing sampled. One unique picture of the whole room, so the wear can be where the people walked:
from the two doors to the chairs, the rack, the printer, the telex; caster arcs round the chairs; wax built up by the
walls and under the desks. Run with Blender's own python, which has numpy:
  ~/.local/opt/blender-5.2.2-linux-x64/5.2/python/bin/python3.13 vct_floor.py [out_dir]
Three.js coordinates: x -6..6, z -4.5..4.5 (U runs with x, V with z; the mesh's UV are made for this).
"""
import math,struct,sys,zlib
from pathlib import Path
import numpy as np
OUT=Path(sys.argv[1]) if len(sys.argv)>1 else Path(__file__).parent/'textures'
OUT.mkdir(parents=True,exist_ok=True)
N=4096
W,D=12.,9.
TILE=.30
rng=np.random.default_rng(1986)
def png(path,a):
    a=np.clip(a,0,1);a=(a*255+.5).astype(np.uint8)
    h,w=a.shape[:2];c=1 if a.ndim==2 else a.shape[2]
    raw=b''.join(b'\0'+a[y].tobytes() for y in range(h))
    def chunk(t,d):return struct.pack('>I',len(d))+t+d+struct.pack('>I',zlib.crc32(t+d)&0xffffffff)
    Path(path).write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',w,h,8,{1:0,3:2}[c],0,0,0))+chunk(b'IDAT',zlib.compress(raw,6))+chunk(b'IEND',b''))
def srgb2lin(c):c=np.asarray(c,dtype=np.float32);return np.where(c<=.04045,c/12.92,((c+.055)/1.055)**2.4)
def lin2srgb(c):return np.where(c<=.0031308,c*12.92,1.055*np.power(np.maximum(c,1e-7),1/2.4)-.055)
px=(np.arange(N,dtype=np.float32)+.5)/N
X=(px*W-W/2)[None,:];Z=(px*D-D/2)[:,None]
XX=np.broadcast_to(X,(N,N));ZZ=np.broadcast_to(Z,(N,N))
pmx,pmz=W/N,D/N # metres per pixel
print('mm/px',pmx*1000,pmz*1000)
def blur_noise(sx,sz,seed):
    """white noise blurred to a size in metres (anisotropic gaussian by FFT), to mean 0 std 1"""
    r=np.random.default_rng(seed);a=r.standard_normal((N,N),dtype=np.float32)
    fx=np.fft.rfftfreq(N)[None,:];fz=np.fft.fftfreq(N)[:,None]
    g=np.exp(-2*(math.pi**2)*((fx*sx/pmx)**2+(fz*sz/pmz)**2)).astype(np.float32)
    o=np.fft.irfft2(np.fft.rfft2(a)*g,s=(N,N)).astype(np.float32)
    return (o-o.mean())/(o.std()+1e-6)
# The tile grid, not lined up with the walls: nobody ever gets that right, so there are cut tiles at every wall.
OX,OZ=-.07,.11
fx=(XX-OX)/TILE;fz=(ZZ-OZ)/TILE
ix=np.floor(fx).astype(np.int32);iz=np.floor(fz).astype(np.int32)
ux=fx-ix;uz=fz-iz
edge=np.minimum(np.minimum(ux,1-ux),np.minimum(uz,1-uz))*TILE # metres to the nearest joint
key=(ix+100)*1000+(iz+100)
ukeys,inv=np.unique(key,return_inverse=True);inv=inv.reshape(N,N)
n=len(ukeys);print('tiles',n)
t=np.random.default_rng(47)
tile_light=t.normal(0,.016,n).astype(np.float32);tile_warm=t.normal(0,.014,n).astype(np.float32)
tile_tiltx=t.normal(0,.0075,n).astype(np.float32);tile_tiltz=t.normal(0,.0075,n).astype(np.float32)
tile_turn=((ukeys//1000)+(ukeys%1000))%2==0
tile_dx=t.integers(0,N,n);tile_dz=t.integers(0,N,n)
tile_chipped=t.random(n)<.006;tile_corner=t.integers(0,4,n)
# Flecks: three sizes and two directions, each tile cut from its own place in the sheet and laid a quarter turn from its neighbours.
print('flecks')
def tiled(field):
    return field[(np.arange(N)[:,None]+tile_dz[inv])%N,(np.arange(N)[None,:]+tile_dx[inv])%N]
streakA=blur_noise(.055,.008,1);streakB=blur_noise(.008,.055,2)
streak=np.where(tile_turn[inv],tiled(streakA),tiled(streakB));del streakA,streakB
fleck1=tiled(blur_noise(.0028,.0024,3));fleck2=tiled(blur_noise(.0052,.0042,4));fleck3=tiled(blur_noise(.011,.009,5))
cloud=tiled(blur_noise(.07,.07,6))
# Colours, sRGB: warm grey beige field, pale and dark and a little old green in the chips.
field=srgb2lin(np.array([.640,.590,.500],dtype=np.float32))
pale=srgb2lin(np.array([.745,.705,.625],dtype=np.float32))
dark=srgb2lin(np.array([.410,.390,.350],dtype=np.float32))
green=srgb2lin(np.array([.470,.495,.435],dtype=np.float32))
col=np.ones((N,N,3),dtype=np.float32)*field
def mix(c,m,to):return c*(1-m[...,None])+to*m[...,None]
s=streak*.9+.4*cloud
col=mix(col,np.clip((s-.1)*.16,0,.22),pale)
col=mix(col,np.clip((-s-.1)*.14,0,.18),dark)
col=mix(col,np.clip((fleck2-1.5)*1.6,0,.42),pale)
col=mix(col,np.clip((fleck1-1.5)*1.7,0,.5),dark)
col=mix(col,np.clip((fleck3-1.7)*1.5,0,.3)*np.clip((streak+.4),0,1),green)
col=mix(col,np.clip((-fleck2-1.75)*1.8,0,.45),dark)
del fleck1,fleck3,streak
# Each tile its own batch.
col*=(1+tile_light[inv])[...,None];col[...,0]*=1+tile_warm[inv];col[...,2]*=1-tile_warm[inv]
# A later repair: tiles from another lot, greener and cleaner, over a cable run from the rack to the console.
def inrect(x0,x1,z0,z1):return (XX>=x0)&(XX<=x1)&(ZZ>=z0)&(ZZ<=z1)
patch=np.zeros((N,N),dtype=bool)
tcx=(ix+.5)*TILE+OX;tcz=(iz+.5)*TILE+OZ
patch|=(tcx>3.1)&(tcx<5.2)&(tcz>-2.25)&(tcz<-1.65)
patch|=(tcx>-1.1)&(tcx<-.5)&(tcz>-3.45)&(tcz<-2.1)
patchc=srgb2lin(np.array([.545,.56,.50],dtype=np.float32))/field
col=np.where(patch[...,None],col*patchc*(1+.03*cloud[...,None]),col)
# Where the people went. Width and strength in metres and 0..1; a path is a polyline.
print('wear')
paths=[([(-2.4,4.3),(-2.25,2.6),(-1.2,.2),(-.2,-1.6),(.05,-2.7)],.42,.85), # south door to the middle console
 ([(-2.3,2.6),(-2.2,-.5),(-2.23,-2.6)],.34,.65),                            # to the left console
 ([(-1.2,.2),(.9,-1),(2.07,-2.6)],.34,.65),                                  # to the right console
 ([(5.95,1.6),(4.6,1.5),(3.9,1.9),(3.2,1.7),(2.3,1.72)],.36,.75),             # east door to the desk's front
 ([(5.95,1.6),(4.9,.9),(4.75,-.4),(4.8,-1.5),(4.8,-2.4)],.34,.8),             # east door along the printer and decoder to the rack
 ([(2.07,-2.6),(3.6,-2.2),(4.8,-2.4)],.26,.45),                               # console to rack
 ([(-2.3,2.8),(-3.6,3.2),(-4.1,3.55)],.28,.5),                                # to the telex
 ([(-2.0,1.7),(-3.6,2.3),(-4.75,2.45)],.3,.55),                               # to the ops terminal
 ([(3.9,1.9),(4.2,3.1)],.22,.5),                                              # to the coffee
 ([(-1.2,.2),(.8,1.55),(2.3,1.72)],.3,.55),                                   # south door to the desk
]
wear=np.zeros((N,N),dtype=np.float32)
# (coarse grid is plenty for this)
C=512;cx=((np.arange(C)+.5)/C*W-W/2)[None,:];cz=((np.arange(C)+.5)/C*D-D/2)[:,None]
w=np.zeros((C,C),dtype=np.float32)
for pts,width,amount in paths:
    d=np.full((C,C),1e9,dtype=np.float32)
    for (a,b),(c,e) in zip(pts,pts[1:]):
        vx,vz=c-a,e-b;l2=vx*vx+vz*vz
        tt=np.clip(((cx-a)*vx+(cz-b)*vz)/l2,0,1)
        d=np.minimum(d,np.hypot(cx-(a+tt*vx),cz-(b+tt*vz)))
    w=np.maximum(w,amount*np.exp(-(d/width)**2))
# The chairs: a worn disc under each, the casters' ring round it.
chairs=[(-2.23,-2.95,.62),(.05,-2.95,.62),(2.07,-2.95,.62),(2.5,3.42,.55),(-4.8,2.45,.5)]
ring=np.zeros((C,C),dtype=np.float32)
for x,z,r in chairs:
    d=np.hypot(cx-x,cz-z)
    w=np.maximum(w,.8*np.exp(-(d/(r*.95))**4))
    ring=np.maximum(ring,np.exp(-((d-r*.55)/.11)**2))
def up(a):
    # Bilinear up to N.
    idx=(np.arange(N)+.5)/N*a.shape[0]-.5;i0=np.clip(np.floor(idx).astype(int),0,a.shape[0]-1);i1=np.clip(i0+1,0,a.shape[0]-1);f=(idx-i0).astype(np.float32)
    r=a[i0][:,i0]*((1-f)[:,None]*(1-f)[None,:])+a[i1][:,i0]*(f[:,None]*(1-f)[None,:])+a[i0][:,i1]*((1-f)[:,None]*f[None,:])+a[i1][:,i1]*(f[:,None]*f[None,:])
    return r
wear=up(w);ring=up(ring)
wn=blur_noise(.14,.14,7);wn2=blur_noise(.025,.025,8)
wear=np.clip(wear*(1+.28*wn)+.05*wn2*wear,0,1)
# Against the walls and under the furniture: never walked on, thirty coats of wax and the dirt in them.
walls=np.minimum(np.minimum(XX+W/2,W/2-XX),np.minimum(ZZ+D/2,D/2-ZZ))
foot=[(-3.5,3.5,-4.5,-3.42),(1.68,3.52,2.08,3.02),(2.17,2.73,3.02,3.58),(4.05,4.71,3.55,4.5),(5.3,6,-2.75,-2.05),(5.4,6,-1.92,-1.18),(5.2,6,-1,.2),(5.6,6,.3,1.05),
 (-6,-5.4,-1.5,.3),(-6,-5.25,3.7,4.5),(-6,-5.15,1.62,3.18),(-5.13,-4.57,2.27,2.83),(-4.6,-3.8,3.9,4.5),(-5.43,-5.08,-.26,.26),(-6,-5.42,-2.6,-1.55)]
dist=walls.copy()
for x0,x1,z0,z1 in foot:
    dx=np.maximum(np.maximum(x0-XX,XX-x1),0);dz=np.maximum(np.maximum(z0-ZZ,ZZ-z1),0)
    dist=np.minimum(dist,np.hypot(dx,dz))
build=np.exp(-(dist/.13)**1.4)*(1+.25*wn2*.6)
build=np.clip(build,0,1)
# Wear: the wax is gone, the chips go flat and paler, and walked-in dirt greys it.
grey=col.mean(axis=2,keepdims=True)*np.array([1.02,1.0,.96],dtype=np.float32)
col=col*(1-(wear*.62)[...,None])+grey*.97*(wear*.62)[...,None]
col*=(1-.17*wear)[...,None]
# Wax and dirt by the walls: browner, darker.
col=col*(1-(build*.42)[...,None])+col*np.array([.60,.50,.36],dtype=np.float32)*(build*.42)[...,None]
# Ground-in grime where the feet stop: in front of each chair and at both doors, brown and blotchy.
blot=np.zeros((C,C),dtype=np.float32)
for x,z,r in chairs+[(-2.4,4.0,.9),(5.6,1.6,.8),(4.6,-.4,.6),(-4.4,3.2,.5)]:
    d=np.hypot(cx-x,cz-z);blot=np.maximum(blot,np.exp(-(d/(r*1.15))**2))
blot=up(blot)*np.clip(.55+.5*blur_noise(.09,.09,30),0,1.3)
col=col*(1-(blot*.30)[...,None])+col*np.array([.70,.58,.42],dtype=np.float32)*(blot*.30)[...,None]
spots=np.clip((blur_noise(.03,.03,31)-1.6)*1.5,0,1)*np.clip(wear+blot,0,1)
col*=(1-.25*spots)[...,None]
# Long slow stains in the field.
big=blur_noise(.5,.5,9);col*=(1+.02*big)[...,None]
# Joints: dark with old dirt, darker where it is walked on, a soft edge where the wax stopped.
joint=np.exp(-(edge/.0021)**2)
halo=np.exp(-(edge/.008)**1.2)
dirt=srgb2lin(np.array([.145,.125,.10],dtype=np.float32))
k=np.clip(joint*(.62+.25*wear),0,1)
col=mix(col,k,dirt)
col*=(1-(halo*(.05+.10*wear)))[...,None]
print('marks')
# Everything laid on by hand: heel marks, caster arcs, cigarette burns, chips, coffee.
mark=np.zeros((N,N),dtype=np.float32)
burn=np.zeros((N,N),dtype=np.float32)
stain=np.zeros((N,N),dtype=np.float32)
def to_px(x,z):return (x+W/2)/pmx,(z+D/2)/pmz
def stroke(x0,z0,angle,length,curve,width,amount,field):
    steps=max(3,int(length/.004))
    a=angle
    x,z=x0,z0
    wpx=max(1,int(width/pmx))
    for i in range(steps):
        x+=math.cos(a)*length/steps;z+=math.sin(a)*length/steps;a+=curve/steps
        amt=amount*(1-abs(i/steps-.5)*1.3)
        cxp,czp=to_px(x,z);ci,cj=int(cxp),int(czp)
        if not(wpx<=ci<N-wpx and wpx<=cj<N-wpx):continue
        field[cj-wpx:cj+wpx+1,ci-wpx:ci+wpx+1]=np.maximum(field[cj-wpx:cj+wpx+1,ci-wpx:ci+wpx+1],amt)
r=np.random.default_rng(1)
# Heel marks where the paths are.
for _ in range(620):
    i,j=r.integers(0,C,2)
    if w[j,i]<r.random()*.9+.1:continue
    stroke(i/C*W-W/2,j/C*D-D/2,r.random()*6.28,r.uniform(.02,.11),r.normal(0,.8),.0024,r.uniform(.3,.85),mark)
# Caster arcs round each chair, in bundles.
for x,z,cr in chairs:
    for _ in range(90):
        a=r.random()*6.28;rad=cr*r.uniform(.25,.85)
        stroke(x+math.cos(a)*rad,z+math.sin(a)*rad,a+1.57,r.uniform(.05,.3),r.uniform(.5,2.2)*r.choice([-1,1]),.0022,r.uniform(.2,.55),mark)
# Burns: dropped cigarettes, by the chairs and the desk.
for x,z,cr in chairs:
    for _ in range(9 if z<0 else 6):
        a=r.random()*6.28;d=r.uniform(.2,cr+.35);bx,bz=x+math.cos(a)*d,z+math.sin(a)*d
        L=r.uniform(.012,.028);ang=r.random()*3.14
        ci,cj=to_px(bx,bz);R=int(.05/pmx)
        ci,cj=int(ci),int(cj)
        if not(R<ci<N-R and R<cj<N-R):continue
        yy,xx=np.mgrid[-R:R+1,-R:R+1].astype(np.float32);xx*=pmx;yy*=pmz
        u=xx*math.cos(ang)+yy*math.sin(ang);v=-xx*math.sin(ang)+yy*math.cos(ang)
        dd=np.hypot(u/L,v/(L*.42))
        burn[cj-R:cj+R+1,ci-R:ci+R+1]=np.maximum(burn[cj-R:cj+R+1,ci-R:ci+R+1],np.exp(-dd**2.4))
# Old coffee, by the desk and the console's middle, and a wide one by the coffee table.
for bx,bz,rad in [(3.9,2.6,.11),(4.2,3.3,.2),(1.4,-2.7,.09),(-.9,-2.4,.13),(-3.8,2.9,.1),(.4,1.0,.07)]:
    ci,cj=to_px(bx,bz);R=int(rad*2/pmx);ci,cj=int(ci),int(cj)
    yy,xx=np.mgrid[-R:R+1,-R:R+1].astype(np.float32);xx*=pmx;yy*=pmz
    dd=np.hypot(xx,yy*1.15)+(.25*rad)*np.sin(np.arctan2(yy,xx)*3+bx)
    v=np.clip(1-dd/rad,0,1)**.5*.35+np.exp(-((dd-rad)/(rad*.08))**2)*.28
    stain[cj-R:cj+R+1,ci-R:ci+R+1]=np.maximum(stain[cj-R:cj+R+1,ci-R:ci+R+1],v.astype(np.float32))
scuff=srgb2lin(np.array([.08,.075,.07],dtype=np.float32))
col=mix(col,np.clip(mark*.62,0,.8),scuff)
col=mix(col,np.clip(burn,0,1)*.55,srgb2lin(np.array([.17,.10,.05],dtype=np.float32)))
col=mix(col,np.clip(burn-.55,0,1)*1.9,srgb2lin(np.array([.03,.025,.02],dtype=np.float32)))
col*=(1-stain*np.array([.34,.42,.58],dtype=np.float32)[None,None,:].repeat(N,0)[...]) if False else 1
col[...,0]*=1-stain*.30;col[...,1]*=1-stain*.40;col[...,2]*=1-stain*.55
# A few tiles have lost a corner, down to the dark adhesive.
c_x=np.where(tile_corner[inv]%2==0,ux,1-ux);c_z=np.where(tile_corner[inv]//2==0,uz,1-uz)
chip=(tile_chipped[inv])&((c_x+c_z)*TILE+.008*wn2<.032)
col=np.where(chip[...,None],srgb2lin(np.array([.20,.18,.15],dtype=np.float32))*(1+.2*wn2[...,None]),col)
# Base colour.
base=lin2srgb(np.clip(col,0,1))
png(OUT/'T_Lived_VCT_BC.png',base)
print('base done',base.reshape(-1,3).mean(0)*255)
# Roughness, half size. Waxed .30ish at the walls, .62 in the lanes, joints rough, marks rough.
def half(a):
    while a.shape[0]>2048:a=(a[::2,::2]+a[1::2,::2]+a[::2,1::2]+a[1::2,1::2])*.25
    return a
rough=.36+.27*wear+.03*wn+.015*wn2-.05*build+.28*joint+.2*np.clip(mark,0,1)+.22*chip+.18*stain+.12*burn
rough=np.where(patch,rough-.03,rough)
png(OUT/'T_Lived_VCT_R.png',half(np.clip(rough,.14,1)))
# Normal (DirectX): each tile tips very slightly on its own, the joints are a groove, the wear is pitted.
h=(-np.exp(-(edge/.0035)**2)*.0009-np.exp(-(edge/.012)**1.5)*.00025+.00012*wn2*(.4+wear)-.0007*chip).astype(np.float32)
h=half(h);m=2048
sx,sz=W/m,D/m
dhdx=(np.roll(h,-1,1)-np.roll(h,1,1))/(2*sx);dhdz=(np.roll(h,-1,0)-np.roll(h,1,0))/(2*sz)
tx=half(tile_tiltx[inv]);tz=half(tile_tiltz[inv])
# Tiles curl up at the joints a little after years of mopping.
curl=half((np.exp(-(edge/.03)**2)*.0024*np.cos(2*math.pi*fx)).astype(np.float32))
curlz=half((np.exp(-(edge/.03)**2)*.0024*np.cos(2*math.pi*fz)).astype(np.float32))
nx=-(dhdx+tx+curl*.0);nz=-(dhdz+tz+curlz*.0);ny=np.ones_like(nx)
ll=np.sqrt(nx*nx+nz*nz+1);nx/=ll;nz/=ll;ny/=ll
# U runs +x, V runs +z. DirectX green points down the picture (+V) negated: green = 0.5 - ...
nmap=np.stack([nx*.5+.5,-(-nz)*.5+.5,ny*.5+.5],axis=2) if False else np.stack([nx*.5+.5,nz*.5+.5,ny*.5+.5],axis=2)
png(OUT/'T_Lived_VCT_N.png',nmap)
# A view of the mean colours over regions, for checking.
def m(rect):
    a=base[int((rect[2]+D/2)/pmz):int((rect[3]+D/2)/pmz),int((rect[0]+W/2)/pmx):int((rect[1]+W/2)/pmx)]
    return (a.reshape(-1,3).mean(0)*255).round(1).tolist()
print('field',m((-1,1,1,2)),'lane',m((-2.5,-2,0,1)),'wall',m((-.5,.5,4.3,4.5)))
