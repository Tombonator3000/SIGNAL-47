"""Reopen authored Blender source and independently check exported mesh contract."""
from pathlib import Path
import bpy,json,hashlib,math,os
from mathutils import Vector
HERE=Path(__file__).resolve().parent
WORK=Path(os.environ.get('S47_ARRIVAL_WORK',str(Path.home()/'scratch-bm/Signal47-arrival-office-20261026')))
WORK.mkdir(parents=True,exist_ok=True)
contract=json.loads((HERE/'model_contract.json').read_text())
report={'status':'RUNNING','checks':[],'source':str(HERE/'SARO_ArrivalEntrance.blend')}
def check(name,ok,detail=None):
    report['checks'].append({'name':name,'pass':bool(ok),'detail':detail})
    (WORK/'entrance-source-verify.json').write_text(json.dumps(report,indent=2)+'\n')
    assert ok,(name,detail)
bpy.ops.wm.open_mainfile(filepath=report['source'])
check('metric Blender source',bpy.context.scene.unit_settings.system=='METRIC' and bpy.context.scene.unit_settings.scale_length==1)
for name,row in contract['parts'].items():
    counts=[]
    for lod in range(3):
        asset=name if lod==0 else name+'_LOD'+str(lod);ob=bpy.data.objects.get(asset)
        check(asset+' actual mesh',ob is not None and ob.type=='MESH')
        check(asset+' pivot at zero',ob.location.length<1e-6 and (ob.scale-Vector((1,1,1))).length<1e-6)
        counts.append(len(ob.data.polygons))
        check(asset+' triangulated',all(len(p.vertices)==3 for p in ob.data.polygons))
        check(asset+' no degenerate faces',all(p.area>1e-12 for p in ob.data.polygons))
        check(asset+' usable UV layer',len(ob.data.uv_layers)>0 and len(ob.data.uv_layers.active.data)==len(ob.data.loops))
        check(asset+' finite vertices',all(math.isfinite(c) for v in ob.data.vertices for c in v.co))
        check(asset+' material set',set(m.name for m in ob.data.materials)==set(row['materials']) if lod==0 else set(m.name for m in ob.data.materials)<=set(row['materials']))
        path=HERE/'Export'/(asset+'.fbx');check(asset+' FBX saved',path.is_file() and path.stat().st_size>1024)
        if lod==0:
            points=[ob.matrix_world@v.co for v in ob.data.vertices]
            bounds=[[min(p[k]for p in points)for k in range(3)],[max(p[k]for p in points)for k in range(3)]]
            check(asset+' independently measured bounds',max(abs(a-b) for old,new in zip(bounds,row['bounds_local_m'])for a,b in zip(old,new))<1e-6)
    check(name+' exact three LODs',counts==row['lod_triangles'],counts)
    check(name+' strictly reduced LODs',counts[0]>counts[1]>counts[2]>0)
check('bounded original geometry budget',sum(r['lod_triangles'][0]for r in contract['parts'].values())<15000)
check('all PBR images packed',all(im.packed_file is not None for im in bpy.data.images if im.source=='FILE'))
sign=contract['parts']['SM_Arrival_VisitorSign']['bounds_local_m']
check('46 cm visitor sign',abs(sign[1][0]-sign[0][0]-.46)<.001)
report['status']='PASS';report['blend_sha256']=hashlib.sha256((HERE/'SARO_ArrivalEntrance.blend').read_bytes()).hexdigest()
(WORK/'entrance-source-verify.json').write_text(json.dumps(report,indent=2)+'\n')
print('ARRIVAL FRESH BLENDER VERIFY PASS',len(report['checks']))
