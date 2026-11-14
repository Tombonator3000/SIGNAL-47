#!/usr/bin/env python3
"""Triangles of a web geometry (GLB) : connected pieces with bounds inside a box, or the count inside a box.
  geo_tris.py <geo dir> <geo id> pieces [x0 x1 y0 y1 z0 z1]
  geo_tris.py <geo dir> <geo id> count x0 x1 y0 y1 z0 z1
Read-only. Positions are the geometry's own (node at identity for the merged batches)."""
import json,struct,sys
from pathlib import Path
import numpy as np
M=json.load(open('/home/tom/SIGNAL-47/unreal/Signal47/Content/WebData/manifest.json'))
def load(geodir,i):
    for f in M['geoFiles']:
        if f['from']<=i<f['to']:
            b=(Path(geodir)/f['file']).read_bytes()
            n=struct.unpack('<I',b[12:16])[0];g=json.loads(b[20:20+n]);bin0=20+n+8
            me=g['meshes'][i-f['from']]
            P=[];I=[];off=0
            def acc(k):
                a=g['accessors'][k];bv=g['bufferViews'][a['bufferView']]
                start=bin0+bv.get('byteOffset',0)+a.get('byteOffset',0)
                ct={5126:('f4',4),5123:('u2',2),5125:('u4',4),5121:('u1',1)}[a['componentType']]
                nc={'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4}[a['type']]
                stride=bv.get('byteStride',ct[1]*nc)
                if stride==ct[1]*nc:
                    return np.frombuffer(b,dtype='<'+ct[0],count=a['count']*nc,offset=start).reshape(a['count'],nc)
                raw=np.frombuffer(b,dtype='u1',count=a['count']*stride,offset=start).reshape(a['count'],stride)[:,:ct[1]*nc]
                return np.frombuffer(raw.tobytes(),dtype='<'+ct[0]).reshape(a['count'],nc)
            for pr in me['primitives']:
                p=acc(pr['attributes']['POSITION']).astype(np.float64)
                idx=acc(pr['indices']).astype(np.int64).reshape(-1,3) if 'indices' in pr else np.arange(len(p)).reshape(-1,3)
                P.append(p);I.append(idx+off);off+=len(p)
            return np.concatenate(P),np.concatenate(I)
geodir,gid,mode=sys.argv[1],int(sys.argv[2]),sys.argv[3]
P,I=load(geodir,gid)
box=[float(v) for v in sys.argv[4:10]] if len(sys.argv)>=10 else None
def inside(tri):
    p=P[tri];return bool(((p[:,0]>=box[0]-5e-5)&(p[:,0]<=box[1]+5e-5)&(p[:,1]>=box[2]-5e-5)&(p[:,1]<=box[3]+5e-5)&(p[:,2]>=box[4]-5e-5)&(p[:,2]<=box[5]+5e-5)).all())
if mode=='count':
    print(sum(inside(t) for t in I),'of',len(I))
else:
    # weld by rounded position, union-find over triangles
    key={};w=np.zeros(len(P),dtype=np.int64)
    for k,p in enumerate(np.round(P,4)):
        w[k]=key.setdefault(tuple(p),len(key))
    par=list(range(len(key)))
    def find(a):
        while par[a]!=a:
            par[a]=par[par[a]];a=par[a]
        return a
    for t in I:
        a,b,c=(find(w[t[0]]),find(w[t[1]]),find(w[t[2]]))
        par[b]=a;par[c]=a
    groups={}
    for t in I:
        groups.setdefault(find(w[t[0]]),[]).append(t)
    rows=[]
    for g,ts in groups.items():
        v=P[np.unique(np.array(ts))]
        lo=v.min(0);hi=v.max(0)
        if box and not (lo[0]>=box[0] and hi[0]<=box[1] and lo[1]>=box[2] and hi[1]<=box[3] and lo[2]>=box[4] and hi[2]<=box[5]): continue
        rows.append((lo,hi,len(ts)))
    rows.sort(key=lambda r:(round(r[0][0],1),r[0][2]))
    for lo,hi,n in rows:
        print('x[%.3f,%.3f] y[%.3f,%.3f] z[%.3f,%.3f] tris=%d'%(lo[0],hi[0],lo[1],hi[1],lo[2],hi[2],n))
    print(len(rows),'pieces',sum(r[2] for r in rows),'tris of',len(I))
