#!/usr/bin/env python3
"""Own tiling surfaces and the grime atlas for the lived-in rooms. Blender's python (numpy). No samples."""
import math,struct,sys,zlib
from pathlib import Path
import numpy as np
OUT=Path(sys.argv[1]) if len(sys.argv)>1 else Path(__file__).parent/'textures'
OUT.mkdir(parents=True,exist_ok=True)
def png(path,a):
    a=np.clip(a,0,1);a=(a*255+.5).astype(np.uint8)
    h,w=a.shape[:2];c=1 if a.ndim==2 else a.shape[2]
    raw=b''.join(b'\0'+a[y].tobytes() for y in range(h))
    def chunk(t,d):return struct.pack('>I',len(d))+t+d+struct.pack('>I',zlib.crc32(t+d)&0xffffffff)
    Path(path).write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',w,h,8,{1:0,3:2,4:6}[c],0,0,0))+chunk(b'IDAT',zlib.compress(raw,6))+chunk(b'IEND',b''))
def noise(n,sx,sy,seed):
    r=np.random.default_rng(seed);a=r.standard_normal((n,n))
    fx=np.fft.rfftfreq(n)[None,:];fy=np.fft.fftfreq(n)[:,None]
    o=np.fft.irfft2(np.fft.rfft2(a)*np.exp(-2*math.pi**2*((fx*sx)**2+(fy*sy)**2)),s=(n,n))
    return (o-o.mean())/(o.std()+1e-6)
def scratches(n,count,seed,long=.08):
    r=np.random.default_rng(seed);a=np.zeros((n,n),dtype=np.float32)
    for _ in range(count):
        x,y=r.random(2)*n;ang=r.random()*6.28;L=r.uniform(.01,long)*n;amt=r.uniform(.25,1)
        for i in range(int(L)):
            xi,yi=int(x)%n,int(y)%n;a[yi,xi]=max(a[yi,xi],amt*(1-abs(i/L-.5)))
            x+=math.cos(ang);y+=math.sin(ang);ang+=r.normal(0,.01)
    return a
n=1024
# Enamel paint over steel, tinted per material: dirt clouds, drips, scratches through to primer.
d=noise(n,28,28,1);d2=noise(n,6,6,2);d3=noise(n,120,120,3);sc=scratches(n,520,4);drip=noise(n,3,60,5)
b=.86+.05*d3+.035*d+.015*d2-.05*np.clip(drip-1.2,0,1)
b=b*(1-.30*sc)
png(OUT/'T_Lived_Enamel_BC.png',np.stack([b,b*.985,b*.955],2))
png(OUT/'T_Lived_Enamel_R.png',np.clip(.5+.06*d+.05*d3+.25*sc,.2,1))
# Plastic: same idea, finer, cleaner.
d=noise(n,14,14,6);sc=scratches(n,260,7,.03)
b=.9+.03*d+.012*noise(n,2,2,8);b=b*(1-.1*sc)
png(OUT/'T_Lived_Plastic_BC.png',np.stack([b,b*.99,b*.97],2))
png(OUT/'T_Lived_Plastic_R.png',np.clip(.42+.05*d+.2*sc,.15,1))
# Cardboard: fibres one way, a little patchy.
f=noise(n,60,1.6,9);c=noise(n,30,30,10);b=.92+.03*f+.04*c
png(OUT/'T_Lived_Card_BC.png',np.stack([b,b*.98,b*.93],2))
png(OUT/'T_Lived_Card_R.png',np.clip(.86+.04*f,0,1))
# Cloth: a tight weave for the jacket.
yy,xx=np.mgrid[0:n,0:n];w=(np.sin(xx*2*math.pi/8)*np.sin(yy*2*math.pi/8))*.5+.5;c=noise(n,24,24,11)
b=.82+.11*w+.05*c
png(OUT/'T_Lived_Cloth_BC.png',np.stack([b,b,b],2))
png(OUT/'T_Lived_Cloth_R.png',np.clip(.88+.06*w,0,1))
# Grime atlas 2048, RGBA, straight alpha. Bands wrap in U, cells do not.
N=2048;A=np.zeros((N,N,4),dtype=np.float32)
def band(row0,row1,fn):
    h=row1-row0;v=(np.arange(h)+.5)/h
    fn(A[row0:row1],v[:,None],h)
nb=noise(N,40,40,20);nb2=noise(N,5,5,21);nb3=noise(N,180,180,22)
vv=np.linspace(0,1,256)[:,None]
# Band 0, rows 0..256 (v .875..1 for Blender): the dirt at the foot of a wall. Row 255 (lowest in the picture) is the floor.
a=np.clip((vv**2.2)*(.55+.18*nb[:256]+.20*nb3[:256])+np.clip(nb2[:256]-1.6,0,1)*.08*(vv**.5),0,.82)
A[0:256,:,0]=.11;A[0:256,:,1]=.085;A[0:256,:,2]=.06;A[0:256,:,3]=a
# Band 1, 256..512: rubbed through: chips of bright metal on an edge, patchy. Grey.
nx=noise(N,10,10,23)[256:512];mask=np.clip((nx-.5)*1.4,0,1)*np.sin(vv*math.pi)
A[256:512,:,:3]=np.array([.56,.55,.52]);A[256:512,:,3]=np.clip(mask*.9,0,1)
# Band 2, 512..768: a chair back's rub along a wall: darker, greasy, soft.
n3=noise(N,70,70,24)[512:768];m=np.exp(-((vv-.5)/.28)**2)*np.clip(.55+.35*n3+.12*nb2[512:768],0,1)
A[512:768,:,:3]=np.array([.09,.08,.07]);A[512:768,:,3]=m*.5
# Cells, 512 each, rows from 1024 down.
def cell(ix,iy):
    return 1024+iy*512,ix*512
yy,xx=np.mgrid[0:512,0:512];u=(xx+.5)/512*2-1;v=(yy+.5)/512*2-1;r=np.hypot(u,v)
def put(ix,iy,rgb,alpha):
    y,x=cell(ix,iy);A[y:y+512,x:x+512,:3]=rgb;A[y:y+512,x:x+512,3]=np.clip(alpha,0,1)*(r<1.42)
n4=noise(512,14,14,25);n5=noise(512,3,3,26);n6=noise(512,50,50,27)
# (0,0) the nicotine halo round a ceiling fixture.
put(0,0,np.array([.52,.40,.17]),np.clip(1-r*(1+.18*n6),0,1)**1.8*(.40+.10*n4+.04*n5))
# (1,0) a leak's ring on a ceiling tile.
ring=np.exp(-((r+.14*n6-.68)/.07)**2)*.38;fill=np.clip(.72-r+.14*n6,0,1)*.22
put(1,0,np.array([.24,.15,.07]),(ring+fill)*(.85+.1*n5))
# (2,0) a tile gone a shade yellower overall.
put(2,0,np.array([.46,.37,.20]),np.clip((1.2-np.maximum(abs(u),abs(v)))*4,0,1)*(.34+.12*n4+.06*n5))
# (3,0) a scuffed patch: scratches through paint, low on doors and cabinets.
sc=scratches(512,400,28,.2);put(3,0,np.array([.20,.19,.17]),np.clip(1-r,0,1)**.7*(np.clip(.2+.3*n4,0,1)+sc*.75))
# (0,1) a hand's smear at a door's edge, greasy.
g=np.exp(-(u/.34)**2-(v/.75)**2);put(0,1,np.array([.07,.06,.05]),g*(.34+.2*n4+.12*n5))
# (1,1) dust film, for the tops of things: pale, broken.
put(1,1,np.array([.60,.56,.50]),np.clip(1.25-np.maximum(abs(u),abs(v)),0,.25)*4*(.42+.2*n4+.08*n5))
# (2,1) a coffee ring on a desk.
ring=np.exp(-((r-.72)/.035)**2)*(.6+.25*n5);put(2,1,np.array([.20,.11,.05]),ring)
# (3,1) cigarette burn on a laminate edge.
b1=np.exp(-((u/.62)**2+(v/.26)**2)*2.4);put(3,1,np.array([.05,.035,.025]),b1*(.9+.1*n5))
png(OUT/'T_Lived_Grime_BC.png',A)
print('ok')
# ---- painted concrete block, 1.6 m a tile: 40 x 20 cm running bond, paint over the pits. Tint per wall.
n=2048;yy,xx=np.mgrid[0:n,0:n];cw,ch=n//4,n//8
row=yy//ch;xo=(xx+(row%2)*cw//2)%cw;yo=yy%ch
edge=np.minimum(np.minimum(xo,cw-xo),np.minimum(yo,ch-yo)).astype(np.float32)
joint=np.clip((edge-1.5)/4.5,0,1) # 12.8 px a... 1.25 px per mm: a 10 mm joint is ~6 px each side
pits=noise(n,1.2,1.2,40);p2=noise(n,3,3,41);cl=noise(n,40,40,42);cl2=noise(n,160,160,43)
blockid=(row*97+(xx+(row%2)*cw//2)//cw*13)%64
ton=np.random.default_rng(44).normal(0,.018,64)[blockid]
b=.86+.04*cl+.03*cl2+ton+.02*p2
b*=(.80+.20*joint)
pitm=np.clip((pits-1.35)*2.2,0,1);b*=1-.22*pitm
chalk=np.clip((noise(n,8,8,45)-1.5)*2,0,1)
png(OUT/'T_Lived_Block_BC.png',np.stack([b*(1-.03*chalk),b*.985,b*.95],2))
png(OUT/'T_Lived_Block_R.png',np.clip(.84+.06*cl+.1*(1-joint),0,1))
h=joint*1.6+.16*p2-.55*pitm+.04*cl
dx=(np.roll(h,-1,1)-np.roll(h,1,1))*.9;dy=(np.roll(h,-1,0)-np.roll(h,1,0))*.9;l=np.sqrt(dx*dx+dy*dy+1)
png(OUT/'T_Lived_Block_N.png',np.stack([-dx/l*.5+.5,-dy/l*.5+.5,1/l*.5+.5],2))
# ---- outside grime, 2048 x 2048, RGBA: bands wrap in U. 0..512: road dust rising from the foot (bottom of the band is the ground); 512..1024 rain from under a coping (top is the coping)
#      1024..1536: rust below a fitting, wide; 1536..2048: dark damp running down from a scupper.
N=2048;B=np.zeros((N,N,4),dtype=np.float32);vv=np.linspace(0,1,512)[:,None]
t1=noise(N,14,140,50);t2=noise(N,4,80,51);t3=noise(N,70,30,52);t4=noise(N,3,3,53)
a=np.clip((vv**1.6)*(.72+.16*t3[:512]+.06*t4[:512])+np.clip(t1[:512]-.2,0,1)*.2*vv,0,.9)
B[0:512,:,0]=.56;B[0:512,:,1]=.45;B[0:512,:,2]=.32;B[0:512,:,3]=a
vr=(1-vv)
a=np.clip(vr**1.6*np.clip((t1[512:1024]+.3)*.55+t2[512:1024]*.12,0,1)*.45,0,.45)
B[512:1024,:,0]=.17;B[512:1024,:,1]=.14;B[512:1024,:,2]=.11;B[512:1024,:,3]=a
a=np.clip(vr**1.8*np.clip((t2[1024:1536]+.5)*.5,0,1)*.6,0,.5)
B[1024:1536,:,0]=.26;B[1024:1536,:,1]=.115;B[1024:1536,:,2]=.04;B[1024:1536,:,3]=a
a=np.clip(vr**1.1*np.clip((t1[1536:2048]-.05)*.8,0,1),0,.72)
B[1536:2048,:,0]=.10;B[1536:2048,:,1]=.085;B[1536:2048,:,2]=.07;B[1536:2048,:,3]=a
png(OUT/'T_Lived_OutGrime_BC.png',B)
print('outside ok')
