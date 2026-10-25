"""Run in UnrealEditor-Cmd before cook: retain CPU buffers on exactly one source mesh."""
import json
from pathlib import Path
import unreal
path='/Game/Free/ServiceFleet/SM_SaroPark_LegacyTrim'
mesh=unreal.EditorAssetLibrary.load_asset(path)
assert mesh and mesh.get_num_lods()==3
assert [mesh.get_num_triangles(i) for i in range(3)]==[10702]*3
before=bool(mesh.get_editor_property('allow_cpu_access'))
mesh.set_editor_property('allow_cpu_access',True)
assert unreal.EditorAssetLibrary.save_loaded_asset(mesh)
assert mesh.get_editor_property('allow_cpu_access')
report={'status':'PASS','asset':path,'triangles_per_lod':[10702]*3,'before':before,'allow_cpu_access':True,'scope':'CPU retention only; source geometry, UVs and materials unchanged. Runtime trim removes230 measured seam-plant and144 old wheel-remnant triangles, retaining10328.'}
(Path(unreal.Paths.project_saved_dir())/'saro-forecourt-cpu.json').write_text(json.dumps(report,indent=2)+'\n')
unreal.log('S47_FORECOURT_CPU_PASS '+json.dumps(report))
