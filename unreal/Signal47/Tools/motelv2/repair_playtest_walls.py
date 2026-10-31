"""Repair every proven inward closed solid in motel production meshes, preserving originals.
Root's serial Blender command: --python repair_playtest_walls.py -- --source PACKAGE --out NEW_DIR
Open surfaces (glass/water/leaves) are never guessed. Same R6 UV/custom-normal preserving helper.
No engine/process launch. Original accepted package remains byte-for-byte untouched.
"""
import argparse,importlib.util,json,sys,shutil
from pathlib import Path
import bpy
R=Path(__file__).resolve().parent
sp=importlib.util.spec_from_file_location('motel_orient',R/'repair_face_orientation.py');O=importlib.util.module_from_spec(sp);sp.loader.exec_module(O);G=O.G
ap=argparse.ArgumentParser();ap.add_argument('--source',type=Path,required=True);ap.add_argument('--out',type=Path,required=True)
a=ap.parse_args(sys.argv[sys.argv.index('--')+1:] if '--'in sys.argv else [])
source=a.source.resolve();out=a.out.resolve();assert source!=out and not out.exists(),'new output directory required'
out.mkdir(parents=True);(out/'exports').mkdir()
manifest=json.loads((source/'manifest.json').read_text());bpy.ops.wm.open_mainfile(filepath=str(source/'Sierra_Production.blend'))
objects={ob.name:ob for ob in bpy.data.objects if ob.type=='MESH'};report={'status':'RUNNING','parts':{},'source':str(source)}
for part,ob in objects.items():
 if part not in manifest['parts'] or any(x in part for x in ['Glass','Water','Glow','Planting']):continue
 bpy.context.window.scene=next(scene for scene in bpy.data.scenes if ob.name in scene.objects)
 before=G.mesh_snapshot(ob.data,include_normals=True);islands=O.island_inventory(before['positions'],before['faces']);flips=[]
 for row in islands:
  topo=O.topology(before['positions'],before['faces'],row)
  if topo['closed'] and topo['consistently_wound'] and topo['signed_volume_m3'] < -1e-8:flips.extend(row['faces'])
 if not flips:continue
 expected=O.flip_snapshot(before,flips);O.replace_mesh(bpy,ob,before,expected)
 # Verify the chosen closed solids now face outward without moving any point or UV.
 after=G.mesh_snapshot(ob.data,include_normals=True);assert after['positions']==before['positions']
 for row in islands:
  if set(row['faces']).intersection(flips):assert O.topology(after['positions'],after['faces'],row)['signed_volume_m3']>0
 spec=manifest['parts'][part];lods=[]
 for lod,ratio in [(0,1.),(1,.5),(2,.22)]:
  selected=ob
  if lod:
   selected=ob.copy();selected.data=ob.data.copy();selected.name=part+'_LOD'+str(lod);bpy.context.scene.collection.objects.link(selected)
   bpy.ops.object.select_all(action='DESELECT');selected.select_set(True);bpy.context.view_layer.objects.active=selected
   mod=selected.modifiers.new('Authored distance simplification','DECIMATE');mod.ratio=ratio;mod.use_collapse_triangulate=True;bpy.ops.object.modifier_apply(modifier=mod.name)
  snap=G.mesh_snapshot(selected.data);row={'triangles':len(snap['faces']),'bounds_local_m':G.bounds(snap['positions'],range(len(snap['positions']))),'materials':spec['materials']};lods.append(row)
  G.export_fbx(bpy,selected,out/'exports'/(selected.name+'.fbx'))
  if lod:bpy.data.objects.remove(selected,do_unlink=True)
 spec['lods']=lods;spec['lod_triangles']=[l['triangles']for l in lods];report['parts'][part]={'flipped_triangles':len(flips),'lod_triangles':spec['lod_triangles']}
assert report['parts'],'no inward solids found; inspect hydrated source version'
bpy.ops.wm.save_as_mainfile(filepath=str(out/'Sierra_Walls_Repaired.blend'),compress=True)
(out/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n');report['status']='SOURCE_READY_NATIVE_UNVERIFIED';(out/'repair.json').write_text(json.dumps(report,indent=2)+'\n')
print('MOTEL_WALL_REPAIR_SOURCE_READY',len(report['parts']),flush=True)
