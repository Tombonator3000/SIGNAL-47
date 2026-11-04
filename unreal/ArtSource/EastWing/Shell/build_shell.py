"""Build the authored EastWing shell with editable sources and separate native parts.

Run only in the parent's production_lease/require-idle Blender window. This script
does not launch Unreal, place runtime actors, overwrite the old building, or render.
Geometry/UV/export conventions reuse the project's Props and SARO-site generators.
"""
from pathlib import Path
import hashlib
import json
import math
import os
import struct
import sys

import bpy
from mathutils import Vector

HERE=Path(__file__).resolve().parent
sys.path.insert(0,str(HERE))
import layout

OUT=Path(os.environ.get('S47_EASTWING_SHELL_OUT',str(HERE/'exports'))).resolve()
TEX=HERE.parent/'Props/Textures'
SURFACES={r['key']:r for r in json.loads((TEX/'materials.json').read_text())['materials']}
MATERIALS={
    'Stucco':dict(color=[.66,.59,.45,1],rough=.86,texture='EW_Stucco'),
    'Masonry':dict(color=[.68,.63,.49,1],rough=.83,texture='EW_CinderblockCream'),
    'Concrete':dict(color=[.38,.37,.34,1],rough=.72,texture='EW_Concrete'),
    'Plinth':dict(color=[.29,.27,.23,1],rough=.82,texture='EW_Concrete'),
    'Ceiling':dict(color=[.54,.50,.41,1],rough=.82),
    'CeilingSteel':dict(color=[.40,.38,.32,1],rough=.66,metal=.15),
    'GreenTrim':dict(color=[.085,.135,.105,1],rough=.49,metal=.12),
    'GreenSteel':dict(color=[.13,.20,.16,1],rough=.52,texture='EW_GreenSteel'),
    'Galvanized':dict(color=[.46,.49,.48,1],rough=.37,metal=.85,texture='EW_Galvanized'),
    'Conduit':dict(color=[.30,.32,.30,1],rough=.43,metal=.72),
    'Glass':dict(color=[.70,.80,.76,1],rough=.10,transmission=.92,ior=1.46),
    'BlindWood':dict(color=[.34,.23,.12,1],rough=.65),
    'BlindCord':dict(color=[.47,.43,.34,1],rough=.88),
    'Scuff':dict(color=[.035,.04,.035,1],rough=.78),
    'Yellow':dict(color=[.68,.43,.055,1],rough=.55,metal=.05),
    'SignFace':dict(color=[.055,.085,.065,1],rough=.57),
}


def three_to_blender(p):
    return Vector((p[0],-p[2],p[1]))


def blender_to_three(p):
    return [float(p[0]),float(p[2]),float(-p[1])]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def select(objects):
    bpy.ops.object.select_all(action='DESELECT')
    for obj in objects:
        obj.hide_set(False)
        obj.select_set(True)
    bpy.context.view_layer.objects.active=objects[0]


def create_materials():
    result={}
    for key,spec in MATERIALS.items():
        mat=bpy.data.materials.new('EW_Shell_'+key)
        mat.use_nodes=True
        mat.diffuse_color=spec['color']
        bs=mat.node_tree.nodes.get('Principled BSDF')
        bs.inputs['Base Color'].default_value=spec['color']
        bs.inputs['Roughness'].default_value=spec['rough']
        bs.inputs['Metallic'].default_value=spec.get('metal',0.)
        bs.inputs['Transmission Weight'].default_value=spec.get('transmission',0.)
        bs.inputs['IOR'].default_value=spec.get('ior',1.5)
        if spec.get('texture'):
            mat['eastwing_texture_key']=spec['texture']
            for role,socket in [('BaseColor','Base Color'),('Roughness','Roughness'),('Metallic','Metallic'),('NormalGL','Normal')]:
                image_path=TEX/SURFACES[spec['texture']]['maps'][role]['file']
                if not image_path.is_file():raise FileNotFoundError(image_path)
                node=mat.node_tree.nodes.new('ShaderNodeTexImage')
                node.image=bpy.data.images.load(str(image_path),check_existing=True)
                node.image.colorspace_settings.name='sRGB' if role=='BaseColor' else 'Non-Color'
                if role=='NormalGL':
                    normal=mat.node_tree.nodes.new('ShaderNodeNormalMap')
                    mat.node_tree.links.new(node.outputs['Color'],normal.inputs['Color'])
                    mat.node_tree.links.new(normal.outputs['Normal'],bs.inputs[socket])
                else:mat.node_tree.links.new(node.outputs['Color'],bs.inputs[socket])
        result[key]=mat
    return result


def link_only(obj,collection):
    for old in list(obj.users_collection):old.objects.unlink(obj)
    collection.objects.link(obj)


def make_part(row,lod,collection,mats,collision=False):
    kind=row['kind']
    if kind=='box':
        # Render masonry behind the frames without changing the original UCX.
        lo=Vector(row['lo'] if collision else row.get('visual_lo',row['lo']))
        hi=Vector(row['hi'] if collision else row.get('visual_hi',row['hi']))
        center=(lo+hi)*.5
        size=hi-lo
        bpy.ops.mesh.primitive_cube_add(size=1,location=three_to_blender(center))
        obj=bpy.context.object
        obj.dimensions=(size.x,size.z,size.y)
        bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    elif kind=='tube':
        a,b=three_to_blender(row['start']),three_to_blender(row['end'])
        direction=b-a
        bpy.ops.mesh.primitive_cylinder_add(vertices=12 if collision else [24,12,8][lod],
                                          radius=row['radius'],depth=direction.length,
                                          location=(a+b)*.5)
        obj=bpy.context.object
        obj.rotation_mode='QUATERNION'
        obj.rotation_quaternion=direction.to_track_quat('Z','Y')
    elif kind=='ramp':
        x0,x1,z0,z1,y0,y1,bottom=[row[k] for k in ('x0','x1','z0','z1','y0','y1','bottom')]
        points=[(x0,bottom,z0),(x1,bottom,z0),(x1,bottom,z1),(x0,bottom,z1),
                (x0,y0,z0),(x1,y0,z0),(x1,y1,z1),(x0,y1,z1)]
        if row.get('axis')=='x':
            points[4:]=[(x0,y0,z0),(x1,y1,z0),(x1,y1,z1),(x0,y0,z1)]
        # Three->Blender is a proper axis rotation; preserve outward face winding.
        faces=[(1,2,3,0),(7,6,5,4),(4,5,1,0),(5,6,2,1),(6,7,3,2),(7,4,0,3)]
        data=bpy.data.meshes.new(row['name'])
        data.from_pydata([three_to_blender(p) for p in points],[],faces)
        data.update()
        obj=bpy.data.objects.new(row['name'],data)
        collection.objects.link(obj)
    else:raise ValueError(kind)
    obj.name=row['name']
    link_only(obj,collection)
    if collision:
        obj.display_type='WIRE'
        obj.hide_render=True
        return obj
    obj.data.materials.append(mats[row['material']])
    if row.get('exterior_axis') is not None:
        obj.data.materials.append(mats['Stucco'])
        for poly in obj.data.polygons:
            normal_three=blender_to_three(poly.normal)
            if normal_three[row['exterior_axis']]*row['exterior_sign']>.9:
                poly.material_index=1
    width=row.get('bevel',.002 if kind=='tube' else 0)
    if width and lod<2:
        bevel=obj.modifiers.new('Authored edge radius','BEVEL')
        bevel.width=min(width,min(obj.dimensions)*.20)
        bevel.segments=3 if lod==0 else 1
        bevel.affect='EDGES'
        bevel.use_clamp_overlap=True
        obj.modifiers.new('Weighted corner normals','WEIGHTED_NORMAL')
    if kind=='tube':
        for poly in obj.data.polygons:poly.use_smooth=len(poly.vertices)==4
    # UVs also belong to the editable source, not just the evaluated FBX copy.
    # Baking object transforms leaves named source pieces and their bevel modifiers editable.
    select([obj])
    bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
    uv_and_slots(obj,list(obj.data.materials),[0.,0.,0.])
    obj['eastwing_source_part']=row['name']
    obj['eastwing_asset']=row['asset']
    if row.get('sign_uv'):obj['sign_uv']=True
    return obj


def evaluated_copy(objects,name,anchor):
    deps=bpy.context.evaluated_depsgraph_get()
    copies=[]
    for original in objects:
        mesh=bpy.data.meshes.new_from_object(original.evaluated_get(deps),depsgraph=deps)
        copy=bpy.data.objects.new(original.name+'_export',mesh)
        bpy.context.scene.collection.objects.link(copy)
        copy.matrix_world=original.matrix_world.copy()
        copies.append(copy)
    select(copies)
    bpy.ops.object.join()
    obj=bpy.context.object
    obj.name=name
    # Bake exact authored world coordinates, then relocate ONLY movable assets to their anchor.
    bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
    delta=three_to_blender(anchor)
    for vertex in obj.data.vertices:vertex.co-=delta
    tri=obj.modifiers.new('Explicit export triangles','TRIANGULATE')
    bpy.ops.object.modifier_apply(modifier=tri.name)
    return obj


def uv_and_slots(obj,slot_order,anchor):
    mesh=obj.data
    previous=list(mesh.materials)
    indices=[slot_order.index(previous[poly.material_index]) for poly in mesh.polygons]
    mesh.materials.clear()
    for mat in slot_order:mesh.materials.append(mat)
    uv=mesh.uv_layers.new(name='UV0') if not mesh.uv_layers else mesh.uv_layers[0]
    delta=three_to_blender(anchor)
    for poly,index in zip(mesh.polygons,indices):
        poly.material_index=index
        mat=slot_order[index]
        tile=SURFACES.get(mat.get('eastwing_texture_key'),{}).get('physical_tile_metres',[1.,1.])
        axis=max(range(3),key=lambda k:abs(poly.normal[k]))
        axes=[k for k in range(3) if k!=axis]
        for loop_index in poly.loop_indices:
            position=mesh.vertices[mesh.loops[loop_index].vertex_index].co+delta
            if mat.name=='EW_Shell_SignFace' and axis==1:
                uv.data[loop_index].uv=((position.x-23.46)/(30.34-23.46),(position.z-1.11)/(2.69-1.11))
            else:
                uv.data[loop_index].uv=(position[axes[0]]/tile[0],position[axes[1]]/tile[1])
    mesh.update()


def mesh_stats(obj,anchor):
    points=[v.co for v in obj.data.vertices]
    assert points and all(math.isfinite(c) for p in points for c in p)
    bounds=[[fun(p[k] for p in points) for k in range(3)] for fun in (min,max)]
    world=[blender_to_three(p+three_to_blender(anchor)) for p in points]
    world_bounds=[[fun(p[k] for p in world) for k in range(3)] for fun in (min,max)]
    discarded=0
    def f32(value):return struct.unpack('f',struct.pack('f',value))[0]
    for face in obj.data.polygons:
        assert len(face.vertices)==3
        corners=[tuple(f32(points[i][k]*100) for k in range(3)) for i in face.vertices]
        if len(set(corners))<3:discarded+=1
    return dict(triangles=len(obj.data.polygons),expected_native_triangles=len(obj.data.polygons)-discarded,
                exact_degenerate_triangles=discarded,bounds_m=bounds,world_bounds_three_m=world_bounds,
                dimensions_m=[bounds[1][k]-bounds[0][k] for k in range(3)])


def material_records(slots):
    records=[]
    for mat in slots:
        bs=mat.node_tree.nodes.get('Principled BSDF')
        row={'name':mat.name,'texture_key':mat.get('eastwing_texture_key'),'maps':{}}
        for socket in ['Base Color','Roughness','Metallic','Transmission Weight','IOR','Emission Color','Emission Strength']:
            value=bs.inputs[socket].default_value
            row[socket]=list(value) if hasattr(value,'__len__') else float(value)
        for node in mat.node_tree.nodes:
            if node.type=='TEX_IMAGE' and node.image:
                path=Path(bpy.path.abspath(node.image.filepath))
                row['maps'][path.name]=str(path)
        records.append(row)
    return records


def export(path,objects):
    select(objects)
    bpy.ops.export_scene.fbx(filepath=str(path),use_selection=True,object_types={'MESH'},
        apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',axis_forward='-Y',axis_up='Z',
        bake_anim=False,mesh_smooth_type='FACE',path_mode='RELATIVE',use_mesh_modifiers=True,
        use_tspace=True)


def main():
    contract=layout.validate()
    OUT.mkdir(parents=True,exist_ok=True)
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.context.scene.unit_settings.system='METRIC'
    bpy.context.scene.unit_settings.scale_length=1.
    mats=create_materials()
    collections={}
    source_objects={}
    collision_records={}
    all_rows={lod:layout.build(lod) for lod in range(3)}
    for asset in layout.ASSETS:
        coll=bpy.data.collections.new(asset+'_EDITABLE')
        bpy.context.scene.collection.children.link(coll)
        collections[asset]=coll
        rows=[r for r in all_rows[0] if r['asset']==asset]
        source_objects[asset]=[make_part(row,0,coll,mats) for row in rows]
        collision_records[asset]=[r for r in rows if r.get('collide')]
    # Sources keep separate objects and non-destructive bevels. No lights or preview camera is gameplay.
    editable=OUT/'EastWing_Shell.blend'
    bpy.ops.wm.save_as_mainfile(filepath=str(editable),check_existing=False,compress=True)
    manifest={'order':'EASTWING-01','status':'GENERATED_NOT_NATIVE_VERIFIED',
        'coordinate_contract':'Blender=(three.x,-three.z,three.y), metres; native recipe maps to UE=(100x,-100y,100z)',
        'static_origin_three_m':[0,0,0],'blockout_contract':contract,
        'editable':editable.name,'editable_sha256':sha(editable),'assets':[],
        'retained_world_geometry':['Shared house:roof geo17','Shared roof/site dressing geo18/454/455'],
        'open_questions':['New east service door requires matching walking/Story-lock integration; old exterior only had meter/conduit.',
                          'SignFace has 0..1 UV for the retained sign art; texture hookup remains runtime work.',
                          'Roll-up source is a closed slatted curtain with separate source slats; animation remains runtime work.',
                          'Existing Props surfaces are procedural, not a new claim of photo-sourced material or photoreal runtime quality.']}
    for asset in layout.ASSETS:
        anchor=layout.ANCHORS.get(asset,[0.,0.,0.])
        slots=sorted({mat for obj in source_objects[asset] for mat in obj.data.materials},key=lambda mat:mat.name)
        lod_records=[]
        for lod in range(3):
            scratch=None
            if lod==0:objects=source_objects[asset]
            else:
                scratch=bpy.data.collections.new('ExportScratch')
                bpy.context.scene.collection.children.link(scratch)
                objects=[make_part(r,lod,scratch,mats) for r in all_rows[lod] if r['asset']==asset]
            name=asset if lod==0 else asset+'_LOD'+str(lod)
            mesh=evaluated_copy(objects,name,anchor)
            uv_and_slots(mesh,slots,anchor)
            stats=mesh_stats(mesh,anchor)
            collisions=[]
            if lod==0:
                cc=bpy.data.collections.new('CollisionScratch')
                bpy.context.scene.collection.children.link(cc)
                for i,row in enumerate(collision_records[asset]):
                    collision=make_part(row,0,cc,mats,collision=True)
                    select([collision])
                    bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
                    for vertex in collision.data.vertices:vertex.co-=three_to_blender(anchor)
                    collision.name=f'UCX_{asset}_{i:03}'
                    collisions.append(collision)
            path=OUT/(name+'.fbx')
            export(path,[mesh]+collisions)
            lod_records.append(dict(file=path.name,sha256=sha(path),**stats))
            for obj in [mesh]+collisions:bpy.data.objects.remove(obj,do_unlink=True)
            if lod==0:bpy.data.collections.remove(cc)
            if scratch:
                for obj in list(scratch.objects):bpy.data.objects.remove(obj,do_unlink=True)
                bpy.data.collections.remove(scratch)
        row={'asset':asset,'fbx':lod_records[0]['file'],'fbx_sha256':lod_records[0]['sha256'],
             'editable':editable.name,'editable_collection':asset+'_EDITABLE',
             'units':'metres; FBX unit metadata centimetres conversion','front':'-Y','up':'Z',
             'pivot':[0,0,0],'anchor_three_m':anchor,'rotation_three_radians':[0,0,0],
             'materials':[m.name for m in slots],'material_parameters':material_records(slots),
             'uv_layers':1,'lods':lod_records[1:],
             'collision':'Segmented authored convex UCX, never one building-wide box; disable on decorative parts.',
             'collision_hulls':len(collision_records[asset]),
             'collision_source_parts':[r['name'] for r in collision_records[asset]],
             'license':{'id':'LicenseRef-ProjectOwned','sources':['unreal/ArtSource/EastWing/Shell/layout.py','unreal/ArtSource/EastWing/Shell/build_shell.py']},
             'runtime_status':'NOT_IMPORTED',**{k:v for k,v in lod_records[0].items() if k not in ('file','sha256')}}
        if asset=='SM_EW_PersonnelDoor':row['open_rotation_three_radians']=[0,math.pi/2,0]
        if asset=='SM_EW_ServiceDoor':row['open_rotation_three_radians']=[0,-math.pi/2,0]
        (OUT/(asset+'.asset.json')).write_text(json.dumps(row,indent=2)+'\n')
        (OUT/(asset+'.license.json')).write_text(json.dumps({'asset':asset,**row['license']},indent=2)+'\n')
        manifest['assets'].append({'id':asset,'manifest':asset+'.asset.json','anchor_three_m':anchor,
                                   'native_path':'/Game/EastWing/Shell/'+asset,'lod_triangles':[r['triangles'] for r in lod_records]})
    (OUT/'shell-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    print('EASTWING_SHELL_GENERATED',json.dumps(manifest),flush=True)


if __name__=='__main__':main()
