"""Root runs Blender under lease. Original GLB untouched; exports two exact derivatives."""
import bpy,bmesh,hashlib,json,os,sys
from pathlib import Path
from mathutils import Matrix
HERE=Path(__file__).resolve().parent
sys.path.insert(0,str(HERE))
import legacy_access_recipe as R
SOURCE=Path('/home/tom/scratch-bm/Signal47-web/geo/geo_003.glb')
OUT=Path(os.environ['S47_EASTWING_ACCESS_OUT']).resolve()
assert not OUT.is_relative_to(HERE)
assert hashlib.sha256(SOURCE.read_bytes()).hexdigest()==R.SOURCE_SHA
OUT.mkdir(parents=True,exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.context.scene.unit_settings.system='METRIC'
bpy.context.scene.unit_settings.scale_length=1.
bpy.ops.import_scene.gltf(filepath=str(SOURCE))
contract={'source_sha256':R.SOURCE_SHA,'delta_three_m':[R.DX,0,0],
          'old_walking_feet':R.FEET,'new_walking_feet':[[r[0]+R.DX,r[1]+R.DX,r[2],r[3]] for r in R.FEET],
          'parts':[]}
DOWNPIPE,DOWNPIPE_STAIN=72,4  # east-wall rain leader and its streak (legacy_access_recipe.downpipe*)
for original,name,total,expected in [('g454','SM_EW_AccessDressing',13550,10328-DOWNPIPE),('g455','SM_EW_AccessStains',1712,1233)]:
    obj=bpy.data.objects[original]
    world=obj.matrix_world.copy();obj.parent=None;obj.matrix_world=Matrix.Identity(4);obj.data.transform(world)
    bm=bmesh.new();bm.from_mesh(obj.data);uv=bm.loops.layers.uv.active
    assert len(bm.faces)==total and uv is not None
    def point(v):return (float(v.co.x),float(v.co.z),float(-v.co.y))
    def signature(f):return (f.material_index,tuple((tuple(v.co),tuple(loop[uv].uv)) for v,loop in zip(f.verts,f.loops)))
    deleted=[];counts={};moving=[[],[],[]];unchanged=[];clipped=[]
    colors=list(bm.loops.layers.color.values())+list(bm.loops.layers.float_color.values())
    for face in bm.faces:
        assert len(face.verts)==3
        points=[point(v) for v in face.verts]
        # glTF TEXCOORD V points down; Blender importer flips V to 1-V.
        # Select in immutable raw-glTF UV space; preserve actual Blender UV untouched.
        source_uv=[(float(l[uv].uv.x),1.-float(l[uv].uv.y)) for l in face.loops]
        reason=R.removed(original,points,source_uv)
        selected=R.moved(original,points)
        assert reason is None or selected is None
        if reason:counts[reason]=counts.get(reason,0)+1;deleted.append(face)
        elif selected is not None:moving[selected].append(face)
        else:
            pieces=R.service_clip(original,points)
            if pieces is None:unchanged.append(face)
            else:clipped.append((face,pieces))
    assert counts==({'cars':976,'windows':1872,'plants':230,'wheel_remnants':144,'downpipe':DOWNPIPE} if original=='g454' else {'grime':488,'downpipe_stain':DOWNPIPE_STAIN}),counts
    assert [len(g) for g in moving]==([216]*3 if original=='g454' else [2]*3)
    vertices={v for group in moving for f in group for v in f.verts}
    assert not any(v in vertices for f in unchanged for v in f.verts),'Moved vertices shared with unrelated triangles'
    before=[signature(f) for f in unchanged]
    assert (len(clipped),sum(len(t) for _,t in clipped))==((12,25) if original=='g455' else (0,0))
    for face,triangles in clipped:
        source_uv=[tuple(loop[uv].uv) for loop in face.loops]
        source_colors={layer:[tuple(loop[layer]) for loop in face.loops] for layer in colors}
        for triangle in triangles:
            # Recipe uses glTF x/y/z; Blender imported x/-z/y. Preserve winding.
            vertices_new=[bm.verts.new((p[0],-p[2],p[1])) for p,_ in triangle]
            added=bm.faces.new(vertices_new);added.material_index=face.material_index;added.smooth=face.smooth
            for loop,(_,weights) in zip(added.loops,triangle):
                loop[uv].uv=tuple(sum(weights[i]*source_uv[i][k] for i in range(3)) for k in range(2))
                for layer,values in source_colors.items():
                    loop[layer]=tuple(sum(weights[i]*values[i][k] for i in range(3)) for k in range(4))
        deleted.append(face)
    # Translation preserves normals, all loop UVs, materials, winding and topology.
    for v in vertices:v.co.x+=R.DX
    assert [signature(f) for f in unchanged]==before
    bmesh.ops.delete(bm,geom=deleted,context='FACES')
    assert len(bm.faces)==expected
    bm.to_mesh(obj.data);bm.free();obj.name=name
    bpy.ops.object.select_all(action='DESELECT');obj.select_set(True);bpy.context.view_layer.objects.active=obj
    assert len(obj.data.materials)==1
    points=[tuple(obj.data.vertices[i].co) for p in obj.data.polygons for i in p.vertices]
    bounds=[[fn(p[k] for p in points) for k in range(3)] for fn in (min,max)]
    files=[]
    for lod in range(3):
        filename=name+('' if lod==0 else '_LOD'+str(lod))+'.fbx'
        bpy.ops.export_scene.fbx(filepath=str(OUT/filename),use_selection=True,object_types={'MESH'},
            axis_forward='-Y',axis_up='Z',apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',
            use_mesh_modifiers=True,mesh_smooth_type='FACE',use_triangles=True,add_leaf_bones=False,bake_anim=False,path_mode='AUTO')
        files.append({'file':filename,'sha256':hashlib.sha256((OUT/filename).read_bytes()).hexdigest(),'triangles':expected})
    contract['parts'].append({'name':name,'source_triangles':total,'triangles':expected,'removed':counts,
        'moved_triangles':[len(g) for g in moving],'unchanged_triangles':len(before),
        'service_opening':list(R.SERVICE_OPENING),'clipped_triangles':len(clipped),'replacement_triangles':sum(len(t) for _,t in clipped),
        'unchanged_signature_sha256':hashlib.sha256(json.dumps(before).encode()).hexdigest(),
        'bounds_blender_m':bounds,'lods':files})
(OUT/'access-manifest.json').write_text(json.dumps(contract,indent=2)+'\n')
print('PASS_ACCESS_SOURCE',OUT)
