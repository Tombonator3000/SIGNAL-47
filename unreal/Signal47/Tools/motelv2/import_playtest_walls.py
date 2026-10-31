"""Root's serial Unreal import of only the repaired motel parts.
S47_MOTEL_WALL_SOURCE, S47_MOTEL_REPORT. Original materials/textures stay untouched.
"""
import json,os,sys,traceback
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent));import native as N
root=Path(os.environ['S47_MOTEL_WALL_SOURCE']);report=N.Report(os.environ['S47_MOTEL_REPORT'],'playtest-wall-import')
try:
 repaired=json.loads((root/'repair.json').read_text());manifest=json.loads((root/'manifest.json').read_text());assembly_path=Path(N.unreal.Paths.project_content_dir())/'MotelV2/assembly.json';assembly=json.loads(assembly_path.read_text())
 for name in repaired['parts']:
  spec=manifest['parts'][name];mesh=N.import_mesh(root/'exports'/(name+'.fbx'),N.DEST,name,report)
  for i,slot in enumerate(mesh.get_editor_property('static_materials')):
   ma=str(slot.get_editor_property('material_slot_name'));mat=N.E.load_asset(N.DEST+'/Materials/'+ma);report.check(name+' retain original material',isinstance(mat,N.unreal.MaterialInterface));mesh.set_material(i,mat)
  for lod in [1,2]:report.check(name+' repaired LOD'+str(lod),N.S.import_lod(mesh,lod,str(root/'exports'/(name+'_LOD'+str(lod)+'.fbx')))==lod)
  N.finish_mesh(mesh,report,N.AUTHORED_SCREEN_SIZES)
  N.E.save_loaded_asset(mesh)
  assembly['parts'][name]['lod_triangles']=[mesh.get_num_triangles(i)for i in range(N.S.get_lod_count(mesh))]
 assembly_path.write_text(json.dumps(assembly,indent=2)+'\n');report.data['status']='PASS';report.flush();N.unreal.log('MOTEL_PLAYTEST_WALL_IMPORT_PASS')
except Exception:report.data.update(status='FAIL',error=traceback.format_exc());report.flush();raise
