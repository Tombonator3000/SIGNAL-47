"""Editor-only preparation, invoked by parent's strict guarded editor slot.
Retains CPU copies of exactly the two shared legacy wire meshes for a runtime
index-only clone. Does not reimport or modify geometry/materials/other assets.
"""
import json
from pathlib import Path
import unreal
rows=[]
for geo,tris,folder in [(684,7776,'geo_005'),(918,46992,'geo_007')]:
 path=f'/Game/Web/Geo/{folder}/StaticMeshes/g{geo}.g{geo}'
 mesh=unreal.load_asset(path)
 assert isinstance(mesh,unreal.StaticMesh),(path,'missing imported legacy wire mesh')
 actual=mesh.get_num_triangles(0)
 assert actual==tris,(path,actual,tris)
 before=bool(mesh.get_editor_property('allow_cpu_access'))
 mesh.set_editor_property('allow_cpu_access',True)
 assert unreal.EditorAssetLibrary.save_loaded_asset(mesh,only_if_is_dirty=False)
 assert mesh.get_editor_property('allow_cpu_access')
 rows.append({'asset':path,'triangles':actual,'before':before,'allow_cpu_access':True})
report={'status':'PASS','scope':'CPU data preservation only; no geometry or material modification','assets':rows}
out=Path(unreal.Paths.project_saved_dir())/'highway-power-cpu.json';out.write_text(json.dumps(report,indent=2)+'\n')
unreal.log('S47_HIGHWAY_POWER_CPU_PASS '+json.dumps(report))
