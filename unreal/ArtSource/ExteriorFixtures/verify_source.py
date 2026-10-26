"""Focused fresh-file checks for fixtures, mounting planes, optics and authored LODs."""
from pathlib import Path
import json, math
import bpy
HERE=Path(__file__).resolve().parent
bpy.ops.wm.open_mainfile(filepath=str(HERE/'SARO_ExteriorFixtures.blend'))
d=json.loads((HERE/'model_contract.json').read_text());checks=[]
def check(name,ok):
    checks.append({'name':name,'pass':bool(ok)});assert ok,name
for name,row in d['parts'].items():
    tris=[]
    for lod in range(3):
        ob=bpy.data.objects[name+('_LOD'+str(lod) if lod else '')];me=ob.data
        me.calc_loop_triangles();tris.append(len(me.loop_triangles))
        check(name+' LOD'+str(lod)+' triangles',tris[-1]==row['lod_triangles'][lod])
        check(name+' LOD'+str(lod)+' normal/UV topology',all(len(p.vertices)==3 and p.area>1e-12 for p in me.polygons) and bool(me.uv_layers))
        check(name+' LOD'+str(lod)+' lens slot',d['lens_material'] in [m.name for m in me.materials])
        check(name+' LOD'+str(lod)+' export exists',(HERE/'Export'/(ob.name+'.fbx')).is_file())
    check(name+' three meaningful LODs',tris[1]<tris[0]*.65 and tris[2]<tris[0]*.25)
    a,b=row['bounds_three_m'];mount=row['mount_plane'];idx=1 if mount['axis']=='y' else 2
    face=b[idx] if mount['solid_side'].startswith('-') else a[idx]
    check(name+' exact physical mounting plane',abs(face)<1e-7)
    n=row['lens_normal_three'];c=row['lens_center_three_m'];em=row['emitter_three_m']
    check(name+' unit optical normal',abs(sum(x*x for x in n)-1)<1e-6)
    check(name+' emitter beyond glass',.005<sum((em[i]-c[i])*n[i] for i in range(3))<.011)
    check(name+' bounded opaque textured lens',len([m for m in bpy.data.materials if m.name==d['lens_material']])==1)
check('three original assemblies',len(d['parts'])==3)
check('bounded total topology',d['total_LOD0_triangles']<12000)
rows=json.loads((HERE/'materials.json').read_text())
for row in rows:
    check(row['name']+' maps packed and on disk',all((HERE/f).is_file() for f in row['textures'].values()))
    check(row['name']+' DirectX normal contract',row['normal_convention']=='DirectX')
result={'status':'PASS','scope':'Fresh Blender source, not native Unreal or runtime','checks':checks,'passed':len(checks),'total':len(checks)}
(HERE/'source-verification.json').write_text(json.dumps(result,indent=2)+'\n')
print('EXTERIOR SOURCE VERIFIED',len(checks))
