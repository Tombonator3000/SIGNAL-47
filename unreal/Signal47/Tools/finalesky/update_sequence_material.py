"""Build only the isolated nine-frame cloud material; root runs the guarded editor.

Duplicate the existing owned cloud graph once, then edit only the candidate.
Never call make_materials or save/recompile the original three-frame material.
The game thread must bind each bank's A/B pair and FrameMix together before drawing.
"""
from pathlib import Path
import math
import unreal


HERE = Path(__file__).resolve().parent
SOURCE = '/Game/FinaleSky/M_S47_FinalCloud'
TARGET = '/Game/FinaleSky/M_S47_FinalCloudSequence'
VOLUME_ROOT = '/Game/FinaleSky/CloudSequence'
BASE_INPUTS = ['WP','Camera','SceneDepth','PixelDepth','BankCenter','BankExtent',
               'Forward','Right','Phase','Growth','Side','Samples','Glow',
               'DensityA','DensityB','DensityC','Density','DecodeScale']
E = unreal.EditorAssetLibrary
M = unreal.MaterialEditingLibrary


def main():
    volumes = [E.load_asset(VOLUME_ROOT+'/V_S47_CloudFrame%02d'%i) for i in range(9)]
    assert all(isinstance(v,unreal.VolumeTexture) for v in volumes), 'Import all nine candidate volumes first'
    scales = [float(E.get_metadata_tag(v,'S47DensityScale')) for v in volumes]
    assert all(math.isfinite(x) and x > 0 for x in scales) and max(scales)-min(scales) < 1.e-6
    for i,v in enumerate(volumes):
        assert int(E.get_metadata_tag(v,'S47DensityFrame')) == 48+6*i
        assert int(E.get_metadata_tag(v,'S47SequenceIndex')) == i
    compression = volumes[0].get_editor_property('compression_settings')
    assert all(v.get_editor_property('compression_settings') == compression for v in volumes)
    sampler = (unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_GRAYSCALE
               if compression == unreal.TextureCompressionSettings.TC_GRAYSCALE
               else unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)

    source = E.load_asset(SOURCE)
    assert isinstance(source,unreal.Material), 'Existing owned three-frame master required'
    mat = E.load_asset(TARGET) if E.does_asset_exist(TARGET) else E.duplicate_asset(SOURCE,TARGET)
    assert isinstance(mat,unreal.Material) and mat != source
    assert mat.get_path_name() == TARGET+'.M_S47_FinalCloudSequence'
    expressions = list(M.get_material_expressions(mat))
    fields = [n for n in expressions if isinstance(n,unreal.MaterialExpressionCustom)]
    assert len(fields) == 1
    field = fields[0]
    # Guard the duplicate's ownership before mutating any graph object.
    assert field.get_outer() == mat, 'Candidate must own its field expression'
    names = list(M.get_material_expression_input_names(field))
    assert names in (BASE_INPUTS,BASE_INPUTS+['FrameMix'])
    if names == BASE_INPUTS:
        rows = list(field.get_editor_property('inputs'))
        row = unreal.CustomInput(); row.set_editor_property('input_name','FrameMix'); rows.append(row)
        field.set_editor_property('inputs',rows)

    def parameter(cls,name):
        matches = [n for n in expressions if isinstance(n,cls) and str(n.get_editor_property('parameter_name')) == name]
        assert len(matches) == 1, ('Expected one candidate parameter',name)
        assert matches[0].get_outer() == mat, ('Candidate must own parameter',name)
        return matches[0]

    # The graph retains C for compatibility, while the candidate HLSL samples only A/B.
    for name,texture in zip(('DensityA','DensityB','DensityC'),volumes[:3]):
        n = parameter(unreal.MaterialExpressionTextureObjectParameter,name)
        n.set_editor_property('texture',texture)
        n.set_editor_property('sampler_type',sampler)
    parameter(unreal.MaterialExpressionScalarParameter,'DecodeScale').set_editor_property('default_value',scales[0])
    mixes = [n for n in expressions if isinstance(n,unreal.MaterialExpressionScalarParameter)
             and str(n.get_editor_property('parameter_name')) == 'FrameMix']
    assert len(mixes) <= 1
    mix = mixes[0] if mixes else M.create_material_expression(mat,unreal.MaterialExpressionScalarParameter)
    assert mix.get_outer() == mat
    mix.set_editor_property('parameter_name','FrameMix'); mix.set_editor_property('default_value',0.)
    assert M.connect_material_expressions(mix,'',field,'FrameMix')
    field.set_editor_property('code',(HERE/'cloud_volume_sequence.hlsl').read_text())
    assert list(M.get_material_expression_input_names(field)) == BASE_INPUTS+['FrameMix']
    linked = list(M.get_inputs_for_material_expression(mat,field))
    assert len(linked) == 19 and all(n is not None for n in linked) and linked[-1] == mix
    mat.set_editor_property('material_domain',unreal.MaterialDomain.MD_SURFACE)
    mat.set_editor_property('blend_mode',unreal.BlendMode.BLEND_TRANSLUCENT)
    mat.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property('two_sided',False)
    mat.set_editor_property('disable_depth_test',True)
    mat.set_editor_property('allow_variable_rate_shading',False)
    mat.set_editor_property('translucency_pass',unreal.MaterialTranslucencyPass.MTP_AFTER_DOF)
    E.set_metadata_tag(mat,'S47CloudSequenceFrames',','.join(str(48+6*i) for i in range(9)))
    M.recompile_material(mat)
    assert E.save_loaded_asset(mat,False)
    unreal.log('S47 FINALE SEQUENCE MATERIAL COMPLETE; own candidate only; 19 pins; A/B pair plus FrameMix; source material unchanged')


if __name__ == '__main__':
    main()
