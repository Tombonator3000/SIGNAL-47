"""Original SIGNAL / 47 1420 AM building. Blender 5.2.2, metres, Z up.
Run: blender -b --factory-startup --python build_station.py
No third party geometry or textures. Generated exports are rebuildable from this file.
"""
import bpy, math, random, json, os
from mathutils import Vector
from pathlib import Path
random.seed(1420)
OUT=Path(__file__).resolve().parent
EXPORTS=OUT/'exports'; EXPORTS.mkdir(exist_ok=True)
import sys
sys.path.insert(0,str(OUT))
from make_textures import generate, STONE_SETTINGS

def configure_stone_material(m,prefix):
 """Shader-only refresh: keep the mesh's original UVs and material slots intact."""
 n=m.node_tree.nodes; links=m.node_tree.links; p=n.get('Principled BSDF')
 for node in list(n):
  if node.type in {'TEX_IMAGE','NORMAL_MAP','TEX_COORD','VECT_MATH','MAPPING'}: n.remove(node)
 coord=n.new('ShaderNodeTexCoord')
 scale=n.new('ShaderNodeMapping'); scale.vector_type='POINT'; scale.label='StoneUVScale'
 k=STONE_SETTINGS[prefix]['uv_scale']; scale.inputs['Scale'].default_value=(k,k,k)
 links.new(coord.outputs['UV'],scale.inputs['Vector'])
 for suffix,socket in [('color','Base Color'),('roughness','Roughness'),('normal','Normal')]:
  tex=n.new('ShaderNodeTexImage'); path=str(OUT/'textures'/(prefix+'_'+suffix+'.png'))
  tex.image=bpy.data.images.load(path,check_existing=True); tex.image.reload()
  if suffix!='color': tex.image.colorspace_settings.name='Non-Color'
  links.new(scale.outputs['Vector'],tex.inputs['Vector'])
  if suffix=='normal':
   norm=n.new('ShaderNodeNormalMap'); norm.inputs['Strength'].default_value=STONE_SETTINGS[prefix]['normal_strength']
   links.new(tex.outputs['Color'],norm.inputs['Color']); links.new(norm.outputs['Normal'],p.inputs[socket])
  else: links.new(tex.outputs['Color'],p.inputs[socket])

if '--materials-only' in sys.argv:
 # No geometry regeneration, FBX import or render; original mesh/UV/transform hash must hold.
 import hashlib
 bpy.ops.wm.open_mainfile(filepath=str(OUT/'1420_AM_station.blend'))
 def geometry_hash():
  rows=[]
  for o in sorted(bpy.data.objects,key=lambda ob:ob.name):
   if o.type!='MESH': continue
   m=o.data
   rows.append((o.name,[list(row) for row in o.matrix_world],[tuple(v.co) for v in m.vertices],
                [(tuple(p.vertices),p.material_index,p.use_smooth) for p in m.polygons],
                [(layer.name,[tuple(d.uv) for d in layer.data]) for layer in m.uv_layers],
                [slot.name for slot in m.materials]))
  return hashlib.sha256(json.dumps(rows,separators=(',',':')).encode()).hexdigest()
 before=geometry_hash()
 for name,prefix in [('M_1420_Stucco_cream','stucco'),('M_1420_Concrete','concrete')]: configure_stone_material(bpy.data.materials[name],prefix)
 parts=[o for o in bpy.context.scene.objects if o.type=='MESH' and o.name.startswith('SM_')]
 saved_locations={o.name:o.location.copy() for o in parts}
 bpy.ops.object.select_all(action='DESELECT')
 for o in parts: o.select_set(True); o.location-=Vector((0,5,0))
 bpy.ops.export_scene.gltf(filepath=str(OUT/'1420_AM_station.glb'),export_format='GLB',use_selection=True,export_materials='EXPORT',export_extras=True)
 for o in parts: o.location=saved_locations[o.name]
 bpy.context.view_layer.update()
 after=geometry_hash(); assert before==after, 'Material refresh changed authored mesh data'
 for img in bpy.data.images:
  if img.source=='FILE': img.filepath=bpy.path.relpath(img.filepath,start=str(OUT))
 bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'1420_AM_station.blend'))
 proof={'geometry_hash_before':before,'geometry_hash_after':after,'geometry_unchanged':before==after,'settings':STONE_SETTINGS,'scope':'shader-only and GLB material packaging; no geometry or FBX regenerated'}
 (OUT/'material_refresh.json').write_text(json.dumps(proof,indent=2))
 print('STONE_MATERIAL_REFRESH',json.dumps(proof))
 raise SystemExit(0)
generate(OUT/'textures')
bpy.ops.object.select_all(action='SELECT'); bpy.ops.object.delete(use_global=False)
for d in list(bpy.data.materials): bpy.data.materials.remove(d)
scene=bpy.context.scene; scene.unit_settings.system='METRIC'
groups={k:[] for k in ['Shell','Interior','Door','Glass','Mast','PreviewSign']}
current='Shell'

def mat(name,color,metal=0,rough=.65,emission=0):
 m=bpy.data.materials.new(name); m.diffuse_color=(*color,1); m.use_nodes=True
 p=m.node_tree.nodes.get('Principled BSDF'); p.inputs['Base Color'].default_value=(*color,1); p.inputs['Metallic'].default_value=metal; p.inputs['Roughness'].default_value=rough
 if emission: p.inputs['Emission Color'].default_value=(*color,1); p.inputs['Emission Strength'].default_value=emission
 return m
stucco=mat('M_1420_Stucco_cream',(.67,.59,.45),rough=.9)
concrete=mat('M_1420_Concrete',(.3,.28,.24),rough=.95)
trim=mat('M_1420_Painted_olive',(.14,.17,.12),rough=.6)
metal=mat('M_1420_Galvanized_metal',(.32,.35,.32),.72,.4)
dark=mat('M_1420_Black_enamel',(.018,.023,.020),.25,.45)
wood=mat('M_1420_Walnut',(.16,.065,.023),rough=.6)
woodlight=mat('M_1420_Desk_walnut',(.28,.13,.045),rough=.47)
acoustic=mat('M_1420_Acoustic_fabric',(.25,.20,.12),rough=.95)
silver=mat('M_1420_Brushed_aluminium',(.50,.53,.50),.78,.32)
ivory=mat('M_1420_Ivory_plastic',(.68,.59,.41),rough=.43)
paper=mat('M_1420_Paper',(.66,.60,.43),rough=.9)
ink=mat('M_1420_Ink',(.02,.025,.021),rough=.8)
red=mat('M_1420_Brick_red',(.43,.035,.017),rough=.5)
amber=mat('M_1420_VU_backlight',(.95,.48,.09),rough=.5,emission=.9)
glow=mat('M_1420_Lamp_glow',(1,.66,.25),rough=.3,emission=4)
onair=mat('M_1420_On_air',(1,.025,.008),rough=.4,emission=2)
green=mat('M_1420_Vinyl_green',(.028,.07,.045),rough=.5)
glass=mat('M_1420_Clear_glass',(.18,.24,.22),rough=.12)
p=glass.node_tree.nodes.get('Principled BSDF'); p.inputs['Transmission Weight'].default_value=.95; p.inputs['IOR'].default_value=1.45

def detail_material(m,scale,strength):
 n=m.node_tree.nodes; l=m.node_tree.links; p=n.get('Principled BSDF')
 tex=n.new('ShaderNodeTexNoise'); tex.inputs['Scale'].default_value=scale; tex.inputs['Detail'].default_value=3
 bump=n.new('ShaderNodeBump'); bump.inputs['Strength'].default_value=strength; bump.inputs['Distance'].default_value=.035
 l.new(tex.outputs['Fac'],bump.inputs['Height']); l.new(bump.outputs['Normal'],p.inputs['Normal'])
TEXTURE_MATS={stucco:'stucco',wood:'walnut',woodlight:'desk_walnut',concrete:'concrete'}
for m,prefix in TEXTURE_MATS.items():
 if prefix in STONE_SETTINGS:
  configure_stone_material(m,prefix); continue
 n=m.node_tree.nodes; l=m.node_tree.links; p=n.get('Principled BSDF')
 for suffix,socket in [('color','Base Color'),('roughness','Roughness'),('normal','Normal')]:
  tex=n.new('ShaderNodeTexImage'); tex.image=bpy.data.images.load(str(OUT/'textures'/(prefix+'_'+suffix+'.png')))
  if suffix!='color': tex.image.colorspace_settings.name='Non-Color'
  if suffix=='normal':
   normal=n.new('ShaderNodeNormalMap'); normal.inputs['Strength'].default_value=.35; l.new(tex.outputs['Color'],normal.inputs['Color']); l.new(normal.outputs['Normal'],p.inputs[socket])
  else: l.new(tex.outputs['Color'],p.inputs[socket])

def add(o,name,m):
 o.name=name; o.data.materials.append(m); groups[current].append(o); return o
def cube(name,loc,size,m,bevel=0):
 bpy.ops.mesh.primitive_cube_add(size=1,location=loc); o=bpy.context.object; o.dimensions=size
 bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
 if bevel:
  mod=o.modifiers.new('Machined_edges','BEVEL'); mod.width=bevel; mod.segments=2; bpy.context.view_layer.objects.active=o; bpy.ops.object.modifier_apply(modifier=mod.name)
 return add(o,name,m)
def cyl(name,a,b,r,m,vertices=12):
 a=Vector(a); b=Vector(b); d=b-a
 bpy.ops.mesh.primitive_cylinder_add(vertices=vertices,radius=r,depth=d.length,location=(a+b)/2)
 o=bpy.context.object; o.rotation_euler=d.to_track_quat('Z','Y').to_euler(); return add(o,name,m)
def text(name,body,loc,size,m,rot=(math.pi/2,0,0),align='CENTER'):
 bpy.ops.object.text_add(location=loc,rotation=rot); o=bpy.context.object; o.data.body=body; o.data.size=size; o.data.align_x=align; o.data.extrude=.001
 bpy.ops.object.convert(target='MESH'); return add(bpy.context.object,name,m)
def torus(name,loc,major,minor,m,rot=(0,0,0)):
 bpy.ops.mesh.primitive_torus_add(major_radius=major,minor_radius=minor,major_segments=24,minor_segments=6,location=loc,rotation=rot)
 return add(bpy.context.object,name,m)
def panel_front(name,x,z,w,h,m,y=-.035): return cube(name,(x,y,z),(w,.10,h),m,.012)
def frame(x,z,w,h,y=-.065):
 for sx in [-1,1]: cube('Studio_window_vertical',(x+sx*(w/2+.045),y,z),(.09,.15,h+.18),metal,.012)
 for sz in [-1,1]: cube('Studio_window_horizontal',(x,y,z+sz*(h/2+.045)),(w,.15,.09),metal,.012)

# Facade remains at the legacy hit proxy. Real open wall segments allow an interior.
cube('Foundation',(0,5,-.17),(12.25,10.25,.34),concrete)
cube('Floor',(0,5,.025),(11.6,9.6,.05),wood)
cube('Left_wall',(-5.87,5,1.65),(.26,10,3.3),stucco)
cube('Right_wall',(5.87,5,1.65),(.26,10,3.3),stucco)
cube('Rear_wall',(0,9.87,1.65),(12,.26,3.3),stucco)
# D x [-5.1,-4.1], z [.025,2.175]. W x [-.65,3.05], z [.725,2.375].
for x0,x1 in [(-6,-5.1),(-4.1,-.65),(3.05,6)]: cube('Facade_pier',((x0+x1)/2,.13,1.65),(x1-x0,.26,3.3),stucco)
cube('Door_lintel',(-4.6,.13,2.7375),(1,.26,1.125),stucco)
cube('Window_sill_wall',(1.2,.13,.3625),(3.7,.26,.725),stucco)
cube('Window_lintel',(1.2,.13,2.8375),(3.7,.26,.925),stucco)
cube('Flat_roof',(0,5,3.27),(12,10,.12),concrete)
for y in [.125,9.875]: cube('Parapet',(0,y,3.47),(12.08,.25,.35),stucco)
for x in [-5.875,5.875]: cube('Parapet',(x,5,3.47),(.25,10,.35),stucco)
for x in [-5.9,5.9]: cyl('Downpipe',(x,-.04,.05),(x,-.04,3.3),.045,metal)
cube('Door_step',(-4.6,-.48,-.045),(1.55,1,.12),concrete,.025)
# Shallow accessible approach rising from the legacy street to the stoop.
verts=[(-5.2,-1.2,-.34),(-4.0,-1.2,-.34),(-4.0,-.5,0),(-5.2,-.5,0),(-5.2,-1.2,-.4),(-4,-1.2,-.4),(-4,-.5,-.4),(-5.2,-.5,-.4)]
mesh=bpy.data.meshes.new('Ramp_mesh'); mesh.from_pydata(verts,[],[(0,1,2,3),(4,7,6,5),(0,4,5,1),(1,5,6,2),(2,6,7,3),(3,7,4,0)]); mesh.update()
o=bpy.data.objects.new('Door_approach_ramp',mesh); scene.collection.objects.link(o); add(o,o.name,concrete)
frame(1.2,1.55,3.7,1.65)
frame(-4.6,1.1,1,2.15)
cube('Window_stone_sill',(1.2,-.12,.67),(4.05,.4,.12),stucco,.025)
# Physical lettering occupies header and stays below parapet.
current='PreviewSign'
text('Facade_1420_AM','1420 AM',(1.2,-.025,2.65),.51,ink)
text('Facade_Night_Line','THE NIGHT LINE',(-2.05,-.028,2.35),.16,ink)
current='Shell'
cube('Door_buzzer',(-3.4,-.10,1.35),(.13,.12,.20),dark,.015)
cyl('Buzzer_button',(-3.4,-.17,1.35),(-3.4,-.19,1.35),.025,metal)
cube('Mail_box',(-3.7,-.16,1.22),(.24,.24,.32),red,.025)
cube('Mail_slot',(-3.7,-.29,1.27),(.16,.012,.018),dark)
# Three goose neck lamps; curved silhouette built from short rods.
for x in [-1.2,1.3,3.8]:
 cyl('Lamp_arm',(x,-.02,3.1),(x,-.5,3.1),.025,dark)
 cyl('Lamp_neck',(x,-.5,3.1),(x,-.57,2.99),.025,dark)
 bpy.ops.mesh.primitive_cone_add(vertices=20,radius1=.18,radius2=.07,depth=.14,location=(x,-.57,2.98)); add(bpy.context.object,'Gooseneck_lamp_shade',dark)
 cyl('Lamp_emitter',(x,-.57,2.905),(x,-.57,2.915),.13,glow,20)
# Metal slatted bench, outside on the right.
for x in [3.6,5.35]:
 for y in [-.40,-.90]: cyl('Bench_leg',(x,y,0),(x,y,.47),.028,dark)
 cyl('Bench_back_support',(x,-.4,.3),(x,-.25,.97),.028,dark)
for i in range(7): cube('Bench_seat_slat',(4.48,-.87+i*.073,.48),(2.05,.042,.04),metal,.012)
for i in range(8): cube('Bench_back_slat',(4.48,-.29,.57+i*.052),(2.05,.035,.035),metal,.008)
for x in [3.5,5.45]:
 cyl('Bench_arm',(x,-.88,.69),(x,-.36,.69),.025,dark)
 cyl('Bench_arm_post',(x,-.88,.47),(x,-.88,.69),.022,dark)
# Rooftop aerial and service duct.
cube('Roof_service_unit',(3,7,3.67),(1.2,.85,.7),metal,.025)
for i in range(9): cube('HVAC_grille',(3.0,6.565,3.4+i*.06),(.9,.018,.018),dark)
cyl('TV_aerial_mast',(-3.8,7,3.3),(-3.8,7,5.5),.03,metal)
cyl('TV_aerial_spine',(-4.9,7,5.35),(-2.7,7,5.35),.018,metal)
for i in range(8):
 x=-4.85+i*.3; span=.9-i*.065; cyl('TV_aerial_element',(x,7-span,5.35),(x,7+span,5.35),.012,metal,8)

current='Door'
cube('Door_leaf',(-4.6,.09,1.1),(.94,.085,2.10),trim,.025)
cube('Door_vision_dark',(-4.6,.039,1.45),(.60,.035,.92),dark,.01)
cube('Door_vision_glass',(-4.6,.014,1.45),(.53,.02,.84),glass)
for z in [1.02,1.88]: cube('Door_vision_trim',(-4.6,-.005,z),(.61,.025,.035),ivory)
cube('Door_kick_plate',(-4.6,.03,.18),(.8,.026,.23),metal)
cyl('Door_handle',(-4.22,-.02,.99),(-4.22,-.02,1.20),.018,metal)
text('Door_hours','1420 AM\nNIGHT LINE',(-4.6,-.025,1.48),.065,paper)
current='Glass'
cube('Studio_clear_glass',(1.2,.035,1.55),(3.68,.018,1.63),glass)

current='Interior'
# Timber lining, structural battens and sound absorption blocks.
for x in [-5.7,5.7]: cube('Walnut_side_lining',(x,5,1.55),(.035,9.5,3.04),wood)
cube('Walnut_back_lining',(0,9.7,1.55),(11.4,.04,3.04),wood)
cube('Studio_corridor_partition',(-3.0,4,1.6),(.12,7.5,3.15),wood)
for x in [-2.7+i*.34 for i in range(25)]:
 if x<=-.7 or x>=3.15: cube('Front_wood_batten',(x,.29,1.6),(.022,.04,3.08),woodlight)
for y in [.5+i*.48 for i in range(19)]:
 for x in [-2.91,5.65]: cube('Wall_wood_batten',(x,y,1.6),(.045,.035,3.08),woodlight)
for x in [-1.9,-.5,.9,2.3,3.7,5.1]:
 cube('Front_absorber',(x,.34,2.88),(1.2,.11,.46),acoustic,.02)
 for i in range(11):
  for j in range(4): cube('Acoustic_perforation',(x-.51+i*.10,.275,2.73+j*.09),(.018,.005,.018),dark)
cube('Interior_ceiling',(1.35,4.9,3.17),(8.55,9.5,.04),acoustic)
for x in [-2,0,2,4]: cube('Ceiling_beam',(x,5,3.1),(.09,9.4,.12),woodlight)
# Baseboards and square timber floor strips.
for y in [.4+i*.23 for i in range(40)]: cube('Floor_seam',(1.3,y,.056),(8.4,.007,.003),woodlight)
for x in [-2.9,5.6]: cube('Baseboard',(x,5,.12),(.06,9.4,.20),woodlight)
cube('Interior_window_sill',(1.2,.36,.72),(3.95,.37,.10),woodlight,.025)
current='PreviewSign'
cube('ON_AIR_housing',(1.2,.38,2.57),(.78,.16,.24),dark,.018)
cube('ON_AIR_lens',(1.2,.475,2.57),(.70,.025,.18),onair)
text('ON_AIR_letters','ON AIR',(1.2,.496,2.51),.13,paper,rot=(math.pi/2,0,math.pi))
current='Interior'
# Desk facing front window; warm timber top and pedestal legs.
cube('Broadcast_desk',(1.4,1.65,.84),(5.6,1.15,.13),woodlight,.045)
for x in [-.9,3.7]:
 cube('Desk_pedestal',(x,1.7,.42),(.52,.88,.75),wood,.018)
 for z in [.25,.47,.69]:
  cube('Desk_drawer',(x,2.15,z),(.45,.025,.18),woodlight,.01); cube('Desk_drawer_pull',(x,2.18,z),(.14,.035,.018),metal,.006)
cube('Console_chassis',(1.20,1.53,1.00),(2.75,.85,.20),silver,.025)
cube('Console_meter_bridge',(1.2,1.17,1.18),(2.75,.16,.28),dark,.015)
for i in range(5):
 x=.13+i*.53
 cube('VU_meter_bezel',(x,1.269,1.2),(.43,.025,.20),silver,.009)
 cube('VU_meter_dial',(x,1.286,1.2),(.36,.012,.14),amber,.006)
 cyl('VU_meter_needle',(x-.1,1.297,1.155),(x+.04,1.297,1.245),.004,ink,6)
 text('VU_label','VU',(x,1.299,1.17),.035,ink,rot=(math.pi/2,0,math.pi))
for i in range(14):
 x=-.04+i*.19
 cube('Console_channel_strip',(x,1.58,1.108),(.175,.65,.008),ivory)
 for j in range(3): cyl('Console_rotary_pot',(x,1.35+j*.095,1.11),(x,1.35+j*.095,1.145),.023,dark,12)
 cube('Console_fader_track',(x,1.76,1.116),(.009,.25,.008),dark)
 cube('Console_fader',(x,1.69+random.random()*.13,1.14),(.10,.046,.035),silver,.007)
 cube('Console_channel_button',(x,1.98,1.13),(.072,.05,.024),red if i%3==0 else trim,.005)
 text('Channel_number',str(i+1),(x,1.55,1.12),.035,ink,rot=(0,0,0))
# Reel-to-reel tape transport on left return; visible metal reels and spokes.
cube('Reel_machine',(-1.80,1.30,1.48),(.93,.45,1.28),dark,.035)
cube('Reel_machine_face',(-1.80,1.54,1.49),(.86,.035,1.17),silver,.012)
for x in [-2.03,-1.57]:
 cyl('Tape_reel_spool',(x,1.57,1.79),(x,1.63,1.79),.205,silver,32)
 cyl('Tape_reel_tape',(x,1.63,1.79),(x,1.65,1.79),.14,wood,24)
 cyl('Tape_reel_hub',(x,1.65,1.79),(x,1.68,1.79),.043,dark,16)
 for a in [0,2.094,4.189]:
  cyl('Tape_reel_spoke',(x,1.66,1.79),(x+.17*math.sin(a),1.66,1.79+.17*math.cos(a)),.02,silver)
cyl('Exposed_tape',(-2.03,1.68,1.6),(-1.57,1.68,1.6),.006,dark,6)
cube('Tape_head_cover',(-1.8,1.59,1.44),(.31,.08,.15),dark,.015)
for x in [-2.06,-1.8,-1.54]: cyl('Tape_controls',(x,1.58,1.18),(x,1.64,1.18),.035,dark)
for x in [-1.99,-1.62]: cube('Tape_meter',(x,1.58,1.31),(.27,.03,.12),amber,.005)
# Record/cassette archive in right corner.
cube('Archive_shelf_back',(5.34,3.5,1.35),(.12,2.8,2.5),wood)
for y in [2.1,3.5,4.9]: cube('Archive_divider',(5.06,y,1.35),(.65,.07,2.5),woodlight)
for z in [.18,.72,1.28,1.84,2.60]: cube('Archive_shelf',(5.06,3.5,z),(.65,2.85,.055),woodlight)
record_mats=[woodlight,red,trim,ivory,dark,paper,wood]
for row in range(4):
 for j in range(43):
  y=2.18+j*.061; z=.20+row*.55
  cube('Record_sleeve',(4.96,y,z+.22),(.40,.037,.43),record_mats[(j+row)%len(record_mats)])
  if j%4==0: cube('Record_spine_label',(4.75,y,z+.29),(.008,.04,.12),paper)
# Rear storage rack and cart machines.
cube('Equipment_rack',(-2.35,3.5,1.14),(.75,.65,2.15),dark,.03)
for z in [.27+i*.24 for i in range(8)]:
 cube('Rack_unit',(-2.35,3.844,z),(.67,.03,.20),metal,.007)
 cube('Rack_tape_slot',(-2.37,3.866,z+.025),(.36,.014,.06),dark)
 for x in [-2.58,-2.12]: cyl('Rack_knob',(x,3.86,z-.04),(x,3.9,z-.04),.022,dark)
# Office chair with upholstered back, arms and five spoke caster base.
cube('Chair_seat',(1.55,2.8,.51),(.67,.62,.15),green,.07)
o=cube('Chair_back',(1.55,3.07,.92),(.68,.14,.82),green,.07); o.rotation_euler.x=math.radians(-8)
cyl('Chair_stem',(1.55,2.8,.13),(1.55,2.8,.47),.05,metal)
for i in range(5):
 a=i*math.tau/5; x=1.55+.38*math.cos(a); y=2.8+.38*math.sin(a)
 cyl('Chair_base_spoke',(1.55,2.8,.15),(x,y,.1),.023,dark)
 cyl('Chair_caster',(x-.035,y,.075),(x+.035,y,.075),.055,dark)
for x in [1.14,1.96]:
 cyl('Chair_arm_support',(x,2.87,.5),(x,2.87,.75),.02,metal)
 cube('Chair_armrest',(x,2.75,.76),(.10,.47,.07),dark,.025)
# Boom arm, cable and period microphone.
for a,b in [((3.15,1.85,.92),(3.15,1.85,1.64)),((3.15,1.85,1.64),(2.28,1.45,1.96)),((2.28,1.45,1.96),(1.72,1.68,1.54))]:
 cyl('Microphone_boom',a,b,.016,metal)
 for dz in [-.038,.038]: cyl('Boom_parallel',(a[0],a[1],a[2]+dz),(b[0],b[1],b[2]+dz),.008,dark,8)
 cyl('Boom_cable',(a[0],a[1]+.03,a[2]),(b[0],b[1]+.03,b[2]),.006,dark,6)
cyl('Microphone_body',(1.72,1.68,1.54),(1.72,1.68,1.27),.065,dark,24)
for z in [1.29+i*.022 for i in range(9)]: torus('Microphone_grille',(1.72,1.68,z),.066,.007,metal)
# Ivory pushbutton phone.
cube('Phone_base',(3.65,1.65,1.00),(.48,.36,.17),ivory,.04)
cube('Phone_handset',(3.65,1.57,1.14),(.51,.095,.075),ivory,.035)
for x in [3.45,3.85]: cube('Phone_receiver_end',(x,1.57,1.12),(.13,.14,.10),ivory,.035)
for i in range(3):
 for j in range(4): cube('Phone_key',(3.55+i*.075,1.69+j*.044,1.10),(.052,.028,.018),dark,.004)
for i in range(28):
 a=i*math.tau/4; cyl('Phone_coil',(3.92+.024*math.cos(a),1.6+.024*math.sin(a),1.08-i*.012),(3.92+.024*math.cos(a+.9),1.6+.024*math.sin(a+.9),1.08-(i+1)*.012),.005,dark,6)
# Turntable and LP on side table.
cube('Turntable_table',(-.65,3.55,.73),(1.55,.85,.12),woodlight,.025)
for x in [-1.2,-.1]:
 for y in [3.25,3.85]: cube('Side_table_leg',(x,y,.35),(.055,.055,.7),wood)
cube('Turntable_base',(-.65,3.53,.84),(1.15,.65,.14),dark,.025)
cyl('Turntable_platter',(-.84,3.53,.91),(-.84,3.53,.93),.27,metal,40)
cyl('Vinyl_record',(-.84,3.53,.933),(-.84,3.53,.939),.25,ink,48)
cyl('Vinyl_label',(-.84,3.53,.94),(-.84,3.53,.945),.078,red,24)
for r in [.12,.16,.20,.23]: torus('Vinyl_groove',(-.84,3.53,.942),r,.0015,dark)
cyl('Tone_arm',(-.25,3.72,.96),(-.40,3.39,.96),.012,metal)
cube('Tone_arm_cartridge',(-.4,3.39,.947),(.05,.07,.04),ivory)
# Papers, script pages, mug, pencil cup and desk lamp.
for i in range(5):
 o=cube('Broadcast_script',(2.8+random.uniform(-.06,.06),2.0,.922+i*.004),(.44,.29,.003),paper); o.rotation_euler.z=random.uniform(-.12,.12)
 for j in range(6): cube('Script_line',(2.8,1.92+j*.026,.944+i*.004),(.32,.004,.002),ink)
cube('Program_log',(.05,2.05,.925),(.38,.24,.018),red,.005)
text('Program_log_title','PROGRAM LOG',(.05,2.05,.936),.035,paper,rot=(0,0,0))
cyl('Coffee_mug',(3.0,2.0,.92),(3.0,2.0,1.04),.055,ivory,24)
cyl('Coffee_surface',(3,2,1.041),(3,2,1.043),.045,wood,24)
torus('Mug_handle',(3.06,2,1.0),.035,.009,ivory,rot=(math.pi/2,0,0))
cyl('Desk_lamp_base',(4.0,1.25,.92),(4,1.25,.95),.14,dark,24)
cyl('Desk_lamp_stem',(4,1.25,.95),(4,1.25,1.52),.018,metal)
bpy.ops.mesh.primitive_cone_add(vertices=24,radius1=.20,radius2=.10,depth=.18,location=(4,1.25,1.55)); add(bpy.context.object,'Desk_lamp_shade',ivory)
cyl('Desk_lamp_bulb',(4,1.25,1.45),(4,1.25,1.47),.11,glow)
# Wall clock mounted on right wall, facing studio.
cyl('Clock_case',(5.59,1.25,2.45),(5.50,1.25,2.45),.36,dark,40)
cyl('Clock_dial',(5.49,1.25,2.45),(5.48,1.25,2.45),.32,ivory,40)
for i in range(12):
 a=i*math.tau/12; cyl('Clock_hour_tick',(5.465,1.25+.28*math.sin(a),2.45+.28*math.cos(a)),(5.465,1.25+.25*math.sin(a),2.45+.25*math.cos(a)),.008,ink,6)
cyl('Clock_minute_hand',(5.45,1.25,2.45),(5.45,1.06,2.60),.009,ink)
cyl('Clock_hour_hand',(5.44,1.25,2.45),(5.44,1.39,2.52),.013,ink)

# Small public lobby is separated from closed broadcast studio.
current='Interior'
cube('Lobby_back_wall',(-4.4,7.4,1.6),(2.85,.12,3.15),wood)
cube('Studio_closed_door',(-3.075,3.5,1.1),(.035,1.0,2.15),trim,.01)
text('Studio_door_notice','STUDIO\nRECORDING',(-3.10,3.5,1.6),.11,ivory,rot=(math.pi/2,0,-math.pi/2))
cube('Lobby_noticeboard',(-5.63,3.1,1.7),(.08,1.25,.90),woodlight,.015)
for j in range(3):
 cube('Lobby_notice',(-5.575,2.7+j*.38,1.72),(.01,.27,.48),paper)
cube('Lobby_reception_table',(-4.8,5.8,.78),(1.5,.65,.09),woodlight,.025)
for x in [-5.4,-4.2]:
 for y in [5.58,6.02]: cube('Reception_table_leg',(x,y,.39),(.055,.055,.78),wood)
text('Lobby_frontdoor_sign','1420 AM',(-4.6,.31,2.48),.20,ivory,rot=(math.pi/2,0,math.pi))

# Separate mast may be omitted when preserving legacy mast and red aviation lights.
current='Mast'
mx,my=12,6; h=18; rr=.24
leg=[(mx+math.cos(i*math.tau/3)*rr,my+math.sin(i*math.tau/3)*rr) for i in range(3)]
cube('Mast_footing',(mx,my,.10),(1.2,1.2,.2),concrete)
for x,y in leg: cyl('Mast_leg',(x,y,.2),(x,y,h),.028,metal,8)
for k in range(18):
 for i in range(3):
  a,b=leg[i],leg[(i+1)%3]
  cyl('Mast_brace',(a[0],a[1],k+.2),(b[0],b[1],k+1.2),.009,metal,6)
for z in [9,18]: cyl('Mast_red_beacon',(mx,my,z),(mx,my,z+.13),.07,onair,12)

# Revision R2 is authored against the original component and world-space contracts.
import subprocess
subprocess.run(['python3',str(OUT/'make_revision_textures.py')],check=True)
from station_revision_two import apply as apply_revision_two
r2_materials=apply_revision_two(groups,mat,cube,cyl,torus,text,globals())

def join_group(key):
 obs=groups[key]; bpy.ops.object.select_all(action='DESELECT')
 for o in obs: o.select_set(True)
 bpy.context.view_layer.objects.active=obs[0]; bpy.ops.object.join(); o=bpy.context.object; o.name='SM_1420_'+key
 scene.cursor.location=(0,5,0) if key!='Door' else (-5.07,.09,.025)
 bpy.ops.object.origin_set(type='ORIGIN_CURSOR'); scene.cursor.location=(0,0,0)
 # UVs retained from primitives; text adds no texture dependency.
 o['original_asset']='SIGNAL / 47 original procedural mesh, 2026-10-23'
 o['part']=key; o['units']='metres'; return o
# Preserve every named component for Blender editing, hidden to avoid double rendering.
source=bpy.data.collections.new('EDITABLE_SOURCE_enable_to_edit'); scene.collection.children.link(source)
for key,objects in groups.items():
 for original in objects:
  dup=original.copy(); dup.data=original.data.copy(); dup.name='SRC_'+original.name; source.objects.link(dup)
source.hide_render=True; source.hide_viewport=True
parts=[join_group(k) for k in groups if k!='PreviewSign']
from station_mesh_cleanup import clean_triangulated_mesh
cleanup_proof={o.name:clean_triangulated_mesh(o) for o in parts}
(OUT/'mesh_cleanup.json').write_text(json.dumps(cleanup_proof,indent=2))
for o in parts:
 bpy.ops.object.select_all(action='DESELECT'); o.select_set(True); bpy.context.view_layer.objects.active=o
 original_location=o.location.copy(); o.location=(0,0,0)
 bpy.ops.export_scene.fbx(filepath=str(EXPORTS/(o.name+'.fbx')),use_selection=True,object_types={'MESH'},apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',axis_forward='-Y',axis_up='Z',bake_anim=False,add_leaf_bones=False,mesh_smooth_type='FACE')
 o.location=original_location
bpy.ops.object.select_all(action='DESELECT')
saved_locations={o.name:o.location.copy() for o in parts}
for o in parts: o.select_set(True); o.location-=Vector((0,5,0))
bpy.ops.export_scene.gltf(filepath=str(OUT/'1420_AM_station.glb'),export_format='GLB',use_selection=True,export_materials='EXPORT',export_extras=True)
for o in parts: o.location=saved_locations[o.name]
stats={o.name:{'vertices':len(o.data.vertices),'triangles':len(o.data.polygons),'triangulated':True,'zero_area_triangles':0,'materials':[m.name for m in o.data.materials],'bounds_local_m':[list(map(float,minv)) for minv in [tuple(min(v.co[i] for v in o.data.vertices) for i in range(3)),tuple(max(v.co[i] for v in o.data.vertices) for i in range(3))]],'origin':list(o.location)} for o in parts}
contract={'asset':'1420 AM Caliche radio station','authoring':'Original geometry and materials; concept reference is visual direction only','world_three_pivot_m':[8025,4.442166614532471,2036],'mapping':'worldThree=(8025+localY,4.442166614532471+localZ,2036+localX)','building_dimensions_m':[12,10,3.65],'front':'local -Y, facade at Y=-5m','door_hinge_local_m':[-5.07,-4.91,.025],'window_center_local_m':[1.2,-5,1.55],'window_opening_m':[3.7,1.65],'door_center_local_m':[-4.6,-5,1.1],'parts':stats,'notes':['Glass is separate; preserve story photo/anomaly panels in front of local Y=-5.','Mast is separate and optional: legacy 36m mast, guy wires and aviation lights can remain.','All static parts other than the door share origin at Blender scene 0,5,0, world building centre. Door FBX has neutral translation and hinge-local vertices; place it at door_hinge_local_m relative to the building pivot.','FBX PBR uses named material slots with base color, metal and roughness; Original tiling color, normal and roughness PNG maps are referenced by materials.json.','No gameplay or collision changes; shell uses actual openings and interior corridor.']}
contract['export_triangulation']='Explicit Blender loop triangulation, exact area=0 triangles omitted; visible triangle coordinates/UVs/materials unchanged.'
contract['lobby_walk_world_three_xz_m']=[8020.32,8026.5,2030.4,2032.8]
material_data={}
for m in bpy.data.materials:
 p=m.node_tree.nodes.get('Principled BSDF') if m.use_nodes else None
 if p: material_data[m.name]={'base_color':list(p.inputs['Base Color'].default_value),'metallic':p.inputs['Metallic'].default_value,'roughness':p.inputs['Roughness'].default_value,'emission':list(p.inputs['Emission Color'].default_value),'emission_strength':p.inputs['Emission Strength'].default_value,'alpha':p.inputs['Alpha'].default_value,'transmission':p.inputs['Transmission Weight'].default_value}
for m,prefix in TEXTURE_MATS.items(): material_data[m.name]['textures']={'base_color':'textures/'+prefix+'_color.png','normal':'textures/'+prefix+'_normal.png','roughness':'textures/'+prefix+'_roughness.png'}
for m,prefix in TEXTURE_MATS.items():
 if prefix in STONE_SETTINGS:
  material_data[m.name].update(STONE_SETTINGS[prefix]); material_data[m.name]['normal_convention']='DirectX tangent space'
  material_data[m.name]['textures']['normal']='textures/'+prefix+'_normal_dx.png'
material_data.update(r2_materials)
material_data['M_1420_Clear_glass']['alpha']=.14
(OUT/'materials.json').write_text(json.dumps(material_data,indent=2))
# Basis probe is excluded from final scene and exports.
bpy.ops.object.select_all(action='DESELECT')
bpy.ops.mesh.primitive_cube_add(size=1,location=(2,4,6)); probe=bpy.context.object; probe.name='AXIS_PROBE'; probe.dimensions=(.2,.4,.6)
bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
bpy.ops.export_scene.fbx(filepath=str(EXPORTS/'axis_probe.fbx'),use_selection=True,object_types={'MESH'},apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',axis_forward='-Y',axis_up='Z',bake_anim=False,add_leaf_bones=False)
bpy.data.objects.remove(probe,do_unlink=True)
contract['source_revision']='R2-concept-match-2026-10-24'
contract['r2_texture_authoring']={'facade_1420_weathered':[2048,1024],'tiling_families':[1024,1024],'paint_losses':'Sparse sub-centimetre chips; fine random field 720x360, threshold250/255','source':'make_revision_textures.py','normal_convention':'DirectX in materials.json, OpenGL in Blender/GLB'}
contract['facade_identity']='1420 AM painted in opaque facade PBR; hide legacy roof sign in native integration'
contract['preserved_anchors']=['door hinge','window opening','mixing console','main microphone boom/body','studio photo and ON AIR runtime objects']
contract['new_material_families']=list(r2_materials)
(OUT/'model_contract.json').write_text(json.dumps(contract,indent=2))

# Presentation stage is excluded from exports and visible only in Blender renders.
current='Shell'
ground=cube('PREVIEW_ground',(0,4,-.38),(100,100,.05),mat('PREVIEW_Desert',(.14,.115,.08),rough=1))
def area(name,loc,power,color,size,target):
 d=bpy.data.lights.new(name,'AREA'); d.energy=power; d.color=color; d.shape='DISK'; d.size=size
 o=bpy.data.objects.new(name,d); scene.collection.objects.link(o); o.location=loc; o.rotation_euler=(Vector(target)-o.location).to_track_quat('-Z','Y').to_euler(); o.visible_glossy=False; o.visible_transmission=False; o.visible_camera=False; d.specular_factor=0; return o
area('Preview_blue_sky',(-4,-6,12),1600,(.44,.58,1),15,(0,3,1))
area('Preview_sunset',(-10,-3,6),1300,(1,.56,.28),9,(0,2,1))
area('Preview_interior_ceiling',(1.3,3.0,2.99),190,(1,.62,.30),4,(1.3,2,0))
area('Preview_desk_lamp',(4,1.25,1.43),18,(1,.55,.20),.25,(2.8,1.7,.8))
area('Preview_window_fill',(1.2,-1,2),100,(.40,.55,1),3,(1.2,3,1))
for x in [-1.2,1.3,3.8]: area('Preview_gooseneck',(x,-.57,2.90),30,(1,.63,.25),.25,(x,-.05,2))
scene.world.color=(.07,.07,.07)
scene.render.engine='CYCLES'; scene.render.threads_mode='FIXED'; scene.render.threads=2
scene.cycles.samples=48
scene.cycles.use_denoising=True
scene.render.resolution_x=1440; scene.render.resolution_y=1080; scene.render.resolution_percentage=100
scene.view_settings.view_transform='AgX'
def camera(name,loc,target,lens):
 d=bpy.data.cameras.new(name); d.lens=lens; o=bpy.data.objects.new(name,d); scene.collection.objects.link(o); o.location=loc; o.rotation_euler=(Vector(target)-o.location).to_track_quat('-Z','Y').to_euler(); return o
ext=camera('CAM_Exterior',(-10,-16,3.9),(0,2,1.7),34)
inside=camera('CAM_Interior',(4.40,5.25,2.05),(.55,1.40,1.35),24)
scene.camera=ext
for img in bpy.data.images:
 if img.source=='FILE': img.filepath=bpy.path.relpath(img.filepath,start=str(OUT))
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'1420_AM_station.blend'))
if '--render' in sys.argv:
 scene.render.filepath=str(OUT/'exterior.png'); bpy.ops.render.render(write_still=True)
 scene.camera=inside; scene.render.filepath=str(OUT/'interior.png'); bpy.ops.render.render(write_still=True)
print('STATION_COMPLETE',json.dumps({k:v['triangles'] for k,v in stats.items()}))
