"""Extract real Poly Haven Grass Medium02 blade islands into restrained CC0 tufts.

No Geometry Nodes evaluation or generated cards. UV corners come unchanged
from source faces. Whole blade islands are layered to give crowns visible volume;
atlas UVs are never simplified across unrelated blades.
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
# Layer intact source blades at distinct height, yaw and crown offsets.
# Every layer uses real source geometry and its original atlas UV corners.
recipes={
 'A':[('grass_medium_02_a',.20,800,0,0,0,1.15),('grass_medium_02_a',.17,800,117,.018,0,1.35),('grass_medium_02_a',.13,800,243,-.015,.012,1.5)],
 'B':[('grass_medium_02_c',.30,1300,0,0,0,1.05),('grass_medium_02_a',.22,800,91,.024,.015,1.45),('grass_medium_02_a',.16,540,218,-.025,-.016,1.65)],
 'C':[('grass_medium_02_e',.45,1750,0,0,0,1.1),('grass_medium_02_a',.29,800,139,.035,-.024,1.75)]}
for label,layers in recipes.items():
    verts=[];face_indices=[];face_uv=[];face_smooth=[];layer_reports=[]
    for li,(srcname,height,budget,yaw,offsetx,offsety,spread) in enumerate(layers):
        mesh=bpy.data.objects[srcname].data
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
        groups=[[] for _ in components]
        for face in mesh.polygons:groups[island_for[face.vertices[0]]].append(face)
        order=list(range(len(components)));random.Random(470+ord(label)+li*31).shuffle(order)
        tallest=max(order,key=lambda i:max(mesh.vertices[v].co.z for v in components[i]));order.remove(tallest);order.insert(0,tallest)
        chosen=[];tris=0
        for i in order:
            count=sum(len(f.vertices)-2 for f in groups[i])
            if tris+count<=budget:chosen.append(i);tris+=count
        faces=[f for i in chosen for f in groups[i]]
        used=sorted({v for f in faces for v in f.vertices});indices={v:i+len(verts) for i,v in enumerate(used)}
        zmin=min(mesh.vertices[i].co.z for i in used);zmax=max(mesh.vertices[i].co.z for i in used)
        scale=height/(zmax-zmin);angle=math.radians(yaw);ca=math.cos(angle);sa=math.sin(angle)
        for i in used:
            co=mesh.vertices[i].co;x=co.x*scale*spread;y=co.y*scale*spread
            verts.append((ca*x-sa*y+offsetx,sa*x+ca*y+offsety,(co.z-zmin)*scale))
        for f in faces:
            face_indices.append([indices[v] for v in f.vertices]);face_uv.append([tuple(mesh.uv_layers.active.data[i].uv) for i in f.loop_indices]);face_smooth.append(f.use_smooth)
        layer_reports.append({'source_object':srcname,'blade_islands':len(chosen),'triangles':tris,'height_m':height,'yaw_degrees':yaw,'radial_spread':spread,'crown_offset_m':[offsetx,offsety]})
    m=bpy.data.meshes.new('V10_DryTuft_'+label);m.from_pydata(verts,[],face_indices);m.update();uv=m.uv_layers.new(name='UVMap')
    for poly,corners,smooth in zip(m.polygons,face_uv,face_smooth):
        poly.use_smooth=smooth
        for dst,co in zip(poly.loop_indices,corners):uv.data[dst].uv=co
    ob=bpy.data.objects.new('V10_DryTuft_'+label,m);bpy.context.scene.collection.objects.link(ob);m.materials.append(mat);objects.append(ob)
    tris=sum(len(p.vertices)-2 for p in m.polygons);assert tris<2500
    results.append({'name':ob.name,'source_layers':layer_reports,'triangles':tris,'uv_corners_copied_exactly':all((uv.data[d].uv-Vector(co)).length<1e-8 for poly,corners in zip(m.polygons,face_uv) for d,co in zip(poly.loop_indices,corners)),'dimensions_blender_m':[max(v[i] for v in verts)-min(v[i] for v in verts) for i in range(3)],'floor_z_m':min(v[2] for v in verts)})
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
report={'blender':bpy.app.version_string,'source':'https://polyhaven.com/a/grass_medium_02','author':'Rico Cilliers','license':'CC0-1.0','source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'method':'Layered actual source blade islands at varied height/yaw/radial spread, exact UV corner copying, floor pivot','geometry_nodes_evaluated':False,'material':'V10_DryGrass_Atlas','assets':results}
(OUT/'dry-tuft-verification.json').write_text(json.dumps(report,indent=2)+'\n')
# License/source notice maintained by integration; source and CC0 unchanged.
print('VISUAL10_DRY_TUFTS '+json.dumps(report))
