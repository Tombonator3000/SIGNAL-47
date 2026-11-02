"""Explicit leased-editor job. Clone one material; never modify source assets or geometry.
Run after root applies the reviewed C++ patch. Graph compilation is not GPU pixel acceptance.
"""
import importlib.util,json,hashlib,os,traceback
from pathlib import Path
import unreal
project=Path(unreal.Paths.project_dir()).resolve()
spec=importlib.util.spec_from_file_location('s47_motel_native',project/'Tools/motelv2/native.py')
N=importlib.util.module_from_spec(spec);spec.loader.exec_module(N)
E,M=N.E,N.M
SOURCE='/Game/MotelV2/Materials/M_MotelV2_GlowOffice'
TARGET='/Game/MotelV2/Materials/M_MotelV2_GlowOfficeDaylight'
OWNER='LIGHT01-localXcm200-v1'
output=Path(os.environ.get('S47_LIGHT01_REPORT','/home/tom/scratch-bm/Signal47-lights-final-20261102/material-graph.json'))
report=N.Report(output,'LIGHT01 exact mixed Office material clone')
def check(label,condition,detail=None):report.check(label,condition,detail)
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def prepare():
 source_file=project/'Content/MotelV2/Materials/M_MotelV2_GlowOffice.uasset'
 before=sha(source_file)
 source=unreal.load_asset(SOURCE);check('exact source is graph material',isinstance(source,unreal.Material))
 existed=E.does_asset_exist(TARGET)
 clone=unreal.load_asset(TARGET)if existed else E.duplicate_asset(SOURCE,TARGET)
 check('separate clone only',isinstance(clone,unreal.Material)and clone!=source)
 if existed:
  check('existing clone owned',E.get_metadata_tag(clone,'S47Light01Owner')==OWNER)
  check('source package unchanged since clone',E.get_metadata_tag(clone,'S47Light01SourceSHA256')==before)
 # Compare every pre-existing material property connection except the one deliberately wrapped.
 props=[unreal.MaterialProperty.MP_BASE_COLOR,unreal.MaterialProperty.MP_METALLIC,unreal.MaterialProperty.MP_ROUGHNESS,unreal.MaterialProperty.MP_NORMAL,unreal.MaterialProperty.MP_OPACITY,unreal.MaterialProperty.MP_OPACITY_MASK]
 def connection(material,prop):
  node=M.get_material_property_input_node(material,prop)
  return (node.get_class().get_name(),str(M.get_material_property_input_node_output_name(material,prop)))if node else None
 for prop in props:check('unchanged connection '+str(prop),connection(source,prop)==connection(clone,prop))
 for prop in ['blend_mode','two_sided','shading_model','translucency_lighting_mode']:
  check('unchanged material '+prop,source.get_editor_property(prop)==clone.get_editor_property(prop))
 existing=[n for n in M.get_material_expressions(clone)if str(n.get_editor_property('desc'))=='LIGHT01_EmissiveOutput']
 check('single owned output',len(existing)<=1)
 if not existing:
  emissive=M.get_material_property_input_node(clone,unreal.MaterialProperty.MP_EMISSIVE_COLOR)
  channel=str(M.get_material_property_input_node_output_name(clone,unreal.MaterialProperty.MP_EMISSIVE_COLOR))
  check('existing emissive expression retained',emissive is not None)
  pos=N.expr(clone,unreal.MaterialExpressionLocalPosition,desc='LIGHT01_LocalPositionCentimetres')
  mask=N.expr(clone,unreal.MaterialExpressionComponentMask,r=True,g=False,b=False,a=False,desc='LIGHT01_LocalX')
  threshold=N.expr(clone,unreal.MaterialExpressionConstant,r=200.,desc='LIGHT01_ExactGapThreshold')
  one=N.expr(clone,unreal.MaterialExpressionConstant,r=1.,desc='LIGHT01_IndoorUnchanged')
  daylight=N.expr(clone,unreal.MaterialExpressionScalarParameter,parameter_name='ExteriorDaylightMask',default_value=1.,desc='LIGHT01_ExteriorRuntimeScalar')
  choose=N.expr(clone,unreal.MaterialExpressionIf,desc='LIGHT01_OnlyLocalXGreater200')
  multiply=N.expr(clone,unreal.MaterialExpressionMultiply,desc='LIGHT01_EmissiveOutput')
  def link(a,out,b,pin):check('link '+pin,M.connect_material_expressions(a,out,b,pin))
  link(pos,'',mask,'');names=list(M.get_material_expression_input_names(choose));check('If exact five pins',len(names)==5,names)
  link(mask,'',choose,names[0]);link(threshold,'',choose,names[1]);link(daylight,'',choose,names[2]);link(one,'',choose,names[3]);link(one,'',choose,names[4]);link(emissive,channel,multiply,'A');link(choose,'',multiply,'B')
  check('only emissive wrapped',M.connect_material_property(multiply,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR))
 errors=list(M.recompile_material(clone));check('shader compilation',not errors,errors)
 check('runtime scalar exists',str('ExteriorDaylightMask')in[str(v)for v in M.get_scalar_parameter_names(clone)])
 check('existing emissive vector preserved','EmissiveFactor'in[str(v)for v in M.get_vector_parameter_names(clone)])
 for prop in props:check('postcompile unchanged connection '+str(prop),connection(source,prop)==connection(clone,prop))
 E.set_metadata_tag(clone,'S47Light01Owner',OWNER);E.set_metadata_tag(clone,'S47Light01SourceSHA256',before)
 check('saved only cloned material',E.save_loaded_asset(clone,only_if_is_dirty=False))
 check('source package bytes untouched',sha(source_file)==before,before)
 report.data.update(status='GRAPH_COMPILED_SAVED_GPU_UNVERIFIED',source=SOURCE,target=TARGET,source_sha256=before,
  geometry='No mesh read/write, no source material modification; all four active mesh LODs retained',
  measured_source='31Oct rear-pool GlowOffice FBX MD5 703fe4b04a672d40fe9770cf6689d4c2; outside localXcm268.85..270.15, inside9.50..70.50',
  next_acceptance='Root must render scalar=0/1 on real installed mesh: exterior emission changes, ceiling remains unchanged, then production court0/6 and daylight scalar day/night.')
 report.flush();unreal.log('S47_LIGHT01_GRAPH_READY_GPU_UNVERIFIED '+str(output))

try:
 prepare()
except Exception:
 report.data.update(status='FAIL',error=traceback.format_exc());report.flush();raise
