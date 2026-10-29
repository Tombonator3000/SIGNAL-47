"""Import original Operations room assets with the project's measured FBX/PBR recipe.

-S47OfficeDressingRefresh refreshes Cork plus Dressing only after independent source verification.
"""
from pathlib import Path
import hashlib, json, sys, traceback
import unreal
HERE=Path(__file__).resolve().parent; PROJECT=HERE.parents[1]
SOURCE=PROJECT.parent/'ArtSource/Office'; DEST='/Game/Signal47/Art/Office'
sys.path.insert(0,str(HERE.parent/'fieldintegration'))
sys.path.insert(0,str(HERE))
import rt_proxy
import import_recipe as R
R.REPORT=Path(unreal.Paths.project_saved_dir())/'office-native-import.json'
DRESSING_REFRESH='S47OfficeDressingRefresh' in unreal.SystemLibrary.get_command_line()
REFRESH_MESH='SM_Office_Dressing';REFRESH_MATERIAL='M_Office_Cork'
R.report={'status':'RUNNING','mode':'dressing_refresh' if DRESSING_REFRESH else 'full_import',
          'engine':unreal.SystemLibrary.get_engine_version(),'checks':[],'meshes':[],
          'materials_refreshed':[],'materials_reused':[]}
R.flush()
try:
    contract=json.loads((SOURCE/'model_contract.json').read_text())
    rows=json.loads((SOURCE/'materials.json').read_text())
    if DRESSING_REFRESH:
        R.check('Dressing refresh exact selected mesh',len(contract['parts'])==8 and REFRESH_MESH in contract['parts'])
        cork=[row for row in rows if row['name']==REFRESH_MATERIAL]
        R.check('Dressing refresh exact Cork material',len(cork)==1 and REFRESH_MATERIAL in contract['parts'][REFRESH_MESH]['materials'])
        R.check('Cork three-map DirectX source',set(cork[0]['textures'])=={'base_color','roughness','normal'} and
                cork[0].get('normal_convention')=='DirectX')
        proof_path=SOURCE/'source-verification.json';proof=json.loads(proof_path.read_text())
        checked_parts={part['mesh']:part for part in proof['parts']}
        R.check('Independent source verification covers current eight parts',proof['status']=='PASS' and
                len(proof['parts'])==8 and set(checked_parts)==set(contract['parts']) and all(
                    checked_parts[name]['triangles']==row['triangles'] and
                    checked_parts[name]['components']==row['original_components'] for name,row in contract['parts'].items()))
        inputs=[SOURCE/'Signal47_Operations.blend',SOURCE/'model_contract.json',SOURCE/'materials.json',
                SOURCE/'Export'/(REFRESH_MESH+'.fbx'),*[SOURCE/path for path in cork[0]['textures'].values()]]
        R.check('Independent source proof is newer than refresh inputs',all(path.is_file() for path in inputs) and
                proof_path.stat().st_mtime_ns>=max(path.stat().st_mtime_ns for path in inputs))
        R.report['source_verification_sha256']=hashlib.sha256(proof_path.read_bytes()).hexdigest()
        R.report['source_contract_sha256']=hashlib.sha256((SOURCE/'model_contract.json').read_bytes()).hexdigest()
        R.report['cork_texture_sha256']={role:hashlib.sha256((SOURCE/path).read_bytes()).hexdigest()
                                        for role,path in cork[0]['textures'].items()}
    mats={}
    for row in rows:
        name=row['name'];path=DEST+'/Materials/'+name
        if DRESSING_REFRESH and name!=REFRESH_MATERIAL:
            material=R.E.load_asset(path)
            R.check(name+' existing typed Office material',isinstance(material,unreal.MaterialInterface) and
                    material.get_path_name()==path+'.'+name)
            mats[name]=material;R.report['materials_reused'].append(name)
        else:
            mats[name]=R.material(row,DEST+'/Materials',SOURCE);R.report['materials_refreshed'].append(name)
    for name,row in contract['parts'].items():
        if DRESSING_REFRESH and name!=REFRESH_MESH:continue
        # Match the existing field-kit importer: regenerate only our named mesh.
        # Atomic FBX reimport can retain a previous derived source description;
        # R6 read back 963 source faces from the unchanged 2944-face cabinet FBX.
        # Materials, original Web geometry and gameplay IDs keep their identities.
        asset=DEST+'/'+name
        if DRESSING_REFRESH:
            R.check(name+' exact owned Dressing refresh path',name==REFRESH_MESH and asset==DEST+'/'+REFRESH_MESH and
                    (SOURCE/'Export'/(name+'.fbx')).is_file())
        if R.E.does_asset_exist(asset):
            if DRESSING_REFRESH:
                existing=R.E.load_asset(asset)
                R.check(name+' existing typed exact owned mesh before delete',isinstance(existing,unreal.StaticMesh) and
                        existing.get_path_name()==asset+'.'+REFRESH_MESH)
                existing=None
            R.check(name+' replace only authored Office mesh',R.E.delete_asset(asset))
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
    if not DRESSING_REFRESH:
        exec(compile((HERE/'prepare_cpu.py').read_text(),str(HERE/'prepare_cpu.py'),'exec'),{'__file__':str(HERE/'prepare_cpu.py')})
    R.report['portal_source_preparation']='skipped_unchanged' if DRESSING_REFRESH else 'performed'
    R.report['scope']='Cork material and Dressing only; fresh verify_assets.py must still prove all eight' if DRESSING_REFRESH else 'all Office materials and eight meshes'
    R.report['status']='PASS';R.flush();unreal.log('S47 OFFICE NATIVE IMPORT PASS mode='+R.report['mode'])
except Exception:
    R.report['status']='FAIL';R.report['error']=traceback.format_exc();R.flush();raise
