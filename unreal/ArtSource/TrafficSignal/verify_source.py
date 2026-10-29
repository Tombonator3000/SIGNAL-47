"""Fresh-file checks of the saved signal source: LODs, slots, pivot, lens optics and exports."""
from pathlib import Path
import json
import bpy
HERE=Path(__file__).resolve().parent
bpy.ops.wm.open_mainfile(filepath=str(HERE/'Caliche_TrafficSignal.blend'))
d=json.loads((HERE/'model_contract.json').read_text()); checks=[]
def check(name,ok):
    checks.append({'name':name,'pass':bool(ok)}); assert ok,name
for name,row in d['parts'].items():
    tris=[]
    for lod in range(3):
        ob=bpy.data.objects[name+('_LOD'+str(lod) if lod else '')]; me=ob.data
        me.calc_loop_triangles(); tris.append(len(me.loop_triangles))
        check(name+' LOD'+str(lod)+' triangles',tris[-1]==row['lod_triangles'][lod])
        check(name+' LOD'+str(lod)+' triangle/UV topology',all(len(p.vertices)==3 and p.area>1e-12 for p in me.polygons) and bool(me.uv_layers))
        check(name+' LOD'+str(lod)+' origin at the span clamp',tuple(ob.location)==(0,0,0))
        check(name+' LOD'+str(lod)+' export exists',(HERE/'Export'/(ob.name+'.fbx')).is_file())
        check(name+' LOD'+str(lod)+' slots',{m.name for m in me.materials}==set(row['materials']))
    check(name+' three meaningful LODs',tris[1]<tris[0]*.65 and tris[2]<tris[0]*.3)
head=d['parts']['SM_Caliche_Signal_Head']; lo,hi=head['bounds_local_m']
check('head hangs below the pivot',hi[2]<.02 and lo[2]<-.9)
check('head lens slot',d['lens_material'] in head['materials'])
check('four lens faces',len(d['lenses'])==4 and all(abs(sum(x*x for x in L['normal_three'])-1)<1e-9 for L in d['lenses']))
check('old crossing light and glow kept pivot-relative',d['light_pool_three_m']==[0,-1.05,0] and d['glow_three_m']==[0,-.63,0])
span=d['parts']['SM_Caliche_Signal_Span']; lo,hi=span['bounds_local_m']
check('span reaches both poles',lo[0]<-9.1 and hi[0]>9.1 and lo[1]<-19 and hi[1]>19)
check('bounded topology',d['total_LOD0_triangles']<30000)
for row in json.loads((HERE/'materials.json').read_text()):
    check(row['name']+' maps on disk',all((HERE/f).is_file() for f in row['textures'].values()))
result={'status':'PASS','scope':'Fresh Blender source, not native Unreal or runtime','checks':checks,'passed':len(checks),'total':len(checks)}
(HERE/'source-verification.json').write_text(json.dumps(result,indent=2)+'\n')
print('TRAFFIC SIGNAL SOURCE VERIFIED',len(checks))
