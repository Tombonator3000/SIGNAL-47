# Imports the Blender models (FBX) from the Unity archive into /Game/Models and gives them flat
# materials with the colours from the Unity material files, so the Unreal test uses the same
# originals as the archive. Run from the project folder:
#   UnrealEditor-Cmd Signal47.uproject -run=pythonscript -script=Scripts/import-models.py -unattended -nullrhi
# The Unity folder is read only; nothing there is changed.
import os
import re
import unreal

eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
root = os.path.normpath(os.path.join(unreal.Paths.project_dir(), '..', '..'))
art = os.path.join(root, 'Unity', 'Assets', 'Signal47', 'Art')

MODELS = {
    # name in /Game/Models: file under Unity/Assets/Signal47/Art
    'ControlDesk': 'Refined/SM_ControlDesk_A.fbx',
    'CrtTerminal': 'Refined/SM_CRT_Terminal_A.fbx',
    'DeskPhone': 'Refined/SM_DeskPhone_A.fbx',
    'Printer': 'Refined/SM_DotMatrixPrinter_A.fbx',
    'MugIntact': 'Refined/SM_Mug_Intact_A.fbx',
    'ReceiverRack': 'Refined/SM_ReceiverRack_A.fbx',
    'DishPedestal': 'Refined/SM_RadioDish_Pedestal_A.fbx',
    'DishBowl': 'Visual10/Models/V10_RadioDish_BowlCorrected.fbx',
    'Chair': 'Workstation18/W18_Chair.fbx',
    'WorkCrt': 'Workstation18/W18_CRT.fbx',
    'Keyboard': 'Workstation18/W18_Keyboard.fbx',
    'StoryPhone': 'Authored/SM_StoryPhone.fbx',
    'LampPole': 'Environment27/Models/E27_LampPole.fbx',
    'Generator': 'Environment27/Models/E27_DieselGenerator.fbx',
    'Cabinet': 'Environment27/Models/E27_DistributionCabinet.fbx',
    'Boulder': 'Environment27/Models/E27_Boulder_LOD0.fbx',
    'SandstoneA': 'Environment27/Models/E27_Sandstone_A.fbx',
    'SandstoneB': 'Environment27/Models/E27_Sandstone_B.fbx',
    'TuftA': 'Visual10/Models/V10_DryTuft_A.fbx',
    'TuftB': 'Visual10/Models/V10_DryTuft_B.fbx',
    'TuftC': 'Visual10/Models/V10_DryTuft_C.fbx',
    'WallLamp': 'Visual10/Models/V10_WallLamp.fbx',
    'TwinFluorescent': 'Visual10/Models/V10_TwinFluorescent.fbx',
    'OfficeDesk': 'ThirdParty/PolyHaven/metal_office_desk/metal_office_desk.fbx',
    'DeskLamp': 'ThirdParty/PolyHaven/desk_lamp_arm_01/desk_lamp_arm_01.fbx',
    'Transceiver': 'ThirdParty/PolyHaven/vintage_radio_transceiver/vintage_radio_transceiver.fbx',
}


def unity_material(name):
    """Colour, smoothness, metallic and emission from a Unity (URP) material file, by name."""
    for folder, _, files in os.walk(art):
        if name + '.mat' in files:
            text = open(os.path.join(folder, name + '.mat'), errors='ignore').read()
            def color(key, default):
                m = re.search(r'- %s: \{r: ([\d.eE+-]+), g: ([\d.eE+-]+), b: ([\d.eE+-]+)' % key, text)
                return tuple(float(v) for v in m.groups()) if m else default
            def num(key, default):
                m = re.search(r'- %s: ([\d.eE+-]+)' % key, text)
                return float(m.group(1)) if m else default
            return {'color': color('_BaseColor', color('_Color', (0.5, 0.5, 0.5))), 'smooth': num('_Smoothness', 0.3),
                    'metal': num('_Metallic', 0.0), 'emission': color('_EmissionColor', (0, 0, 0))}
    return None


def fresh_material(name, folder):
    path = folder + '/' + name
    if eal.does_asset_exist(path):
        eal.delete_asset(path)
    return tools.create_asset(name, folder, unreal.Material, unreal.MaterialFactoryNew())


# One flat base material: Color, Roughness, Metallic, Emissive.
base = fresh_material('M_S47Flat', '/Game/Materials')
color = mel.create_material_expression(base, unreal.MaterialExpressionVectorParameter, -400, -100)
color.set_editor_property('parameter_name', 'Color')
color.set_editor_property('default_value', unreal.LinearColor(0.5, 0.5, 0.5, 1.0))
rough = mel.create_material_expression(base, unreal.MaterialExpressionScalarParameter, -400, 100)
rough.set_editor_property('parameter_name', 'Roughness')
rough.set_editor_property('default_value', 0.7)
metal = mel.create_material_expression(base, unreal.MaterialExpressionScalarParameter, -400, 200)
metal.set_editor_property('parameter_name', 'Metallic')
metal.set_editor_property('default_value', 0.0)
emis = mel.create_material_expression(base, unreal.MaterialExpressionVectorParameter, -400, 300)
emis.set_editor_property('parameter_name', 'Emissive')
emis.set_editor_property('default_value', unreal.LinearColor(0, 0, 0, 1.0))
mel.connect_material_property(color, '', unreal.MaterialProperty.MP_BASE_COLOR)
mel.connect_material_property(rough, '', unreal.MaterialProperty.MP_ROUGHNESS)
mel.connect_material_property(metal, '', unreal.MaterialProperty.MP_METALLIC)
mel.connect_material_property(emis, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
mel.recompile_material(base)
eal.save_loaded_asset(base)

# The night sky: an unlit, two-sided material that shows a texture parameter (the stars are a
# texture the game makes at start).
sky = fresh_material('M_S47Sky', '/Game/Materials')
sky.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
sky.set_editor_property('two_sided', True)
stars = mel.create_material_expression(sky, unreal.MaterialExpressionTextureSampleParameter2D, -400, 0)
stars.set_editor_property('parameter_name', 'Stars')
stars.set_editor_property('sampler_type', unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
mel.connect_material_property(stars, 'RGB', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
mel.recompile_material(sky)
eal.save_loaded_asset(sky)

# The CRT pictures: unlit, a render target the HUD draws into, times Glow (0 when the screen is off).
screen = fresh_material('M_S47Screen', '/Game/Materials')
screen.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
suv = mel.create_material_expression(screen, unreal.MaterialExpressionTextureCoordinate, -900, 0)
stex = mel.create_material_expression(screen, unreal.MaterialExpressionTextureSampleParameter2D, -600, 0)
stex.set_editor_property('parameter_name', 'Screen')
stex.set_editor_property('sampler_type', unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
mel.connect_material_expressions(suv, '', stex, 'UVs')
sglow = mel.create_material_expression(screen, unreal.MaterialExpressionScalarParameter, -600, 200)
sglow.set_editor_property('parameter_name', 'Glow')
sglow.set_editor_property('default_value', 1.0)
smul = mel.create_material_expression(screen, unreal.MaterialExpressionMultiply, -300, 50)
mel.connect_material_expressions(stex, 'RGB', smul, 'A')
mel.connect_material_expressions(sglow, '', smul, 'B')
mel.connect_material_property(smul, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
mel.recompile_material(screen)
eal.save_loaded_asset(screen)

# Window glass: translucent, nearly clear, with a faint blue tint.
glass = fresh_material('M_S47Glass', '/Game/Materials')
glass.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
glass.set_editor_property('two_sided', True)
gc = mel.create_material_expression(glass, unreal.MaterialExpressionConstant3Vector, -400, 0)
gc.set_editor_property('constant', unreal.LinearColor(0.55, 0.7, 0.8, 1.0))
go = mel.create_material_expression(glass, unreal.MaterialExpressionConstant, -400, 150)
go.set_editor_property('r', 0.06)
gr = mel.create_material_expression(glass, unreal.MaterialExpressionConstant, -400, 250)
gr.set_editor_property('r', 0.1)
mel.connect_material_property(gc, '', unreal.MaterialProperty.MP_BASE_COLOR)
mel.connect_material_property(go, '', unreal.MaterialProperty.MP_OPACITY)
mel.connect_material_property(gr, '', unreal.MaterialProperty.MP_ROUGHNESS)
mel.recompile_material(glass)
eal.save_loaded_asset(glass)

instances = {}


def instance_for(mat_name):
    """A material instance of M_S47Flat with the Unity material's values, cached by name."""
    if mat_name in instances:
        return instances[mat_name]
    u = unity_material(mat_name)
    path = '/Game/Materials/MI_' + mat_name
    if eal.does_asset_exist(path):
        eal.delete_asset(path)
    mi = tools.create_asset('MI_' + mat_name, '/Game/Materials', unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mi.set_editor_property('parent', base)
    if u:
        c = u['color']
        mel.set_material_instance_vector_parameter_value(mi, 'Color', unreal.LinearColor(c[0], c[1], c[2], 1.0))
        mel.set_material_instance_scalar_parameter_value(mi, 'Roughness', 1.0 - u['smooth'])
        mel.set_material_instance_scalar_parameter_value(mi, 'Metallic', u['metal'])
        e = u['emission']
        mel.set_material_instance_vector_parameter_value(mi, 'Emissive', unreal.LinearColor(e[0], e[1], e[2], 1.0))
    eal.save_loaded_asset(mi)
    instances[mat_name] = (mi, u is not None)
    return instances[mat_name]


missing = []
for name, rel in MODELS.items():
    src = os.path.join(art, rel)
    if not os.path.exists(src):
        unreal.log_warning('S47: missing ' + src)
        continue
    task = unreal.AssetImportTask()
    task.set_editor_property('filename', src)
    task.set_editor_property('destination_path', '/Game/Models')
    task.set_editor_property('destination_name', name)
    task.set_editor_property('replace_existing', True)
    task.set_editor_property('automated', True)
    task.set_editor_property('save', True)
    ui = unreal.FbxImportUI()
    ui.set_editor_property('import_mesh', True)
    ui.set_editor_property('import_materials', False)
    ui.set_editor_property('import_textures', False)
    ui.set_editor_property('import_as_skeletal', False)
    ui.set_editor_property('import_animations', False)
    ui.set_editor_property('mesh_type_to_import', unreal.FBXImportType.FBXIT_STATIC_MESH)
    sm = ui.static_mesh_import_data
    sm.set_editor_property('combine_meshes', True)
    sm.set_editor_property('generate_lightmap_u_vs', False)
    sm.set_editor_property('auto_generate_collision', True)
    sm.set_editor_property('build_nanite', False)
    task.set_editor_property('options', ui)
    tools.import_asset_tasks([task])
    mesh = eal.load_asset('/Game/Models/' + name)
    if not mesh:
        unreal.log_warning('S47: import failed for ' + name)
        continue
    bounds = mesh.get_bounding_box()
    size = bounds.max - bounds.min
    slots = []
    for i, m in enumerate(mesh.get_editor_property('static_materials')):
        slot = str(m.get_editor_property('material_slot_name'))
        slots.append(slot)
        mi, known = instance_for(slot)
        if not known:
            missing.append(name + ':' + slot)
        mesh.set_material(i, mi)
    eal.save_loaded_asset(mesh)
    unreal.log('S47: %s  size %.0f x %.0f x %.0f cm  slots %s' % (name, size.x, size.y, size.z, ', '.join(slots)))
if missing:
    unreal.log_warning('S47: no Unity material found for: ' + ', '.join(missing))
unreal.log('S47: models imported')
