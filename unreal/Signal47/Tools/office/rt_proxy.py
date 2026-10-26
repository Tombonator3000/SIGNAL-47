"""Asset-local Nanite build plus independent RT proxies for two office assemblies.

UE5.8.4 StaticMeshBuilder.cpp 166-170 and 845-856 only build dedicated RT
proxies when Nanite is enabled. Proxy settings alone on a non-Nanite mesh do
nothing. Preserve all source/fallback triangles and use 60% only for RT.
EngineTypes.h defines independent FMeshNaniteSettings/FMeshRayTracingProxySettings;
NaniteResources.cpp uses the dedicated proxy in default RT fallback mode.
No global cvar, collision, material or hardware ray tracing support change.
"""
from pathlib import Path
import json, hashlib, struct, sys
import unreal
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
import material_usage
NAMES=('SM_Office_Room','SM_Office_Dressing')
KEEP=.60
# A rebuilt aggregate bound includes reduced LOD extrema. Limit any outward
# face growth to 2 mm, anchored to the immutable Blender contract (not previous
# rebuilds); independently require unchanged LOD0 vertex bytes. The measured
# Dressing drift was 0.734 mm in centre/half extent, 1.467 mm at its lower face.
AGGREGATE_FACE_GROWTH_CM=.20
SOURCE_BOUND_TOL_CM=.025

def source_geometry(mesh):
    desc=mesh.get_static_mesh_description(0);assert desc is not None,mesh.get_name()
    count=desc.get_vertex_count();vertices=[];idx=0
    while len(vertices)<count and idx<count*4+64:
        vid=unreal.VertexID();assert vid.import_text('(IDValue='+str(idx)+')')
        if desc.is_vertex_valid(vid):
            p=desc.get_vertex_position(vid);vertices.append((p.x,p.y,p.z))
        idx+=1
    assert len(vertices)==count,(mesh.get_name(),'incomplete source vertex scan')
    return {'vertices':count,'triangles':desc.get_triangle_count(),
            'vertex_sha256':hashlib.sha256(b''.join(struct.pack('<ddd',*p) for p in vertices)).hexdigest(),
            'bounds_cm':[[fn(p[k] for p in vertices) for k in range(3)] for fn in (min,max)]}

def verify_metric_bounds(mesh,name,source=None):
    contract=Path(__file__).resolve().parents[3]/'ArtSource/Office/model_contract.json'
    row=json.loads(contract.read_text())['parts'][name];lo,hi=row['bounds_local_m']
    expected=[[lo[0]*100,-hi[1]*100,lo[2]*100],[hi[0]*100,-lo[1]*100,hi[2]*100]]
    source=source or source_geometry(mesh)
    assert source['triangles']==row['triangles'],(name,'source topology')
    assert max(abs(a-b) for aa,bb in zip(source['bounds_cm'],expected) for a,b in zip(aa,bb))<SOURCE_BOUND_TOL_CM,(name,'authored source metric bounds',source,expected)
    b=mesh.get_bounds();origin=[b.origin.x,b.origin.y,b.origin.z];extent=[b.box_extent.x,b.box_extent.y,b.box_extent.z]
    actual=[[origin[k]-extent[k] for k in range(3)],[origin[k]+extent[k] for k in range(3)]]
    outward=[expected[0][k]-actual[0][k] for k in range(3)]+[actual[1][k]-expected[1][k] for k in range(3)]
    assert all(-SOURCE_BOUND_TOL_CM<=v<=AGGREGATE_FACE_GROWTH_CM for v in outward),(name,'aggregate bound must contain original and remain within 2mm',actual,expected,outward)
    return {'source':source,'expected_source_bounds_cm':expected,'aggregate_bounds_cm':actual,
            'aggregate_outward_face_delta_cm':outward,'max_outward_face_growth_cm':AGGREGATE_FACE_GROWTH_CM}

def signature(mesh):
    b=mesh.get_bounds()
    return {'triangles':[mesh.get_num_triangles(i) for i in range(mesh.get_num_lods())],
            'materials':[(str(s.material_slot_name),s.material_interface.get_path_name() if s.material_interface else None) for s in mesh.get_editor_property('static_materials')],
            'bounds':[b.origin.x,b.origin.y,b.origin.z,b.box_extent.x,b.box_extent.y,b.box_extent.z],
            'source':source_geometry(mesh),
            'rt':mesh.get_editor_property('support_ray_tracing'),
            'collision':str(mesh.get_editor_property('body_setup').get_editor_property('collision_trace_flag'))}

def configure(mesh):
    nanite=mesh.get_editor_property('nanite_settings')
    nanite.set_editor_property('enabled',True)
    nanite.set_editor_property('keep_percent_triangles',1.)
    nanite.set_editor_property('trim_relative_error',0.)
    nanite.set_editor_property('generate_fallback',unreal.NaniteGenerateFallback.ENABLED)
    nanite.set_editor_property('fallback_target',unreal.NaniteFallbackTarget.PERCENT_TRIANGLES)
    nanite.set_editor_property('fallback_percent_triangles',1.)
    nanite.set_editor_property('fallback_relative_error',0.)
    mesh.set_editor_property('nanite_settings',nanite)
    settings=mesh.get_editor_property('ray_tracing_proxy_settings')
    settings.set_editor_property('enabled',True)
    settings.set_editor_property('fallback_target',unreal.NaniteFallbackTarget.PERCENT_TRIANGLES)
    settings.set_editor_property('fallback_percent_triangles',KEEP)
    settings.set_editor_property('fallback_relative_error',0.)
    settings.set_editor_property('foliage_over_occlusion_bias',0.)
    mesh.set_editor_property('ray_tracing_proxy_settings',settings)
    # Reuse the project's UE5.8 usage helper: set_base_material_usage recompiles
    # the new Nanite permutation and saves the owned material interface.
    for row in material_usage.ensure_mesh_material_usage(mesh,instanced=False):
        assert row['after'],row
        if not row['before']:
            material=unreal.load_asset(row['material'])
            errors=list(unreal.MaterialEditingLibrary.recompile_material(material))
            assert not errors,(row['material'],'Nanite material compile',errors)
            assert unreal.EditorAssetLibrary.save_loaded_asset(material),row['material']

def verify_configuration(mesh):
    nanite=mesh.get_editor_property('nanite_settings')
    expected={'enabled':True,'keep_percent_triangles':1.,'trim_relative_error':0.,
              'generate_fallback':unreal.NaniteGenerateFallback.ENABLED,
              'fallback_target':unreal.NaniteFallbackTarget.PERCENT_TRIANGLES,
              'fallback_percent_triangles':1.,'fallback_relative_error':0.}
    for field,value in expected.items():
        actual=nanite.get_editor_property(field)
        assert actual==value,(mesh.get_name(),'Nanite prerequisite/source-preserving fallback',field,actual,value)
    proxy=mesh.get_editor_property('ray_tracing_proxy_settings')
    assert proxy.get_editor_property('enabled'),mesh.get_name()
    assert proxy.get_editor_property('fallback_target')==unreal.NaniteFallbackTarget.PERCENT_TRIANGLES,mesh.get_name()
    assert abs(proxy.get_editor_property('fallback_percent_triangles')-KEEP)<1e-5,mesh.get_name()
    assert proxy.get_editor_property('fallback_relative_error')==0.,mesh.get_name()
    assert proxy.get_editor_property('foliage_over_occlusion_bias')==0.,mesh.get_name()
    for slot in mesh.get_editor_property('static_materials'):
        material=slot.material_interface
        assert isinstance(material,unreal.Material) and material.get_editor_property('blend_mode')==unreal.BlendMode.BLEND_OPAQUE,(mesh.get_name(),'opaque Nanite-compatible material',str(slot.material_slot_name))
        assert unreal.MaterialEditingLibrary.has_material_usage(material,unreal.MaterialUsage.MATUSAGE_NANITE),(mesh.get_name(),'saved Nanite material usage',material.get_path_name())
    return {'nanite_enabled':True,'source_keep_percent':1.,'source_trim_error':0.,
            'raster_fallback_keep_percent':1.,'raster_fallback_error':0.,
            'rt_proxy_enabled':True,'rt_proxy_keep_percent':KEEP,'opaque_nanite_material_usage_verified':True,
            'runtime_geometry_bytes':'must measure in cooked game; settings alone are not proof'}

def apply_all():
    rows=[]
    for name in NAMES:
        mesh=unreal.load_asset('/Game/Signal47/Art/Office/'+name)
        assert isinstance(mesh,unreal.StaticMesh),name
        before=signature(mesh);assert before['rt'],name
        configure(mesh)
        assert unreal.EditorAssetLibrary.save_loaded_asset(mesh,only_if_is_dirty=False),name
        after=signature(mesh)
        metric=verify_metric_bounds(mesh,name,after['source'])
        assert {k:v for k,v in after.items() if k!='bounds'}=={k:v for k,v in before.items() if k!='bounds'},(name,before,after)
        configured=verify_configuration(mesh)
        rows.append({'asset':mesh.get_path_name(),'raster_collision_materials_before':before,'after':after,'metric_validation':metric,**configured})
    report={'status':'PASS','scope':'two owned Nanite-enabled assets prerequisite for dedicated RT proxy build; source vertices/fallback topology/collision/materials unchanged; aggregate LOD bounds at most 2mm outside source contract; no global budgets or RT suppression; actual runtime saving still requires cooked measurement','assets':rows}
    (Path(unreal.Paths.project_saved_dir())/'office-rt-proxy.json').write_text(json.dumps(report,indent=2)+'\n')
    unreal.log('S47 OFFICE RT PROXY SETTINGS PASS '+json.dumps(report))

if __name__=='__main__':apply_all()
