"""Root-run Blender FBX roundtrip. No render. Reports actual triangles/UV/materials/pivots."""
import bpy,json,sys,math
from pathlib import Path
HERE=Path(__file__).resolve().parent;sys.path.insert(0,str(HERE));from design import WORK
contract=json.loads((WORK/'model_contract.json').read_text());checks=[]
def check(name,ok,detail=None):checks.append({'name':name,'pass':bool(ok),'detail':detail})
for name,row in contract['parts'].items():
 for lod in range(3):
  bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
  for m in list(bpy.data.materials):bpy.data.materials.remove(m)
  path=WORK/'exports'/(name+(''if lod==0 else'_LOD'+str(lod))+'.fbx')
  bpy.ops.import_scene.fbx(filepath=str(path));objs=[o for o in bpy.context.scene.objects if o.type=='MESH'];check(name+str(lod)+' single mesh',len(objs)==1)
  if len(objs)!=1:continue
  o=objs[0];count=sum(len(p.vertices)-2 for p in o.data.polygons);check(name+str(lod)+' exact tris',count==row['lods'][lod]['triangles'],[count,row['lods'][lod]['triangles']]);check(name+str(lod)+' UV',bool(o.data.uv_layers));check(name+str(lod)+' slots',[m.name for m in o.data.materials]==row['materials']);check(name+str(lod)+' base pivot',o.location.length<1e-5)
  o.data.calc_loop_triangles();areas=[(o.data.vertices[t.vertices[1]].co-o.data.vertices[t.vertices[0]].co).cross(o.data.vertices[t.vertices[2]].co-o.data.vertices[t.vertices[0]].co).length/2 for t in o.data.loop_triangles];check(name+str(lod)+' no null triangles',min(areas,default=0)>1e-10,min(areas,default=0))
  if lod==0:
   points=[o.matrix_world@v.co for v in o.data.vertices];lo=[min(p[i]for p in points)for i in range(3)];hi=[max(p[i]for p in points)for i in range(3)]
   check(name+' roundtrip bounds',max(abs(v-w)for a,b in zip([lo,hi],row['bounds_local_m'])for v,w in zip(a,b))<1e-4,[lo,hi])
check('service width fits existing collider',contract['parts']['SM_Saro07_Body']['bounds_local_m'][1][0]-contract['parts']['SM_Saro07_Body']['bounds_local_m'][0][0]<=2.10)
check('driver eye and instrument contract',contract['driver_eye_three']==[-.42,1.48,.2])
report={'status':'PASS'if all(c['pass']for c in checks)else'FAIL','checks':checks,'count':len(checks),'revision':contract['revision']};(WORK/'source-verification.json').write_text(json.dumps(report,indent=2));print('S47_FLEET_SOURCE_'+report['status'],len(checks));assert report['status']=='PASS'
