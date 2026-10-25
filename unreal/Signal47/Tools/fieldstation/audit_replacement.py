"""Read immutable legacy buffers and count conservative hut-only retirement.
No asset writes outside this source folder. Bounds are mesh-local Three metres.
"""
import hashlib
import json
import struct
from pathlib import Path

R=Path(__file__).resolve().parent
MANIFEST=Path('/home/tom/SIGNAL-47/unreal/Signal47/Content/WebData/manifest.json')
GEO=Path('/home/tom/scratch-bm/Signal47-web/geo')
d=json.loads(MANIFEST.read_text());nodes={n['geo']:n for n in d['nodes'] if 'geo' in n}
whole={698:'Hut exterior stucco only',699:'Hut flat roof only',701:'Hut green window and door frames only',702:'Hut outside glass only',
       714:'Hut dado only',715:'Hut plaster only',717:'Hut visual floor only; collider retained',718:'Hut ceiling only',729:'Hut cupboard shell only',
       713:'Old hidden fallback chair only',743:'Old licensed chair only',730:'Old door leaf, retain hutDoorLeaf parent transform',731:'Old door handles, retain hutDoorLeaf parent transform'}
# x0,x1,y0,y1,z0,z1. A triangle is removed only if ALL vertices are inside.
cuts={
    697:[[-13.501,-8.499,-.251,.001,.999,5.001],[-10.651,-9.249,-.251,-.069,4.999,5.421]],
    694:[[-13.486,-8.514,2.594,2.666,1.014,4.986]],
    700:[[-13.001,-8.50,.80,2.0,1.499,4.701]],
    690:[[-9.20,-8.60,1.38,3.75,4.09,4.61],[-9.151,-9.049,-.01,1.11,4.669,4.811]],
    691:[[-12.57,-12.23,2.75,3.70,1.83,2.17],[-9.16,-8.94,3.55,3.80,4.33,4.57],[-10.261,-9.639,2.161,2.363,4.699,4.712]],
    693:[[-10.261,-9.639,2.161,2.363,4.699,4.725],[-9.211,-8.989,1.199,1.341,4.809,4.829]],
    705:[[-9.231,-8.969,1.089,1.411,4.699,4.821]],
    716:[[-13.001,-8.999,2.479,2.601,2.159,2.241],[-13.001,-8.999,2.479,2.601,3.759,3.841],
         [-13.001,-8.999,.881,.919,1.479,1.526],[-13.001,-8.999,.881,.919,4.474,4.521],
         [-13.026,-12.974,.881,.919,1.499,4.501],[-9.026,-8.974,.881,.919,1.499,4.501],
         [-12.951,-12.049,1.319,1.496,1.499,1.771]],
    719:[[-12.91,-12.08,1.49,1.76,1.55,1.71]],
    720:[[-12.91,-12.08,1.49,1.76,1.55,1.71],[-12.535,-12.525,1.419,1.481,3.619,3.741]],
    721:[[-12.535,-12.52,.039,1.741,3.892,3.908]],
    722:[[-12.54,-12.49,.919,1.081,3.819,3.981],[-12.22,-12.08,1.489,1.671,1.559,1.701]],
}
cache={}
def load(g):
    fn=next(item['file'] for item in d['geoFiles'] if item['from']<=g<item['to'])
    if fn not in cache:
        raw=(GEO/fn).read_bytes();n=struct.unpack_from('<I',raw,12)[0];cache[fn]=(json.loads(raw[20:20+n]),raw[28+n:])
    doc,blob=cache[fn];mesh=next(m for m in doc['meshes'] if m['name']=='g'+str(g))
    assert len(mesh['primitives'])==1
    p=mesh['primitives'][0]
    def accessor(i):
        a=doc['accessors'][i];v=doc['bufferViews'][a['bufferView']];off=v.get('byteOffset',0)+a.get('byteOffset',0)
        fmt={5126:'f',5125:'I',5123:'H',5121:'B'}[a['componentType']];count={'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4}[a['type']];stride=v.get('byteStride',struct.calcsize('<'+fmt*count))
        return [struct.unpack_from('<'+fmt*count,blob,off+j*stride) for j in range(a['count'])]
    xyz=accessor(p['attributes']['POSITION']);ix=[x[0] for x in accessor(p['indices'])]
    return xyz,[ix[i:i+3] for i in range(0,len(ix),3)]
def inside(p,b):return b[0]<=p[0]<=b[1] and b[2]<=p[1]<=b[3] and b[4]<=p[2]<=b[5]
whole_rows=[];partial=[]
for g,reason in whole.items():
    n=nodes[g];xyz,tris=load(g)
    whole_rows.append({'node':n['i'],'parent':n['p'],'geo':g,'source_material_ids':n['mats'],'triangles':len(tris),'reason':reason})
for g,boxes in cuts.items():
    n=nodes[g];xyz,tris=load(g);remove=[];keep=[];boxcounts=[0]*len(boxes)
    for t in tris:
        matches=[i for i,b in enumerate(boxes) if all(inside(xyz[v],b) for v in t)]
        if matches:
            remove.append(t)
            for i in matches:boxcounts[i]+=1
        else:keep.append(t)
    assert remove,(g,'No triangle matched, audit recipe invalid')
    partial.append({'node':n['i'],'parent':n['p'],'geo':g,'source_material_ids':n['mats'],'source_coordinate_space':'geometry-local Three metres before station root translation',
                    'remove_whole_triangles_inside':boxes,'expected_original_triangles':len(tris),'expected_removed_triangles':len(remove),'expected_retained_triangles':len(keep),'box_match_counts':boxcounts,
                    'retained_triangle_position_sha256':hashlib.sha256(b''.join(struct.pack('<fff',*xyz[v]) for t in keep for v in t)).hexdigest(),
                    'policy':'Whole triangles only, all three vertices inside. Preserve original vertex/normal/UV/color data and all indices of retained triangles. Overlapping cuts from the separately authored instrument set must be unioned before count comparison.'})
report={'status':'SOURCE_MANIFEST_REPLACEMENT_AUDIT_NATIVE_UNVERIFIED','manifest_path':str(MANIFEST),'manifest_sha256':hashlib.sha256(MANIFEST.read_bytes()).hexdigest(),
        'legacy_geometry_root':str(GEO),'whole_hide_nodes':whole_rows,'partial_mesh_cuts':partial,'never_hide_parents':['world.site.group','world.site.st','world.site.interior','world.site.inSt','world.site.objs.hutDoorLeaf'],
        'retain_geometry':[704,708,711,724,725,726,727,728,732,733,734,735,736,737,738,739,740,741,742],
        'retain_extra_nodes':[n['i'] for n in d['nodes'] if n.get('type')=='Mesh' and n.get('geo') in [704,708,711,724,725,726,727,728,732,733,734,735,736,737,738,739,740,741,742]],
        'all_gameplay_proxies_retained':True,'collision_policy':'Old hut floor collider, hutBounds and all original gameplay colliders stay. The old floor visual g717 is retired only because matching .004-m floorboards are supplied.',
        'scope':'Hut architecture and static furniture only; instruments are a separately owned replacement. Material IDs are from the CORE manifest, not guessed from material names.'}
(R/'runtime_replacement.json').write_text(json.dumps(report,indent=2)+'\n')
print('FIELDSTATION_LEGACY_REPLACEMENT_AUDIT',len(whole_rows),'whole',len(partial),'partial',sum(x['triangles'] for x in whole_rows),'whole triangles',sum(x['expected_removed_triangles'] for x in partial),'partial triangles')
for row in partial:print('g'+str(row['geo']),row['expected_removed_triangles'],'/',row['expected_original_triangles'],'box matches',row['box_match_counts'])
