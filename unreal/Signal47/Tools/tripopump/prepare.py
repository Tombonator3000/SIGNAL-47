"""Prepare the private Caliche pump and a REGULAR-only station cut.
Uses existing Tripo fleet functions without modifying fleet/Jeep scripts.
No Blender is imported in --validate-only. Never writes source files.
"""
from pathlib import Path
import argparse,importlib.util,json,sys,hashlib,shutil
HERE=Path(__file__).resolve().parent
REPO=HERE.parents[3]
PUMP_ID='caliche_regular_pump'
def module(name,path):
 spec=importlib.util.spec_from_file_location(name,path);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m
F=module('s47_pump_fleet_prepare',HERE.parent/'tripofleet/prepare.py')
def pump_destination(entry,out):
 # Fleet preparation intentionally owns its original namespace; this bounded
 # adapter relocates only metadata for our separately-owned pump candidate.
 contract=(out/entry['contract']).resolve();assert contract.is_relative_to(out.resolve())
 data=json.loads(contract.read_text());assert data['id']==PUMP_ID and data['source_sha256']==entry['source_sha256']
 expected='/Game/Licensed/TripoPump/'+PUMP_ID
 for part in data['parts'].values():
  assert part['asset_folder'] in (expected,'/Game/Licensed/TripoFleet/'+PUMP_ID)
  part['asset_folder']=expected
 F.write_json(contract,data)
 return entry
def prepare_station(source,placement,output):
 import bpy,bmesh
 from mathutils import Vector
 source=source.resolve();output=output.resolve();assert source.is_file() and not output.is_relative_to(REPO)
 bpy.ops.wm.open_mainfile(filepath=str(source));original=bpy.data.objects.get('Station');assert original and original.type=='MESH'
 output.mkdir(parents=True,exist_ok=True)
 obj=original.copy();obj.data=original.data.copy();obj.name='SM_CalicheStation_NoRegularPump';bpy.context.scene.collection.objects.link(obj)
 # Original source stores Blender=(townX-pivotX, -(townZ-pivotZ), townY-pivotY).
 pivot=json.loads(placement.read_text())['town']
 lo=(-10.85,4.48,2051.79);hi=(-9.65,6.76,2053.01)
 def town(v):
  v=obj.matrix_world@v;return (v.x+pivot[0],v.z+pivot[1],-v.y+pivot[2])
 before=F.measurements(obj);bm=bmesh.new();bm.from_mesh(obj.data);remove=[]
 for face in bm.faces:
  name=obj.data.materials[face.material_index].name
  if name.startswith(('M_Concrete','M_CurbYellow')):continue
  if all(all(lo[k]<=p[k]<=hi[k] for k in range(3)) for p in (town(v.co) for v in face.verts)):remove.append(face)
 removed=sum(len(face.verts)-2 for face in remove);assert 500<removed<20000,('Unexpected pump cut',removed)
 bmesh.ops.delete(bm,geom=remove,context='FACES');bm.to_mesh(obj.data);bm.free();F.clean_unused(obj)
 after=F.measurements(obj);assert before['triangles']-after['triangles']==removed
 assert before['materials']==after['materials'] and before['uv_layers']==after['uv_layers']
 assert all(abs(a-b)<.0001 for aa,bb in zip(before['bounds_local_m'],after['bounds_local_m']) for a,b in zip(aa,bb)),'Station footprint changed'
 fbx=output/(obj.name+'.fbx');F.export_fbx(obj,fbx)
 bpy.ops.wm.save_as_mainfile(filepath=str(output/'station-cut.blend'))
 data={'version':1,'status':'STATION_CUT_NATIVE_UNVERIFIED','source':str(source),'source_sha256':F.sha(source),'fbx':str(fbx),'name':obj.name,'protectedMaterials':['M_Concrete','M_CurbYellow'],'removedTriangles':removed,'cutTownBounds':[lo,hi],'placementTown':pivot,'before':before,'after':after,'originalAsset':'/Game/Free/Town/gas_station/StaticMeshes/gas_station_Station.gas_station_Station'}
 F.write_json(output/'station-contract.json',data);return data

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--sources',required=True,type=Path);p.add_argument('--stage',choices=['pump','station','all'],default='all');p.add_argument('--validate-only',action='store_true');p.add_argument('--repair-metadata',action='store_true',help='Only relocate existing pump contract destination; no Blender or geometry writes');a=p.parse_args(sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else sys.argv[1:])
 path=a.sources.expanduser().resolve();config=json.loads(path.read_text());out,rows=F.read_sources(path,{PUMP_ID});assert len(rows)==1
 station=Path(config['station_source']).expanduser();placement=Path(config['station_placement']).expanduser();assert station.is_file() and placement.is_file()
 if a.validate_only:print('S47_PUMP_PREFLIGHT',rows[0]['source_sha256'],out);return
 if a.repair_metadata:
  manifest=json.loads((out/'manifest.json').read_text());assert len(manifest['models'])==1 and manifest['models'][0]['id']==PUMP_ID
  assert manifest['models'][0]['source_sha256']==rows[0]['source_sha256']
  pump_destination(manifest['models'][0],out);print('S47_PUMP_METADATA_REPAIRED',out,flush=True);return
 assert shutil.disk_usage(out.parent).free>=102*1024**3,'Keep at least100GiB after preparation; reserve2GiB before starting'
 if a.stage in ('pump','all'):
  entry=pump_destination(F.prepare(rows[0],out/PUMP_ID),out);F.write_json(out/'manifest.json',{'version':1,'coordinates':F.COORDINATES,'models':[entry]})
 if a.stage in ('station','all'):prepare_station(station,placement,out/'station')
 assert shutil.disk_usage(out.parent).free>=100*1024**3,'Disk floor below100GiB after preparation'
 print('S47_PUMP_PREPARED',out,flush=True)
if __name__=='__main__':main()
