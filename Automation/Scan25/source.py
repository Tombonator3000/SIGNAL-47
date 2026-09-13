#!/usr/bin/env python3
"""Preserve and spatially crop a trained CC BY rock; never resample its Gaussians."""
import argparse, hashlib, json, math, struct, urllib.request
from pathlib import Path
import numpy as np

REVISION = 'f8c5eeae5ff5410dc537a27762c8927ba2fd29a2'
BASE = 'https://huggingface.co/datasets/loopce/gaussiansplats/resolve/' + REVISION + '/'
FILES = {
 'point_cloud.ply': ('rock/model/point_cloud/iteration_30000/point_cloud.ply',1820132556,'7cc0092ef49843f6c6bbcd53289f9f29cf69bb10b8480f0b3e7a23ed6a18a359'),
 '0001.png': ('rock/images/0001.png',4981722,'73af76687516f12312207ffeaf325e8e2c7ac7f96968eba35762c95df764f77c'),
 'cameras.bin': ('rock/sparse/0/cameras.bin',64,'e33a406ce0411a6175fb42cd45a62bfbc8997efe211d78154f9ed62e491f103f'),
 'images.bin': ('rock/sparse/0/images.bin',123262487,'e02f64b5db4fc7d1aea9e15b460a9a271f19786c71106c67cc663813ef68b2ee')
}

def digest(p):
 with p.open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()

def rotation(q):
 w,x,y,z=q
 return np.array([[1-2*y*y-2*z*z,2*x*y-2*z*w,2*x*z+2*y*w],
 [2*x*y+2*z*w,1-2*x*x-2*z*z,2*y*z-2*x*w],
 [2*x*z-2*y*w,2*y*z+2*x*w,1-2*x*x-2*y*y]])

def poses(path):
 result=[]
 with path.open('rb') as f:
  n=struct.unpack('<Q',f.read(8))[0]
  if not 1<=n<10000:raise ValueError('Unexpected camera count')
  for _ in range(n):
   a=struct.unpack('<idddddddi',f.read(64));name=b''
   while (c:=f.read(1))!=b'\0':
    if not c or len(name)>4096:raise ValueError('Invalid image name')
    name+=c
   count=struct.unpack('<Q',f.read(8))[0];f.seek(count*24,1)
   result.append({'name':name.decode(),'rotation':rotation(a[1:5]),'t':np.array(a[5:8])})
 return result

def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--source',required=True,type=Path);p.add_argument('--out',required=True,type=Path)
 p.add_argument('--download',action='store_true')
 p.add_argument('--min',nargs=3,type=float,default=[-1.85,-2.50,-.48])
 p.add_argument('--max',nargs=3,type=float,default=[1.0,-1.18,1.6])
 p.add_argument('--max-scale',type=float,default=.08,help='Remove broad stray Gaussians; retained rows remain unchanged')
 a=p.parse_args();a.source.mkdir(parents=True,exist_ok=True)
 if a.out.exists():raise SystemExit('Use a fresh crop directory')
 for name,(path,size,sha) in FILES.items():
  dst=a.source/name
  if not dst.exists() and a.download:
   part=dst.with_suffix(dst.suffix+'.partial')
   with urllib.request.urlopen(BASE+path,timeout=30) as response,part.open('xb') as f:
    written=0
    while block:=response.read(1048576):
     written+=len(block)
     if written>size:raise ValueError('Unexpected source length')
     f.write(block)
   part.rename(dst)
  if not dst.is_file() or dst.stat().st_size!=size or digest(dst)!=sha:raise SystemExit('Missing or mismatched source: '+name)
 registered=poses(a.source/'images.bin');first=next(p for p in registered if p['name']=='0001.png')
 up=np.mean([-p['rotation'][1] for p in registered],axis=0);up/=np.linalg.norm(up)
 forward=first['rotation'][2]-up*np.dot(first['rotation'][2],up);forward/=np.linalg.norm(forward)
 basis=np.stack([np.cross(forward,up),up,forward])
 assert np.allclose(basis@basis.T,np.eye(3),atol=1e-8) and np.linalg.det(basis)<0
 src=a.source/'point_cloud.ply';header=[]
 with src.open('rb') as f:
  while (line:=f.readline())!=b'end_header\n':
   if not line or f.tell()>65536:raise ValueError('Bad PLY header')
   header.append(line.decode('ascii').rstrip())
  offset=f.tell()
 count=int(next(s.split()[2] for s in header if s.startswith('element vertex ')))
 names=[s.split()[2] for s in header if s.startswith('property float ')]
 if count!=7339238 or len(names)!=62:raise ValueError('Unexpected original trained PLY schema')
 data=np.memmap(src,dtype='<f4',mode='r',offset=offset,shape=(count,len(names)))
 lo,hi=np.array(a.min),np.array(a.max);selected=[];bounds=[np.full(3,np.inf),np.full(3,-np.inf)]
 # Small blocks keep the original 1.82GB mapping from becoming a resident allocation.
 for start in range(0,count,50000):
  block=data[start:start+50000];local=block[:,:3]@basis.T
  mask=np.all((local>=lo)&(local<=hi),axis=1)&np.all(block[:,55:58]<=math.log(a.max_scale),axis=1)
  ids=np.flatnonzero(mask)+start;selected.append(ids)
  if len(ids):
   if not np.isfinite(block[mask]).all():raise ValueError('Nonfinite selected splat')
   bounds[0]=np.minimum(bounds[0],local[mask].min(axis=0));bounds[1]=np.maximum(bounds[1],local[mask].max(axis=0))
 ids=np.concatenate(selected)
 if not 1000<len(ids)<=800000:raise ValueError('Crop count outside 1k..800k budget: '+str(len(ids)))
 a.out.mkdir(parents=True);dst=a.out/'rock-trained.ply'
 text='\n'.join('element vertex '+str(len(ids)) if s.startswith('element vertex ') else s for s in header)+'\nend_header\n'
 with dst.open('wb') as f:
  f.write(text.encode('ascii'))
  for start in range(0,len(ids),20000):f.write(data[ids[start:start+20000]].tobytes())
 # Exporting selected original rows preserves all SH3/covariance/opacity parameters byte-for-byte.
 np.save(a.out/'source-row-indices.npy',ids)
 center=np.array([(lo[0]+hi[0])/2,lo[1],(lo[2]+hi[2])/2])
 views=[{'name':'front','position':[0,1.5,-3.7],'target':[0,.7,0]},
        {'name':'oblique','position':[3.4,1.8,-2.1],'target':[0,.7,0]},
        {'name':'detail','position':[-.3,1.25,-1.75],'target':[-.3,.7,0]}]
 pc=np.diag([-1.,-1.,1.])
 for v in views:
  v['playcanvasPosition']=(pc@basis.T@(np.array(v['position'])+center)).tolist()
  v['playcanvasTarget']=(pc@basis.T@(np.array(v['target'])+center)).tolist()
  v['playcanvasUp']=(pc@basis[1]).tolist()
 report={'author':'Loop CE','title':'rock','license':'CC BY 4.0','license_url':'https://creativecommons.org/licenses/by/4.0/',
 'asset_page':'https://loopce.com/gaussian-splats','revision':REVISION,'files':{n:{'url':BASE+path,'bytes':size,'sha256':sha} for n,(path,size,sha) in FILES.items()},
 'registered_cameras':len(registered),'source_count':count,'count':len(ids),'source_columns':names,'output_sha256':digest(dst),
 'selection':'spatial crop and maximum Gaussian scale filter; retained rows unchanged without decimation, color edits or retraining',
 'max_scale':a.max_scale,
 'min':lo.tolist(),'max':hi.tolist(),'actualBounds':[b.tolist() for b in bounds],'worldToLocalRUF':basis.tolist(),'center':center.tolist(),
 'scale_note':'source scale is not metrically certified; one source unit is provisionally one Unity metre for the probe',
 'orientation_note':'up inferred from registered camera orientations; visually inspect floor contact','views':views}
 (a.out/'crop.json').write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps({'count':len(ids),'bounds':report['actualBounds'],'sha256':report['output_sha256']}))

if __name__=='__main__':main()
