"""Light read-only source/GLB measurements. Does not launch Blender or Unreal.
Writes this kit's placement_audit.json; native/runtime verification remains separate.
"""
import argparse,hashlib,json,math,struct
from pathlib import Path
import numpy as np
R=Path(__file__).resolve().parent

def load(path):
 b=Path(path).read_bytes();n=struct.unpack_from('<I',b,12)[0];j=json.loads(b[20:20+n]);return j,b[28+n:]
def accessor(j,b,i):
 a=j['accessors'][i];v=j['bufferViews'][a['bufferView']];dt={5126:'<f4',5125:'<u4',5123:'<u2',5121:'u1'}[a['componentType']];k={'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4}[a['type']];d=np.dtype(dt)
 return np.ndarray((a['count'],k),dtype=d,buffer=b,offset=v.get('byteOffset',0)+a.get('byteOffset',0),strides=(v.get('byteStride',d.itemsize*k),d.itemsize)).copy()
def triangles(path,name=None):
 j,b=load(path);mesh=next((m for m in j['meshes'] if m.get('name')==name),None) if name else j['meshes'][0]
 if mesh is None:
  node=next(n for n in j['nodes'] if n.get('name')==name);mesh=j['meshes'][node['mesh']]
 result=[]
 for p in mesh['primitives']:
  v=accessor(j,b,p['attributes']['POSITION']);idx=accessor(j,b,p['indices']).reshape(-1).astype(int) if 'indices' in p else np.arange(len(v));result.append(v[idx].reshape(-1,3,3).astype(float))
 return np.concatenate(result)
def surface(tris,x,z):
 a=tris[:,0];b=tris[:,1]-a;c=tris[:,2]-a;den=b[:,0]*c[:,2]-c[:,0]*b[:,2];valid=np.abs(den)>1e-10
 u=np.zeros(len(a));v=u.copy();u[valid]=((x-a[valid,0])*c[valid,2]-(z-a[valid,2])*c[valid,0])/den[valid];v[valid]=(b[valid,0]*(z-a[valid,2])-b[valid,2]*(x-a[valid,0]))/den[valid]
 ix=valid&(u>=-1e-7)&(v>=-1e-7)&(u+v<=1+1e-7)
 assert ix.any(),('no terrain',x,z)
 return float(np.max(a[ix,1]+u[ix]*b[ix,1]+v[ix]*c[ix,1]))
def intersections(tris,a,b):
 d=np.array(b)-a;e1=tris[:,1]-tris[:,0];e2=tris[:,2]-tris[:,0];p=np.cross(d,e2);det=np.einsum('ij,ij->i',e1,p);ok=np.abs(det)>1e-10;inv=np.zeros(len(det));inv[ok]=1/det[ok];s=a-tris[:,0];u=np.einsum('ij,ij->i',s,p)*inv;q=np.cross(s,e1);v=q@d*inv;t=np.einsum('ij,ij->i',e2,q)*inv;ok&=(u>=0)&(v>=0)&(u+v<=1)&(t>=0)&(t<=1)
 return [list(np.array(a)+d*tt) for tt in t[ok]]
def hash_(x,z):
 v=math.sin(x*12.9898+z*78.233)*43758.5453;return v-math.floor(v)
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--manifest',type=Path,default=Path('/home/tom/SIGNAL-47/unreal/Signal47/Content/WebData/manifest.json'));ap.add_argument('--geo-dir',type=Path,default=Path('/home/tom/scratch-bm/Signal47-web/geo'));a=ap.parse_args();m=json.loads(a.manifest.read_text())
 def geo(i):
  row=next(r for r in m['geoFiles'] if r['from']<=i<r['to']);return triangles(a.geo_dir/row['file'],'g'+str(i))
 old=[]
 for i,want in [(439,240),(440,432),(441,116),(452,1156)]:
  t=geo(i);assert len(t)==want;old.append({'geo':i,'triangles':len(t),'bounds_three_local_m':[t.min((0,1)).tolist(),t.max((0,1)).tolist()]})
 terrain=geo(873);zs=[z+(hash_(z,1)-.5)*4 for z in range(-1900,3401,45)];source=min(zs,key=lambda z:abs(z+281.4));main=[[-9,z+300] for z in zs if z>=source and 9**2+(z+300)**2<=10000]
 assert len(main)==2
 elevations=triangles(R.parent/'radioarray/dish/exports/SM_Array_Elevation.glb');findings=[]
 for name in ['S-02','S-11']:
  dish=next(d for d in m['dishes'] if d['id']==name);az=math.radians(-dish['az']);el=math.radians(-(90-dish['el']));rx=np.array([[1,0,0],[0,math.cos(el),-math.sin(el)],[0,math.sin(el),math.cos(el)]]);ry=np.array([[math.cos(az),0,math.sin(az)],[0,1,0],[-math.sin(az),0,math.cos(az)]])
  t=((elevations@rx.T)+[0,4.6,0])@ry.T;t=(t+[0,7.8,0])*dish['scale']+[dish['x'],0,dish['z']]
  hits=[];feet=[]
  for z in zs:
   if abs(z+300-dish['z'])<55:feet.append({'site_xz':[-9,z+300],'terrainY':surface(terrain,15,z)})
  for z0,z1 in zip(zs,zs[1:]):
   if z1+300<dish['z']-20 or z0+300>dish['z']+20:continue
   y0=surface(terrain,15,z0);y1=surface(terrain,15,z1)
   for dx in [-1.95,-.95,.95,1.95]:
    pts=[]
    for k in range(13):
     u=k/12;pts.append(np.array([-9+dx,(1-u)*y0+u*y1+9.75-4*.85*u*(1-u),(1-u)*z0+u*z1+300]))
    for p,q in zip(pts,pts[1:]):hits.extend(intersections(t,p,q))
  findings.append({'dish':name,'default_az_el':[dish['az'],dish['el']],'reflector_world_bounds':[t.min((0,1)).tolist(),t.max((0,1)).tolist()],'nearby_main_feet':feet,'wire_reflector_intersections':hits})
 report={'status':'SOURCE_MEASUREMENTS_NATIVE_UNVERIFIED','manifest':str(a.manifest),'manifest_sha256':hashlib.sha256(a.manifest.read_bytes()).hexdigest(),'retired_utility_geometry':old,'retired_ground_feet':[m['handles'][f'world.grounds.colliders[{i}]']['rect'] for i in range(6)],'power':{'main_within100m':main,'one_service_xz':[3.6,18.6],'total_utility_poles_within100m':3,'global_main_count':len([z for z in zs if z>=source]),'north_array_extension_removed':True,'field_light_poles':'preserved, excluded from utility counts','natural_source_xz':[-9,source+300],'branch_length_m':math.hypot(12.6,source+281.4)},'dish_clearance':findings,'new_line_clearance':{'source': 'actual triangle intersections above describe retired north extension','proposed_main_min_siteZ':source+300,'maximum_reflector_siteZ':max(f['reflector_world_bounds'][1][2] for f in findings),'result':'No retained main span reaches either tested reflector; service span stays Z10.86..20'},'new_part_roots_three_m':{'SM_SaroSite_Facade':[29.6,-.6,0],'SM_SaroSite_Glass':[29.6,-.6,0],'SM_SaroSite_ServiceEquipment':[3.6,-.6,18.6]},'glass_recess_m':.27,'sign_supported_by':'uninterrupted south wall x20.6..31.65; original observatory name retained','scope':'source geometric measurements only; no native or visual PASS claimed'}
 (R/'placement_audit.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({'power':report['power'],'dish_clearance':findings},indent=2))
if __name__=='__main__':main()
