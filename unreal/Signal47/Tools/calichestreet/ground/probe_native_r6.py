"""Read-only native FBX/source comparison; never saves or edits authored meshes."""
import bpy,json,math,itertools,collections,sys
from pathlib import Path
from mathutils import Vector
R=Path(__file__).resolve().parent
OUT=Path('/home/tom/scratch-bm/Signal47-caliche-concept')

def read(path):
    bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
    for m in list(bpy.data.materials):bpy.data.materials.remove(m)
    bpy.ops.import_scene.fbx(filepath=str(path))
    data=[];objects=[]
    for ob in bpy.context.scene.objects:
        if ob.type!='MESH':continue
        me=ob.data;me.calc_loop_triangles();color=me.color_attributes.active_color
        objects.append({'name':ob.name,'transform':[list(row) for row in ob.matrix_world],'color':[(a.name,a.domain,a.data_type) for a in me.color_attributes]})
        for tri in me.loop_triangles:
            mat=me.materials[tri.material_index].name.split('.')[0]
            pts=[]
            for li in tri.loops:
                loop=me.loops[li];p=ob.matrix_world@me.vertices[loop.vertex_index].co
                a=color.data[loop.vertex_index if color.domain=='POINT' else li].color[3] if color else None
                pts.append({'p':tuple(p),'uv':tuple(me.uv_layers[0].data[li].uv),'a':a})
            data.append({'mat':mat,'pts':pts})
    return data,objects

def bounds(ts):
    return [[f(v['p'][i] for t in ts for v in t['pts']) for i in range(3)] for f in [min,max]]
source,so=read(R/'exports/SM_Caliche_Surface.fbx')
native,no=read(OUT/'native-r6-SM_Caliche_Surface.fbx')
sb=bounds(source);nb=bounds(native);best=None
for axes in itertools.permutations(range(3)):
 for signs in itertools.product([-1,1],repeat=3):
  for scale in [.01,1,100]:
   bb=[[nb[side if signs[i]>0 else 1-side][axes[i]]*signs[i]*scale for i in range(3)] for side in range(2)]
   error=max(abs(bb[j][i]-sb[j][i]) for i in range(3) for j in range(2))
   if best is None or error<best[0]:best=(error,axes,signs,scale)
err,axes,signs,scale=best;assert err<.0001,best
for t in native:
 for v in t['pts']:v['p']=tuple(v['p'][axes[i]]*signs[i]*scale for i in range(3))
sc=collections.Counter(t['mat'] for t in source);nc=collections.Counter(t['mat'] for t in native)
assert sc==nc,(sc,nc)
index={}
for t in source:
 for v in t['pts']:
  key=(t['mat'],tuple(round(a,3) for a in v['p']))
  index.setdefault(key,[]).append(v)
point_err=uv_err=alpha_err=0;rows={}
for t in native:
 row=rows.setdefault(t['mat'],{'triangles':0,'alphas':[],'matched_corners':0,'position_error_m':0,'UV_error':0,'alpha_error':0})
 row['triangles']+=1
 for v in t['pts']:
  key=(t['mat'],tuple(round(a,3) for a in v['p']))
  candidates=index.get(key,[])
  if not candidates:
   cell=key[1]
   for delta in itertools.product([-1,0,1],repeat=3):
    candidates+=index.get((key[0],tuple(round(cell[i]+delta[i]*.001,3) for i in range(3))),[])
  assert candidates,('native corner not in source',key)
  def cost(s):return sum((v['p'][i]-s['p'][i])**2+(v['uv'][i%2]-s['uv'][i%2])**2 for i in range(3))
  closest=min(candidates,key=cost)
  pe=max(abs(v['p'][i]-closest['p'][i]) for i in range(3));ue=max(abs(v['uv'][i]-closest['uv'][i]) for i in range(2));ae=abs(v['a']-closest['a'])
  point_err=max(pe,point_err);uv_err=max(ue,uv_err);alpha_err=max(ae,alpha_err)
  row['position_error_m']=max(pe,row['position_error_m']);row['UV_error']=max(ue,row['UV_error']);row['alpha_error']=max(ae,row['alpha_error'])
  row['alphas'].append(v['a']);row['matched_corners']+=1
assert point_err<.0001,point_err
assert uv_err<.0021,('Native render UV0 uses half precision',uv_err)
for name in ['M_CS_CompactedUse','M_CS_ConcreteUse']:assert rows[name]['UV_error']<.00025,(name,rows[name])
assert alpha_err<.005,alpha_err
for mat,row in rows.items():
 a=row.pop('alphas');row.update(alpha_range=[min(a),max(a)],partial_alpha_corners=sum(.001<x<.999 for x in a))
report={'status':'PASS','source_face_counts':dict(sc),'native_face_counts':dict(nc),'native_to_source':{'axes':axes,'signs':signs,'scale':scale,'bounds_error_m':err},'maximum_position_error_m':point_err,'maximum_UV_error':uv_err,'maximum_vertex_alpha_error':alpha_err,'all_material_assigned_corners_matched':sum(r['matched_corners'] for r in rows.values()),'source_objects':so,'native_objects':no,'material_comparison':rows,'contact_proof':'Matching geometry in same source coordinates preserves r6_report and wear_contact_samples heights; measured point maximum error listed above.'}
(R/'r6_native_wear_triangles.json').write_text(json.dumps([t for t in native if t['mat'] in ['M_CS_CompactedUse','M_CS_ConcreteUse']],separators=(',',':'))+'\n')
(R/'r6_native_fbx_comparison.json').write_text(json.dumps(report,indent=2)+'\n')
print('R6_NATIVE_FBX_COMPARISON_PASS',json.dumps(report))
