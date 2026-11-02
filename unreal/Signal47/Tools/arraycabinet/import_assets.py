"""Run in Unreal: S47_ARRAY_CABINET=<prepared> S47_ARRAY_CABINET_REPORT=<report>.
Imports only /Game/ArrayCabinet. Source materials are restored by runtime.
"""
import hashlib, json, os, shutil
from pathlib import Path
import unreal
SRC=Path(os.environ['S47_ARRAY_CABINET']); REPORT=Path(os.environ['S47_ARRAY_CABINET_REPORT'])
DEST='/Game/ArrayCabinet'; E=unreal.EditorAssetLibrary; S=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
data=json.loads((SRC/'array-cabinet.json').read_text())
assert data['version']==1 and data['file']=='s03_cabinet.glb'
assert hashlib.sha256((SRC/data['file']).read_bytes()).hexdigest()==data['fileSha256']
rows={**data['meshes'],**data['parts']}; results=[]
if os.environ.get('S47_ARRAY_CABINET_VERIFY_ONLY')!='1':
    task=unreal.AssetImportTask();task.filename=str(SRC/data['file']);task.destination_path=DEST;task.replace_existing=True;task.automated=True;task.save=True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task]);E.make_directory(DEST+'/StaticMeshes')
    for path in E.list_assets(DEST,recursive=True):
        mesh=E.load_asset(path)
        if not isinstance(mesh,unreal.StaticMesh):continue
        name=mesh.get_name();assert name in rows, name
        body=mesh.get_editor_property('body_setup')
        if body:body.set_editor_property('collision_trace_flag',unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
        nanite=mesh.get_editor_property('nanite_settings');nanite.enabled=False;mesh.set_editor_property('nanite_settings',nanite)
        build=S.get_lod_build_settings(mesh,0);build.set_editor_property('use_full_precision_u_vs',True);S.set_lod_build_settings(mesh,0,build)
        E.save_loaded_asset(mesh)
        desired=rows[name]['asset'].split('.')[0]
        if mesh.get_path_name().split('.')[0]!=desired:assert E.rename_asset(path,desired)
for name,row in rows.items():
    mesh=E.load_asset(row['asset']);assert isinstance(mesh,unreal.StaticMesh),row['asset']
    expected=row.get('resultTris',row.get('triangles'));assert mesh.get_num_triangles(0)==expected,(name,mesh.get_num_triangles(0),expected)
    count=len(mesh.get_editor_property('static_materials'));assert count==len(row['sourceMaterialIds']),(name,count,row['sourceMaterialIds'])
    results.append({'name':name,'triangles':expected,'slots':count})
target=Path(unreal.Paths.project_content_dir())/'ArrayCabinet';target.mkdir(parents=True,exist_ok=True)
shutil.copy2(SRC/'array-cabinet.json',target/'array-cabinet.json')
REPORT.parent.mkdir(parents=True,exist_ok=True);REPORT.write_text(json.dumps({'status':'PASS','meshes':results,'scope':'Native import; runtime original material binding and visual inspection remain'},indent=2)+'\n')
unreal.log('ARRAY CABINET IMPORT PASS')
