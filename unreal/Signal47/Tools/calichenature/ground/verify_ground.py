"""Inspect the saved editable source and actual GLB; never overwrites assets."""
from pathlib import Path
import hashlib
import json
import math
import struct
import sys
import bpy
from mathutils import Vector

ROOT=Path(__file__).resolve().parent
sys.path.insert(0,str(ROOT))
from terrain_contract import grid_y,visual_y,box,TOWN


def main():
    source=ROOT/'Caliche_Nature_Ground.blend'
    before=hashlib.sha256(source.read_bytes()).hexdigest()
    contract=json.loads((ROOT/'model_contract.json').read_text())
    checks=[]
    def check(name,value,detail=None):
        checks.append({'name':name,'pass':bool(value),'detail':detail})
        if not value:
            (ROOT/'verification.json').write_text(json.dumps({'status':'FAIL','checks':checks},indent=2)+'\n')
            raise AssertionError((name,detail))
    bpy.ops.wm.open_mainfile(filepath=str(source))
    objects={o.name:o for o in bpy.data.objects if o.type=='MESH'}
    check('Complete original terrain tile set',set(objects)==set(contract['parts']))
    total=0
    for name,part in contract['parts'].items():
        o=objects[name];mesh=o.data;total+=len(mesh.polygons)
        check(name+' actual triangles',len(mesh.polygons)==part['triangles'] and all(len(p.vertices)==3 for p in mesh.polygons))
        check(name+' metre origin',o.location.length<1e-8 and all(abs(s-1)<1e-8 for s in o.scale))
        check(name+' nondegenerate upward faces',all(p.area>5e-9 and p.normal.z>.8 for p in mesh.polygons))
        check(name+' actual material',[m.name for m in mesh.materials]==part['materials'])
        color=mesh.color_attributes.get('Color')
        check(name+' real point RGBA',color is not None and color.domain=='POINT' and len(color.data)==len(mesh.vertices))
        values=[tuple(v.color) for v in color.data]
        check(name+' finite bounded color',all(math.isfinite(v) and -.00001<=v<=1.00001 for rgba in values for v in rgba))
        check(name+' actual nonwhite RGB',any(min(v[:3])<.98 for v in values))
        check(name+' actual boundary alpha',abs(min(v[3] for v in values)-part['vertex_alpha_min'])<.00001 and abs(max(v[3] for v in values)-part['vertex_alpha_max'])<.00001)
        errors=[]
        for p in mesh.vertices:
            x,z=p.co.x,2060-p.co.y
            h=visual_y(x,z) if 'Shoulder' in name or box(x,z,TOWN) else grid_y(x,z)
            errors.append(abs(p.co.z-h-.016))
        check(name+' unchanged drawn height contract',max(errors)<.00015,max(errors))
    check('Measured total terrain budget',total==contract['total_triangles'] and total<300000,total)
    for im in bpy.data.images:
        if im.source!='FILE':continue
        path=Path(bpy.path.abspath(im.filepath))
        check(im.name+' portable existing image',im.filepath.startswith('//') and path.is_file(),im.filepath)
        check(im.name+' valid pixels',min(im.size)>0,list(im.size))

    binary=(ROOT/'exports/Caliche_Nature_Ground.glb').read_bytes()
    length=struct.unpack_from('<I',binary,12)[0]
    data=json.loads(binary[20:20+length]);binstart=28+length
    def rgba(accessor):
        a=data['accessors'][accessor];v=data['bufferViews'][a['bufferView']]
        check('GLB RGBA accessor',a['type']=='VEC4',a)
        fmt={5126:'f',5121:'B',5123:'H'}[a['componentType']]
        scale=255 if a['componentType']==5121 else 65535 if a['componentType']==5123 else 1
        stride=v.get('byteStride',struct.calcsize('<'+fmt*4));start=binstart+v.get('byteOffset',0)+a.get('byteOffset',0)
        return [tuple(c/scale for c in struct.unpack_from('<'+fmt*4,binary,start+i*stride)) for i in range(a['count'])]
    glb_names=set()
    for mesh in data['meshes']:
        glb_names.add(mesh['name']);colors=[]
        for primitive in mesh['primitives']:
            check(mesh['name']+' GLB COLOR_0 present','COLOR_0' in primitive['attributes'])
            colors.extend(rgba(primitive['attributes']['COLOR_0']))
        expected=contract['parts'][mesh['name']]
        check(mesh['name']+' GLB boundary alpha preserved',abs(min(c[3] for c in colors)-expected['vertex_alpha_min'])<.0041 and abs(max(c[3] for c in colors)-expected['vertex_alpha_max'])<.0041)
    check('All source tiles in GLB assembly',glb_names==set(objects))
    check('Saved source bytes unchanged',hashlib.sha256(source.read_bytes()).hexdigest()==before)
    result={'status':'PASS','source_blend_sha256':before,'checks':checks,'passed':len(checks),'total':len(checks),
            'native_status':'UNVERIFIED until separate actual UE import and render-buffer verification'}
    (ROOT/'verification.json').write_text(json.dumps(result,indent=2)+'\n')
    print('NATURE_GROUND_SOURCE_PASS',len(checks),total)


if __name__=='__main__':main()
