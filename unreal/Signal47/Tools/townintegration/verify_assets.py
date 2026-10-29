"""Read saved Unreal meshes against original Blender contract in a fresh process."""
from pathlib import Path
import json,os,traceback
import unreal
R=Path(__file__).resolve().parent.parent/"townbuildings"
OUT=Path(os.environ['S47_TOWN_REPORT'])
report={'checks':[],'meshes':[]}
def check(name,ok,details=None):
 report['checks'].append({'name':name,'pass':bool(ok),'detail':details})
 assert ok,(name,details)
try:
 c=json.loads((R/'model_contract.json').read_text());s=(unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem) or unreal.new_object(unreal.StaticMeshEditorSubsystem))
 for name,p in c['parts'].items():
  mesh=unreal.EditorAssetLibrary.load_asset('/Game/TownBuildings/'+name);check(name+' exists',isinstance(mesh,unreal.StaticMesh))
  count=s.get_lod_count(mesh);wanted=[c['lods'][str(i)][name]['triangles']for i in range(3)];actual=[mesh.get_num_triangles(i)for i in range(count)]
  check(name+' saved LOD triangles',actual==wanted,{'actual':actual,'expected':wanted})
  lo,hi=p['bounds_local_m'];center=[(lo[0]+hi[0])*50,-(lo[1]+hi[1])*50,(lo[2]+hi[2])*50];size=[(b-a)*100 for a,b in zip(lo,hi)]
  b=mesh.get_bounds();nativecenter=[b.origin.x,b.origin.y,b.origin.z];nativesize=[2*b.box_extent.x,2*b.box_extent.y,2*b.box_extent.z]
  check(name+' saved metric bounds/pivot',max(abs(a-b)for a,b in zip(nativecenter+nativesize,center+size))<.25,{'center':nativecenter,'size':nativesize,'expected_center':center,'expected_size':size})
  slots=[str(x.get_editor_property('material_slot_name'))for x in mesh.get_editor_property('static_materials')];bindings=[x.material_interface.get_name()for x in mesh.get_editor_property('static_materials')]
  check(name+' material slots and bindings',set(slots)==set(p['materials'])and slots==bindings,{'slots':slots,'bindings':bindings})
  report['meshes'].append({'name':name,'triangles':actual,'center_cm':nativecenter,'size_cm':nativesize})
 probe=unreal.EditorAssetLibrary.load_asset('/Game/TownBuildings/Verification/axis_probe');b=probe.get_bounds();got=[b.origin.x,b.origin.y,b.origin.z,2*b.box_extent.x,2*b.box_extent.y,2*b.box_extent.z];expected=[200,-400,600,20,40,60]
 check('asymmetric probe saved center and size',max(abs(x-y)for x,y in zip(got,expected))<.15,got)
 rows=json.loads((R/'materials.json').read_text())
 tb2=[v for extra in sorted((R/'tb2').glob('materials_*.json'))for v in json.loads(extra.read_text())]
 for row in tb2:
  mat=unreal.EditorAssetLibrary.load_asset('/Game/TownBuildings/Materials/'+row['name']);check(row['name']+' saved TB2 material',isinstance(mat,unreal.Material))
  check(row['name']+' saved TB2 blend',mat.get_editor_property('blend_mode')==(unreal.BlendMode.BLEND_TRANSLUCENT if float(row.get('alpha',1))<1 else unreal.BlendMode.BLEND_OPAQUE))
 for row in rows+tb2:
  if row.get('opacity_from_base_color'):
   mat=unreal.EditorAssetLibrary.load_asset('/Game/TownBuildings/Materials/'+row['name'])
   check(row['name']+' saved native mask',mat.get_editor_property('blend_mode')==unreal.BlendMode.BLEND_MASKED)
 report['status']='PASS';unreal.log('TOWNBUILDINGS SAVED ASSETS PASS')
except Exception:
 report['status']='FAIL';report['error']=traceback.format_exc();raise
finally:OUT.write_text(json.dumps(report,indent=2)+'\n')
