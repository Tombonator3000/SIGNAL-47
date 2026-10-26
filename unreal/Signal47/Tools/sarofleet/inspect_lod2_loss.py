"""Root-run small diagnostic, only the two staff-car LOD2s. Does not modify delivery assets."""
import bpy,bmesh,json,sys,math
from pathlib import Path
from mathutils import Matrix
from collections import Counter,defaultdict
HERE=Path(__file__).resolve().parent;sys.path.insert(0,str(HERE));from design import WORK
bpy.ops.wm.open_mainfile(filepath=str(WORK/'SaroFleet.blend'))
result={'purpose':'actual source vs FBX LOD2 missing triangle diagnosis','parts':{}}

def rows(obj):
 mesh=obj.data;mesh.calc_loop_triangles();out=[];uv=mesh.uv_layers.active
 for t in mesh.loop_triangles:
  a,b,c=[obj.matrix_world@mesh.vertices[i].co for i in t.vertices]
  points=[tuple(float(x)for x in p)for p in [a,b,c]]
  edge=[(a-b).length,(b-c).length,(c-a).length]
  out.append({'indices':list(t.vertices),'points':points,'area':(b-a).cross(c-a).length/2,'edges':edge,'material':t.material_index,'uv':[list(uv.data[i].uv)for i in t.loops]if uv else[]})
 return out

def key(t,precision=6):return tuple(sorted(tuple(round(x,precision)for x in p)for p in t['points']))

def recreate(name):
 original=bpy.data.objects[name];e=original.copy();e.data=original.data.copy();bpy.context.scene.collection.objects.link(e);e.matrix_world=Matrix.Identity(4)
 bpy.ops.object.select_all(action='DESELECT');e.select_set(True);bpy.context.view_layer.objects.active=e
 d=e.modifiers.new('exact source LOD2 recreation','DECIMATE');d.ratio=.24;bpy.ops.object.modifier_apply(modifier=d.name)
 bm=bmesh.new();bm.from_mesh(e.data);bmesh.ops.triangulate(bm,faces=list(bm.faces));bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=1e-5)
 bad=[f for f in bm.faces if f.calc_area()<1e-8 or any(edge.calc_length()<2e-5 for edge in f.edges)]
 if bad:bmesh.ops.delete(bm,geom=bad,context='FACES')
 bm.to_mesh(e.data);bm.free();e.data.update();return e

for name in ['SM_SaroPark_Sedan','SM_SaroPark_Pickup']:
 e=recreate(name);source=rows(e);source_counter=Counter(key(t)for t in source)
 old=set(bpy.data.objects);bpy.ops.import_scene.fbx(filepath=str(WORK/'exports'/(name+'_LOD2.fbx')))
 imported=[o for o in bpy.data.objects if o not in old and o.type=='MESH'];assert len(imported)==1
 fbx=rows(imported[0]);actual_counter=Counter(key(t)for t in fbx);missing=source_counter-actual_counter;extra=actual_counter-source_counter
 lost=[]
 for t in source:
  k=key(t)
  if missing[k]>0:lost.append({**t,'source_same_position_facets':source_counter[k],'imported_same_position_facets':actual_counter[k]});missing[k]-=1
 duplicate_keys=[{'points':k,'count':v}for k,v in source_counter.items()if v>1]
 before=len(e.data.polygons);changed=e.data.validate(verbose=True,clean_customdata=False);after=len(e.data.polygons)
 validated=rows(e);after_counter=Counter(key(t)for t in validated)
 report={'recreated_source_triangles':len(source),'actual_FBX_triangles':len(fbx),'missing_triangles':lost,'extra_triangle_key_count':sum(extra.values()),'duplicate_position_facets':duplicate_keys,'mesh_validate_changed':changed,'polygons_before_validate':before,'polygons_after_validate':after,'validated_source_triangles':len(validated),'validated_missing_vs_import':sum((after_counter-actual_counter).values()),'validated_extra_vs_import':sum((actual_counter-after_counter).values()),'min_source_area':min(t['area']for t in source),'min_source_edge':min(min(t['edges'])for t in source),'source_contract':json.loads((WORK/'model_contract.json').read_text())['parts'][name]['lods'][2]}
 result['parts'][name]=report
 print('S47_LOD2_DIAG',name,json.dumps({k:v for k,v in report.items()if k not in ['missing_triangles','duplicate_position_facets','source_contract']}),flush=True)
 for t in lost:print('S47_LOD2_MISSING',name,json.dumps(t),flush=True)
path=WORK/'inspect-lod2-loss.json';path.write_text(json.dumps(result,indent=2));print('S47_LOD2_DIAG_READY',path,flush=True)
