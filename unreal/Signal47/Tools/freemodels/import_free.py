# Imports the free models (~/Assets/signal47-free, see unreal/MODELLER.md) into /Game/Free:
#   /Game/Free/<Category>/<model>/<asset>    one static mesh per model (its meshes combined), or one per
#                                            prop for a pack, with its glTF materials and PBR textures
#   Content/Free/free_models.json            the manifest for whoever places them: name, asset path,
#                                            category, licence, author, link, bounds in cm, triangles, Nanite
# gltf_prep.py has already made each model a glTF scaled to its real size with the pivot at the middle
# of its base (S47_FREE_WORK/plan.json); glTF is metres and +Y up, Interchange turns it into centimetres
# and Z up. /Game/Free is ignored by Git, as /Game/Licensed is (CC-BY and CC0 files, but big).
# Run through Tools/freemodels/import_free.sh, which runs gltf_prep.py first, then:
# UnrealEditor-Cmd Signal47.uproject -run=pythonscript -script=Tools/freemodels/import_free.py -unattended -nullrhi
# S47_FREE_ONLY=name,name imports only those models (the manifest keeps the others' rows).
import json
import os
import unreal

eal = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
WORK = os.environ.get('S47_FREE_WORK', os.path.expanduser('~/scratch-bm/Signal47-free'))
ONLY = set(filter(None, os.environ.get('S47_FREE_ONLY', '').split(',')))
ROOT = '/Game/Free'
CONTENT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_content_dir())
# the cooked textures are kept to 2k (the sources stay as they came: 4k and 8k in some packs)
MAX_TEX = 2048


def log(s):
	unreal.log_warning('S47FREE ' + s)


def pipelines():
	"""The project's glTF pipelines (DefaultGLTFAssetsPipeline + DefaultGLTFPipeline), changed in
	memory only: static meshes, a model's meshes combined into one, no animations, no Nanite here
	(set per model afterwards), the materials and textures in the model's own folder."""
	assets = unreal.load_asset('/Interchange/Pipelines/DefaultGLTFAssetsPipeline')
	gltf = unreal.load_asset('/Interchange/Pipelines/DefaultGLTFPipeline')
	# <model>/StaticMeshes, Materials, Textures: no folder per glTF scene (a pack's props share theirs)
	assets.set_editor_property('scene_name_sub_folder', False)
	assets.set_editor_property('asset_type_sub_folders', True)
	common = assets.get_editor_property('common_meshes_properties')
	common.set_editor_property('force_all_mesh_as_type', unreal.InterchangeForceMeshType.IFMT_STATIC_MESH)
	common.set_editor_property('bake_meshes', True)
	mesh = assets.get_editor_property('mesh_pipeline')
	mesh.set_editor_property('combine_static_meshes_behavior', unreal.InterchangeCombineStaticMeshesBehavior.ALL)
	mesh.set_editor_property('build_nanite', False)
	mesh.set_editor_property('import_skeletal_meshes', False)
	anim = assets.get_editor_property('animation_pipeline')
	anim.set_editor_property('import_animations', False)
	return assets, gltf


ASSETS_PIPE, GLTF_PIPE = pipelines()


def import_part(gltf_path, dest, asset_name):
	ASSETS_PIPE.set_editor_property('asset_name', asset_name)
	stack = unreal.InterchangePipelineStackOverride()
	stack.add_pipeline(ASSETS_PIPE)
	stack.add_pipeline(GLTF_PIPE)
	t = unreal.AssetImportTask()
	t.set_editor_property('filename', gltf_path)
	t.set_editor_property('destination_path', dest)
	t.set_editor_property('replace_existing', True)
	t.set_editor_property('automated', True)
	t.set_editor_property('save', True)
	t.set_editor_property('options', stack)
	tools.import_asset_tasks([t])
	return list(t.get_editor_property('imported_object_paths') or [])


def folder_bytes(path):
	n = 0
	for d, _, files in os.walk(path):
		for f in files:
			n += os.path.getsize(os.path.join(d, f))
	return n


def finish_mesh(sm, nanite):
	ns = sm.get_editor_property('nanite_settings')
	if ns.get_editor_property('enabled') != nanite:
		ns.set_editor_property('enabled', nanite)
		sm.set_editor_property('nanite_settings', ns)
	eal.save_loaded_asset(sm)


def tris_of(sm):
	try:
		return sm.get_num_triangles(0)
	except Exception:
		return -1


plan = json.load(open(os.path.join(WORK, 'plan.json')))
manifest_path = os.path.join(CONTENT, 'Free', 'free_models.json')
old = {}
if ONLY and os.path.isfile(manifest_path):
	old = {m['asset']: m for m in json.load(open(manifest_path))['models']}
rows = [m for m in old.values() if m['name'] not in ONLY]
failed = []
for entry in plan:
	name = entry['name']
	if ONLY and name not in ONLY:
		continue
	if 'error' in entry:
		failed.append(name)
		log('FAIL %s: %s' % (name, entry['error']))
		continue
	folder = '%s/%s/%s' % (ROOT, entry['category'], name)
	# afresh: a model's assets are its own folder's
	if eal.does_directory_exist(folder):
		eal.delete_directory(folder)
	for part in entry['parts']:
		try:
			paths = import_part(part['gltf'], folder, part['asset'])
		except Exception as e:
			paths = []
			log('FAIL %s: %s' % (part['asset'], e))
		meshes = []
		for p in paths:
			a = eal.load_asset(p)
			if isinstance(a, unreal.StaticMesh):
				meshes.append(a)
			elif isinstance(a, unreal.Texture2D):
				if a.get_editor_property('max_texture_size') == 0 or a.get_editor_property('max_texture_size') > MAX_TEX:
					a.set_editor_property('max_texture_size', MAX_TEX)
					eal.save_loaded_asset(a)
		if not meshes:
			failed.append(part['asset'])
			log('FAIL %s: no static mesh (%d objects)' % (part['asset'], len(paths)))
			continue
		if len(meshes) > 1:
			log('WARN %s: %d meshes, not combined' % (part['asset'], len(meshes)))
		for sm in meshes:
			finish_mesh(sm, part['nanite'])
		sm = meshes[0]
		box = sm.get_bounding_box()
		lo, hi = box.min, box.max
		rows.append({
			'name': name, 'asset': sm.get_path_name(), 'category': entry['category'],
			'title': entry['title'], 'licence': entry['licence'], 'author': entry['author'], 'link': entry['link'],
			'bounds_cm': {'min': [round(lo.x, 1), round(lo.y, 1), round(lo.z, 1)], 'max': [round(hi.x, 1), round(hi.y, 1), round(hi.z, 1)],
				'size': [round(hi.x - lo.x, 1), round(hi.y - lo.y, 1), round(hi.z - lo.z, 1)]},
			'tris': tris_of(sm), 'nanite': part['nanite'], 'scale': round(entry['scale'], 6),
			'materials': len(sm.get_editor_property('static_materials')),
		})
		log('OK %s %s tris %d size %s cm%s' % (entry['category'], sm.get_path_name(), rows[-1]['tris'], rows[-1]['bounds_cm']['size'], ' NANITE' if part['nanite'] else ''))
	disk = folder_bytes(os.path.join(CONTENT, 'Free', entry['category'], name))
	for r in rows:
		if r['name'] == name:
			r['folder_mb'] = round(disk / 1e6, 1)
	log('SIZE %s %.1f MB' % (name, disk / 1e6))

rows.sort(key=lambda r: (r['category'], r['asset']))
os.makedirs(os.path.dirname(manifest_path), exist_ok=True)
json.dump({'_doc': 'Made by Tools/freemodels/import_free.py. Bounds in Unreal centimetres, pivot at the middle of the base (min z 0). CC BY 4.0 models must stay credited (Source/Signal47/Private/S47FreeCredits.inl, CREDITS.txt).',
	'models': rows}, open(manifest_path, 'w'), indent=1)
log('done: %d assets, %d failed %s, %.0f MB in Content/Free' % (len(rows), len(failed), failed, folder_bytes(os.path.join(CONTENT, 'Free')) / 1e6))
