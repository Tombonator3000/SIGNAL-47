"""Read freshly exported geometry and packed source; no Unreal/game claims."""
import bpy,json,math,sys
from pathlib import Path
from mathutils import Vector
from mathutils.bvhtree import BVHTree
R=Path(__file__).resolve().parent;sys.path.insert(0,str(R))
from geometry import terrain_height
C=json.loads((R/'model_contract.json').read_text());checks=[]
def check(name,ok,**detail):
 checks.append({'name':name,'pass':bool(ok),**detail})
 if not ok:
  (R/'verification.json').write_text(json.dumps({'status':'FAIL','checks':checks},indent=2)+'\n');raise AssertionError(name,detail)
for lod in range(3):
 for name,e in C['lods'][str(lod)].items():
  bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
  for mat in list(bpy.data.materials):bpy.data.materials.remove(mat)
  suffix='' if lod==0 else '_LOD'+str(lod)
  bpy.ops.import_scene.fbx(filepath=str(R/'exports'/(name+suffix+'.fbx')))
  obs=[o for o in bpy.context.scene.objects if o.type=='MESH'];check(name+suffix+' one actual mesh',len(obs)==1);ob=obs[0];me=ob.data;me.calc_loop_triangles();ps=[ob.matrix_world@v.co for v in me.vertices];b=[[f(v[k] for v in ps)for k in range(3)]for f in [min,max]]
  check(name+suffix+' exact geometry contract',len(me.loop_triangles)==e['triangles'] and max(abs(a-b)for v,w in zip(b,e['bounds_local_m'])for a,b in zip(v,w))<.00008 and ob.location.length<1e-5,triangles=len(me.loop_triangles),bounds=b)
  mats=[m.name for m in me.materials];check(name+suffix+' material order',mats==e['materials'],materials=mats)
  finite=all(math.isfinite(v)for uv in me.uv_layers[0].data for v in uv.uv) if len(me.uv_layers)==1 else False
  minimum_uv=math.inf
  if finite:
   for tri in me.loop_triangles:
    ua,ub,uc=[me.uv_layers[0].data[i].uv for i in tri.loops];minimum_uv=min(minimum_uv,abs((ub.x-ua.x)*(uc.y-ua.y)-(ub.y-ua.y)*(uc.x-ua.x))*.5)
  check(name+suffix+' stable triangles and UV',finite and minimum_uv>1e-12 and min(t.area for t in me.loop_triangles)>=5e-9,minimum_uv_area=minimum_uv)
  if name.endswith('InspectionFittings'):
   tree=BVHTree.FromPolygons(ps,[list(t.vertices)for t in me.loop_triangles],all_triangles=True)
   origin=Vector(C['emitter_site_m']);rays=[Vector((0,-1,0)),Vector((0,-1,-.4)),Vector((.4,-1,0)),Vector((-.4,-1,0)),Vector((0,-1,.4))]
   clear=[tree.ray_cast(origin,d.normalized(),.5)[0] is None for d in rays]
   check(name+suffix+' real emitted front/down aperture',all(clear),five_ray_results=clear)
  if name.endswith('GroundShoulder'):
   outer=[v for v in ps if abs(v.z+.075)>.0002]
   check(name+suffix+' measured outer-edge terrain contact',bool(outer) and all(abs(v.z-terrain_height(v.x,v.y)-.006)<.0001 for v in outer))
   check(name+suffix+' excludes front gameplay areas',all(v.y>=-7.11 or v.x<-2.79 for v in ps))
  if name.endswith('GravelContact'):
   check(name+suffix+' known bed contact only',all(-10<v.x<10 and -8<v.y<8 and -.11<v.z<.1 for v in ps),bounds=b)
bpy.ops.wm.open_mainfile(filepath=str(R/'Substation_Detail.blend'))
check('saved editable five parts',all('SRC_'+n in bpy.data.objects for n in C['parts']))
images=[im for im in bpy.data.images if im.source=='FILE'];check('source PBR images packed',len(images)>=9 and all(im.packed_file and im.size[0]>0 for im in images),images=len(images))
check('native axis/root declared',C['site_world_three_m']==[7890,-1.10,536.20] and C['world_yaw_degrees']==180 and C['axis_probe_expected_UE_center_cm']==[200,-400,600])
check('bounded authored budget',C['total_LOD0_triangles']<=15000 and len(C['parts'])==5)
j={'status':'PASS','checks':checks,'check_count':len(checks),'lod_totals':{lod:sum(v['triangles']for v in rows.values())for lod,rows in C['lods'].items()},'limits':'Actual imported FBX for all three LOD, UVs and five light-aperture rays per LOD, fresh packed editable Blend. Native/game/raycast/terrain-contact outside known bed not claimed.'};(R/'verification.json').write_text(json.dumps(j,indent=2)+'\n');print('SUBSTATION_DETAIL_VERIFY_PASS',len(checks))
