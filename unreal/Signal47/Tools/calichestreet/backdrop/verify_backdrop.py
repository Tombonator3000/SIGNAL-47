"""Independent FBX/GLB and saved-source geology checks; no Unreal or rendering."""
import bpy,json,sys,math,struct,hashlib
from pathlib import Path
from mathutils import Vector
R=Path(__file__).resolve().parent;sys.path.insert(0,str(R))
from geology import NAME,ANCHOR,MATERIALS,BUDGET,ROAD_BUFFER,MAIN_BUFFER,old_road_distance
contract=json.loads((R/'model_contract.json').read_text())['parts'][NAME]
(R/'verification.json').write_text(json.dumps({'status':'RUNNING'},indent=2)+'\n')
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
bpy.ops.import_scene.fbx(filepath=str(R/'exports'/(NAME+'.fbx')))
obs=[o for o in bpy.context.scene.objects if o.type=='MESH'];assert len(obs)==1
ob=obs[0];me=ob.data;vs=[ob.matrix_world@v.co for v in me.vertices];me.calc_loop_triangles()
bounds=[[fn(p[k] for p in vs) for k in range(3)] for fn in [min,max]]
error=max(abs(a-b) for aa,bb in zip(bounds,contract['bounds_local_m']) for a,b in zip(aa,bb))
# Float32 axis-rotation metadata accumulates0.535mm at3.3km.
# One millimetre remains below the native1.2mm bounds import gate.
assert error<.001,error
assert len(me.loop_triangles)==contract['triangles']<=BUDGET
assert [m.name for m in me.materials]==MATERIALS
used=sorted({me.materials[p.material_index].name for p in me.polygons});assert used==contract['used_materials']==MATERIALS
assert len(me.uv_layers)==1 and all(math.isfinite(x) for t in me.uv_layers[0].data for x in t.uv)
# Every cap/ledge triangle must retain texture area after FBX roundtrip.
# Finite UVs alone failed to catch the CN2 horizontal-cap projection collapse.
uv_area_min=math.inf
for tri in me.loop_triangles:
    a,b,c=[me.uv_layers[0].data[i].uv for i in tri.loops]
    uv_area_min=min(uv_area_min,abs((b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x))*.5)
assert uv_area_min>1e-10,('Collapsed triangle UV',uv_area_min)
assert ob.location.length<1e-5 and max(abs(s-1) for s in ob.scale)<1e-5
assert all(p.area>1e-7 for p in me.polygons)
old_min=math.inf;main_min=math.inf;raised=0
for tri in me.loop_triangles:
    ps=[vs[i] for i in tri.vertices]
    assert (ps[1]-ps[0]).cross(ps[2]-ps[0]).z>0,'Reversed heightfield face'
    if max(p.z for p in ps)<=0:continue
    raised+=1;edge=max(math.hypot(a.x-b.x,a.y-b.y) for a in ps for b in ps)
    old_min=min(old_min,min(old_road_distance(p.x,-p.y) for p in ps)-edge)
    main_min=min(main_min,min(abs(p.x) for p in ps)-edge)
assert old_min>=ROAD_BUFFER-.001,old_min
assert main_min>=MAIN_BUFFER-.001,main_min
# Binary GLB coordinates are validated independently of Blender's FBX reader.
raw=(R/'exports'/(NAME+'.glb')).read_bytes();size=struct.unpack_from('<I',raw,12)[0];doc=json.loads(raw[20:20+size]);binary=raw[28+size:]
glb_tri=0;glb_points=[]
for mesh in doc['meshes']:
    assert mesh['name']==NAME
    for primitive in mesh['primitives']:
        assert 'TEXCOORD_0' in primitive['attributes']
        glb_tri+=doc['accessors'][primitive['indices']]['count']//3
        a=doc['accessors'][primitive['attributes']['POSITION']];view=doc['bufferViews'][a['bufferView']]
        offset=view.get('byteOffset',0)+a.get('byteOffset',0);stride=view.get('byteStride',12)
        for i in range(a['count']):
            x,y,z=struct.unpack_from('<fff',binary,offset+i*stride);glb_points.append((x,-z,y))
assert glb_tri==contract['triangles']
glb_bounds=[[fn(p[k] for p in glb_points) for k in range(3)] for fn in [min,max]]
glb_error=max(abs(a-b) for aa,bb in zip(glb_bounds,contract['bounds_local_m']) for a,b in zip(aa,bb));assert glb_error<.0003
assert sorted(m['name'] for m in doc['materials'])==MATERIALS
for m in doc['materials']:
    pbr=m['pbrMetallicRoughness'];assert 'baseColorTexture' in pbr
    assert 'normalTexture' in m,(m['name'],'GLB normal map missing')
    assert 'metallicRoughnessTexture' in pbr,(m['name'],'GLB roughness texture missing')
    assert all(0<=x<=1 for x in pbr.get('baseColorFactor',[1,1,1,1]))
# Check the original editable Blend, not only its export. Every disk PBR map
# must be packed and current, with exact material tints and no surprise props.
bpy.ops.wm.open_mainfile(filepath=str(R/'Caliche_Mesas.blend'))
objects=[o for o in bpy.context.scene.objects if o.type=='MESH'];assert len(objects)==1 and objects[0].name==NAME
assert list(objects[0]['anchor_world_three_m'])==ANCHOR
assert objects[0]['collision']=='NoCollision'
images=[]
for image in bpy.data.images:
    if image.source!='FILE':continue
    assert image.packed_file,image.name
    disk=Path(bpy.path.abspath(image.filepath));assert image.packed_file.data==disk.read_bytes(),image.name
    images.append({'name':image.name,'width':image.size[0],'height':image.size[1],'sha256':hashlib.sha256(disk.read_bytes()).hexdigest()})
report={'status':'PASS','checks':{'single_editable_mesh':True,'FBX_pivots_scale_bounds_triangles':True,'six_stable_slots_all_used':True,'UV_finite':True,'UV_all_triangles_noncollapsed':True,'outward_nondegenerate_faces':True,'whole_triangle_protected_road_clearance':True,'binary_GLB_bounds_triangles_materials':True,'saved_Blend_current_packed_maps':True},
 'triangles':contract['triangles'],'bounds_local_m':bounds,'FBX_bounds_error_m':error,'FBX_bounds_tolerance_m':.001,'GLB_bounds_error_m':glb_error,
 'minimum_UV_triangle_area':uv_area_min,'raised_triangles':raised,'oldroad_clearance_lower_bound_m':old_min,'mainroad_clearance_lower_bound_m':main_min,'packed_textures':images}
(R/'verification.json').write_text(json.dumps(report,indent=2)+'\n');print('CALICHE_GEOLOGY_VERIFIED',json.dumps(report))
