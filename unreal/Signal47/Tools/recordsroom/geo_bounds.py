#!/usr/bin/env python3
"""Bounds, triangle count and material of the annex's web geometries (GLB accessors' min/max). Read-only."""
import json,struct,sys
from pathlib import Path
GEO=Path(sys.argv[1])
M=json.load(open('/home/tom/SIGNAL-47/unreal/Signal47/Content/WebData/manifest.json'))
nodes=M['nodes'];mats=M['materials'];tex=M['textures']
def glb_json(p):
    b=p.read_bytes();n=struct.unpack('<I',b[12:16])[0];return json.loads(b[20:20+n])
cache={}
def geo(i):
    for f in M['geoFiles']:
        if f['from']<=i<f['to']:
            if f['file'] not in cache: cache[f['file']]=glb_json(GEO/f['file'])
            g=cache[f['file']];k=i-f['from']
            # one mesh per geo id, in order
            me=g['meshes'][k];acc=g['accessors']
            lo=[1e9]*3;hi=[-1e9]*3;tris=0
            for pr in me['primitives']:
                a=acc[pr['attributes']['POSITION']]
                lo=[min(lo[j],a['min'][j]) for j in range(3)];hi=[max(hi[j],a['max'][j]) for j in range(3)]
                tris+=(acc[pr['indices']]['count'] if 'indices' in pr else a['count'])//3
            return me.get('name',''),lo,hi,tris,len(g['meshes'])
def col(m):
    c=m.get('color',[1,1,1]);return '#%02x%02x%02x'%tuple(int(max(0,min(1,x))**(1/2.2)*255+.5) for x in c[:3])
a,b=int(sys.argv[2]),int(sys.argv[3])
for n in nodes[a:b+1]:
    if 'geo' not in n: continue
    name,lo,hi,tris,cnt=geo(n['geo'])
    t=n['m'][12:15]
    ms=[]
    for mi in n.get('mats',[]):
        m=mats[mi] if 0<=mi<len(mats) else {}
        ms.append('m%d %s%s r%.2f%s'%(mi,col(m),' map='+tex[m['map']]['name'] if m.get('map') is not None and m.get('map',-1)>=0 else '',m.get('roughness',0),' flood' if m.get('flood') else ''))
    print('n%d g%d "%s" x[%.2f,%.2f] y[%.2f,%.2f] z[%.2f,%.2f] tris=%d %s'%(n['i'],n['geo'],n.get('name',''),lo[0]+t[0],hi[0]+t[0],lo[1]+t[1],hi[1]+t[1],lo[2]+t[2],hi[2]+t[2],tris,' | '.join(ms)))
