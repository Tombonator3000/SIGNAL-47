"""Explicit editor job: import only relocated ground g4/g663/g456, retain materials.

S47_MOTEL_GROUND_SOURCE points to prepared+converted private trim packet.
S47_MOTEL_REPORT is a private JSON report; no authored motel mesh is imported.
"""
import json
import os
from pathlib import Path
import sys
import traceback
sys.path.insert(0,str(Path(__file__).resolve().parent))
import native as N
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'calicheaccess'))
from lod_geometry import verify_lod

root=Path(os.environ['S47_MOTEL_GROUND_SOURCE']).resolve()
report=N.Report(os.environ['S47_MOTEL_REPORT'],'rear-pool-ground-import')
flag='Interchange.FeatureFlags.Import.FBX'
previous=N.unreal.SystemLibrary.get_console_variable_int_value(flag)
try:
    N.unreal.SystemLibrary.execute_console_command(None,flag+' 0')
    data=json.loads((root/'trim-manifest.json').read_text())
    converted=json.loads((root/'trim-fbx.json').read_text())
    report.check('independent serialized excavation verified',data['ground_hole_verification']['status']=='PASS')
    report.check('converted current packet',converted['status']=='PASS' and converted['trim_manifest_sha256']==N.sha(root/'trim-manifest.json'))
    assembly_path=Path(N.unreal.Paths.project_content_dir())/'MotelV2/assembly.json'
    assembly=json.loads(assembly_path.read_text())
    report.check('exact ground scope',set(data['meshes'])=={'g4','g456','g663'})
    for name,row in data['meshes'].items():
        mesh=N.import_mesh(root/row['fbx'],N.DEST+'/Trim',name,report)
        fallback=N.E.load_asset('/Engine/EngineMaterials/DefaultMaterial')
        for i in range(len(mesh.get_editor_property('static_materials'))):mesh.set_material(i,fallback)
        N.finish_mesh(mesh,report)
        report.check('native source material slots retained',len(mesh.get_editor_property('static_materials'))==len(row['original_material_indices']))
        geometry=verify_lod(mesh,0,root/row['fbx'],row['result_tris'],exact_render_sections=True)
        report.data.setdefault('actual_render_geometry',{})[name]=geometry
        existing=next(p for p in assembly['patches'] if p['geo']==row['geo'])
        for key in ['result_tris','asset','original_material_indices','hidden']:existing[key]=row[key]
    assembly['relocated_ground']={'report':str(root/'trim-manifest.json'),'runtime_layout_sha256':data['runtime_layout_sha256']}
    assembly_path.write_text(json.dumps(assembly,indent=2)+'\n')
    report.data['status']='PASS';report.flush()
    N.unreal.log('MOTEL_RELOCATED_GROUND_NATIVE_PASS')
except Exception:
    report.data.update(status='FAIL',error=traceback.format_exc());report.flush();raise
finally:
    N.unreal.SystemLibrary.execute_console_command(None,flag+' '+str(previous))
