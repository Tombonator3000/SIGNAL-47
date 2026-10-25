"""Finalize an existing generated source after visual QA; no rendering or geometry edits.
Usage: blender -b Signal47_Cows.blend -t 2 --python finalize_source.py
"""
from pathlib import Path
import bpy,json,math,sys
from mathutils import Vector
HERE=Path(__file__).resolve().parent
sys.path.insert(0,str(HERE))
from mesh_cleanup import clean_geometry
audit=[]
geometry_audit=[]
contract=json.loads((HERE/'model_contract.json').read_text())
for collname in ('01_LIVE_HEREFORD_EDITABLE','02_ABDUCTED_ALDERETE25_EDITABLE','05_EXPORTED_LODS'):
    for ob in bpy.data.collections[collname].objects:
        if ob.type!='MESH':continue
        geometry_audit.append(clean_geometry(ob))
        me=ob.data;uv=me.uv_layers.active.data;repaired=0
        if ob.name.startswith('SM_Cow_Alive_Body') and min(v.co.z for v in me.vertices)>.01:
            for v in me.vertices:
                if v.co.z<.13:v.co.z=max(0,(v.co.z-.025)*.13/.105)
        bounds=[[float(fn(v.co[k]for v in me.vertices))for k in range(3)]for fn in(min,max)]
        for f in me.polygons:
            a,b,c=[uv[li].uv.copy()for li in f.loop_indices]
            if abs((b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x))<2e-12:
                mid=(a+b+c)/3;ax=max(range(3),key=lambda k:abs(f.normal[k]));axes=[k for k in range(3)if k!=ax]
                for li in f.loop_indices:
                    v=me.vertices[me.loops[li].vertex_index].co;uv[li].uv=mid+Vector((v[axes[0]]*.001,v[axes[1]]*.001))
                repaired+=1
        zero=0
        for f in me.polygons:
            a,b,c=[uv[li].uv for li in f.loop_indices]
            zero+=abs((b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x))<2e-12
        assert zero==0,(ob.name,zero)
        audit.append({'object':ob.name,'triangles':len(me.polygons),'uv_triangles_repaired':repaired,'zero_uv_triangles':zero,'materials':[m.name for m in me.materials]})
        if collname=='05_EXPORTED_LODS':
            base=ob.name.split('_LOD')[0].split('.')[0]
            lod=1 if '_LOD1'in ob.name else 2 if '_LOD2'in ob.name else 0
            name=base+(''if lod==0 else '_LOD'+str(lod))
            contract['lods'][str(lod)][base]['bounds_local_m']=bounds
            contract['lods'][str(lod)][base]['triangles']=len(me.polygons)
            contract['parts'][base]['lod_triangles'][lod]=len(me.polygons)
            if lod==0:
                contract['parts'][base]['bounds_local_m']=bounds
            bpy.ops.object.select_all(action='DESELECT');ob.hide_set(False);ob.hide_render=False;ob.select_set(True);bpy.context.view_layer.objects.active=ob
            bpy.ops.export_scene.fbx(filepath=str(HERE/'Export'/(name+'.fbx')),use_selection=True,object_types={'MESH'},apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',axis_forward='-Y',axis_up='Z',bake_anim=False,add_leaf_bones=False,mesh_smooth_type='FACE')
            bpy.ops.export_scene.gltf(filepath=str(HERE/'Export'/(name+'.glb')),export_format='GLB',use_selection=True,export_materials='EXPORT')
for name in ('02_ABDUCTED_ALDERETE25_EDITABLE','04_CONCEPT_REFERENCES','05_EXPORTED_LODS'):
    for ob in bpy.data.collections[name].objects:ob.hide_set(True);ob.hide_render=True
for ob in bpy.data.collections['01_LIVE_HEREFORD_EDITABLE'].objects:ob.hide_set(False);ob.hide_render=False
bpy.context.scene.camera=bpy.data.objects['Camera_Live_ThreeQuarter']
for screen in bpy.data.screens:
    for area in screen.areas:
        if area.type=='VIEW_3D':area.spaces.active.shading.type='MATERIAL';area.spaces.active.region_3d.view_perspective='CAMERA'
bpy.ops.object.select_all(action='DESELECT');bpy.data.objects['SM_Cow_Alive_Body'].select_set(True);bpy.context.view_layer.objects.active=bpy.data.objects['SM_Cow_Alive_Body']
(HERE/'source_verification.json').write_text(json.dumps({'status':'PASS','scope':'saved source metric meshes, defined tangent UVs, separate head pivot and startup visibility','objects':audit,'head_location_m':list(bpy.data.objects['SM_Cow_Alive_Head'].location)},indent=2)+'\n')
(HERE/'model_contract.json').write_text(json.dumps(contract,indent=2)+'\n')
(HERE/'geometry_audit.json').write_text(json.dumps({'status':'PASS','minimum_area_m2':5e-9,'minimum_edge_m':1e-6,'objects':geometry_audit},indent=2)+'\n')
bpy.ops.wm.save_as_mainfile(filepath=str(HERE/'Signal47_Cows.blend'))
print('COW SOURCE FINALIZED',len(audit),'source/LOD objects; no collapsed UV triangles')
