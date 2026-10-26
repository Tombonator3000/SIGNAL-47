"""Fresh read of the saved .blend; writes evidence only, never changes model sources."""
from pathlib import Path
import bpy,json,math,hashlib
HERE=Path(__file__).resolve().parent
contract=json.loads((HERE/'model_contract.json').read_text());checks=[]
def check(name,ok,detail=None):
    checks.append({'name':name,'pass':bool(ok),'detail':detail});assert ok,(name,detail)
check('saved live body and neck-root head',set(o.name for o in bpy.data.collections['01_LIVE_HEREFORD_EDITABLE'].objects)=={'SM_Cow_Alive_Body','SM_Cow_Alive_Head'})
head=bpy.data.objects['SM_Cow_Alive_Head'];body=bpy.data.objects['SM_Cow_Alive_Body']
check('head assembled at unchanged NECK',all(abs(a-b)<1e-6 for a,b in zip(head.location,[0,.74,1.12])),list(head.location))
check('live soles on exact ground pivot',abs(min(v.co.z for v in body.data.vertices))<1e-7)
for name in ('02_ABDUCTED_ALDERETE25_EDITABLE','04_CONCEPT_REFERENCES','05_EXPORTED_LODS'):
    check(name+' hidden in startup viewport',all(o.hide_get() for o in bpy.data.collections[name].objects))
for coll in ('01_LIVE_HEREFORD_EDITABLE','02_ABDUCTED_ALDERETE25_EDITABLE','05_EXPORTED_LODS'):
    for ob in bpy.data.collections[coll].objects:
        me=ob.data;uv=me.uv_layers.active.data
        check(ob.name+' finite vertex positions',all(math.isfinite(c)for v in me.vertices for c in v.co))
        check(ob.name+' explicit triangles',all(len(f.vertices)==3 for f in me.polygons))
        check(ob.name+' all material slots resolved',all(m is not None for m in me.materials))
        check(ob.name+' explicit positive-area source triangles',min(f.area for f in me.polygons)>=5e-9,min(f.area for f in me.polygons))
        minedge=min((me.vertices[e.vertices[0]].co-me.vertices[e.vertices[1]].co).length for e in me.edges)
        check(ob.name+' no coincident FBX corners',minedge>=1e-6,minedge)
        zero=0
        for f in me.polygons:
            a,b,c=[uv[li].uv for li in f.loop_indices];zero+=abs((b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x))<2e-12
        check(ob.name+' no collapsed tangent UV triangles',zero==0,zero)
        if coll=='05_EXPORTED_LODS':
            name=ob.name.split('_LOD')[0].split('.')[0];lod=1 if '_LOD1'in ob.name else 2 if '_LOD2'in ob.name else 0
            row=contract['lods'][str(lod)][name]
            check(ob.name+' triangle contract',len(me.polygons)==row['triangles'])
            bounds=[[float(fn(v.co[k]for v in me.vertices))for k in range(3)]for fn in(min,max)]
            check(ob.name+' metric bounds contract',all(abs(a-b)<1e-6 for a,b in zip(sum(bounds,[]),sum(row['bounds_local_m'],[]))))
for im in bpy.data.images:
    if im.source=='FILE':check(im.name+' embedded image',bool(im.packed_file),im.filepath)
report={'status':'PASS','engine':bpy.app.version_string,'blender_source_sha256':hashlib.sha256((HERE/'Signal47_Cows.blend').read_bytes()).hexdigest(),'checks':checks,'scope':'fresh saved Blender source, not native Unreal or packaged gameplay'}
(HERE/'source_verification.json').write_text(json.dumps(report,indent=2)+'\n')
print('COW FRESH SOURCE VERIFICATION PASS',len(checks))
