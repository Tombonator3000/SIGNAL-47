"""Build SARO's continuous world-projected asphalt and worn bay paint.

Reuses the project's own CalicheStreet OldAsphalt maps without modifying them.
The asphalt has no baked tile border; dust follows only the exterior of the
L-shaped forecourt, never the seam between its two mesh rectangles.
Run through run_parking_import.py under the production lease.
"""
import json
import os
from pathlib import Path
import traceback

import unreal

DEST = '/Game/Backdrop'
OWNER = 'S47Parking_v1'
NAMES = ('M_S47_ParkingAsphalt', 'M_S47_ParkingPaint')
SOURCE = '/Game/CalicheStreet/Ground/Materials/Textures/T_M_CS_OldAsphalt_'
E = unreal.EditorAssetLibrary
L = unreal.MaterialEditingLibrary
T = unreal.AssetToolsHelpers.get_asset_tools()


def node(mat, cls, x, y, **properties):
    item = L.create_material_expression(mat, cls, x, y)
    for name, value in properties.items():
        item.set_editor_property(name, value)
    return item


def link(source, pin, target, target_pin):
    if not L.connect_material_expressions(source, pin, target, target_pin):
        raise RuntimeError('Material connection failed: ' + target_pin)


def output(source, pin, prop):
    if not L.connect_material_property(source, pin, prop):
        raise RuntimeError('Material output connection failed: ' + str(prop))


def custom(mat, code, inputs, output_type, x, y):
    item = node(mat, unreal.MaterialExpressionCustom, x, y, code=code, output_type=output_type)
    pins = []
    for name, _, _ in inputs:
        entry = unreal.CustomInput()
        entry.set_editor_property('input_name', name)
        pins.append(entry)
    item.set_editor_property('inputs', pins)
    for name, source, pin in inputs:
        link(source, pin, item, name)
    return item


def sample(mat, texture, world, metres, x, y, sampler, rotated=False):
    # The retained source is not edge-periodic. Mirror coordinates instead of
    # wrapping opposite source edges, eliminating the square colour jumps.
    # Shared clamp also prevents bilinear/trilinear filtering across wrap seams.
    coord = 'float2(P.x,P.y) / %.6f' % (metres * 100.)
    if rotated:
        coord = 'float2(P.x*.8-P.y*.6,P.x*.6+P.y*.8) / %.6f + float2(.371,.173)' % (metres * 100.)
    code = ('float2 u=' + coord + '; float2 mirrored=1.-abs(frac(u*.5)*2.-1.); '
            'return clamp(mirrored,1./4096.,1.-1./4096.);')
    uv = custom(mat, code, [('P', world, '')], unreal.CustomMaterialOutputType.CMOT_FLOAT2, x - 180, y)
    item = node(mat, unreal.MaterialExpressionTextureSample, x, y, texture=texture,
                sampler_type=sampler, sampler_source=unreal.SamplerSourceMode.SSM_CLAMP_WORLD_GROUP_SETTINGS)
    link(uv, '', item, 'UVs')
    return item


def build(name, textures, existing):
    mat = existing or T.create_asset(name, DEST, unreal.Material, unreal.MaterialFactoryNew())
    if mat is None:
        raise RuntimeError('Could not create ' + name)
    if existing:
        L.delete_all_material_expressions(mat)
    mat.set_editor_property('tangent_space_normal', False)
    paint = name == NAMES[1]
    mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_MASKED if paint else unreal.BlendMode.BLEND_OPAQUE)
    world = node(mat, unreal.MaterialExpressionWorldPosition, -1300, 0)
    color = sample(mat, textures['base_color'], world, 3.6, -850, -300, unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
    roughness_sampler = unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR
    compression = textures['roughness'].get_editor_property('compression_settings')
    if compression == unreal.TextureCompressionSettings.TC_GRAYSCALE:
        roughness_sampler = unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_GRAYSCALE
    elif compression == unreal.TextureCompressionSettings.TC_MASKS:
        roughness_sampler = unreal.MaterialSamplerType.SAMPLERTYPE_MASKS
    rough = sample(mat, textures['roughness'], world, 3.6, -850, 100, roughness_sampler)
    normal = sample(mat, textures['normal'], world, 3.6, -850, 400, unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    geometric = node(mat, unreal.MaterialExpressionVertexNormalWS, -600, 600)
    normal_world = custom(mat,
        'float3 n=normalize(Macro); float3 t=normalize(float3(1,0,0)-n*n.x+float3(0,.00001,0)); '
        'float3 b=normalize(cross(n,t)); float2 f=frac(P.xy/720.); '
        'float2 direction=1.-2.*step(.5,f); float2 mirrored=1.-abs(f*2.-1.); '
        'float2 fade=smoothstep(0.,.018,min(mirrored,1.-mirrored)); '
        'float2 detail=Detail.xy*direction*fade*.24; '
        'return normalize(t*detail.x+b*detail.y+n*Detail.z);',
        [('Detail', normal, 'RGB'), ('Macro', geometric, ''), ('P', world, '')], unreal.CustomMaterialOutputType.CMOT_FLOAT3, 300, 400)
    output(normal_world, '', unreal.MaterialProperty.MP_NORMAL)
    if paint:
        base = custom(mat, 'float wear=saturate((C.r-.025)*5.); return float3(.48,.44,.33)*( .82+.18*wear);',
            [('C', color, 'RGB')], unreal.CustomMaterialOutputType.CMOT_FLOAT3, 100, -300)
        mask = custom(mat, 'return saturate((C.r-.027)*42.);', [('C', color, 'RGB')],
            unreal.CustomMaterialOutputType.CMOT_FLOAT1, 100, -100)
        output(mask, '', unreal.MaterialProperty.MP_OPACITY_MASK)
        rough_out = node(mat, unreal.MaterialExpressionConstant, 100, 100, r=.92)
    else:
        secondary = sample(mat, textures['base_color'], world, 8.3, -850, -500,
                           unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, rotated=True)
        # Exact L perimeter, not independent rectangle edges: the shared x20.1..39.8,
        # z18 join has no artificial dust stripe. Coordinates are site-local Three X/Z.
        dust = custom(mat, '''
float2 p=float2(P.x,-P.y)*.01;
float2 v[6]={float2(20.1,5.9),float2(39.8,5.9),float2(39.8,23),float2(10,23),float2(10,18),float2(20.1,18)};
float dist=10000.;
[unroll] for(int i=0;i<6;i++) {
 float2 a=v[i], b=v[(i+1)%6], d=b-a;
 float h=saturate(dot(p-a,d)/dot(d,d));
 dist=min(dist,length(p-(a+h*d)));
}
float variation=.76+.12*sin(p.x*1.7+p.y*1.13)+.12*sin(p.x*4.7-p.y*2.9);
return .55*(1.-smoothstep(.025,.45*variation,dist));
''', [('P', world, '')], unreal.CustomMaterialOutputType.CMOT_FLOAT1, -400, -100)
        base = custom(mat, 'float3 asphalt=lerp(A,B,.18)*1.24; return lerp(asphalt,float3(.30,.235,.155),Dust);',
            [('A', color, 'RGB'), ('B', secondary, 'RGB'), ('Dust', dust, '')],
            unreal.CustomMaterialOutputType.CMOT_FLOAT3, 100, -300)
        rough_out = custom(mat, 'return lerp(clamp(R,.82,.96),.98,Dust);',
            [('R', rough, 'R'), ('Dust', dust, '')], unreal.CustomMaterialOutputType.CMOT_FLOAT1, 100, 100)
    output(base, '', unreal.MaterialProperty.MP_BASE_COLOR)
    output(rough_out, '', unreal.MaterialProperty.MP_ROUGHNESS)
    spec = node(mat, unreal.MaterialExpressionConstant, 300, 650, r=.25)
    output(spec, '', unreal.MaterialProperty.MP_SPECULAR)
    errors = [str(error) for error in L.recompile_material(mat)]
    if errors:
        raise RuntimeError(name + ': ' + '; '.join(errors))
    E.set_metadata_tag(mat, 'S47Owner', OWNER)
    if not E.save_loaded_asset(mat):
        raise RuntimeError('Material save failed: ' + name)
    return {'asset': mat.get_path_name(), 'compile_errors': errors}


def main():
    report = Path(os.environ['S47_PARKING_REPORT'])
    result = {'status': 'RUNNING'}
    try:
        textures = {}
        for role in ('base_color', 'normal', 'roughness'):
            texture = unreal.load_asset(SOURCE + role)
            if not isinstance(texture, unreal.Texture2D):
                raise RuntimeError('Missing source texture: ' + SOURCE + role)
            if texture.get_editor_property('srgb') != (role == 'base_color'):
                raise RuntimeError('Source texture has unexpected color space: ' + role)
            textures[role] = texture
        if (textures['normal'].get_editor_property('compression_settings') != unreal.TextureCompressionSettings.TC_NORMALMAP
                or textures['normal'].get_editor_property('flip_green_channel')):
            raise RuntimeError('Expected existing DirectX normal map without green flip')
        existing = {}
        for name in NAMES:
            path = DEST + '/' + name
            asset = E.load_asset(path) if E.does_asset_exist(path) else None
            if asset and (not isinstance(asset, unreal.Material) or E.get_metadata_tag(asset, 'S47Owner') != OWNER):
                raise RuntimeError('Refusing unowned material: ' + path)
            existing[name] = asset
        result['sources'] = {role: asset.get_path_name() for role, asset in textures.items()}
        result['materials'] = [build(name, textures, existing[name]) for name in NAMES]
        result['status'] = 'PASS_PARKING_MATERIALS'
    except Exception:
        result['status'] = 'FAIL'
        result['error'] = traceback.format_exc()
    report.write_text(json.dumps(result, indent=2) + '\n')


if os.environ.get('S47_PARKING_APPLY') == '1':
    main()
