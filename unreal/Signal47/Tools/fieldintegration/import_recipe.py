# Original FBX/PBR recipe copied from our validated townintegration importer.
# Functions only; native execution belongs to import_assets.py.
"""Reusable measured FBX/PBR helpers. Destinations are explicit call arguments; no import on load."""
import json,os,traceback,hashlib
from pathlib import Path
import unreal
E=unreal.EditorAssetLibrary
T=unreal.AssetToolsHelpers.get_asset_tools()
M=unreal.MaterialEditingLibrary
S=(unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem) or unreal.new_object(unreal.StaticMeshEditorSubsystem))
REPORT=Path(os.environ.get('S47_FIELD_REPORT', '/tmp/s47-field-import.json'))
TEXTURE_CACHE={}
report={'engine':unreal.SystemLibrary.get_engine_version(),'checks':[],'meshes':[]}
def flush():
 REPORT.parent.mkdir(parents=True,exist_ok=True)
 REPORT.write_text(json.dumps(report,indent=2)+'\n')
def check(name,condition,detail=None):
 report['checks'].append({'name':name,'pass':bool(condition),'detail':detail})
 REPORT.write_text(json.dumps(report,indent=2)+'\n')
 assert condition,(name,detail)

def expression(mat, cls, **values):
    node = M.create_material_expression(mat, cls)
    for key, value in values.items():
        node.set_editor_property(key, value)
    return node

def material(row, dest, src):
    name = row['name']
    mat = E.load_asset(dest + '/' + name) if E.does_asset_exist(dest + '/' + name) else T.create_asset(name, dest, unreal.Material, unreal.MaterialFactoryNew())
    # UE 5.8's DeleteAllMaterialExpressions mutates the same array it iterates, leaving
    # old parameter nodes behind. Delete a snapshot so names cannot resolve to stale defaults.
    for previous in list(M.get_material_expressions(mat)):
        M.delete_material_expression(mat, previous)
    check(name+' previous graph fully removed', M.get_num_material_expressions(mat)==0)
    mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_OPAQUE)
    mat.set_editor_property('two_sided', bool(row.get('two_sided', False)))
    col = row.get('base_color_linear', row.get('base_color', [.5, .5, .5, 1]))
    color = expression(mat, unreal.MaterialExpressionVectorParameter, parameter_name='Color', default_value=unreal.LinearColor(*col[:3], 1))
    M.connect_material_property(color, 'RGB', unreal.MaterialProperty.MP_BASE_COLOR)
    for key, prop in [('metallic', unreal.MaterialProperty.MP_METALLIC), ('roughness', unreal.MaterialProperty.MP_ROUGHNESS), ('specular', unreal.MaterialProperty.MP_SPECULAR)]:
        n = expression(mat, unreal.MaterialExpressionScalarParameter, parameter_name=key.title(), default_value=float(row.get(key, .5)))
        M.connect_material_property(n, '', prop)
    strength = float(row.get('emission_strength', 0))
    em = row.get('emission', [0, 0, 0, 1])
    # Props::AddLit/ApplyLit uses this existing glTF-compatible parameter to gate studio power.
    n = expression(mat, unreal.MaterialExpressionVectorParameter, parameter_name='EmissiveFactor', default_value=unreal.LinearColor(*[float(v) * strength for v in em[:3]], 1))
    M.connect_material_property(n, 'RGB', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    emission_node = n
    base_sample = None
    texture_alpha = None
    for role, filename in row.get('textures', {}).items():
        if role not in ('base_color', 'normal', 'roughness'):
            continue
        path = Path(filename)
        if not path.is_absolute():
            path = src / path
        check(name + ' texture source ' + role, path.is_file(), str(path))
        key=(hashlib.sha256(path.read_bytes()).hexdigest(),role)
        tex=TEXTURE_CACHE.get(key)
        if tex is None:
            task = unreal.AssetImportTask()
            task.filename = str(path)
            task.destination_path = dest + '/Textures'
            task.destination_name = 'T_' + name + '_' + role
            task.replace_existing = True
            task.automated = True
            task.save = True
            T.import_asset_tasks([task])
            tex = E.load_asset(task.destination_path + '/' + task.destination_name)
            TEXTURE_CACHE[key]=tex
        check(name + ' texture ' + role, isinstance(tex, unreal.Texture2D))
        tex.set_editor_property('srgb', role == 'base_color')
        if role == 'base_color' and row.get('opacity_from_base_color') and row.get('alpha_mode','masked') == 'masked':
            tex.set_editor_property('do_scale_mips_for_alpha_coverage', True)
            tex.set_editor_property('alpha_coverage_thresholds', unreal.Vector4(0,0,0,float(row.get('alpha_cutoff',.333))))
        if role == 'normal':
            tex.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_NORMALMAP)
            # Source normals are DirectX tangent-space, exported explicitly by the generator.
            tex.set_editor_property('flip_green_channel', False)
        elif role == 'roughness':
            # L-mode source PNGs otherwise auto-import as Grayscale while this graph
            # deliberately samples linear colour and takes R.
            tex.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_DEFAULT)
        E.save_loaded_asset(tex)
        sample = expression(mat, unreal.MaterialExpressionTextureSample, texture=tex,
                            sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL if role == 'normal' else
                            unreal.MaterialSamplerType.SAMPLERTYPE_COLOR if role == 'base_color' else
                            unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
        scale = float(row.get('uv_scale', 1))
        if scale != 1:
            uv = expression(mat, unreal.MaterialExpressionTextureCoordinate, u_tiling=scale, v_tiling=scale)
            check(name+' UV scale connection', M.connect_material_expressions(uv, '', sample, 'UVs'))
        prop = {'base_color': unreal.MaterialProperty.MP_BASE_COLOR, 'normal': unreal.MaterialProperty.MP_NORMAL,
                'roughness': unreal.MaterialProperty.MP_ROUGHNESS}[role]
        if role == 'normal':
            # Decode first, then blend towards the neutral tangent normal; avoid embossed stucco.
            neutral = expression(mat, unreal.MaterialExpressionConstant3Vector, constant=unreal.LinearColor(0,0,1))
            strength = expression(mat, unreal.MaterialExpressionScalarParameter, parameter_name='NormalStrength', default_value=float(row.get('normal_strength', .2)))
            blend = expression(mat, unreal.MaterialExpressionLinearInterpolate)
            check(name+' normal A connection', M.connect_material_expressions(neutral, '', blend, 'A'))
            check(name+' normal B connection', M.connect_material_expressions(sample, 'RGB', blend, 'B'))
            check(name+' normal Alpha connection', M.connect_material_expressions(strength, '', blend, 'Alpha'))
            normalized = expression(mat, unreal.MaterialExpressionNormalize)
            check(name+' normal VectorInput connection', M.connect_material_expressions(blend, '', normalized, 'VectorInput'))
            M.connect_material_property(normalized, '', prop)
        else:
            if role == 'base_color':
                tint = expression(mat, unreal.MaterialExpressionVectorParameter, parameter_name='TextureTint', default_value=unreal.LinearColor(*row.get('texture_tint_linear', [1,1,1])[:3],1))
                multiply = expression(mat, unreal.MaterialExpressionMultiply)
                check(name+' tint texture', M.connect_material_expressions(sample, 'RGB', multiply, 'A'))
                check(name+' tint color', M.connect_material_expressions(tint, 'RGB', multiply, 'B'))
                M.connect_material_property(multiply, '', prop)
            else:
                M.connect_material_property(sample, 'RGB' if role != 'roughness' else 'R', prop)
        if role == 'base_color':
            base_sample = sample
        if role == 'base_color' and row.get('opacity_from_base_color'):
            texture_alpha = sample
            if row.get('vertex_alpha_opacity') or row.get('alpha_mode')=='dithered_masked':
                continue  # The perimeter fade multiplies the atlas alpha below.
            masked = row.get('alpha_mode', 'masked') == 'masked'
            mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_MASKED if masked else unreal.BlendMode.BLEND_TRANSLUCENT)
            if masked:
                mat.set_editor_property('opacity_mask_clip_value', float(row.get('alpha_cutoff', .333)))
            else:
                mat.set_editor_property('translucency_lighting_mode', unreal.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING)
            check(name+' texture alpha connection', M.connect_material_property(sample, 'A', unreal.MaterialProperty.MP_OPACITY_MASK if masked else unreal.MaterialProperty.MP_OPACITY))
    if row.get('vertex_alpha_opacity') or row.get('alpha_mode')=='dithered_masked':
        check(name+' vertex fade contract', row.get('alpha_mode')=='dithered_masked')
        mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_MASKED)
        mat.set_editor_property('opacity_mask_clip_value', .333)
        vertex = expression(mat, unreal.MaterialExpressionVertexColor) if row.get('vertex_alpha_opacity') else None
        alpha, output = (vertex, 'A') if vertex else (texture_alpha, 'A')
        check(name+' authored fade input', alpha is not None)
        if texture_alpha and vertex:
            alpha = expression(mat, unreal.MaterialExpressionMultiply)
            check(name+' atlas fade', M.connect_material_expressions(texture_alpha, 'A', alpha, 'A'))
            check(name+' perimeter fade', M.connect_material_expressions(vertex, 'A', alpha, 'B'))
            output = ''
        # Engine function packages need not have been scanned by the unattended
        # Asset Registry. Resolve the installed package directly, as the editor does.
        function = unreal.load_object(None, '/Engine/Functions/Engine_MaterialFunctions02/Utility/DitherTemporalAA.DitherTemporalAA')
        check(name+' temporal fade function', function is not None)
        dither = expression(mat, unreal.MaterialExpressionMaterialFunctionCall, material_function=function)
        check(name+' temporal alpha', M.connect_material_expressions(alpha, output, dither, 'Alpha Threshold'))
        check(name+' dithered mask', M.connect_material_property(dither, '', unreal.MaterialProperty.MP_OPACITY_MASK))
    if float(row.get('alpha', 1)) < 1:
        mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
        mat.set_editor_property('translucency_lighting_mode', unreal.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING)
        mat.set_editor_property('two_sided', True)
        n = expression(mat, unreal.MaterialExpressionScalarParameter, parameter_name='Opacity', default_value=float(row['alpha']))
        M.connect_material_property(n, '', unreal.MaterialProperty.MP_OPACITY)
    if base_sample is not None:
        glow = expression(mat, unreal.MaterialExpressionMultiply)
        check(name+' emissive print texture', M.connect_material_expressions(base_sample, 'RGB', glow, 'A'))
        check(name+' emissive instance state', M.connect_material_expressions(emission_node, 'RGB', glow, 'B'))
        M.connect_material_property(glow, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    errors=list(M.recompile_material(mat))
    check(name+' material compiles',not errors,errors)
    E.save_loaded_asset(mat)
    return mat


def import_mesh(path, dest):
    options = unreal.FbxImportUI()
    options.automated_import_should_detect_type = False
    options.import_as_skeletal = False
    options.import_mesh = True
    options.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH
    options.import_materials = False
    options.import_textures = False
    data = options.static_mesh_import_data
    data.combine_meshes = True
    data.remove_degenerates = False
    data.auto_generate_collision = False
    data.generate_lightmap_u_vs = True
    data.convert_scene = True
    data.convert_scene_unit = True
    data.force_front_x_axis = True
    data.set_editor_property('coordinate_system_policy', unreal.CoordinateSystemPolicy.MATCH_UP_FORWARD_AXES)
    # The asymmetric probe measured FBX (x,y,z) -> UE (-y,-x,z); bake +90 degrees to (x,-y,z).
    data.import_rotation = unreal.Rotator(pitch=0, yaw=90, roll=0)
    data.vertex_color_import_option = unreal.VertexColorImportOption.REPLACE
    data.normal_import_method = unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS_AND_TANGENTS
    task = unreal.AssetImportTask()
    task.filename = str(path)
    task.destination_path = dest
    task.destination_name = path.stem
    task.replace_existing = True
    task.replace_existing_settings = True
    task.automated = True
    task.save = True
    task.options = options
    task.factory = unreal.FbxFactory()
    T.import_asset_tasks([task])
    mesh = E.load_asset(dest + '/' + path.stem)
    check(path.stem + ' imported', isinstance(mesh, unreal.StaticMesh))
    return mesh
