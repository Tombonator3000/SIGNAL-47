"""Extract real Poly Haven Grass Medium02 blade islands into restrained CC0 tufts.

No Geometry Nodes evaluation or generated cards. UV corners come unchanged
from source faces. Densities are reduced by complete connected blade islands,
so this does not simplify atlas UVs across unrelated blades.
"""
import bpy, json, math, random, hashlib
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[2]
SRC=ROOT.parent/'Artifacts/Visual10Research/TerrainRefinement/grass_medium_02'
OUT=ROOT/'Assets/Signal47/Art/Visual10/Models'
source=SRC/'grass_medium_02_1k.blend'
assert hashlib.sha256(source.read_bytes()).hexdigest()=='fd53aafc0650f0e7d9a10bc95f669d396ea7eb746bf2ca5a14497e331410eff1'
bpy.ops.wm.open_mainfile(filepath=str(source))
# Only copy actual source meshes; no evaluation of the meadow modifier.
mat=bpy.data.materials.new('V10_DryGrass_Atlas');mat.use_nodes=True
nodes=mat.node_tree.nodes;bsdf=nodes.get('Principled BSDF')
for name,socket in [('grass_medium_02_dry_diff_1k.png','Base Color'),('grass_medium_02_alpha_1k.png','Alpha')]:
    im=bpy.data.images.load(str(SRC/'textures'/name));im.pack()
    if socket=='Alpha':im.colorspace_settings.name='Non-Color'
    n=nodes.new('ShaderNodeTexImage');n.image=im;n.name='Source '+socket
    mat.node_tree.links.new(n.outputs['Color'],bsdf.inputs[socket])
bsdf.inputs['Roughness'].default_value=.86
mat.surface_render_method='DITHERED'
results=[];objects=[]
for label,srcname,height,budget in [('A','grass_medium_02_a',.17,800),('B','grass_medium_02_c',.26,900),('C','grass_medium_02_e',.38,950)]:
    original=bpy.data.objects[srcname];mesh=original.data
    adjacency=[set() for _ in mesh.vertices]
    for edge in mesh.edges:
        a,b=edge.vertices;adjacency[a].add(b);adjacency[b].add(a)
    unseen=set(range(len(mesh.vertices)));components=[]
    while unseen:
        seed=min(unseen);stack=[seed];unseen.remove(seed);island={seed}
        while stack:
            current=stack.pop()
            for neighbor in adjacency[current]:
                if neighbor in unseen:unseen.remove(neighbor);island.add(neighbor);stack.append(neighbor)
        components.append(island)
    island_for={v:i for i,c in enumerate(components) for v in c}
    facegroups=[[] for _ in components]
    for face in mesh.polygons:facegroups[island_for[face.vertices[0]]].append(face)
    # Keep tallest island for original silhouette, then sample complete islands.
    order=list(range(len(components)));random.Random(470+ord(label)).shuffle(order)
    tallest=max(order,key=lambda i:max(mesh.vertices[v].co.z for v in components[i]))
    order.remove(tallest);order.insert(0,tallest)
    chosen=[];tris=0
    for i in order:
        count=sum(len(f.vertices)-2 for f in facegroups[i])
        if tris+count<=budget:chosen.append(i);tris+=count
    faces=[f for i in chosen for f in facegroups[i]]
    used=sorted({v for f in faces for v in f.vertices});indices={v:i for i,v in enumerate(used)}
    zmin=min(mesh.vertices[i].co.z for i in used);zmax=max(mesh.vertices[i].co.z for i in used)
    scale=height/(zmax-zmin)
    verts=[(mesh.vertices[i].co.x*scale,mesh.vertices[i].co.y*scale,(mesh.vertices[i].co.z-zmin)*scale) for i in used]
    m=bpy.data.meshes.new('V10_DryTuft_'+label);m.from_pydata(verts,[],[[indices[v] for v in f.vertices] for f in faces]);m.update()
    uv=m.uv_layers.new(name='UVMap')
    for p,f in zip(m.polygons,faces):
        p.use_smooth=f.use_smooth
        for dst,src in zip(p.loop_indices,f.loop_indices):uv.data[dst].uv=mesh.uv_layers.active.data[src].uv
    ob=bpy.data.objects.new('V10_DryTuft_'+label,m);bpy.context.scene.collection.objects.link(ob);m.materials.append(mat);objects.append(ob)
    results.append({'name':ob.name,'source_object':srcname,'source_triangles':sum(len(f.vertices)-2 for f in mesh.polygons),'source_blade_islands':len(components),'retained_blade_islands':len(chosen),'triangles':tris,'uv_corners_copied_exactly':all((uv.data[d].uv-mesh.uv_layers.active.data[s].uv).length<1e-8 for p,f in zip(m.polygons,faces) for d,s in zip(p.loop_indices,f.loop_indices)),'dimensions_blender_m':[max(v[i] for v in verts)-min(v[i] for v in verts) for i in range(3)],'floor_z_m':min(v[2] for v in verts)})
for ob in list(bpy.data.objects):
    if ob not in objects:bpy.data.objects.remove(ob,do_unlink=True)
bpy.context.scene.unit_settings.system='METRIC';bpy.context.scene.unit_settings.scale_length=1
bpy.context.preferences.filepaths.save_version=0
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'Blender/SIGNAL47_visual10_dry_tufts.blend'))
for ob,r in zip(objects,results):
    bpy.ops.object.select_all(action='DESELECT');ob.select_set(True);bpy.context.view_layer.objects.active=ob
    bpy.ops.export_scene.fbx(filepath=str(OUT/(ob.name+'.fbx')),use_selection=True,object_types={'MESH'},add_leaf_bones=False,bake_anim=False,axis_forward='-Z',axis_up='Y',apply_unit_scale=True,mesh_smooth_type='FACE',path_mode='STRIP')
for ob in list(bpy.data.objects):bpy.data.objects.remove(ob,do_unlink=True)
for r in results:
    bpy.ops.import_scene.fbx(filepath=str(OUT/(r['name']+'.fbx')))
    ob=next(o for o in bpy.context.selected_objects if o.type=='MESH');m=ob.data
    points=[ob.matrix_world@v.co for v in m.vertices]
    dims=[max(v[i] for v in points)-min(v[i] for v in points) for i in range(3)]
    r['roundtrip_scale_pass']=all(abs(x-y)<1e-5 for x,y in zip(dims,r['dimensions_blender_m']))
    r['roundtrip_uv_pass']=bool(m.uv_layers)
    r['roundtrip_unit_normals_pass']=all(abs(p.normal.length-1)<1e-5 for p in m.polygons)
    r['roundtrip_floor_pass']=abs(min(p.z for p in points))<1e-5
    assert all(r[k] for k in ('roundtrip_scale_pass','roundtrip_uv_pass','roundtrip_unit_normals_pass','roundtrip_floor_pass'))
    bpy.data.objects.remove(ob,do_unlink=True)
report={'blender':bpy.app.version_string,'source':'https://polyhaven.com/a/grass_medium_02','author':'Rico Cilliers','license':'CC0-1.0','source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'method':'Subsets of actual source connected blade islands, exact UV corner copying, uniform scale/floor pivot','geometry_nodes_evaluated':False,'material':'V10_DryGrass_Atlas','assets':results}
(OUT/'dry-tuft-verification.json').write_text(json.dumps(report,indent=2)+'\n')
(OUT/'DRY_TUFT_SOURCE_AND_LICENSE.md').write_text('''# Dry tuft source and license

Derived from **Grass Medium 02**, Rico Cilliers, Poly Haven:
https://polyhaven.com/a/grass_medium_02 . CC0-1.0:
https://polyhaven.com/license and https://creativecommons.org/publicdomain/zero/1.0/ .

Three reduced tufts retain complete actual source blade islands and unchanged
atlas UV corners. No meadow Geometry Nodes system is exported or evaluated.
Source manifest and original downloads: Artifacts/Visual10Research/TerrainRefinement/.
Source SHA256 is recorded in dry-tuft-verification.json. Rebuild with
Unity/Blender/Source/visual10_dry_tufts.py; editable output is
Unity/Blender/SIGNAL47_visual10_dry_tufts.blend, with dry color and alpha packed.

Unity material slot V10_DryGrass_Atlas requires source dry_diff RGB and separate
alpha combined into base-map alpha, alpha clipping (start at .4), double-sided
rendering and rough nonmetal shading. Preserve atlas UVs, use local scale 1,
floor pivot and Y-up import (-Z forward / Y up FBX). Heights are .17/.26/.38 m.
Use restrained small clumps, avoid dense carpet and verify alpha overdraw in
runtime. No claim of a specific native New Mexico plant species is made.

Blender roundtrip checks are not Unity or performance verification.
''')
print('VISUAL10_DRY_TUFTS '+json.dumps(report))
