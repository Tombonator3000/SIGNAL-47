"""Run in a NEW guarded Blender process after full source+roundtrip passes.
Loads saved source, removes every third-party object/dependency, saves normal .blend.
No libraries.write or cross-scene copying of importer datablocks.
"""
import bpy,json,sys,shutil,hashlib
from pathlib import Path
HERE=Path(__file__).resolve().parent;sys.path.insert(0,str(HERE));from design import WORK,REVISION
bpy.ops.wm.open_mainfile(filepath=str(WORK/'SaroFleet.blend'))
scene=bpy.context.scene
names={'SM_Saro07_Cabin','SM_Saro07_Branding','SM_Saro07_HeadLens','SM_Saro07_TailLens'}
editable=[c for c in bpy.data.collections if c.name.startswith('Editable_SM_Saro07_')]
keep=set(o for c in editable for o in c.objects)|{bpy.data.objects[n]for n in names}
assert len(editable)==4 and all(n in bpy.data.objects for n in names)
keep_names={o.name for o in keep}
# Native addon parts were linked through the original 'Collection'. Link every kept
# object directly to the surviving scene BEFORE removing those other collections,
# otherwise orphan purge deletes the native addon objects and leaves stale RNA refs.
for o in keep:
 if o.name not in scene.collection.objects:scene.collection.objects.link(o)
for o in list(bpy.data.objects):
 if o not in keep:bpy.data.objects.remove(o,do_unlink=True)
for c in list(bpy.data.collections):
 if c not in editable:bpy.data.collections.remove(c)
for other in list(bpy.data.scenes):
 if other!=scene:bpy.data.scenes.remove(other)
bpy.data.orphans_purge(do_recursive=True)
keep={bpy.data.objects[n]for n in keep_names if n in bpy.data.objects}
assert len(keep)==len(keep_names),('own objects lost during cleanup',sorted(keep_names-set(o.name for o in keep)))
used=set(m for o in keep for m in o.data.materials)
assert all(m.name.startswith('M_Fleet_')for m in used),('third-party material leaked',[m.name for m in used])
images=set(n.image for m in used if m.use_nodes for n in m.node_tree.nodes if n.type=='TEX_IMAGE'and n.image)
assert all(i.packed_file for i in images),('unpacked own texture',[i.name for i in images if not i.packed_file])
assert all(Path(i.filepath).name.startswith(('Paint_','Vinyl_','Dash_'))for i in images),('foreign texture',[i.name for i in images])
for m in list(bpy.data.materials):
 if m not in used:bpy.data.materials.remove(m)
for i in list(bpy.data.images):
 if i not in images:bpy.data.images.remove(i)
texture_dir=HERE/'textures';texture_dir.mkdir(exist_ok=True);relative_textures={}
assert len(images)==9,('expected original Paint/Vinyl/Dash PBR triplets',len(images))
for image in images:
 name=Path(image.filepath).name;source=WORK/'textures'/name;assert source.is_file(),source
 target=texture_dir/name;shutil.copyfile(source,target);raw=target.read_bytes()
 image.filepath='//textures/'+name;image.pack(data=raw,data_len=len(raw))
 relative_textures[image.filepath]=hashlib.sha256(raw).hexdigest()
scene.name='SARO07 original addons';scene['source_revision']=REVISION;scene['credits']='SIGNAL / 47 original fitted cab, SARO branding and lamp lenses'
# Normal saved file with only original data; editable source collections remain available.
bpy.ops.wm.save_as_mainfile(filepath=str(HERE/'Saro07_Addons.blend'),relative_remap=False)
exports=HERE/'exports';exports.mkdir(exist_ok=True)
for name in names:shutil.copyfile(WORK/'exports'/(name+'.fbx'),exports/(name+'.fbx'))
bpy.ops.object.select_all(action='DESELECT')
for name in names:bpy.data.objects[name].select_set(True)
bpy.ops.export_scene.gltf(filepath=str(exports/'Saro07_Addons.glb'),export_format='GLB',use_selection=True,export_apply=True)
report={'status':'PASS','revision':REVISION,'objects':len(keep),'editable_collections':[c.name for c in editable],'parts':sorted(names),'materials':sorted(m.name for m in used),'packed_original_images':sorted(i.name for i in images),'relative_original_textures_sha256':relative_textures,'third_party_objects':0,'third_party_materials':0}
(WORK/'own-addons-verification.json').write_text(json.dumps(report,indent=2));print('S47_FLEET_OWN_ADDONS_PASS '+str(HERE/'Saro07_Addons.blend'))
