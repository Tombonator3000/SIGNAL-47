"""Fresh Unreal-process verification of delivered Operations room meshes/materials."""
from pathlib import Path
import json, sys
import unreal
sys.path.insert(0,str(Path(__file__).resolve().parent))
import rt_proxy
PROJECT=Path(unreal.Paths.project_dir()).resolve();SOURCE=PROJECT.parent/'ArtSource/Office'
contract=json.loads((SOURCE/'model_contract.json').read_text());rows=[]
for name,row in contract['parts'].items():
    path='/Game/Signal47/Art/Office/'+name;mesh=unreal.load_asset(path)
    assert isinstance(mesh,unreal.StaticMesh),(path,'missing asset')
    tris=[mesh.get_num_triangles(i) for i in range(mesh.get_num_lods())]
    assert len(tris)==3 and tris[0]==row['triangles'] and tris[0]>tris[1]>tris[2]>0,(path,tris)
    slots=mesh.get_editor_property('static_materials');names=[str(slot.material_slot_name) for slot in slots]
    assert set(names)==set(row['materials']),(path,names,row['materials'])
    assert all(slot.material_interface and slot.material_interface.get_path_name().startswith('/Game/Signal47/Art/Office/Materials/') for slot in slots),path
    b=mesh.get_bounds();center=[b.origin.x,b.origin.y,b.origin.z];size=[b.box_extent.x*2,b.box_extent.y*2,b.box_extent.z*2];low,high=row['bounds_local_m']
    want=[(low[0]+high[0])*50,-(low[1]+high[1])*50,(low[2]+high[2])*50]
    assert max(abs(a-b) for a,b in zip(center,want))<.35,(path,center,want)
    assert max(abs(size[k]-(high[k]-low[k])*100)for k in range(3))<.35,(path,size)
    assert mesh.get_editor_property('support_ray_tracing'),path
    if name in rt_proxy.NAMES:
        rt_proxy.verify_metric_bounds(mesh,name)
        rt_proxy.verify_configuration(mesh)
    else:
        assert not mesh.get_editor_property('nanite_settings').enabled,path
    assert mesh.get_editor_property('body_setup').get_editor_property('collision_trace_flag')==unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE,path
    rows.append({'asset':path,'triangles':tris,'materials':names,'center_cm':center,'size_cm':size,
                 'rt_configuration':rt_proxy.verify_configuration(mesh) if name in rt_proxy.NAMES else 'standard render LODs'})
assert len(rows)==8
for geo in (341,342,346,349,351,352):
    mesh=unreal.load_asset(f'/Game/Web/Geo/geo_002/StaticMeshes/g{geo}')
    assert mesh and mesh.get_editor_property('allow_cpu_access'),geo
report={'status':'PASS','scope':'fresh native reload, all 8 mesh topologies, LODs, PBR materials, metric bounds, complex interaction collision and six CPU portal sources','meshes':rows}
(Path(unreal.Paths.project_saved_dir())/'office-native-verification.json').write_text(json.dumps(report,indent=2)+'\n')
unreal.log('S47 OFFICE NATIVE VERIFY PASS '+json.dumps(report))
