"""Read a freshly opened PhotoLab .blend; no source or export mutation.
Use: blender -b ArtSource/PhotoLab/Signal47_PhotoLab_Detail.blend --python this.py
"""
from pathlib import Path
import hashlib
import json
import traceback
import bpy
from mathutils import Vector
from mathutils.bvhtree import BVHTree

ROOT = Path(__file__).resolve().parents[3] / 'ArtSource/PhotoLab'
REPORT = ROOT / 'source-verification.json'
report = {'status':'RUNNING', 'scope':'Fresh saved Blender geometry, material files and physical light apertures',
          'checks':[], 'meshes':[], 'apertures':[], 'native_tested':False}

def flush():
    REPORT.write_text(json.dumps(report, indent=2)+'\n')

def check(name, ok, detail=None):
    report['checks'].append({'name':name, 'pass':bool(ok), 'detail':detail})
    flush()
    assert ok, (name, detail)

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def xyz(p):
    return Vector((p[0], -p[2], p[1]))

try:
    contract = json.loads((ROOT/'model_contract.json').read_text())
    rows = json.loads((ROOT/'materials.json').read_text())
    report['source_sha256'] = sha(ROOT/'Signal47_PhotoLab_Detail.blend')
    report['contract_sha256'] = sha(ROOT/'model_contract.json')
    check('Ten additive meshes, no removal contract',len(contract['parts'])==10 and contract['legacy_removal_bounds']==[])
    check('Correct blend reopened',Path(bpy.data.filepath).resolve()==(ROOT/'Signal47_PhotoLab_Detail.blend').resolve())
    trees = []
    for name, row in contract['parts'].items():
        ob = bpy.data.objects.get(name)
        assert ob and ob.type=='MESH', name
        bounds = [[float(fn(v.co[k] for v in ob.data.vertices)) for k in range(3)] for fn in (min,max)]
        err = max(abs(a-b) for actual,want in zip(bounds,row['bounds_local_m']) for a,b in zip(actual,want))
        geometry_ok = (len(ob.data.polygons)==row['triangles'] and all(len(p.vertices)==3 and p.area>1e-12 for p in ob.data.polygons)
                       and bool(ob.data.uv_layers) and err<1e-5 and all(abs(v)<1e-6 for v in ob.location))
        fbx = ROOT/'Export'/(name+'.fbx')
        check(name+' geometry/UVs/bounds/pivot/export',geometry_ok and fbx.is_file() and fbx.stat().st_size>1000,
              {'triangles':len(ob.data.polygons),'bounds_error_m':err})
        check(name+' exact material slots',set(m.name for m in ob.data.materials)==set(row['materials']))
        tree=BVHTree.FromPolygons([ob.matrix_world@v.co for v in ob.data.vertices],
                                 [tuple(p.vertices) for p in ob.data.polygons],all_triangles=True)
        trees.append((name,tree))
        report['meshes'].append({'name':name,'triangles':len(ob.data.polygons),'bounds_blender_m':bounds,'fbx_sha256':sha(fbx)})
    total=sum(m['triangles'] for m in report['meshes'])
    check('Actual total detail budget',0<total<80000 and total==contract['triangles_lod0'],total)
    texture_hashes={}
    for row in rows:
        mat=bpy.data.materials.get(row['name'])
        assert mat and mat.use_nodes,row['name']
        bsdf=mat.node_tree.nodes.get('Principled BSDF')
        no_glow=(bsdf and not bsdf.inputs['Emission Color'].is_linked and
                 (bsdf.inputs['Emission Strength'].default_value==0 or max(bsdf.inputs['Emission Color'].default_value[:3])==0))
        check(row['name']+' nonemissive PBR',bool(no_glow) and row.get('emission_strength',0)==0)
        tex_nodes=[n for n in mat.node_tree.nodes if n.type=='TEX_IMAGE']
        wanted={str((ROOT/p).resolve()) for p in row['textures'].values()}
        actual={str(Path(bpy.path.abspath(n.image.filepath)).resolve()) for n in tex_nodes if n.image}
        check(row['name']+' packed and portable PBR images',actual==wanted and all(n.image.packed_file for n in tex_nodes)
              and all(Path(p).is_file() for p in wanted))
        for path in wanted:texture_hashes[str(Path(path).relative_to(ROOT))]=sha(Path(path))
    report['texture_sha256']=texture_hashes
    # Five rays per opening, grouped into one meaningful check. Test only NEW exported geometry.
    # White/red glass remains old runtime geometry and is deliberately absent here.
    openings=[('enlarger emission', (17,1.49,-5.45),(17,1.04,-5.45),'Y',.020),
              ('wall diffuser', (15,2.43,-5.55),(15,2.43,-5.40),'Z',.035),
              ('ceiling diffuser',(15.9,2.638,-2.4),(15.9,2.43,-2.4),'Y',.06),
              ('sink open bowl',(17.78,1.00,-5.44),(17.78,.79,-5.44),'Y',.07)]
    for name,start,end,axis,off in openings:
        hits=[]
        for u,v in ((0,0),(off,0),(-off,0),(0,off),(0,-off)):
            delta=Vector((u,0,v)) if axis=='Y' else Vector((u,v,0))
            a,b=xyz(Vector(start)+delta),xyz(Vector(end)+delta)
            direction=b-a
            for mesh,tree in trees:
                location,normal,index,distance=tree.ray_cast(a,direction.normalized(),direction.length)
                if location is not None:hits.append({'mesh':mesh,'ray_offset':[u,v],'distance_m':distance})
        check(name+' free aperture',not hits,hits)
        report['apertures'].append({'name':name,'start_three_m':start,'end_three_m':end,'offset_axis':axis,'offset_m':off,'rays':5,'clear':not hits})
    check('No export lights',all(o.type=='MESH' for o in bpy.data.collections['02_NATIVE_EXPORTS_HIDDEN'].objects))
    report['status']='PASS'
    report['limitations']='Source checks only. Native LOD apertures, lighting modes, collision placement and final game images need actual engine review.'
    flush()
    print('PHOTOLAB SOURCE VERIFY PASS',len(report['meshes']),'parts;',total,'triangles')
except Exception:
    report['status']='FAIL';report['error']=traceback.format_exc();flush();raise
