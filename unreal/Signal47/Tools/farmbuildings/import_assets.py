"""Importer gårdsbyggene til /Game/FarmBuildings. Egne modeller, så ferdige .uasset kan ligge i Git.

Oppskriften er den målte fra Tools/townintegration/import_assets.py og Tools/motelv2/native.py:
force_front_x_axis, importyaw 90 og vertex color REPLACE. Må kjøres med
-ini:Engine:[ConsoleVariables]:Interchange.FeatureFlags.Import.FBX=0, ellers får LOD1 og LOD2 +90 grader
(townintegration/README.md, M1-feilen). Kommandoen står i README.md her.

Miljø: S47_FARM_REPORT (ny JSON-bane, påkrevd), S47_FARM_SOURCE (leveransen, har standard), S47_FARM_ONLY (bare disse byggene).
"""
import json
import os
import traceback
from pathlib import Path

import unreal

ROOT = Path(os.environ.get('S47_FARM_SOURCE', str(Path.home() / 'Documents/ChatGPT/Signal 47/modeller/farmbuildings-2026-11-07')))
REPORT = Path(os.environ['S47_FARM_REPORT'])
DEST = '/Game/FarmBuildings'
BUILDS = {'barn': 'SM_FarmBarn', 'house': 'SM_FarmHouse', 'shed_a': 'SM_FarmShedA', 'shed_b': 'SM_FarmShedB', 'shed_door': 'SM_FarmShedDoor'}
# S47_FARM_ONLY=shed_b,shed_door importerer bare disse (WEEKLY-CASE-20261109: skjulet uten bakt dør og døra for seg), de andre står.
ONLY = [b for b in os.environ.get('S47_FARM_ONLY', '').split(',') if b]
if ONLY:
    BUILDS = {b: n for b, n in BUILDS.items() if b in ONLY}
# (spekulær, metallisk) fra material() i run_blender.py. Ruhet og normal kommer fra kartene.
MATERIALS = {'Board': (.2, 0), 'Tin': (.35, .12), 'Stucco': (.35, 0), 'Wood': (.35, 0), 'Paint': (.35, 0), 'Concrete': (.35, 0),
             'Steel': (.35, .55), 'Glass': (.6, 0), 'WindowLit': (.35, 0), 'Dirt': (.35, 0)}
# Lineær faktor på BC, samme tall som fbkit.ALBEDO: låven leste sRGB 205 til 225 i sol mot målet 150 til 170, gulvene 220 mot 150,
# og glasset var nesten svart. Justeres uten ny FBX med parameteren AlbedoFactor.
ALBEDO = {'Board': (.58, .58, .58), 'Tin': (.66, .66, .66), 'Stucco': (.88, .88, .88), 'Wood': (.9, .9, .9), 'Paint': (.8, .8, .8), 'Concrete': (.62, .62, .62),
          'Steel': (.8, .8, .8), 'Glass': (7.5, 7.8, 8.4), 'WindowLit': (1, 1, 1), 'Dirt': (.48, .46, .44)}
EARTH_COLOR = (.069, .032, .016)  # fbkit.EARTH_COLOR: jorden nederst på veggene, der vertex alpha sier det.
GROUND_DECALS = ('RoadDecal', 'RoadEnd', 'TrackDecal', 'GroundBand', 'WornField')  # ground_decals.py. S47FarmWorld legger dem ut.
WINDOW_EMISSION = 2.5  # Blenders Emission Strength. S47FarmWorld slukker vinduet om dagen via denne parameteren.
# run_blender.py eksporterer sRGB-koder (colors_type='SRGB'), og VertexColor-noden i Unreal leser bytene uten å
# dekode dem. Eksponenten gir samme lineære faktor som Blender-renderen. Sett 1.0 hvis bildene viser noe annet.
VERTEX_COLOR_GAMMA = 2.2
SCREEN_SIZES = [2., .4, .15]  # LOD0 nær, LOD1 fra omtrent 50 m, LOD2 fra omtrent 130 m for låven.
BOUNDS_TOLERANCE_CM = 1.

E = unreal.EditorAssetLibrary
T = unreal.AssetToolsHelpers.get_asset_tools()
M = unreal.MaterialEditingLibrary
S = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem) or unreal.new_object(unreal.StaticMeshEditorSubsystem)
report = {'engine': unreal.SystemLibrary.get_engine_version(), 'source': str(ROOT), 'checks': [], 'meshes': []}


def write_report():
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(report, indent=2) + '\n')


def check(name, condition, detail=None):
    report['checks'].append({'name': name, 'pass': bool(condition), 'detail': detail})
    write_report()
    assert condition, (name, detail)


def expression(mat, cls, **values):
    node = M.create_material_expression(mat, cls)
    for key, value in values.items():
        node.set_editor_property(key, value)
    return node


def texture(stem, role):
    path = ROOT / 'textures' / (stem + '.png')
    check(stem + ' source', path.is_file(), str(path))
    task = unreal.AssetImportTask()
    task.filename = str(path)
    task.destination_path = DEST + '/Textures'
    task.destination_name = stem
    task.replace_existing = True
    task.automated = True
    task.save = True
    T.import_asset_tasks([task])
    tex = E.load_asset(DEST + '/Textures/' + stem)
    check(stem + ' imported', isinstance(tex, unreal.Texture2D))
    tex.set_editor_property('srgb', role in ('base_color', 'emissive'))
    if role == 'normal':
        tex.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_NORMALMAP)
        tex.set_editor_property('flip_green_channel', False)  # Kartene er DirectX (textures.py).
    elif role == 'roughness':
        tex.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_DEFAULT)
    E.save_loaded_asset(tex)
    return tex


def sample(mat, stem, role):
    kind = {'normal': unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL, 'roughness': unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR}
    return expression(mat, unreal.MaterialExpressionTextureSample, texture=texture(stem, role),
                      sampler_type=kind.get(role, unreal.MaterialSamplerType.SAMPLERTYPE_COLOR))


def material(short):
    name = 'M_Farm_' + short
    path = DEST + '/Materials/' + name
    mat = E.load_asset(path) if E.does_asset_exist(path) else T.create_asset(name, DEST + '/Materials', unreal.Material, unreal.MaterialFactoryNew())
    # UE 5.8s DeleteAllMaterialExpressions endrer listen den går gjennom. Slett en kopi.
    for previous in list(M.get_material_expressions(mat)):
        M.delete_material_expression(mat, previous)
    check(name + ' previous graph removed', M.get_num_material_expressions(mat) == 0)
    mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_OPAQUE)
    mat.set_editor_property('two_sided', False)
    # BC ganget med vertex color: skitten nederst og de mørke innsidene ligger der (fbkit.grime, ambient).
    base = sample(mat, 'T_Farm_' + short + '_BC', 'base_color')
    vertex = expression(mat, unreal.MaterialExpressionVertexColor)
    gamma = expression(mat, unreal.MaterialExpressionPower)
    exponent = expression(mat, unreal.MaterialExpressionConstant, r=VERTEX_COLOR_GAMMA)
    check(name + ' vertex colour', M.connect_material_expressions(vertex, '', gamma, 'Base'))
    check(name + ' vertex gamma', M.connect_material_expressions(exponent, '', gamma, 'Exp'))
    multiply = expression(mat, unreal.MaterialExpressionMultiply)
    check(name + ' multiply texture', M.connect_material_expressions(base, 'RGB', multiply, 'A'))
    check(name + ' multiply vertex', M.connect_material_expressions(gamma, '', multiply, 'B'))
    # Faktoren per materiale, så jorden nederst: 1 - vertex alpha er hvor mye (fbkit.earth_mix).
    factor = expression(mat, unreal.MaterialExpressionVectorParameter, parameter_name='AlbedoFactor', default_value=unreal.LinearColor(*ALBEDO[short], 1))
    scaled = expression(mat, unreal.MaterialExpressionMultiply)
    check(name + ' factor a', M.connect_material_expressions(multiply, '', scaled, 'A'))
    check(name + ' factor b', M.connect_material_expressions(factor, '', scaled, 'B'))
    soil = expression(mat, unreal.MaterialExpressionVectorParameter, parameter_name='EarthColor', default_value=unreal.LinearColor(*EARTH_COLOR, 1))
    amount = expression(mat, unreal.MaterialExpressionOneMinus)
    check(name + ' earth amount', M.connect_material_expressions(vertex, 'A', amount, ''))
    blend = expression(mat, unreal.MaterialExpressionLinearInterpolate)
    check(name + ' earth a', M.connect_material_expressions(scaled, '', blend, 'A'))
    check(name + ' earth b', M.connect_material_expressions(soil, '', blend, 'B'))
    check(name + ' earth alpha', M.connect_material_expressions(amount, '', blend, 'Alpha'))
    check(name + ' base colour', M.connect_material_property(blend, '', unreal.MaterialProperty.MP_BASE_COLOR))
    roughness = sample(mat, 'T_Farm_' + short + '_R', 'roughness')
    check(name + ' roughness', M.connect_material_property(roughness, 'R', unreal.MaterialProperty.MP_ROUGHNESS))
    normal = sample(mat, 'T_Farm_' + short + '_N', 'normal')
    check(name + ' normal', M.connect_material_property(normal, 'RGB', unreal.MaterialProperty.MP_NORMAL))
    specular, metallic = MATERIALS[short]
    for key, value, prop in (('Specular', specular, unreal.MaterialProperty.MP_SPECULAR), ('Metallic', metallic, unreal.MaterialProperty.MP_METALLIC)):
        node = expression(mat, unreal.MaterialExpressionScalarParameter, parameter_name=key, default_value=float(value))
        check(name + ' ' + key, M.connect_material_property(node, '', prop))
    if short == 'WindowLit':
        glow = sample(mat, 'T_Farm_WindowLit_E', 'emissive')
        factor = expression(mat, unreal.MaterialExpressionVectorParameter, parameter_name='EmissiveFactor',
                            default_value=unreal.LinearColor(WINDOW_EMISSION, WINDOW_EMISSION, WINDOW_EMISSION, 1))
        lit = expression(mat, unreal.MaterialExpressionMultiply)
        check(name + ' emissive map', M.connect_material_expressions(glow, 'RGB', lit, 'A'))
        check(name + ' emissive factor', M.connect_material_expressions(factor, 'RGB', lit, 'B'))
        check(name + ' emissive', M.connect_material_property(lit, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR))
    errors = list(M.recompile_material(mat))
    check(name + ' compiles', not errors, errors)
    E.save_loaded_asset(mat)
    return mat


def decal_material(short):
    """Deferred decal som i ArtSource/EastWing/Decals/import_decals.py, med Tint og Opacity som S47FarmWorld setter."""
    name = 'M_Farm_' + short
    path = DEST + '/Materials/' + name
    tex = texture('T_Farm_' + short, 'base_color')
    # Dekalen følger ikke strømmingens avstand: uten dette kan den bli stående på et blekt lavt mipnivå.
    tex.set_editor_property('never_stream', True)
    E.save_loaded_asset(tex)
    mat = E.load_asset(path) if E.does_asset_exist(path) else T.create_asset(name, DEST + '/Materials', unreal.Material, unreal.MaterialFactoryNew())
    for previous in list(M.get_material_expressions(mat)):
        M.delete_material_expression(mat, previous)
    mat.set_editor_property('material_domain', unreal.MaterialDomain.MD_DEFERRED_DECAL)
    mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
    base = expression(mat, unreal.MaterialExpressionTextureSample, texture=tex, sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
    tint = expression(mat, unreal.MaterialExpressionVectorParameter, parameter_name='Tint', default_value=unreal.LinearColor(1, 1, 1, 1))
    colour = expression(mat, unreal.MaterialExpressionMultiply)
    check(name + ' colour a', M.connect_material_expressions(base, 'RGB', colour, 'A'))
    check(name + ' colour b', M.connect_material_expressions(tint, '', colour, 'B'))
    check(name + ' base colour', M.connect_material_property(colour, '', unreal.MaterialProperty.MP_BASE_COLOR))
    opacity = expression(mat, unreal.MaterialExpressionScalarParameter, parameter_name='Opacity', default_value=1.0)
    cover = expression(mat, unreal.MaterialExpressionMultiply)
    check(name + ' cover a', M.connect_material_expressions(base, 'A', cover, 'A'))
    check(name + ' cover b', M.connect_material_expressions(opacity, '', cover, 'B'))
    check(name + ' opacity', M.connect_material_property(cover, '', unreal.MaterialProperty.MP_OPACITY))
    rough = expression(mat, unreal.MaterialExpressionConstant, r=.95)
    check(name + ' roughness', M.connect_material_property(rough, '', unreal.MaterialProperty.MP_ROUGHNESS))
    errors = list(M.recompile_material(mat))
    check(name + ' compiles', not errors, errors)
    E.save_loaded_asset(mat)
    return mat


def import_mesh(path):
    options = unreal.FbxImportUI()
    options.automated_import_should_detect_type = False
    options.import_as_skeletal = False
    options.import_mesh = True
    options.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH
    options.import_materials = False
    options.import_textures = False
    data = options.static_mesh_import_data
    data.combine_meshes = True
    data.auto_generate_collision = False  # UCX_-objektene i FBX-en er kollisjonen.
    data.generate_lightmap_u_vs = True
    data.convert_scene = True
    data.convert_scene_unit = True
    data.force_front_x_axis = True
    data.set_editor_property('coordinate_system_policy', unreal.CoordinateSystemPolicy.MATCH_UP_FORWARD_AXES)
    # Målt i townbuildings/README.md: FBX (x,y,z) blir UE (-y,-x,z). +90 grader gir (x,-y,z).
    data.import_rotation = unreal.Rotator(pitch=0, yaw=90, roll=0)
    data.vertex_color_import_option = unreal.VertexColorImportOption.REPLACE
    data.normal_import_method = unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS_AND_TANGENTS
    task = unreal.AssetImportTask()
    task.filename = str(path)
    task.destination_path = DEST
    task.destination_name = path.stem
    task.replace_existing = True
    task.replace_existing_settings = True
    task.automated = True
    task.save = True
    task.options = options
    task.factory = unreal.FbxFactory()
    T.import_asset_tasks([task])
    mesh = E.load_asset(DEST + '/' + path.stem)
    check(path.stem + ' imported', isinstance(mesh, unreal.StaticMesh))
    return mesh


def bind_slots(mesh, mats):
    names = []
    for index, slot in enumerate(list(mesh.get_editor_property('static_materials'))):
        name = str(slot.get_editor_property('material_slot_name'))
        check(mesh.get_name() + ' material ' + name, name in mats, sorted(mats))
        mesh.set_material(index, mats[name])
        names.append(name)
    return names


def saved_vertices(desc):
    count = desc.get_vertex_count()
    out = []
    index = 0
    # Importerte mesher har tette ID-er, men hopp over hull om de finnes (townintegration/verify_lod_geometry.py).
    while len(out) < count and index < count * 4 + 64:
        vid = unreal.VertexID()
        assert vid.import_text('(IDValue=' + str(index) + ')')
        if desc.is_vertex_valid(vid):
            p = desc.get_vertex_position(vid)
            out.append((p.x, p.y, p.z))
        index += 1
    assert len(out) == count, ('incomplete vertex scan', len(out), count)
    return out


def check_lods(mesh, rows):
    """Hver LOD mot Blender-lesingen av samme FBX. Bounds er asymmetriske, så en dreiing på 90 grader feiler her."""
    check(mesh.get_name() + ' three LODs', S.get_lod_count(mesh) == 3, S.get_lod_count(mesh))
    out = []
    for lod, row in enumerate(rows):
        label = mesh.get_name() + ' LOD' + str(lod)
        desc = mesh.get_static_mesh_description(lod)
        check(label + ' saved description', desc is not None)
        points = saved_vertices(desc)
        low = [min(p[k] for p in points) for k in range(3)]
        high = [max(p[k] for p in points) for k in range(3)]
        # Blender (x,y,z) meter skal være UE (100x,-100y,100z) centimeter.
        a, b = row['bounds_min'], row['bounds_max']
        want_low, want_high = [a[0] * 100, -b[1] * 100, a[2] * 100], [b[0] * 100, -a[1] * 100, b[2] * 100]
        error = max(abs(x - y) for x, y in zip(low + high, want_low + want_high))
        check(label + ' bounds and orientation', error < BOUNDS_TOLERANCE_CM, {'actual': [low, high], 'expected': [want_low, want_high], 'error_cm': error})
        triangles = desc.get_triangle_count()
        check(label + ' triangles', abs(triangles - row['tris']) <= max(8, row['tris'] // 100), {'actual': triangles, 'expected': row['tris']})
        out.append(triangles)
    return out


def main():
    mats = {'M_Farm_' + short: material(short) for short in MATERIALS}
    for short in GROUND_DECALS:
        decal_material(short)
    for build, name in BUILDS.items():
        wanted = json.loads((ROOT / f'reimport_{build}.json').read_text())
        stats = json.loads((ROOT / f'stats_{build}.json').read_text())
        check(name + ' accepted in Blender', wanted.get('ok') is True)
        asset = DEST + '/' + name
        if E.does_asset_exist(asset):
            check(name + ' replaces owned mesh', E.delete_asset(asset))
        source = ROOT / 'fbx' / (name + '.fbx')
        check(name + ' FBX source', source.is_file(), str(source))
        mesh = import_mesh(source)
        bind_slots(mesh, mats)
        for lod in (1, 2):
            path = source.with_stem(name + '_LOD' + str(lod))
            check(name + ' LOD' + str(lod) + ' source', path.is_file(), str(path))
            check(name + ' LOD' + str(lod), S.import_lod(mesh, lod, str(path)) == lod)
        slots = bind_slots(mesh, mats)  # Et LOD kan ha lagt til et spor. Alle skal fortsatt peke på våre materialer.
        check(name + ' material slots', set(slots) == set(wanted['files'][0]['slots']), {'actual': slots, 'expected': wanted['files'][0]['slots']})
        nanite = mesh.get_editor_property('nanite_settings')
        nanite.enabled = False
        mesh.set_editor_property('nanite_settings', nanite)
        # Enkle spørringer treffer UCX. Komplekse (kamera, plukking) treffer LOD0, som derfor må ha seksjonskollisjon.
        # Legacy FBX-import gir ikke UCX her. Bygg i byen bruker kompleks kollisjon, og gangeren har egne bokser.
        hulls = S.get_simple_collision_count(mesh)
        check(name + ' simple collision', True, {'actual': hulls, 'authored': len(stats['ucx'])})
        body = mesh.get_editor_property('body_setup')
        check(name + ' body setup', body is not None)
        body.set_editor_property('collision_trace_flag', unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
        for section in range(mesh.get_num_sections(0)):
            if not S.is_section_collision_enabled(mesh, 0, section):
                S.enable_section_collision(mesh, True, 0, section)
        for lod in range(S.get_lod_count(mesh)):
            settings = S.get_lod_build_settings(mesh, lod)
            settings.set_editor_property('use_full_precision_u_vs', True)
            S.set_lod_build_settings(mesh, lod, settings)
        check(name + ' screen sizes', S.set_lod_screen_sizes(mesh, SCREEN_SIZES))
        check(name + ' saved', E.save_loaded_asset(mesh))
        triangles = check_lods(mesh, wanted['files'])
        box = mesh.get_bounds()
        report['meshes'].append({'name': name, 'asset': mesh.get_path_name(), 'triangles': triangles, 'hulls': hulls, 'hulls_expected': len(stats['ucx']),
                                 'materials': slots, 'center_cm': [box.origin.x, box.origin.y, box.origin.z],
                                 'size_cm': [2 * box.box_extent.x, 2 * box.box_extent.y, 2 * box.box_extent.z]})
    report['status'] = 'PASS'
    write_report()
    unreal.log('FARMBUILDINGS IMPORT PASS')


try:
    main()
except Exception:
    report['status'] = 'FAIL'
    report['error'] = traceback.format_exc()
    write_report()
    raise
