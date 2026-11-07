"""Importer ørkenplantene til /Game/DesertPlants. Egne modeller, så ferdige .uasset kan ligge i Git (LFS).

Oppskriften er den målte fra Tools/farmbuildings/import_assets.py: legacy FBX, force_front_x_axis, importyaw 90.
Må kjøres med -ini:Engine:[ConsoleVariables]:Interchange.FeatureFlags.Import.FBX=0, ellers får LOD1 og LOD2 +90 grader.
Forskjellene fra gårdsbyggene står i README.md her. Den viktigste: vinden ligger i UV-kanal 1 (UVChannel_2.x), så
ingen lightmap-UV-er genereres, på noen LOD. Materialene er instanser av flora-mastrene i /Game/Web/Base, som må være
importert først (genereres av Tools/webexport/import_web.py og ligger ikke i Git).

Miljø: S47_DESERT_REPORT (ny JSON-bane, påkrevd), S47_DESERT_SOURCE (leveransen, har standard).
Kommandoen står i README.md.
"""
import json
import os
import traceback
from pathlib import Path

import unreal

ROOT = Path(os.environ.get('S47_DESERT_SOURCE', str(Path.home() / 'Documents/ChatGPT/Signal 47/modeller/desertplants-2026-11-07')))
REPORT = Path(os.environ['S47_DESERT_REPORT'])
DEST = '/Game/DesertPlants'
MASTERS = '/Game/Web/Base/'
PLANTS = ['SM_DP_Cholla_A', 'SM_DP_Cholla_B', 'SM_DP_Yucca_A', 'SM_DP_Yucca_B', 'SM_DP_Yucca_Bloom', 'SM_DP_Yucca_DryStalk', 'SM_DP_Mesquite_A', 'SM_DP_Mesquite_B']
# flora.ts floraMaterial: Back slipper lampelys gjennom kort. Yucca og cholla er tette, mesquiten er løv.
BACK = {'M_DP_Yucca': 0., 'M_DP_Cholla': 0., 'M_DP_Mesquite': 1.}
SCREEN_SIZES = [1., .12, .025]  # Som Tools/calichenature.
BOUNDS_TOLERANCE_CM = 1.
SWAY_TOLERANCE = .01

E = unreal.EditorAssetLibrary
T = unreal.AssetToolsHelpers.get_asset_tools()
M = unreal.MaterialEditingLibrary
S = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem) or unreal.new_object(unreal.StaticMeshEditorSubsystem)
report = {'engine': unreal.SystemLibrary.get_engine_version(), 'source': str(ROOT), 'checks': [], 'materials': [], 'meshes': []}


def write_report():
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(report, indent=2) + '\n')


def check(name, condition, detail=None):
    report['checks'].append({'name': name, 'pass': bool(condition), 'detail': detail})
    write_report()
    assert condition, (name, detail)


def texture(relative, srgb):
    path = ROOT / relative
    stem = path.stem
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
    # Må stemme med mastrenes prøvetakere: Map er SAMPLERTYPE_COLOR, RoughnessMap er SAMPLERTYPE_LINEAR_COLOR.
    tex.set_editor_property('srgb', srgb)
    tex.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_DEFAULT)
    E.save_loaded_asset(tex)
    return tex


def material(name, row):
    """En instans av flora-masteren: Tools/webexport/import_web.py setter de samme parameterne på sine (se fk der)."""
    master_path = MASTERS + row['master']
    check(row['master'] + ' exists (import_web.py run?)', E.does_asset_exist(master_path), master_path)
    master = E.load_asset(master_path)
    short = 'MI_DP_' + name[len('M_DP_'):]
    path = DEST + '/Materials/' + short
    mi = E.load_asset(path) if E.does_asset_exist(path) else T.create_asset(short, DEST + '/Materials', unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    check(short + ' created', isinstance(mi, unreal.MaterialInstanceConstant))
    mi.set_editor_property('parent', master)
    scalars = {'UseMap': 1., 'UseRoughnessMap': 1., 'UseVertexColor': 1., 'UseInstanceColor': 1., 'Roughness': 1., 'Metallic': 0.,
               'Parts': 0., 'Shade': 1., 'Back': BACK[name]}
    for key, value in scalars.items():
        M.set_material_instance_scalar_parameter_value(mi, key, value)
    M.set_material_instance_texture_parameter_value(mi, 'Map', texture(row['base_color'], True))
    M.set_material_instance_texture_parameter_value(mi, 'RoughnessMap', texture(row['roughness'], False))
    if row['masked']:
        overrides = mi.get_editor_property('base_property_overrides')
        overrides.set_editor_property('override_opacity_mask_clip_value', True)
        overrides.set_editor_property('opacity_mask_clip_value', float(row['alpha_clip']))
        mi.set_editor_property('base_property_overrides', overrides)
    M.update_material_instance(mi)
    check(short + ' saved', E.save_loaded_asset(mi))
    read = {key: M.get_material_instance_scalar_parameter_value(mi, key) for key in scalars}
    check(short + ' parameters', all(abs(read[key] - scalars[key]) < 1e-6 for key in scalars), read)
    report['materials'].append({'name': short, 'parent': master.get_path_name(), 'parameters': read, 'masked': bool(row['masked'])})
    write_report()
    return mi


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
    data.auto_generate_collision = False
    data.generate_lightmap_u_vs = False  # Må: ellers overskrives UV-kanal 1, og den er vinden.
    data.convert_scene = True
    data.convert_scene_unit = True
    data.force_front_x_axis = True
    data.set_editor_property('coordinate_system_policy', unreal.CoordinateSystemPolicy.MATCH_UP_FORWARD_AXES)
    # Målt i townbuildings/README.md: FBX (x,y,z) blir UE (-y,-x,z). +90 grader gir (x,-y,z).
    data.import_rotation = unreal.Rotator(pitch=0, yaw=90, roll=0)
    data.vertex_color_import_option = unreal.VertexColorImportOption.REPLACE
    data.normal_import_method = unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS
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


def wind_range(desc):
    """UV-kanal 1 slik den er lagret. x er vinden. FBX-importen snur V (1 - v), så y er 1, ikke 0; flora-skyggeren leser y bare når Parts er 1."""
    count = desc.get_vertex_instance_count()
    xs, ys = [], []
    index = 0
    while len(xs) < count and index < count * 4 + 64:
        vid = unreal.VertexInstanceID()
        assert vid.import_text('(IDValue=' + str(index) + ')')
        if desc.is_vertex_instance_valid(vid):
            uv = desc.get_vertex_instance_uv(vid, 1)
            xs.append(uv.x)
            ys.append(uv.y)
        index += 1
    assert len(xs) == count, ('incomplete vertex instance scan', len(xs), count)
    return min(xs), max(xs), min(ys), max(ys)


def check_lods(mesh, name, wanted):
    check(name + ' three LODs', S.get_lod_count(mesh) == 3, S.get_lod_count(mesh))
    out = []
    for lod in range(3):
        row = wanted[name if lod == 0 else name + '_LOD' + str(lod)]
        check(name + ' LOD' + str(lod) + ' accepted in Blender', row.get('ok') is True)
        label = name + ' LOD' + str(lod)
        desc = mesh.get_static_mesh_description(lod)
        check(label + ' saved description', desc is not None)
        points = saved_vertices(desc)
        low = [min(p[k] for p in points) for k in range(3)]
        high = [max(p[k] for p in points) for k in range(3)]
        # Blender (x,y,z) meter skal være UE (100x,-100y,100z) centimeter (model_contract.json).
        a, b = row['bounds_m']
        want_low, want_high = [a[0] * 100, -b[1] * 100, a[2] * 100], [b[0] * 100, -a[1] * 100, b[2] * 100]
        error = max(abs(x - y) for x, y in zip(low + high, want_low + want_high))
        check(label + ' bounds and orientation', error < BOUNDS_TOLERANCE_CM, {'actual': [low, high], 'expected': [want_low, want_high], 'error_cm': error})
        triangles = desc.get_triangle_count()
        check(label + ' triangles', abs(triangles - row['tris']) <= max(8, row['tris'] // 100), {'actual': triangles, 'expected': row['tris']})
        channels = S.get_num_uv_channels(mesh, lod)
        check(label + ' two UV channels', channels == 2, channels)
        settings = S.get_lod_build_settings(mesh, lod)
        check(label + ' no lightmap UVs', not settings.get_editor_property('generate_lightmap_u_vs'))
        x0, x1, y0, y1 = wind_range(desc)
        check(label + ' wind kept in UV channel 1', abs(x0 - row['sway'][0]) < SWAY_TOLERANCE and abs(x1 - row['sway'][1]) < SWAY_TOLERANCE,
              {'actual': [x0, x1], 'expected': row['sway'], 'v': [y0, y1]})
        sections = mesh.get_num_sections(lod)
        slots = [S.get_lod_material_slot(mesh, lod, section) for section in range(sections)]
        out.append({'lod': lod, 'triangles': triangles, 'uv_channels': channels, 'wind_x': [x0, x1], 'wind_v': [y0, y1], 'sections': sections, 'material_slots': slots})
    return out


def main():
    wanted = json.loads((ROOT / 'reimport.json').read_text())
    rows = json.loads((ROOT / 'materials.json').read_text())
    mats = {name: material(name, row) for name, row in rows.items()}
    for name in PLANTS:
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
        check(name + ' one material', len(set(slots)) == 1 and len(slots) == 1, slots)
        nanite = mesh.get_editor_property('nanite_settings')
        nanite.enabled = False
        mesh.set_editor_property('nanite_settings', nanite)
        for lod in range(S.get_lod_count(mesh)):
            # Hver LOD for seg: en importert LOD kan ha fått standardinnstillingene, og de genererer lightmap-UV-er i kanal 1.
            settings = S.get_lod_build_settings(mesh, lod)
            settings.set_editor_property('generate_lightmap_u_vs', False)
            settings.set_editor_property('use_full_precision_u_vs', True)
            settings.set_editor_property('recompute_normals', False)
            settings.set_editor_property('recompute_tangents', True)
            settings.set_editor_property('distance_field_resolution_scale', 0.)  # Komponentene påvirker ikke distance field-lys.
            S.set_lod_build_settings(mesh, lod, settings)
        # Ingen kollisjon: flora-komponentene har NoCollision, og plantene stopper ikke spilleren.
        S.remove_collisions(mesh)
        for section in range(mesh.get_num_sections(0)):
            if S.is_section_collision_enabled(mesh, 0, section):
                S.enable_section_collision(mesh, False, 0, section)
        check(name + ' no simple collision', S.get_simple_collision_count(mesh) == 0, S.get_simple_collision_count(mesh))
        check(name + ' screen sizes', S.set_lod_screen_sizes(mesh, SCREEN_SIZES))
        check(name + ' saved', E.save_loaded_asset(mesh))
        lods = check_lods(mesh, name, wanted)
        check(name + ' vertex colours', S.has_vertex_colors(mesh))
        box = mesh.get_bounds()
        row = {'name': name, 'asset': mesh.get_path_name(), 'lods': lods, 'materials': [m.get_name() for m in [mesh.get_material(0)] if m],
               'screen_sizes': list(S.get_lod_screen_sizes(mesh)), 'center_cm': [box.origin.x, box.origin.y, box.origin.z],
               'size_cm': [2 * box.box_extent.x, 2 * box.box_extent.y, 2 * box.box_extent.z]}
        report['meshes'].append(row)
        write_report()
        unreal.log('DESERTPLANTS ' + name + ' materials=' + ','.join(row['materials']) + ' ' +
                   ' '.join('LOD%d tris=%d uv=%d wind_x=%.3f..%.3f' % (x['lod'], x['triangles'], x['uv_channels'], x['wind_x'][0], x['wind_x'][1]) for x in lods))
    report['status'] = 'PASS'
    write_report()
    unreal.log('DESERTPLANTS IMPORT PASS')


try:
    main()
except Exception:
    report['status'] = 'FAIL'
    report['error'] = traceback.format_exc()
    write_report()
    raise
