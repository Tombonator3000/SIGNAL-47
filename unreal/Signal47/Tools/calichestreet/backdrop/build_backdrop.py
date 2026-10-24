"""Original continuous Caliche escarpments, talus and layered distant ridges.

Run texture_geology.py first, only in the granted heavy slot. Blender Z-up,
metres; export axis policy is intentionally unchanged from the accepted mesh.
"""
import bpy,json,sys,math
from pathlib import Path
from mathutils import Vector
R=Path(__file__).resolve().parent;sys.path.insert(0,str(R))
from geology import NAME,ANCHOR,MATERIALS,BUDGET,ROAD_BUFFER,MAIN_BUFFER,TOWN_BOX,OLD_ROAD,build_geometry,old_road_distance
OUT=R/'exports';OUT.mkdir(exist_ok=True)
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
bpy.context.scene.unit_settings.system='METRIC';bpy.context.scene.unit_settings.scale_length=1
rows=json.loads((R/'materials.json').read_text());assert [m['name'] for m in rows]==MATERIALS
mats=[]
for row in rows:
    m=bpy.data.materials.new(row['name']);m.use_nodes=True;bs=m.node_tree.nodes.get('Principled BSDF')
    bs.inputs['Roughness'].default_value=row['roughness'];bs.inputs['Metallic'].default_value=0
    bs.inputs['Specular IOR Level'].default_value=row.get('specular',.25)
    for role,filename in row['textures'].items():
        image=bpy.data.images.load(str(R/filename),check_existing=True)
        image.colorspace_settings.name='sRGB' if role=='base_color' else 'Non-Color'
        tex=m.node_tree.nodes.new('ShaderNodeTexImage');tex.image=image;tex.extension='REPEAT'
        if role=='base_color':
            tint=m.node_tree.nodes.new('ShaderNodeMix');tint.data_type='RGBA';tint.blend_type='MULTIPLY';tint.inputs[0].default_value=1
            tint.inputs[7].default_value=(*row['texture_tint_linear'],1)
            m.node_tree.links.new(tex.outputs['Color'],tint.inputs[6]);m.node_tree.links.new(tint.outputs[2],bs.inputs['Base Color'])
        elif role=='roughness':m.node_tree.links.new(tex.outputs['Color'],bs.inputs['Roughness'])
        else:
            sep=m.node_tree.nodes.new('ShaderNodeSeparateXYZ');inv=m.node_tree.nodes.new('ShaderNodeMath');inv.operation='SUBTRACT';inv.inputs[0].default_value=1
            combine=m.node_tree.nodes.new('ShaderNodeCombineXYZ');norm=m.node_tree.nodes.new('ShaderNodeNormalMap');norm.inputs['Strength'].default_value=row['normal_strength']
            m.node_tree.links.new(tex.outputs['Color'],sep.inputs[0]);m.node_tree.links.new(sep.outputs['X'],combine.inputs['X'])
            m.node_tree.links.new(sep.outputs['Y'],inv.inputs[1]);m.node_tree.links.new(inv.outputs[0],combine.inputs['Y']);m.node_tree.links.new(sep.outputs['Z'],combine.inputs['Z'])
            m.node_tree.links.new(combine.outputs[0],norm.inputs['Color']);m.node_tree.links.new(norm.outputs[0],bs.inputs['Normal'])
    mats.append(m)
v,f,slots,uv,bands,filtered=build_geometry()
mesh=bpy.data.meshes.new(NAME);mesh.from_pydata(v,[],f);mesh.update()
for mat in mats:mesh.materials.append(mat)
for poly,slot in zip(mesh.polygons,slots):poly.material_index=slot;poly.use_smooth=slot in [0,4]
layer=mesh.uv_layers.new(name='UVMap')
for poly,coords in zip(mesh.polygons,uv):
    for li,t in zip(poly.loop_indices,coords):layer.data[li].uv=t
ob=bpy.data.objects.new(NAME,mesh);bpy.context.collection.objects.link(ob);bpy.context.view_layer.objects.active=ob;ob.select_set(True)
mod=ob.modifiers.new('Triangles','TRIANGULATE');bpy.ops.object.modifier_apply(modifier=mod.name)
ob['anchor_world_three_m']=ANCHOR;ob['collision']='NoCollision';ob['original']='Original continuous geology, no third-party terrain or images'
ob['mapping']='worldThree=(8000+bx,bz,2060-by)'
coords=[tuple(p.co) for p in ob.data.vertices];triangles=len(ob.data.polygons);assert triangles<=BUDGET
bounds=[[fn(p[k] for p in coords) for k in range(3)] for fn in [min,max]]
row={'bounds_local_m':bounds,'triangles':triangles,'materials':MATERIALS,'used_materials':sorted({mats[p.material_index].name for p in ob.data.polygons})}
contract={'parts':{NAME:row}}
(R/'model_contract.json').write_text(json.dumps(contract,indent=2)+'\n')
# Whole-polygon clearance proof: any face above existing land has an exact
# nearest-road vertex distance minus its longest projected edge >=180m.
old_min=math.inf;main_min=math.inf;raised=0
for poly in ob.data.polygons:
    ps=[ob.data.vertices[i].co for i in poly.vertices]
    if max(p.z for p in ps)<=0:continue
    raised+=1;edge=max(math.hypot(ps[i].x-ps[(i+1)%3].x,ps[i].y-ps[(i+1)%3].y) for i in range(3))
    old_min=min(old_min,min(old_road_distance(p.x,-p.y) for p in ps)-edge)
    main_min=min(main_min,min(abs(p.x) for p in ps)-edge)
assert old_min>=ROAD_BUFFER-1e-4,(old_min,ROAD_BUFFER)
assert main_min>=MAIN_BUFFER-1e-4,(main_min,MAIN_BUFFER)
desc={'status':'SOURCE_GENERATED','anchor_world_three_m':ANCHOR,'mapping':ob['mapping'],'mesh':NAME,'collision':'NoCollision','bounds_local_m':bounds,'triangles':triangles,'budget_triangles':BUDGET,
 'geology_belts':bands,'protected':{'old_road_m':ROAD_BUFFER,'main_road_m':MAIN_BUFFER,'town_box_local_xz':TOWN_BOX,
 'whole_triangle_oldroad_clearance_lower_bound_m':old_min,'whole_triangle_mainroad_clearance_lower_bound_m':main_min,
 'raised_triangles':raised,'filtered_near_protected_quads':filtered,'old_road_path_source':'roadShape.ts/oldRoadLayout.ts reproduced4m midpoint/backward integration','old_road_first4500m_local_xz':OLD_ROAD},
 'materials':rows,'revision':'CN3 native-driven layered rock refinement','notes':'Rock faces are flat-shaded to retain bed breaks; talus remains smooth. Low toes are buried at-4m and blend into existing terrain. Main escarpments beyond700m, middle/far ridges provide actual parallax and depth. No road/town floor or collision changes.'}
(R/'geology_descriptor.json').write_text(json.dumps(desc,indent=2)+'\n')
for image in bpy.data.images:
    if image.source=='FILE':image.pack()
bpy.ops.wm.save_as_mainfile(filepath=str(R/'Caliche_Mesas.blend'),compress=True)
bpy.ops.export_scene.fbx(filepath=str(OUT/(NAME+'.fbx')),use_selection=True,apply_unit_scale=True,axis_forward='-Z',axis_up='Y',use_mesh_modifiers=True,mesh_smooth_type='FACE',add_leaf_bones=False,bake_anim=False)
bpy.ops.export_scene.gltf(filepath=str(OUT/(NAME+'.glb')),export_format='GLB',use_selection=True,export_apply=True,export_yup=True)
print('CALICHE_GEOLOGY_SOURCE',json.dumps({'bounds':bounds,'triangles':triangles,'oldroad_clearance':old_min,'mainroad_clearance':main_min}))
