"""Leased editor -ExecutePythonScript step: retain CPU geometry for only the three final CalicheHomes phone predecessors.
No geometry/material/LOD replacement. Required for the exact runtime index cut in cook.
"""
import json,os
from pathlib import Path
import unreal
E=unreal.EditorAssetLibrary
S=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
assert S is not None,"Use -ExecutePythonScript in the editor; pythonscript commandlet has no editor subsystem"
report={'status':'RUNNING','method':'Retain CPU buffers on final CalicheHomes composition; first Props tick performs exact 24/24/2 phone-only cuts after all native builders. Geometry, materials and LODs are unchanged by this asset job.','assets':[]}
for name,expected_triangles in (('g999',1356),('g1000',108),('g1001',4)):
 path='/Game/CalicheHomes/Trim/'+name+'.'+name
 mesh=unreal.load_asset(path)
 assert isinstance(mesh,unreal.StaticMesh),path
 before=[mesh.get_num_triangles(i)for i in range(S.get_lod_count(mesh))]
 assert before==[expected_triangles],f'{path}: expected verified Trim LOD0 {[expected_triangles]}, got {before}'
 slots=[str(v.material_slot_name)for v in mesh.get_editor_property('static_materials')]
 mesh.set_editor_property('allow_cpu_access',True)
 assert E.save_loaded_asset(mesh,only_if_is_dirty=False),path
 assert mesh.get_editor_property('allow_cpu_access') and before==[mesh.get_num_triangles(i)for i in range(S.get_lod_count(mesh))]
 assert slots==[str(v.material_slot_name)for v in mesh.get_editor_property('static_materials')]
 report['assets'].append({'asset':path,'allowCpuAccess':True,'unchangedTriangles':before,'unchangedSlots':slots})
report['status']='PASS_CPU_RETENTION_ONLY'
output=Path(os.environ.get('S47_PHONE_CPU_REPORT',str(Path(unreal.Paths.project_saved_dir())/'phone-cpu.json')))
output.parent.mkdir(parents=True,exist_ok=True);output.write_text(json.dumps(report,indent=2)+'\n')
unreal.log('S47_PHONE_CPU_READY '+str(output))
