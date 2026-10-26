"""Import original Operations room assets with the project's measured FBX/PBR recipe."""
from pathlib import Path
import json, sys, traceback
import unreal
HERE=Path(__file__).resolve().parent; PROJECT=HERE.parents[1]
SOURCE=PROJECT.parent/'ArtSource/Office'; DEST='/Game/Signal47/Art/Office'
sys.path.insert(0,str(HERE.parent/'fieldintegration'))
sys.path.insert(0,str(HERE))
import rt_proxy
import import_recipe as R
R.REPORT=Path(unreal.Paths.project_saved_dir())/'office-native-import.json'
R.report={'status':'RUNNING','engine':unreal.SystemLibrary.get_engine_version(),'checks':[],'meshes':[]}
R.flush()
try:
    contract=json.loads((SOURCE/'model_contract.json').read_text())
    mats={row['name']:R.material(row,DEST+'/Materials',SOURCE) for row in json.loads((SOURCE/'materials.json').read_text())}
    for name,row in contract['parts'].items():
        mesh=R.import_mesh(SOURCE/'Export'/(name+'.fbx'),DEST)
        slots=[]
        for i,slot in enumerate(mesh.get_editor_property('static_materials')):
            material=str(slot.material_slot_name);R.check(name+' authored material '+material,material in mats)
            mesh.set_material(i,mats[material]);slots.append(material)
        R.check(name+' exact source materials',set(slots)==set(row['materials']))
        R.check(name+' exact authored topology',mesh.get_num_triangles(0)==row['triangles'],mesh.get_num_triangles(0))
        bounds=mesh.get_bounds();low,high=row['bounds_local_m']
        center=[bounds.origin.x,bounds.origin.y,bounds.origin.z];size=[bounds.box_extent.x*2,bounds.box_extent.y*2,bounds.box_extent.z*2]
        expected=[(low[0]+high[0])*50,-(low[1]+high[1])*50,(low[2]+high[2])*50]
        R.check(name+' metric origin',max(abs(a-b) for a,b in zip(center,expected))<.35,{'actual':center,'expected':expected})
        R.check(name+' metric dimensions',max(abs(size[k]-(high[k]-low[k])*100) for k in range(3))<.35,size)
        options=unreal.StaticMeshReductionOptions();options.set_editor_property('auto_compute_lod_screen_size',False);settings=[]
        for ratio,screen in ((1.,1.),(.55,.25),(.25,.08)):
            setting=unreal.StaticMeshReductionSettings();setting.set_editor_property('percent_triangles',ratio);setting.set_editor_property('screen_size',screen);settings.append(setting)
        options.set_editor_property('reduction_settings',settings)
        R.check(name+' three native LODs',R.S.set_lods(mesh,options)==3)
        mesh.set_editor_property('support_ray_tracing',True)
        if name in rt_proxy.NAMES: rt_proxy.configure(mesh)
        else:
            nanite=mesh.get_editor_property('nanite_settings');nanite.enabled=False;mesh.set_editor_property('nanite_settings',nanite)
        body=mesh.get_editor_property('body_setup')
        if body:body.set_editor_property('collision_trace_flag',unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
        for lod in range(R.S.get_lod_count(mesh)):
            build=R.S.get_lod_build_settings(mesh,lod);build.set_editor_property('use_full_precision_u_vs',True);R.S.set_lod_build_settings(mesh,lod,build)
        R.check(name+' saved',R.E.save_loaded_asset(mesh))
        if name in rt_proxy.NAMES: rt_proxy.verify_configuration(mesh)
        tris=[mesh.get_num_triangles(i) for i in range(R.S.get_lod_count(mesh))]
        R.check(name+' reduced actual topology',len(tris)==3 and tris[0]>tris[1]>tris[2]>0,tris)
        R.report['meshes'].append({'asset':mesh.get_path_name(),'triangles':tris,'center_cm':center,'size_cm':size,'materials':slots})
    # Existing exact portal patch requires these six immutable source meshes at runtime.
    exec(compile((HERE/'prepare_cpu.py').read_text(),str(HERE/'prepare_cpu.py'),'exec'),{'__file__':str(HERE/'prepare_cpu.py')})
    R.report['status']='PASS';R.flush();unreal.log('S47 OFFICE NATIVE IMPORT PASS')
except Exception:
    R.report['status']='FAIL';R.report['error']=traceback.format_exc();R.flush();raise
