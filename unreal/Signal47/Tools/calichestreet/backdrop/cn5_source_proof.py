"""CN5 actual export/road-mask/ray proof. Read-only geometry, no DCC/UE."""
from pathlib import Path
import json,struct,math,sys,ast,importlib.util,hashlib,datetime
import numpy as np
R=Path(__file__).resolve().parent;sys.path.insert(0,str(R));import geology as g
OLD=Path('/home/tom/scratch-bm/Signal47-caliche-nature/evidence/CN4-rejected-oldroad-cap-20261024T084318Z')
spec=importlib.util.spec_from_file_location('cn4_preserved',OLD/'geology.py');old=importlib.util.module_from_spec(spec);spec.loader.exec_module(old)
def function_ast(path,name):
    tree=ast.parse(path.read_text());return ast.dump(next(n for n in tree.body if isinstance(n,ast.FunctionDef) and n.name==name))
for name in ['protection','old_road_distance','old_road']:
    assert function_ast(R/'geology.py',name)==function_ast(OLD/'geology.py',name),name
assert g.BELTS==old.BELTS and g.ANCHOR==old.ANCHOR and g.MATERIALS==old.MATERIALS
changed=0;unchanged=0;minimum=math.inf;maximum=-math.inf;distances=[]
for b,ob in zip(g.BELTS,old.BELTS):
    profile=g.split_profile(b['profile']);n=math.ceil((b['along'][1]-b['along'][0])/b['step'])
    for i in range(n+1):
        s=b['along'][0]+(b['along'][1]-b['along'][0])*i/n
        for setback,fraction,material in profile:
            x,z,h=g.surface_point(b,s,setback,fraction);ox,oz,oh=old.surface_point(ob,s,setback,fraction)
            assert x==ox and z==oz,'XY geometry changed'
            delta=h-oh
            if abs(delta)>1e-10:
                d=g.old_road_distance(x,z);assert b['id']=='eastern_north_escarpment' and 286<d<435 and fraction>.48,(b['id'],d,fraction)
                changed+=1;minimum=min(minimum,delta);maximum=max(maximum,delta);distances.append(d)
            else:unchanged+=1
assert changed>0
for p in (R/'textures').glob('*.png'):assert p.read_bytes()==(OLD/'textures'/p.name).read_bytes(),p.name

def load_glb(path):
    raw=path.read_bytes();size=struct.unpack_from('<I',raw,12)[0];doc=json.loads(raw[20:20+size]);binary=raw[28+size:]
    def accessor(i):
        a=doc['accessors'][i];v=doc['bufferViews'][a['bufferView']];dtype=np.dtype({5126:'<f4',5125:'<u4',5123:'<u2'}[a['componentType']]);n={'VEC3':3,'SCALAR':1}[a['type']]
        return np.ndarray((a['count'],n),dtype=dtype,buffer=binary,offset=v.get('byteOffset',0)+a.get('byteOffset',0),strides=(v.get('byteStride',dtype.itemsize*n),dtype.itemsize)).copy()
    triangles=[];materials=[];counts={}
    for mesh in doc['meshes']:
        for p in mesh['primitives']:
            vs=accessor(p['attributes']['POSITION']).astype(float);indices=accessor(p['indices']).ravel().reshape(-1,3);triangles.append(vs[indices]);name=doc['materials'][p['material']]['name'];materials.extend([name]*len(indices));counts[name]=counts.get(name,0)+len(indices)
    return np.concatenate(triangles),materials,counts
current,materials,newcounts=load_glb(R/'exports/SM_Caliche_Mesas.glb');previous,oldmats,oldcounts=load_glb(OLD/'exports/SM_Caliche_Mesas.glb')
assert len(current)==len(previous)==103550 and sorted(newcounts)==g.MATERIALS
# Same source-backed diagnostic camera used for the actual CN4 raymatch.
# Final native eye is not telemetered; this proves export correspondence,
# not exact native raster coverage or acceptance.
origin=np.array([5.2,14.306860317421958,250]);yaw=.02;pitch=-.08;right=np.array([math.cos(yaw),0,-math.sin(yaw)]);forward=np.array([-math.sin(yaw)*math.cos(pitch),math.sin(pitch),-math.cos(yaw)*math.cos(pitch)]);up=np.cross(right,forward);tan=math.tan(math.radians(35));aspect=2560/1440
def hit(tri,mats,pixel):
    px,py=pixel;direction=forward+right*((2*px/2560-1)*aspect*tan)+up*((1-2*py/1440)*tan);direction/=np.linalg.norm(direction)
    e1=tri[:,1]-tri[:,0];e2=tri[:,2]-tri[:,0];pvec=np.cross(np.broadcast_to(direction,e2.shape),e2);det=np.einsum('ij,ij->i',e1,pvec);valid=np.abs(det)>1e-9;inv=np.divide(1,det,out=np.zeros_like(det),where=valid);tvec=origin-tri[:,0];u=np.einsum('ij,ij->i',tvec,pvec)*inv;q=np.cross(tvec,e1);v=np.einsum('j,ij->i',direction,q)*inv;t=np.einsum('ij,ij->i',e2,q)*inv;valid&=(u>=0)&(v>=0)&(u+v<=1)&(t>0);t=np.where(valid,t,np.inf);idx=int(np.argmin(t));assert np.isfinite(t[idx]),pixel
    p=origin+direction*t[idx];normal=np.cross(e1[idx],e2[idx]);normal/=np.linalg.norm(normal)
    return {'material':mats[idx],'localThree_m':p.tolist(),'range_m':float(t[idx]),'triangle_normal_three':normal.tolist(),'oldRoadDistance_m':g.old_road_distance(float(p[0]),float(p[2]))}
rays=[]
for pixel in [[2450,590],[2500,625],[2390,575],[2430,520]]:
    before=hit(previous,oldmats,pixel);after=hit(current,materials,pixel);rays.append({'pixel_2560x1440':pixel,'CN4':before,'CN5':after})
for ray in rays[:2]:assert ray['CN4']['material']=='M_Caliche_Mesa_2' and ray['CN5']['material']!='M_Caliche_Mesa_2',ray
report={'status':'CN5_SOURCE_AND_EXPORTED_TARGET_PROOF_PASS','utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'preserved_CN4':str(OLD),'native_visual_status':'UNVERIFIED','unchanged':{'road_mask_functions_AST_identical':True,'all_source_XY_exactly_identical':True,'belts_anchor_six_names':True,'all_nine_PNGs_byte_identical':True,'triangles':103550},'localized_relief':{'changed_source_points':changed,'unchanged_source_points':unchanged,'height_delta_minmax_m':[minimum,maximum],'all_changes_in_east_OldRoad_distance_m':[min(distances),max(distances)],'corridor_m':180,'road_protection_function':'unchanged;0 through280m,1 at420m; added relief only286..435m and fraction>.48'},'actual_GLB_material_triangle_counts':{'CN4':oldcounts,'CN5':newcounts},'actual_GLB_target_raymatches':rays,'camera':{'localThree_eye_m':origin.tolist(),'yaw':yaw,'pitch':pitch,'vertical_fov':70,'resolution':[2560,1440],'limit':'Source floor and requested camera; historical native package has no final-eye telemetry. No native CN5 capture or acceptance claimed.'},'actual_independent_verification':json.loads((R/'verification.json').read_text()),'source_SHA256':{n:hashlib.sha256((R/n).read_bytes()).hexdigest() for n in ['geology.py','build_backdrop.py','cn5_source_proof.py']}}
(R/'cn5_source_proof.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({k:report[k] for k in ['status','unchanged','localized_relief','actual_GLB_material_triangle_counts','actual_GLB_target_raymatches']},indent=2))
