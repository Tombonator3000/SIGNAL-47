#!/usr/bin/env python3
"""Ground patches in front of the Caliche buildings, as decals: caliche gravel and old asphalt, each with a broken, ragged edge to the sand.
RGBA 1024; U along the frontage, V out from the building (V=0 at the building)."""
import math,struct,sys,zlib
from pathlib import Path
import numpy as np
OUT=Path(__file__).parent/'textures'
def png(path,a):
    a=np.clip(a,0,1);a=(a*255+.5).astype(np.uint8);h,w=a.shape[:2];c=a.shape[2]
    raw=b''.join(b'\0'+a[y].tobytes() for y in range(h))
    def chunk(t,d):return struct.pack('>I',len(d))+t+d+struct.pack('>I',zlib.crc32(t+d)&0xffffffff)
    Path(path).write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',w,h,8,{3:2,4:6}[c],0,0,0))+chunk(b'IDAT',zlib.compress(raw,6))+chunk(b'IEND',b''))
def noise(n,sx,sy,seed):
    r=np.random.default_rng(seed);a=r.standard_normal((n,n))
    fx=np.fft.rfftfreq(n)[None,:];fy=np.fft.fftfreq(n)[:,None]
    o=np.fft.irfft2(np.fft.rfft2(a)*np.exp(-2*math.pi**2*((fx*sx)**2+(fy*sy)**2)),s=(n,n))
    return (o-o.mean())/(o.std()+1e-6)
n=1024;yy,xx=np.mgrid[0:n,0:n];u=(xx+.5)/n;v=(yy+.5)/n
e1=noise(n,40,40,1);e2=noise(n,8,8,2);e3=noise(n,2,2,3)
# edge: the patch fills the middle and breaks up at its sides and its road end; the building end stays solid
edge=np.minimum(np.minimum(u,1-u)*4.0,1.0)*np.minimum((1-v)*3.0,1.0)
mask=np.clip((edge+.12*e1+.05*e2-.30)*5,0,1)
# Gravel: caliche, pale, with stones and two worn wheel tracks out to the road.
stones=np.clip((e3-1.2)*1.5,0,1)
g=np.stack([.54+.035*e1,.49+.03*e1,.41+.025*e1],2)*(1-.18*stones[...,None])+np.array([.80,.76,.68])*(.12*np.clip(e3-1.8,0,1))[...,None]
tracks=np.exp(-((u-.38)/.05)**2)+np.exp(-((u-.62)/.05)**2)
g=g*(1-.10*np.clip(tracks*(.7+.3*e2),0,1))[...,None]
oil=np.clip((noise(n,25,25,4)-2.2)*2,0,1)*np.exp(-((v-.35)/.2)**2);g=g*(1-.35*oil[...,None])
png(OUT/'T_Lived_LotGravel_BC.png',np.concatenate([g,(mask*.92)[...,None]],2))
# Asphalt: old and sun-faded, a warm grey; aggregate showing; cracks filled with tar, two square patches of newer, darker blacktop.
agg=np.clip((e3-.8)*.6,0,1)
a=np.stack([.37+.02*e1,.35+.02*e1,.32+.018*e1],2)*(1-.10*agg[...,None])+np.array([.48,.45,.40])*(.08*np.clip(e3-1.6,0,1))[...,None]
cr=np.zeros((n,n),dtype=np.float32);r=np.random.default_rng(5)
for _ in range(45):
    x,y=r.random(2)*n;ang=r.random()*6.28
    for i in range(int(r.uniform(60,260))):
        xi,yi=int(x)%n,int(y)%n;cr[yi,xi]=1;cr[(yi+1)%n,xi]=1;cr[yi,(xi+1)%n]=1;x+=math.cos(ang)*1.4;y+=math.sin(ang)*1.4;ang+=r.normal(0,.22)
seal=np.maximum(cr,np.roll(cr,2,1)*.6);seal=np.maximum(seal,np.roll(seal,2,0)*.6)
a=a*(1-seal[...,None]*.55)+np.array([.10,.09,.08])*(seal[...,None]*.55)
for (x0,y0,x1,y1) in((.18,.25,.42,.48),(.58,.55,.86,.80)):
    m=(u>x0)&(u<x1)&(v>y0)&(v<y1);a[m]=a[m]*.62+np.array([.11,.105,.10])*.38
edgeA=np.clip((edge+.15*e1+.06*e2-.28)*5,0,1)
a=a*edgeA[...,None]+np.array([.50,.46,.39])*(1-edgeA[...,None])
png(OUT/'T_Lived_LotAsphalt_BC.png',np.concatenate([a,(mask*.95)[...,None]],2))
print('town ok')
