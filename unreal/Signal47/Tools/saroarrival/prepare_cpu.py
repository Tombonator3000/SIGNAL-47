"""Retain CPU data for the exact legacy concrete mesh before the native sign cut."""
import json
from pathlib import Path
import unreal
path='/Game/Web/Geo/geo_003/StaticMeshes/g435.g435'
mesh=unreal.load_asset(path)
assert isinstance(mesh,unreal.StaticMesh),(path,'missing legacy concrete mesh')
assert mesh.get_num_triangles(0)==240,(path,'expected twenty concrete boxes')
before=bool(mesh.get_editor_property('allow_cpu_access'))
mesh.set_editor_property('allow_cpu_access',True)
assert unreal.EditorAssetLibrary.save_loaded_asset(mesh,only_if_is_dirty=False)
assert mesh.get_editor_property('allow_cpu_access')
report={'status':'PASS','asset':path,'triangles':240,'before':before,'allow_cpu_access':True,'scope':'CPU retention only; geometry/materials unchanged'}
(Path(unreal.Paths.project_saved_dir())/'saro-arrival-cpu.json').write_text(json.dumps(report,indent=2)+'\n')
unreal.log('S47_SARO_ARRIVAL_CPU_PASS '+json.dumps(report))
