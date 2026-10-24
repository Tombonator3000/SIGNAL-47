"""Re-export only GLB from the saved blend; preserve FBX/blend/contracts bytes.

Run blender -b Caliche_Ground.blend --threads 2 --python refresh_glb.py.
"""
import bpy
import hashlib
import json
from pathlib import Path
import sys

ROOT=Path(__file__).resolve().parent;sys.path.insert(0,str(ROOT))
from gltf_export import export_glb
from verify_glb import verify

paths=list((ROOT/'exports').glob('*.fbx'))+[ROOT/'Caliche_Ground.blend',ROOT/'model_contract.json',ROOT/'materials.json']
def hashes():return {str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}
before=hashes();reference={}
objects=[o for o in bpy.context.scene.objects if o.type=='MESH' and o.name.startswith('SM_Caliche_')]
for ob in objects:
    color=ob.data.color_attributes['Color'];reference[ob.name]={}
    assert color.domain=='POINT'
    for vertex,c in zip(ob.data.vertices,color.data):
        assert all(abs(v-1)<1e-6 for v in c.color[:3]),'GLB RGB multiply must remain neutral'
        key=tuple(round(x,6) for x in vertex.co)
        reference[ob.name].setdefault(key,set()).add(c.color[3])
    bpy.ops.object.select_all(action='DESELECT');ob.select_set(True);bpy.context.view_layer.objects.active=ob
    export_glb(ROOT/'exports'/(ob.name+'.glb'))
bpy.ops.object.select_all(action='DESELECT')
for ob in objects:ob.select_set(True)
export_glb(ROOT/'exports/Caliche_Ground_Assembly.glb')
report=verify(ROOT,reference)
(ROOT/'glb_verification.json').write_text(json.dumps(report,indent=2)+'\n')
after=hashes();assert before==after,'FBX/blend/contract/material sidecar bytes changed'
(ROOT/'glb_refresh.json').write_text(json.dumps({'status':'PASS','unchanged_sha256':after,'GLB_only':True},indent=2)+'\n')
print('GLB_RGBA_REFRESH_PASS',len(report['files']),'files; source alpha matches, FBX/blend/contracts bytes unchanged')
