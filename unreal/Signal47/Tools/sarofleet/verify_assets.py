"""Fresh-process readback of the complete saved fleet. No reimport or process launch."""
import os,json,traceback
from pathlib import Path
import unreal
W=Path(os.environ['S47_FLEET_WORK']);OUT=Path(os.environ['S47_FLEET_REPORT']);report={'checks':[],'meshes':[]}
def check(name,ok,detail=None):
 report['checks'].append({'name':name,'pass':bool(ok),'detail':detail});assert ok,(name,detail)
try:
 c=json.loads((W/'model_contract.json').read_text());s=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem) or unreal.new_object(unreal.StaticMeshEditorSubsystem)
 for name,row in c['parts'].items():
  path=row['asset_folder']+'/'+name;mesh=unreal.EditorAssetLibrary.load_asset(path);check(name+' saved mesh',isinstance(mesh,unreal.StaticMesh))
  actual=[mesh.get_num_triangles(i)for i in range(s.get_lod_count(mesh))];check(name+' saved LOD counts',actual==[x['triangles']for x in row['lods']],actual)
  lo,hi=row['bounds_local_m'];center=[(lo[0]+hi[0])*50,-(lo[1]+hi[1])*50,(lo[2]+hi[2])*50];size=[(b-a)*100 for a,b in zip(lo,hi)];b=mesh.get_bounds();got=[b.origin.x,b.origin.y,b.origin.z,2*b.box_extent.x,2*b.box_extent.y,2*b.box_extent.z]
  check(name+' saved metric size and pivot',max(abs(a-b)for a,b in zip(got,center+size))<.3,got)
  if name!='SM_SaroPark_LegacyTrim':
   slots=list(mesh.get_editor_property('static_materials'));names=[str(x.material_slot_name)for x in slots];bound=[x.material_interface.get_name()if x.material_interface else''for x in slots]
   check(name+' saved material slots and bindings',names==row['materials'] and bound==names,{'slots':names,'bindings':bound})
  report['meshes'].append({'name':name,'asset':path,'triangles':actual,'bounds_cm':got})
 for row in json.loads((W/'materials.json').read_text()):
  dest=('/Game/SaroFleet'if row['name'].startswith('M_Fleet_')else'/Game/Free/ServiceFleet')+'/Materials';mat=unreal.EditorAssetLibrary.load_asset(dest+'/'+row['name']);check(row['name']+' saved material',isinstance(mat,unreal.Material))
  if row.get('textures',{}).get('normal'):
   tex=unreal.EditorAssetLibrary.load_asset(dest+'/Textures/T_'+row['name']+'_normal');check(row['name']+' actual native normal convention',isinstance(tex,unreal.Texture2D)and tex.get_editor_property('flip_green_channel')==(row.get('normal_format')=='OpenGL'))
 report['status']='PASS';unreal.log('S47 FLEET SAVED ASSETS PASS')
except Exception:
 report['status']='FAIL';report['error']=traceback.format_exc();raise
finally:OUT.write_text(json.dumps(report,indent=2)+'\n')
