"""Read binary glTF RGBA values, not merely COLOR_0 attribute presence.

Uses the standard library; optional reference is exact Blender Color alpha
keyed by object name and local vertex position. No file is changed by inspect.
"""
import json
import math
from pathlib import Path
import struct


def inspect(path,contract,reference=None):
    raw=path.read_bytes();assert raw[:4]==b'glTF',path
    size,kind=struct.unpack_from('<II',raw,12);assert kind==0x4e4f534a,path
    doc=json.loads(raw[20:20+size]);binary=raw[28+size:]
    def accessor(index):
        a=doc['accessors'][index];view=doc['bufferViews'][a['bufferView']]
        code={5121:'B',5123:'H',5125:'I',5126:'f'}[a['componentType']]
        width={'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4}[a['type']]
        fmt='<'+code*width;stride=view.get('byteStride',struct.calcsize(fmt))
        start=view.get('byteOffset',0)+a.get('byteOffset',0)
        values=[struct.unpack_from(fmt,binary,start+i*stride) for i in range(a['count'])]
        if a.get('normalized'):
            maximum={5121:255,5123:65535,5125:4294967295}[a['componentType']]
            values=[tuple(x/maximum for x in v) for v in values]
        return values
    report=[]
    for mesh in doc['meshes']:
        name=mesh['name'];row=contract['parts'][name]
        for primitive in mesh['primitives']:
            material=doc['materials'][primitive['material']]['name']
            assert 'COLOR_0' in primitive['attributes'],(path,name,material)
            rgba=accessor(primitive['attributes']['COLOR_0'])
            assert all(len(v)==4 and all(math.isfinite(x) and 0<=x<=1 for x in v) for v in rgba),(path,'invalid RGBA')
            alpha=[v[3] for v in rgba];fade=material in row.get('edge_fade',{})
            if fade:
                assert min(alpha)<1e-5 and max(alpha)>.99999,(path,name,material,'lost vertex fade',min(alpha),max(alpha))
                assert any(.001<a<.999 for a in alpha),(path,name,material,'no intermediate alpha')
            else:assert min(alpha)>.99999,(path,name,material,'unexpected alpha on opaque vertices')
            error=None
            if reference is not None:
                positions=accessor(primitive['attributes']['POSITION']);errors=[]
                for p,color in zip(positions,rgba):
                    key=tuple(round(x,6) for x in (p[0],-p[2],p[1]))
                    expected=reference[name][key]
                    errors.append(min(abs(color[3]-a) for a in expected))
                error=max(errors,default=0)
                assert error<2e-5,(path,name,material,'wrong alpha at source vertex',error)
            report.append({'mesh':name,'material':material,'vertices':len(alpha),
                           'alpha_min':min(alpha),'alpha_max':max(alpha),
                           'partial_alpha_vertices':sum(.001<a<.999 for a in alpha),
                           'source_alpha_max_error':error})
    return {'file':path.name,'status':'PASS','primitives':report}


def verify(root,reference=None):
    root=Path(root);contract=json.loads((root/'model_contract.json').read_text())
    return {'status':'PASS','files':[inspect(path,contract,reference) for path in sorted((root/'exports').glob('*.glb'))]}


if __name__=='__main__':
    root=Path(__file__).resolve().parent
    report=verify(root);(root/'glb_verification.json').write_text(json.dumps(report,indent=2)+'\n')
    print('GLB_RGBA_VERIFIED',len(report['files']))
