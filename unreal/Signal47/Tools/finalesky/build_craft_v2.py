"""Build a separate original v2/v3/v4 craft source and three authored FBX LODs.

Root runs background Blender through unreal_guard. The output must be fresh.
No v1 contract/archive is read or written, and no native/runtime acceptance is implied.
"""
import argparse
import hashlib
import importlib
import json
from pathlib import Path
import sys

import bpy

HERE = Path(__file__).resolve().parent
sys.path.insert(0,str(HERE))
REVISIONS = dict(v2='S47-FINALE-SKY-2',v3='S47-FINALE-SKY-3',v4='S47-FINALE-SKY-4')
MESH = 'SM_S47_FinalCraft'
SLOTS = ['M_S47_Final'+role for role in ('Hull','Ribs','Recess','Light')]
MATERIALS = {
    'Hull': dict(color=[.075,.09,.105,1],roughness=.53,metallic=.78,glow=0),
    'Ribs': dict(color=[.17,.19,.21,1],roughness=.37,metallic=.85,glow=0),
    'Recess': dict(color=[.008,.014,.024,1],roughness=.66,metallic=.36,glow=.025),
    'Light': dict(color=[.36,.52,.63,1],roughness=.32,metallic=.08,glow=1.8),
}


def log(message,variant='v2'):
    print('S47 CRAFT '+variant.upper()+' '+message,flush=True)


def material_palette():
    result = {}
    for role,row in MATERIALS.items():
        material = bpy.data.materials.new('M_S47_Final'+role)
        material.use_nodes = True
        material.use_fake_user = True
        bsdf = material.node_tree.nodes.get('Principled BSDF')
        bsdf.inputs['Base Color'].default_value = row['color']
        bsdf.inputs['Roughness'].default_value = row['roughness']
        bsdf.inputs['Metallic'].default_value = row['metallic']
        bsdf.inputs['Emission Color'].default_value = row['color']
        bsdf.inputs['Emission Strength'].default_value = row['glow']
        result[material.name] = material
    return result


def mesh_data(name,vertices,triangles,uv,materials,palette,normals=None):
    data = bpy.data.meshes.new(name)
    data.from_pydata(vertices,[],triangles)
    data.update()
    for slot in SLOTS:
        data.materials.append(palette[slot])
    layer = data.uv_layers.new(name='UVMap')
    for polygon,coords,material in zip(data.polygons,uv,materials):
        polygon.material_index = SLOTS.index(material)
        polygon.use_smooth = True
        for loop,tc in zip(polygon.loop_indices,coords):
            layer.data[loop].uv = tc
    data.calc_loop_triangles()
    assert len(data.loop_triangles) == len(triangles) == len(uv)
    if normals is not None:
        assert len(normals) == len(triangles)
        data.normals_split_custom_set([normal for triangle in normals for normal in triangle])
        data.update()
    return data


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--variant',choices=('v2','v3','v4'),default='v2')
    args = parser.parse_args(sys.argv[sys.argv.index('--')+1:])
    revision = REVISIONS[args.variant]
    geometry_name = 'geometry_'+args.variant
    build = importlib.import_module(geometry_name).build
    out = args.output.expanduser().resolve()
    if out.exists():
        raise FileExistsError('Fresh output directory required: '+str(out))
    if not bpy.app.background:
        raise RuntimeError('Use background Blender through the root guard')
    out.mkdir(parents=True,exist_ok=False)
    (out/'exports').mkdir()
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.context.scene.unit_settings.system = 'METRIC'
    bpy.context.scene.unit_settings.scale_length = 1
    sources = bpy.data.collections.new('EDITABLE_ORIGINAL_'+args.variant.upper()+'_SOLIDS')
    bpy.context.scene.collection.children.link(sources)
    sources.hide_viewport = sources.hide_render = True
    palette = material_palette()
    runtime_radius = 1000 if args.variant == 'v4' else 680
    lods = []
    for lod in range(3):
        solids = build(lod)
        vertices,triangles,uv,materials = [],[],[],[]
        normals = [] if args.variant == 'v4' else None
        for solid in solids:
            offset = len(vertices)
            vertices.extend(solid.vertices)
            triangles.extend(tuple(offset+i for i in face) for face in solid.triangles)
            uv.extend(solid.uv)
            face_materials = getattr(solid,'materials',[solid.material]*len(solid.triangles))
            materials.extend(face_materials)
            if normals is not None:
                normals.extend(solid.normals)
            if lod == 0:
                data = mesh_data('SRC_'+solid.name,solid.vertices,solid.triangles,solid.uv,
                                 face_materials,palette,getattr(solid,'normals',None))
                original = bpy.data.objects.new('SRC_'+solid.name,data)
                sources.objects.link(original)
                original['authorship'] = 'Original SIGNAL / 47 '+args.variant
        name = MESH+('' if lod == 0 else '_LOD'+str(lod))
        data = mesh_data(name,vertices,triangles,uv,materials,palette,normals)
        obj = bpy.data.objects.new(name,data)
        bpy.context.scene.collection.objects.link(obj)
        obj['source_revision'] = revision
        obj['radius_m'],obj['runtime_radius_m'] = 1,runtime_radius
        bpy.ops.object.select_all(action='DESELECT')
        obj.select_set(True)
        bpy.context.view_layer.objects.active = obj
        target = out/'exports'/(name+('' if args.variant == 'v2' else '_'+args.variant)+'.fbx')
        bpy.ops.export_scene.fbx(filepath=str(target),use_selection=True,object_types={'MESH'},
            apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',axis_forward='-Y',axis_up='Z',
            bake_anim=False,add_leaf_bones=False,mesh_smooth_type='FACE')
        assert target.is_file() and target.stat().st_size > 0
        lods.append(dict(lod=lod,name=name,fbx=str(target.relative_to(out)),
                         triangles=len(data.loop_triangles),vertices=len(data.vertices),solids=len(solids)))
        obj.hide_render = lod != 0
        obj.hide_set(lod != 0)
        log('EXPORTED LOD%d triangles=%d solids=%d file=%s' % (lod,len(data.loop_triangles),len(solids),target),args.variant)
    source = out/('FinalCraft-'+args.variant+'.blend')
    bpy.ops.wm.save_as_mainfile(filepath=str(source))
    inputs = []
    source_paths = [Path(__file__).resolve(),HERE/(geometry_name+'.py')]
    if args.variant in ('v3','v4'):
        source_paths.append(HERE/'geometry_v2.py')  # Actual read-only shape-helper dependency.
    source_paths.append(HERE/'geometry.py')
    for path in source_paths:
        inputs.append(dict(path=str(path.relative_to(HERE.parents[3])),
                           sha256=hashlib.sha256(path.read_bytes()).hexdigest()))
    manifest = dict(schema=1,revision=revision,authorship='Original SIGNAL / 47 craft geometry '+args.variant,
        variant=args.variant,generator=Path(__file__).name,geometry=geometry_name+'.py',
        mesh=MESH,destination='/Game/FinaleSky',material_slots=SLOTS,materials=MATERIALS,
        runtime_radius_m=runtime_radius,lod_screen_sizes=[1,.35,.12],lods=lods,
        source_blend=source.name,source_inputs=inputs,
        normals=('Authored analytic radial corner normals; sharp profile ledges, smooth lens profile'
                 if args.variant == 'v4' else 'Existing source face-smoothing policy'),
        source_axis='Blender metres Z-up; FBX -Y/Z and FBX_SCALE_UNITS; native forceFrontX plus yaw90',
        native_mapping='(Bx,-By,Bz)*100cm, clockwise front faces; native UV=(u,1-v)',
        status='SOURCE_SAVED_EXPORTS_CREATED',native_imported=False,runtime_viewed=False)
    (out/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    log('SOURCE SAVED '+str(source),args.variant)
    log('COMPLETE '+str(out/'manifest.json'),args.variant)


if __name__ == '__main__':
    main()
