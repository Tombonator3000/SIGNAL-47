from pathlib import Path
import hashlib,json,subprocess,sys,textwrap
base=Path(__file__).resolve().parent;source=base/'iteration2/workstation-probe.blend'
code=textwrap.dedent('''
import bpy, os
from mathutils import Vector
paths=bpy.utils.blend_paths(absolute=True,packed=False)
missing=[p for p in paths if not os.path.exists(p)]
assert not missing,missing
fixture=bpy.data.images.new('MCP missing dependency probe',width=1,height=1)
fixture.source='FILE';fixture.filepath=MISSING_PATH
assert MISSING_PATH in bpy.utils.blend_paths(absolute=True,packed=False)
assert not os.path.exists(MISSING_PATH)
bpy.data.images.remove(fixture)
properties=bpy.ops.export_scene.fbx.get_rna_type().properties
api={name:{'type':properties[name].type,'default':properties[name].default,'enum': [v.identifier for v in properties[name].enum_items] if properties[name].type=='ENUM' else None} for name in ['axis_forward','axis_up','use_selection','apply_unit_scale']}
assert '-Z' in api['axis_forward']['enum'] and 'Y' in api['axis_up']['enum']
objects=[o for o in bpy.context.scene.objects if o.type=='MESH']
def metrics(items):
    bpy.context.view_layer.update()
    points=[o.matrix_world@Vector(v) for o in items for v in o.bound_box]
    tris=0
    for o in items:o.data.calc_loop_triangles();tris+=len(o.data.loop_triangles)
    return {'meshes':len(items),'triangles':tris,'bounds':[max(v[i] for v in points)-min(v[i] for v in points) for i in range(3)]}
before=metrics(objects)
bpy.ops.object.select_all(action='DESELECT')
for o in objects:o.select_set(True)
bpy.context.view_layer.objects.active=objects[0]
bpy.ops.export_scene.fbx(filepath=FBX_PATH,use_selection=True,object_types={'MESH'},add_leaf_bones=False,bake_anim=False,axis_forward='-Z',axis_up='Y',apply_unit_scale=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=FBX_PATH)
after=metrics(list(bpy.context.selected_objects))
assert before['meshes']==after['meshes'] and before['triangles']==after['triangles']
assert all(abs(a-b)<1e-5 for a,b in zip(before['bounds'],after['bounds'])),(before,after)
result={'dependency_paths':paths,'missing':missing,'injected_missing_image_detected':True,'fbx_api':api,'fbx_before':before,'fbx_after':after,'fbx_roundtrip':'PASS'}
''').replace('MISSING_PATH',repr(str(base/'intentionally-missing.png'))).replace('FBX_PATH',repr(str(base/'workstation-probe.fbx')))
request=base/'compatibility.request.json';request.write_text(json.dumps({'tool':'execute_blender_code_for_cli','arguments':{'blend_file':str(source),'code':code}},indent=2))
cmd=[sys.executable,'/home/tombonator3000t/.codex/skills/blender-mcp/scripts/mcp_cli.py','--server','/home/tombonator3000t/signal47-tools/blender-mcp-env/bin/blender-mcp','--blender','/home/tombonator3000t/signal47-tools/blender-4.5.13-linux-x64/blender','--request',str(request),'--output',str(base/'compatibility.json')]
subprocess.run(cmd,check=True,timeout=170)
r=json.loads((base/'compatibility.json').read_text())['response']['structuredContent'];assert r['fbx_roundtrip']=='PASS'
print(json.dumps(r,indent=2))
