"""Own Caliche sky material; retains the original sky and story parameters outside the town."""
import ast
from pathlib import Path
import unreal

ROOT=Path(__file__).resolve().parent
E=unreal.EditorAssetLibrary
M=unreal.MaterialEditingLibrary
T=unreal.AssetToolsHelpers.get_asset_tools()
name='M_Caliche_Sky'; dest='/Game/CalicheStreet/Atmosphere'
mat=E.load_asset(dest+'/'+name) if E.does_asset_exist(dest+'/'+name) else T.create_asset(name,dest,unreal.Material,unreal.MaterialFactoryNew())
M.delete_all_material_expressions(mat)
mat.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT)
mat.set_editor_property('blend_mode',unreal.BlendMode.BLEND_OPAQUE)
mat.set_editor_property('two_sided',True)
mat.set_editor_property('is_sky',True)
def node(cls,**props):
 n=M.create_material_expression(mat,cls)
 for key,value in props.items():n.set_editor_property(key,value)
 return n
def scalar(key,value):return node(unreal.MaterialExpressionScalarParameter,parameter_name=key,default_value=value)
def vector(key,value):return node(unreal.MaterialExpressionVectorParameter,parameter_name=key,default_value=unreal.LinearColor(*value))
def custom(code,inputs,out=3):
 n=node(unreal.MaterialExpressionCustom,code=code,output_type={3:unreal.CustomMaterialOutputType.CMOT_FLOAT3,4:unreal.CustomMaterialOutputType.CMOT_FLOAT4}[out])
 rows=[]
 for key,source,output in inputs:
  ci=unreal.CustomInput();ci.set_editor_property('input_name',key);rows.append(ci)
 n.set_editor_property('inputs',rows)
 for key,source,output in inputs:assert M.connect_material_expressions(source,output,n,key)
 return n

# Read only the two source constants. Do not execute the web importer or regenerate shared assets.
source=ast.parse((ROOT.parent/'webexport/import_web.py').read_text())
constants={}
for n in source.body:
 if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id in ('SKY_DIR','SKY_DOME') for t in n.targets):
  key=n.targets[0].id
  constants[key]=ast.literal_eval(n.value) if isinstance(n.value,ast.Constant) else constants['SKY_DIR']+ast.literal_eval(n.value.right)
base=constants['SKY_DOME'];assert base.endswith('return col;')
code=base[:-len('return col;')]+(ROOT/'atmosphere.hlsl').read_text()
inputs=[('V',node(unreal.MaterialExpressionCameraVectorWS),''),('Yaw',scalar('Yaw',0),''),
 ('Dawn',scalar('Dawn',0),''),('Sun',scalar('Sun',0),''),('Midday',scalar('Midday',0),''),
 ('ConceptBlend',scalar('ConceptBlend',0),''),('SunDir',vector('SunDir',(.98,-.02,-.19,0)),''),
 ('Flash',scalar('Flash',0),''),('FlashDir',vector('FlashDir',(.957,.048,-.287,0)),''),
 ('HoleSky',vector('HoleSky',(.0044,.0066,.016,0)),''),('Hole',scalar('Hole',0),''),
 ('HoleDir',vector('HoleDir',(.3718,.5299,-.7622,0)),''),('HoleR',vector('HoleR',(.0593,.0803,0,0)),''),
 ('Pano',node(unreal.MaterialExpressionTextureObjectParameter,parameter_name='Pano',texture=E.load_asset('/Engine/EngineResources/WhiteSquareTexture')), '')]
for i in range(3):
 v=vector('Glow'+str(i),(1,0,0,1));v4=custom('return float4(C,A);',[('C',v,''),('A',v,'A')],4)
 inputs.append(('Glow'+str(i),v4,''))
color=custom(code,inputs)
assert M.connect_material_property(color,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
M.recompile_material(mat);E.save_loaded_asset(mat)
unreal.log('CALICHE CONCEPT ATMOSPHERE SAVED')

# Reuse our project's established graph builder, without executing its importer or touching /Game/Web.
keys={'SKY_DIR','SKY_DOME','SKY_BAND','STAR_WPO','STAR_COL','UNTONE'}
values={'EL_TOP':8.4375,'EL_BOT':-2.8125,'FOOT':.32}
for n in source.body:
 if isinstance(n,ast.Assign) and isinstance(n.targets[0],ast.Name) and n.targets[0].id in keys:
  values[n.targets[0].id]=eval(compile(ast.Expression(n.value),'<owned material constant>','eval'),{'__builtins__':{}},values)
def fresh(path):
 assert path.startswith('/Game/CalicheStreet/Atmosphere/')
 folder,name=path.rsplit('/',1)
 m=E.load_asset(path) if E.does_asset_exist(path) else T.create_asset(name,folder,unreal.Material,unreal.MaterialFactoryNew())
 M.delete_all_material_expressions(m)
 return m
ns={'unreal':unreal,'fresh':fresh,'mel':M,'eal':E,'mpc':E.load_asset('/Game/Web/Base/MPC_Web'),
    'white':E.load_asset('/Game/Web/Base/T_White'),'white_lin':E.load_asset('/Game/Web/Base/T_WhiteLinear'),
    'UNTONE':values['UNTONE']}
graph=next(n for n in source.body if isinstance(n,ast.ClassDef) and n.name=='Graph')
exec(compile(ast.Module(body=[graph],type_ignores=[]),'<owned material graph>','exec'),ns)
Graph=ns['Graph'];MP=unreal.MaterialProperty;B=unreal.BlendMode
def sun(g):
 return [('Dawn',g.scalar('Dawn',0)),('Sun',g.scalar('Sun',0)),('SunDir',g.vector('SunDir',(.98,-.02,-.19,0))),
         ('ConceptBlend',g.scalar('ConceptBlend',0))]+[('Glow'+str(i),g.vector4('Glow'+str(i),(1,0,0,1))) for i in range(3)]
g=Graph(dest+'/M_Caliche_Horizon',B.BLEND_ALPHA_COMPOSITE)
code=values['SKY_BAND'].replace('return float4(col * a, a);','return float4(col * a, a) * (1.0 - saturate(ConceptBlend));')
col=g.custom(code,[('V',g.builtin(unreal.MaterialExpressionCameraVectorWS)),('Yaw',g.scalar('Yaw',0)),('U',g.mask(g.uv(0),'r')),
 ('Night',g.tex('Night')),('Day',g.tex('Day')),('Gain',g.scalar('Gain',1)),('Rim',g.scalar('Rim',0)),
 ('BodyK',g.scalar('BodyK',.2)),('Dip',g.scalar('Dip',0)),('Texel',g.scalar('Texel',1/128)),
 ('Fog',g.vector('Fog',(.0155,.0212,.0353,0))),('Mesa',g.vector('Mesa',(.0045,.0055,.0085,0))),
 ('Land',g.vector('Land',(.009,.009,.012,0))),('EdgeFog',g.scalar('EdgeFog',1))]+sun(g),4)
g.out(g.mask(col,'rgb'),MP.MP_EMISSIVE_COLOR);g.out(g.mask(col,'a'),MP.MP_OPACITY);g.save()

g=Graph(dest+'/M_Caliche_Stars',B.BLEND_ADDITIVE,ism=True)
VP=unreal.MaterialExposedViewProperty
tan=g.view_prop(getattr(VP,'MEVP_TAN_HALF_FIELD_OF_VIEW',None) or VP.TAN_HALF_FIELD_OF_VIEW,'g')
vsz=g.view_prop(getattr(VP,'MEVP_VIEW_SIZE',None) or VP.VIEW_SIZE,'g')
wpo=g.custom(values['STAR_WPO'],[('PR',g.scalar('PR',1)),('Size',g.cdata(6,1)),('IO',g.instance_origin()),
 ('Cam',g.builtin(unreal.MaterialExpressionCameraPositionWS)),('Tan',tan),('VSize',vsz),('UV',g.uv(0)),
 ('Right',g.view_axis((1,0,0))),('Up',g.view_axis((0,1,0))),('WP',g.builtin(unreal.MaterialExpressionWorldPosition))])
g.out(wpo,MP.MP_WORLD_POSITION_OFFSET)
noise=values['SKY_DOME'][values['SKY_DOME'].index('struct N {'):values['SKY_DOME'].index('float3 bandN')]
cloud='''float3 dd=normalize(Dir); float2 pp=dd.xz/max(.12,dd.y+.15);
float3 cp=float3(pp*.7,4.71); float bank=nz.fbm(cp+float3(1.4,5.7,0));
float detail=nz.fbm(cp*3.8+float3(8.1,0,2.4));
float cover=smoothstep(.43,.64,bank*.78+detail*.22)*smoothstep(-.015,.065,dd.y)*(1.-smoothstep(.72,.99,dd.y));
return c*a*lerp(1.,.44*(1.-cover*.92),saturate(ConceptBlend));'''
code=noise+values['STAR_COL'].replace('return c * a;',cloud)
c=g.custom(code,[('Dir',g.cdata3(0)),('Col',g.cdata3(3)),('Time',g.builtin(unreal.MaterialExpressionTime)),
 ('Ph',g.cdata(7)),('Dawn',g.scalar('Dawn',0)),('UV',g.uv(0)),('Size',g.cdata(6,1)),
 ('HoleDir',g.vector('HoleDir',(.3718,.5299,-.7622,0))),('HoleR',g.vector('HoleR',(.0593,.0803,0,0))),
 ('Hole',g.scalar('Hole',0)),('ConceptBlend',g.scalar('ConceptBlend',0))])
g.out(g.untoned(c),MP.MP_EMISSIVE_COLOR);g.save()
unreal.log('CALICHE HORIZON AND CLOUD-OCCLUDED STARS SAVED')
