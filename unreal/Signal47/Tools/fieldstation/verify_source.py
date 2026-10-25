"""Inspect actual saved Blender source and exported FBX/GLB; root owns DCC slot."""
import bpy
import json
import math
import struct
import hashlib
from pathlib import Path

R=Path(__file__).resolve().parent
C=json.loads((R/'model_contract.json').read_text());checks=[]
def finish(status):
    (R/'verification.json').write_text(json.dumps({'status':status,'source_revision':C['source_revision'],'checks':checks,'scope':'Actual exported geometry, UVs, material slots, hinge pivot, source scene and packed pixels only. Native placement, lighting, interaction and visual acceptance are separate.'},indent=2)+'\n')
def check(name,ok,**detail):
    checks.append({'name':name,'passed':bool(ok),**detail})
    if not ok:finish('FAIL');raise AssertionError(name,detail)
def clear():bpy.ops.wm.read_factory_settings(use_empty=True)
def bounds(points):return [[fn(p[k] for p in points) for k in range(3)] for fn in [min,max]]
def difference(a,b):return max(abs(x-y) for aa,bb in zip(a,b) for x,y in zip(aa,bb))

for lod in range(3):
    for name,row in C['lods'][str(lod)].items():
        suffix='' if lod==0 else '_LOD'+str(lod);stem=name+suffix
        clear();bpy.ops.import_scene.fbx(filepath=str(R/'exports'/(stem+'.fbx')),use_custom_normals=True)
        obs=[o for o in bpy.context.scene.objects if o.type=='MESH']
        check(stem+' one exported part',len(obs)==1)
        ob=obs[0];me=ob.data;me.calc_loop_triangles()
        actual=bounds([ob.matrix_world@v.co for v in me.vertices]);err=difference(actual,row['bounds_local_m'])
        check(stem+' metre geometry and zero pivot',err<.0001 and ob.location.length<1e-5 and max(abs(x-1) for x in ob.scale)<1e-5 and len(me.loop_triangles)==row['triangles'],maximum_error_m=err,triangles=len(me.loop_triangles))
        check(stem+' material slot contract',[m.name for m in me.materials]==row['materials'])
        check(stem+' finite valid normals',all(p.normal.length>.99 for p in me.polygons) and all(math.isfinite(c) for v in me.vertices for c in v.co) and min(t.area for t in me.loop_triangles)>=5e-9)
        check(stem+' actual UV layer',len(me.uv_layers)==1)
        uv=me.uv_layers[0].data;minimum=math.inf
        for t in me.loop_triangles:
            a,b,c=[uv[i].uv for i in t.loops];minimum=min(minimum,abs((b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x))*.5)
        check(stem+' noncollapsed finite UVs',minimum>1e-12 and all(math.isfinite(c) for u in uv for c in u.uv),minimum_uv_area=minimum)
        raw=(R/'exports'/(stem+'.glb')).read_bytes();n=struct.unpack_from('<I',raw,12)[0];doc=json.loads(raw[20:20+n]);binary=raw[28+n:];pts=[];tris=0
        for mesh in doc['meshes']:
            for p in mesh['primitives']:
                tris+=doc['accessors'][p['indices']]['count']//3
                a=doc['accessors'][p['attributes']['POSITION']];view=doc['bufferViews'][a['bufferView']];off=view.get('byteOffset',0)+a.get('byteOffset',0);stride=view.get('byteStride',12)
                for i in range(a['count']):
                    x,y,z=struct.unpack_from('<fff',binary,off+i*stride);pts.append((x,-z,y))
                check(stem+' actual GLB primitive UVs','TEXCOORD_0' in p['attributes'])
        check(stem+' GLB geometry matches FBX',tris==row['triangles'] and difference(bounds(pts),row['bounds_local_m'])<.0001,triangles=tris)

clear();bpy.ops.import_scene.fbx(filepath=str(R/'exports'/'axis_probe.fbx'))
probe=next(o for o in bpy.context.scene.objects if o.type=='MESH');lo,hi=bounds([probe.matrix_world@v.co for v in probe.data.vertices]);got=[(a+b)/2 for a,b in zip(lo,hi)]+[b-a for a,b in zip(lo,hi)]
check('Actual asymmetric FBX axis probe',max(abs(a-b) for a,b in zip(got,[2,4,6,.2,.4,.6]))<1e-5)
bpy.ops.wm.open_mainfile(filepath=str(R/'Station01_Field_Hut.blend'))
check('Saved metric original scene',bpy.context.scene.unit_settings.scale_length==1 and bpy.context.scene.unit_settings.system=='METRIC')
check('Independently editable original collection','EDITABLE_ORIGINAL_STATION01' in bpy.data.collections)
used=set()
for name,row in C['parts'].items():
    ob=bpy.data.objects.get(name);check(name+' editable saved mesh',ob is not None and len(ob.vertex_groups)>0)
    check(name+' retained original component mesh',bpy.data.objects.get('SRC_'+name) is not None)
    check(name+' source local-zero placement',ob.location.length<1e-5 and max(abs(x) for x in ob.rotation_euler)<1e-5 and max(abs(x-1) for x in ob.scale)<1e-5)
    used.update(row['materials'])
door=bpy.data.objects['SM_FieldStation_Hut_Door'];hinge=door.parent
check('Door preview parent preserves native hinge-local part',hinge is not None and hinge['runtime_parent_handle']=='world.site.objs.hutDoorLeaf' and (hinge.location.x+9.45)**2+(hinge.location.y+4.48)**2+hinge.location.z**2<1e-10 and abs(hinge.rotation_euler.z+1.75)<1e-6)
rows={r['name']:r for r in json.loads((R/'materials.json').read_text())}
required={(R/path).resolve() for name in used for path in rows[name].get('textures',{}).values()};packed=set()
for im in bpy.data.images:
    if im.source!='FILE':continue
    path=Path(bpy.path.abspath(im.filepath)).resolve()
    check('Packed original image '+im.name,im.packed_file is not None and path.is_file() and im.packed_file.data==path.read_bytes())
    check('Relative editable image path '+im.name,im.filepath.startswith('//'))
    packed.add(path)
check('All used original PBR maps packed',required<=packed,required=len(required),packed=len(packed))
check('Three authored descending LODs',all(C['lods']['0'][n]['triangles']>=C['lods']['1'][n]['triangles']>=C['lods']['2'][n]['triangles'] for n in C['parts']))
check('Exactly five separate hut chunks',len(C['parts'])==5)
finish('PASS')
exclude={'source_manifest.json'}
files=[{'path':str(p.relative_to(R)),'bytes':p.stat().st_size,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()} for p in sorted(R.rglob('*')) if p.is_file() and p.name not in exclude and '__pycache__' not in str(p) and p.suffix not in ['.blend1','.pyc']]
(R/'source_manifest.json').write_text(json.dumps({'status':'SOURCE_VERIFIED_NATIVE_VISUAL_UNVERIFIED','source_revision':C['source_revision'],'parts':len(C['parts']),'actual_source_checks':len(checks),'LOD_triangles':{lod:sum(row['triangles'] for row in data.values()) for lod,data in C['lods'].items()},'files':files},indent=2)+'\n')
print('FIELDSTATION_SOURCE_PASS',len(checks))
