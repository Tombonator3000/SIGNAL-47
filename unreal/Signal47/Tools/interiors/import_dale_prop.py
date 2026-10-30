"""Import the prepared private Tripo office prop with the existing FBX/PBR recipe."""
from pathlib import Path
import json, os, sys
import unreal

key = os.environ.get('S47_DALE_PROP','chair')
root = Path(os.environ.get('S47_DALE_MODEL_DIR', str(Path.home() / 'Documents/ChatGPT/Signal 47/miljografikk/2026-10-30-dale-realisme/models')))
source = root / key / 'prepared'
data = json.loads((source/'prop.json').read_text())
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'fieldintegration'))
import import_recipe as R
def require(name, condition, detail=None):
    if not condition: raise RuntimeError(str((name,detail)))
R.check = require
dest = '/Game/Licensed/DaleRealism'
mat = R.material(data['material'],dest+'/Materials',source)
path = dest+'/'+data['name']
if R.E.does_asset_exist(path):
    assert isinstance(R.E.load_asset(path),unreal.StaticMesh)
    assert R.E.delete_asset(path)
mesh = R.import_mesh(source/(data['name']+'.fbx'),dest)
mesh.set_material(0,mat)
mesh.set_editor_property('allow_cpu_access',True)
mesh.set_editor_property('support_ray_tracing',True)
body = mesh.get_editor_property('body_setup')
if body:
    instance = body.get_editor_property('default_instance')
    instance.set_editor_property('collision_profile_name','NoCollision')
    body.set_editor_property('default_instance',instance)
options = unreal.StaticMeshReductionOptions()
options.set_editor_property('auto_compute_lod_screen_size',False)
settings = []
for ratio,screen in ((1.,1.),(.45,.28),(.15,.09)):
    setting = unreal.StaticMeshReductionSettings()
    setting.set_editor_property('percent_triangles',ratio)
    setting.set_editor_property('screen_size',screen)
    settings.append(setting)
options.set_editor_property('reduction_settings',settings)
assert R.S.set_lods(mesh,options)
box = mesh.get_bounding_box()
assert abs((box.max.z-box.min.z)-data['height_m']*100)<.2
assert unreal.EditorAssetLibrary.save_loaded_asset(mesh,only_if_is_dirty=False)
unreal.log('DALE PROP IMPORTED '+mesh.get_path_name())
