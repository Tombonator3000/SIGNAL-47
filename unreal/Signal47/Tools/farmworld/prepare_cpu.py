"""Retain exactly the SARO road-props CPU indices for the measured Alderete gate split.
Run in one leased editor. No reimport or geometry/material/LOD change.
"""
import hashlib,json
from pathlib import Path
import unreal
def signature(mesh):
    body = mesh.get_editor_property('body_setup')
    return {
        'lod_triangles': [mesh.get_num_triangles(i) for i in range(mesh.get_num_lods())],
        'materials': [
            {'slot': str(slot.material_slot_name),
             'imported_slot': str(slot.get_editor_property('imported_material_slot_name')),
             'material': slot.material_interface.get_path_name() if slot.material_interface else None}
            for slot in mesh.get_editor_property('static_materials')
        ],
        'support_ray_tracing': bool(mesh.get_editor_property('support_ray_tracing')),
        'nanite_enabled': bool(mesh.get_editor_property('nanite_settings').get_editor_property('enabled')),
        'collision_trace_flag': str(body.get_editor_property('collision_trace_flag')) if body else None,
    }

content=Path(unreal.Paths.project_content_dir())
manifest=json.loads((content/'WebData/manifest.json').read_text())
nodes=[n for n in manifest['nodes'] if n.get('geo')==678]
assert len(nodes)==1 and nodes[0]['i']==984 and nodes[0]['p']==976 and nodes[0]['mats']==[426],nodes
asset='/Game/Web/Geo/geo_005/StaticMeshes/g678.g678'
mesh=unreal.load_asset(asset)
assert isinstance(mesh,unreal.StaticMesh),asset
before=signature(mesh)
assert before['lod_triangles']==[1064] and before['support_ray_tracing'] and not before['nanite_enabled'],before
was=bool(mesh.get_editor_property('allow_cpu_access'))
mesh.set_editor_property('allow_cpu_access',True)
assert unreal.EditorAssetLibrary.save_loaded_asset(mesh,only_if_is_dirty=False),asset
assert signature(mesh)==before and mesh.get_editor_property('allow_cpu_access')
report={'status':'PASS_CPU_RETENTION_ONLY','asset':asset,'was_cpu_retained':was,'before':before,'after':signature(mesh),'scope':'Only CPU retention for native exact gate index split. No geometry/material/LOD change.'}
out=Path(unreal.Paths.project_saved_dir())/'farmworld-cpu.json'
out.write_text(json.dumps(report,indent=2)+'\n')
unreal.log('S47_FARMWORLD_CPU_PASS '+str(out))
