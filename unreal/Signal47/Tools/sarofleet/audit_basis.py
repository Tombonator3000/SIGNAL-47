"""Lightweight source-mesh inspection, not a Blender render. Licensed source stays outside Git."""
import sys,json,struct,math
from pathlib import Path
import numpy as np
from PIL import Image,ImageDraw
FREE=Path(__file__).resolve().parents[1]/'freemodels'
sys.path.insert(0,str(FREE));import gltf_prep as prep

def meshes(path):
 g=json.loads(path.read_text());buffers=[(path.parent/b['uri']).read_bytes() for b in g['buffers']]
 def accessor(i):
  a=g['accessors'][i];v=g['bufferViews'][a['bufferView']];types={5121:'u1',5123:'<u2',5125:'<u4',5126:'<f4'};dt=np.dtype(types[a['componentType']]);n={'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4}[a['type']]
  return np.ndarray((a['count'],n),dt,buffers[v.get('buffer',0)],v.get('byteOffset',0)+a.get('byteOffset',0),strides=(v.get('byteStride',dt.itemsize*n),dt.itemsize)).copy()
 out=[]
 def walk(i,parent):
  n=g['nodes'][i];w=parent@np.array(prep.node_matrix(n))
  if 'mesh' in n:
   for p in g['meshes'][n['mesh']]['primitives']:
    pos=accessor(p['attributes']['POSITION']);v=(np.c_[pos,np.ones(len(pos))]@w.T)[:,:3];idx=accessor(p['indices']).ravel().reshape(-1,3)
    out.append((n.get('name',str(i)),v,idx,p.get('material',-1)))
  for c in n.get('children',[]):walk(c,w)
 for i in g['scenes'][g.get('scene',0)]['nodes']:walk(i,np.eye(4))
 return g,out

def main():
 out=Path(sys.argv[1]);out.mkdir(parents=True,exist_ok=True);report={}
 for model in ['pickup_c10_1966','el_alamo_78']:
  path=Path('/home/tom/scratch-bm/Signal47-free/src')/model/('S47_'+model+'.gltf');g,parts=meshes(path)
  vertices=np.concatenate([v for _,v,_,_ in parts]);lo=vertices.min(0);hi=vertices.max(0)
  report[model]={'bounds_m':[lo.tolist(),hi.tolist()],'parts':[{'name':n,'triangles':len(f),'bounds_m':[v.min(0).tolist(),v.max(0).tolist()]}for n,v,f,m in parts]}
  im=Image.new('RGB',(1100,650),(205,210,210));d=ImageDraw.Draw(im);polys=[]
  for n,v,f,m in parts:
   # Prepared Free models: +X nose, +Y up. Orthographic front-left inspection.
   p=np.c_[.82*v[:,0]-.57*v[:,2],.25*v[:,0]+.36*v[:,2]-v[:,1]]*155;p+=np.array([550,440]);depth=.57*v[:,0]+.82*v[:,2]
   col=np.array(g['materials'][m].get('pbrMetallicRoughness',{}).get('baseColorFactor',[.65,.65,.65,1])[:3])*220
   for tri in f:
    a,b,c=v[tri];normal=np.cross(b-a,c-a);normal/=max(np.linalg.norm(normal),1e-12);shade=.45+.55*abs(normal@np.array([.3,.8,.5]));color=tuple((np.clip(col*shade,25,235)).astype(int))
    polys.append((float(depth[tri].mean()),[tuple(x)for x in p[tri]],color))
  for _,poly,col in sorted(polys,key=lambda p:p[0]):d.polygon(poly,fill=col)
  d.text((20,20),model+' - actual glTF mesh, untextured orthographic diagnostic',fill=(20,25,25));im.save(out/(model+'.png'))
 (out/'basis-audit.json').write_text(json.dumps(report,indent=2))
if __name__=='__main__':main()
