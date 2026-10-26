"""Fresh editor-process verification of the saved original arrival assets."""
import hashlib,json,os,sys,traceback
from pathlib import Path
import unreal
HERE=Path(__file__).resolve().parent;PROJECT=HERE.parents[1]
SOURCE=Path(os.environ.get('S47_ARRIVAL_SOURCE',str(PROJECT.parent/'ArtSource/ArrivalEntrance')))
WORK=Path(os.environ.get('S47_ARRIVAL_WORK',str(Path.home()/'scratch-bm/Signal47-arrival-office-20261026')))
sys.path.insert(0,str(HERE.parent/'fieldintegration'))
import import_recipe as R
R.REPORT=WORK/'entrance-native-verify.json';R.REPORT.parent.mkdir(parents=True,exist_ok=True)
R.report={'status':'RUNNING','checks':[],'meshes':[]}
try:
    imported=json.loads((WORK/'entrance-native-import.json').read_text())
    R.check('completed native import',imported['status']=='PASS' and all(row['pass']for row in imported['checks']))
    R.report['import_report_sha256']=hashlib.sha256((WORK/'entrance-native-import.json').read_bytes()).hexdigest()
    for name,digest in imported['source_sha256'].items():R.check('unchanged source '+name,hashlib.sha256((SOURCE/name).read_bytes()).hexdigest()==digest)
    for row in imported['meshes']:
        mesh=unreal.load_asset(row['asset']);R.check(row['name']+' reopened',isinstance(mesh,unreal.StaticMesh))
        tris=[mesh.get_num_triangles(i)for i in range(mesh.get_num_lods())]
        R.check(row['name']+' persisted LODs',tris==row['triangles'],tris)
        slots=mesh.get_editor_property('static_materials');materials=[str(s.material_slot_name)for s in slots]
        R.check(row['name']+' persistent material names',materials==row['materials'])
        R.check(row['name']+' real authored materials',all(s.material_interface and s.material_interface.get_path_name().startswith('/Game/Signal47/Art/ArrivalEntrance/Materials/')for s in slots))
        R.check(row['name']+' hardware RT',mesh.get_editor_property('support_ray_tracing'))
        b=mesh.get_bounds();size=[b.box_extent.x*2,b.box_extent.y*2,b.box_extent.z*2]
        R.check(row['name']+' metric dimensions persist',max(abs(a-b)for a,b in zip(size,row['size_cm']))<.01)
        R.report['meshes'].append(dict(row))
    R.check('human-scale visitor sign',next(row for row in R.report['meshes']if row['name']=='SM_Arrival_VisitorSign')['size_cm'][0]<=46.1)
    R.report['status']='PASS';R.flush();unreal.log('S47 ARRIVAL FRESH VERIFY PASS')
except Exception:
    R.report['status']='FAIL';R.report['error']=traceback.format_exc();R.flush();raise
