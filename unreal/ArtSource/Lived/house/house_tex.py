#!/usr/bin/env python3
"""The old house's insides: cracked plaster, old planks, kitchen lino, a rug, a sofa's cloth. Own work; Blender's python with numpy. After Tom's concept pictures."""
import math,struct,sys,zlib
from pathlib import Path
import numpy as np
OUT=Path(__file__).parent/'textures'
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
def normal(h,strength):
    dx=(np.roll(h,-1,1)-np.roll(h,1,1))*strength;dy=(np.roll(h,-1,0)-np.roll(h,1,0))*strength
    l=np.sqrt(dx*dx+dy*dy+1);return np.stack([-dx/l*.5+.5,dy/l*.5+.5 if False else -dy/l*.5+.5,1/l*.5+.5],2)
def cracks(n,count,seed,long=.35):
    r=np.random.default_rng(seed);a=np.zeros((n,n),dtype=np.float32)
    for _ in range(count):
        x,y=r.random(2)*n;ang=r.random()*6.28;L=int(r.uniform(.05,long)*n);w=r.uniform(.4,1.)
        stack=[(x,y,ang,L,w)]
        while stack:
            x,y,ang,L,w=stack.pop()
            for i in range(L):
                xi,yi=int(x)%n,int(y)%n;a[yi,xi]=max(a[yi,xi],w)
                x+=math.cos(ang)*1.3;y+=math.sin(ang)*1.3;ang+=r.normal(0,.18)
                if r.random()<.004 and w>.3:stack.append((x,y,ang+r.choice([-1,1])*r.uniform(.5,1.2),L//2,w*.6))
    return a
n=2048
# Old lime plaster over adobe, 3 m a tile: warm off-white, soft clouds, a few fine hairline cracks, tide marks from old damp, and here and there a patch gone back to the mud.
c=cracks(n,22,1,.22)*.9
c=np.maximum(c,np.roll(c,1,1)*.35)
d=noise(n,120,120,2);d2=noise(n,18,18,3);d3=noise(n,3,3,4);dr=noise(n,10,160,5)
b=.82+.028*d+.012*d2+.006*d3-.02*np.clip(dr-1.2,0,2)
col=np.stack([b*1.0,b*.955,b*.865],2)
# tide marks: soft brown rings where water came and went
tide=noise(n,90,90,7)+.25*noise(n,12,12,9);ring=np.clip((tide-1.05)*1.2,0,1)*.16+np.exp(-((tide-1.05)/.05)**2)*.07
col=col*(1-ring[...,None]*np.array([.18,.24,.36]))
col=col*(1-(c*.30)[...,None])
# a few patches gone back to the adobe, with a lighter rim of broken lime
pm=noise(n,70,45,6);patch=pm*0;rim=pm*0  # bare patches are placed in the model, low and by the frames, not tiled
col=col*(1-patch[...,None])+np.array([.47,.36,.26])*(1+.08*d2[...,None])*patch[...,None]
col=col*(1+.10*rim[...,None])
png(OUT/'T_House_Plaster_BC.png',col)
png(OUT/'T_House_Plaster_R.png',np.clip(.88+.03*d2+.06*patch,0,1))
h=d2*.12+d3*.06-c*.9-patch*1.2+rim*.4
png(OUT/'T_House_Plaster_N.png',normal(h,.8))
# Old planks, 2 m a tile, 8 boards across (boards run in V). Dark, scratched, dull in the lanes.
g=noise(n,1.8,70,7);g2=noise(n,5,200,8);ty=noise(n,40,500,9)
yy,xx=np.mgrid[0:n,0:n];board=(xx*8//n);u=(xx*8/n)%1
tone=np.random.default_rng(10).normal(0,.09,8)[board]
offs=np.random.default_rng(11).integers(0,n,8)[board]
joint=np.clip(np.minimum(u,1-u)*n/8/2.2,0,1)
ends=((yy+offs)%(n*3//4))
endj=np.clip(ends/2.0,0,1)
b=.205*(1+tone)*(1+.16*g+.10*g2+.05*ty)
col=np.stack([b*1.12,b*.90,b*.68],2)*(joint*endj)[...,None]**1.1
png(OUT/'T_House_Plank_BC.png',col)
png(OUT/'T_House_Plank_R.png',np.clip(.62+.08*g+.12*noise(n,60,60,12),.3,1))
h=g*.25+g2*.1;h=h*joint-(1-joint)*2.5-(1-endj)*2.5
png(OUT/'T_House_Plank_N.png',normal(h,1.2))
# Dark beam timber, hewn, 2 m a tile.
g=noise(n,3,110,13);k=noise(n,30,30,14)
b=.115*(1+.3*g+.18*k)
png(OUT/'T_House_Beam_BC.png',np.stack([b*1.15,b*.92,b*.72],2))
png(OUT/'T_House_Beam_R.png',np.clip(.8+.08*g,0,1))
png(OUT/'T_House_Beam_N.png',normal(g*.6+k*.3,1.4))
# Linoleum, 1.2 m a tile: 4 by 4 squares in two worn creams and rust, curled and gone at the seams.
s=n//4;cx=(xx//s+yy//s)%2
base=np.stack([np.where(cx==0,.46,.29),np.where(cx==0,.42,.20),np.where(cx==0,.33,.13)],2).astype(np.float32)
d=noise(n,40,40,15);d2=noise(n,9,9,16)
wear=np.clip((d-.15)*.8,0,.75)
base=base*(1-wear[...,None]*.55)+np.array([.26,.22,.17])*(wear[...,None]*.55)
base*=(1+.05*d2)[...,None]
tear=np.clip((noise(n,14,14,17)-2.15)*5,0,1)
base=base*(1-tear[...,None])+np.array([.10,.08,.06])*tear[...,None]
seam=np.clip(np.minimum(np.minimum(xx%s,s-xx%s),np.minimum(yy%s,s-yy%s))/1.5,.25,1)
png(OUT/'T_House_Lino_BC.png',base*seam[...,None])
png(OUT/'T_House_Lino_R.png',np.clip(.46+.25*wear+.3*tear,0,1))
# Rug, one picture: red and brown, borders, medallions; worn to the warp down the middle. 2048.
v,w=(yy+.5)/n*2-1,(xx+.5)/n*2-1
ed=np.maximum(abs(v),abs(w))
rug=np.ones((n,n,3))*np.array([.15,.05,.04])
rug[ed>.86]=[.07,.05,.05];rug[(ed>.70)&(ed<.76)]=[.30,.22,.14];rug[(ed>.78)&(ed<.84)]=[.22,.08,.05]
pat=np.sin(w*44)*np.sin(v*44);mask=(ed>.76)&(ed<.78)
rug[(pat>.1)&(ed<.68)&(np.cos(v*11)*np.cos(w*7)>.15)]=[.26,.17,.10]
r2=np.hypot(v*1.5,w);rug[(abs(r2-.42)<.035)]=[.33,.26,.16];rug[(r2<.2)]=[.08,.09,.12];rug[(r2<.08)]=[.3,.2,.12]
weave=.92+.08*np.sin(xx*2*math.pi/4)*np.sin(yy*2*math.pi/4)
worn=np.clip(np.exp(-(w/.5)**2-(v/.75)**2)*(.55+.3*noise(n,40,40,18))+np.clip(noise(n,6,6,19)-1.5,0,.6),0,.85)
rug=rug*(1-worn[...,None])+np.array([.27,.23,.17])*worn[...,None]
png(OUT/'T_House_Rug_BC.png',rug*weave[...,None])
# Sofa cloth, 1 m a tile: dull red and brown diamonds.
n2=1024;yy,xx=np.mgrid[0:n2,0:n2];d=(np.sin(xx*2*math.pi/128)+np.sin(yy*2*math.pi/128))
col=np.ones((n2,n2,3))*np.array([.15,.055,.05])
col[d>.6]=[.20,.13,.08];col[d<-.8]=[.075,.05,.05];col[abs(d)<.06]=[.25,.20,.12]
wv=.88+.12*np.sin(xx*2*math.pi/5)*np.sin(yy*2*math.pi/5)
png(OUT/'T_House_Sofa_BC.png',col*wv[...,None]*(1+.1*noise(n2,30,30,20))[...,None])
# Where the lime has come away: light adobe with straw in it, a broken pale rim of lime round it. RGBA, 4 shapes in a 2x2 atlas, soft edges.
N2=1024;out=np.zeros((N2,N2,4),dtype=np.float32);yy,xx=np.mgrid[0:512,0:512]
for k in range(4):
    r=np.random.default_rng(60+k);u=(xx+.5)/512*2-1;v=(yy+.5)/512*2-1
    sh=noise(512,45,45,70+k)*.11+noise(512,5,5,80+k)*.035
    d=np.hypot(u*r.uniform(.9,1.3),v*r.uniform(1.,1.5))+sh
    core=np.clip((.55-d)*7,0,1);edge=np.clip((.70-d)*6,0,1)-core
    straw=np.clip((noise(512,1.5,8,90+k)-1.6)*3,0,1)
    adobe=np.stack([.60+.05*sh,.48+.04*sh,.34+.03*sh],2)*(1-.18*straw[...,None])+np.array([.70,.62,.42])*(.18*straw[...,None])
    lime=np.array([.86,.83,.76])
    rgb=adobe*core[...,None]+lime*edge[...,None]
    a=np.clip(core+edge*.75,0,1)
    oy,ox=(k//2)*512,(k%2)*512
    out[oy:oy+512,ox:ox+512,:3]=rgb;out[oy:oy+512,ox:ox+512,3]=a
png(OUT/'T_House_Loss_BC.png',out)
# Dark soil and gravel at the foot of the walls: U along the wall (2 m a tile), V out from it, fading to nothing. RGBA.
n3=1024;yy,xx=np.mgrid[0:n3,0:n3];vv=(yy+.5)/n3
g1=noise(n3,2.2,2.2,95);g2=noise(n3,6,6,96);g3=noise(n3,40,40,97)
stones=np.clip((g1-1.3)*2,0,1)
soil=np.stack([.20+.03*g3,.15+.025*g3,.11+.02*g3],2)
soil=soil*(1-stones[...,None]*.6)+np.array([.46,.40,.33])*(stones[...,None]*.6)
alpha=np.clip((1-vv)**1.4*(.92+.12*g2)+.08*g3*(1-vv),0,1)
png(OUT/'T_House_Soil_BC.png',np.concatenate([soil,alpha[...,None]],2))
print('house textures ok')
