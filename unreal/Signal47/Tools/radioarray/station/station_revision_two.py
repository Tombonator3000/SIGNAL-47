"""R2 authoring pass. Uses the original placement contract and named component groups."""
import bpy,math,random
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parent

def textured(material,stem,color,uv=1,strength=.30):
 n=material.node_tree.nodes;l=material.node_tree.links;p=n.get('Principled BSDF')
 for node in list(n):
  if node.type not in {'BSDF_PRINCIPLED','OUTPUT_MATERIAL'}: n.remove(node)
 material.diffuse_color=(*color,1);p.inputs['Base Color'].default_value=(*color,1)
 p.inputs['Roughness'].default_value=.80 if 'stucco' in stem or 'facade' in stem else .60
 coord=n.new('ShaderNodeTexCoord');mapping=n.new('ShaderNodeMapping');mapping.inputs['Scale'].default_value=(uv,uv,uv)
 l.new(coord.outputs['UV'],mapping.inputs['Vector'])
 for suffix,socket in [('color','Base Color'),('roughness','Roughness'),('normal_gl','Normal')]:
  t=n.new('ShaderNodeTexImage');t.image=bpy.data.images.load(str(ROOT/'textures'/'revision2'/(stem+'_'+suffix+'.png')),check_existing=True)
  if suffix!='color':t.image.colorspace_settings.name='Non-Color'
  l.new(mapping.outputs['Vector'],t.inputs['Vector'])
  if suffix=='normal_gl':
   normal=n.new('ShaderNodeNormalMap');normal.inputs['Strength'].default_value=strength;l.new(t.outputs['Color'],normal.inputs['Color']);l.new(normal.outputs['Normal'],p.inputs[socket])
  else:l.new(t.outputs['Color'],p.inputs[socket])
 return {'base_color_linear':[*color,1],'metallic':0,'roughness':p.inputs['Roughness'].default_value,'emission':[0,0,0,1],'emission_strength':0,'alpha':1,
         'textures':{'base_color':'textures/revision2/'+stem+'_color.png','normal':'textures/revision2/'+stem+'_normal_dx.png','roughness':'textures/revision2/'+stem+'_roughness.png'},
         'normal_strength':strength,'uv_scale':uv,'normal_convention':'DirectX tangent space; Blender/GLB uses explicit normal_gl map'}

def apply(groups,mat,cube,cyl,torus,text,namespace):
 r=random.Random(14202036)
 rows={}
 bpy.context.view_layer.update()
 for name,stem,col,uv,strength in [
  ('M_1420_Stucco_cream','stucco_layers',(.60,.525,.405),2,.34),
  ('M_1420_Walnut','walnut_worked',(.105,.048,.021),1,.25),
  ('M_1420_Desk_walnut','desk_worn',(.23,.105,.038),1,.28)]:
  rows[name]=textured(bpy.data.materials[name],stem,col,uv,strength)
 facade=mat('M_1420_WeatheredFacadePaint',(.60,.525,.405),rough=.88)
 rows[facade.name]=textured(facade,'facade_1420_weathered',(.60,.525,.405),1,.34)
 olive=mat('M_1420_OliveWornTimber',(.062,.079,.041),rough=.64)
 rows[olive.name]=textured(olive,'olive_timber',(.062,.079,.041),1,.24)
 exposed=mat('M_1420_ExposedLimeRender',(.37,.29,.19),rough=.97)
 edge=mat('M_1420_ChalkFlakeEdge',(.58,.52,.41),rough=.98)
 dark=bpy.data.materials['M_1420_Black_enamel'];ink=bpy.data.materials['M_1420_Ink'];paper=bpy.data.materials['M_1420_Paper']
 wood=bpy.data.materials['M_1420_Walnut'];woodlight=bpy.data.materials['M_1420_Desk_walnut'];metal=bpy.data.materials['M_1420_Brushed_aluminium']
 ivory=bpy.data.materials['M_1420_Ivory_plastic'];red=bpy.data.materials['M_1420_Brick_red']
 # One unique facade atlas: painted words and cracks remain part of the opaque plaster.
 facade_names=('Facade_pier','Door_lintel','Window_sill_wall','Window_lintel')
 for o in groups['Shell']:
  if o.name.startswith(facade_names) or (o.name.startswith('Parapet') and o.location.y<.2):
   idx=len(o.data.materials);o.data.materials.append(facade);uv=o.data.uv_layers.active
   for poly in o.data.polygons:
    if poly.normal.y<-.9:
     poly.material_index=idx
     for li in poly.loop_indices:
      co=o.matrix_world@o.data.vertices[o.data.loops[li].vertex_index].co
      uv.data[li].uv=((co.x+6)/12,co.z/3.65)
  if o.name.startswith('Studio_window_'):
   o.data.materials.clear();o.data.materials.append(olive)
   # More substantial reveal, while keeping the original clear aperture and centre.
   for v in o.data.vertices:v.co.y*=1.45
 for o in groups['Door']:
  if o.name.startswith(('Door_leaf','Door_vision_trim')):
   o.data.materials.clear();o.data.materials.append(olive)
 for o in list(groups['PreviewSign']):
  if o.name.startswith('Facade_'):
   groups['PreviewSign'].remove(o);bpy.data.objects.remove(o,do_unlink=True)
 # Small render losses have actual flake edge relief. No patch enters door/window openings.
 namespace['current']='Shell'
 def patch(name,x,z,rad):
  points=[]
  for j in range(7):
   a=j*math.tau/7;points.append((x+math.cos(a)*rad*r.uniform(.7,1.25),-.004,z+math.sin(a)*rad*r.uniform(.7,1.25)))
  mesh=bpy.data.meshes.new(name);mesh.from_pydata(points,[],[tuple(range(7))]);mesh.update()
  o=bpy.data.objects.new(name,mesh);bpy.context.scene.collection.objects.link(o);o.data.materials.append(exposed);groups['Shell'].append(o)
  for a,b in zip(points,points[1:]+points[:1]):
   if r.random()<.5:cyl('BrokenPlasterLip',(a[0],-.011,a[2]),(b[0],-.011,b[2]),.0035,edge,5)
 for x,z in [(-5.72,.28),(-5.68,3.38),(-3.75,.24),(-3.30,2.48),(3.36,.40),(5.72,.27),(5.72,3.38),(1.30,.18)]:
  for j in range(4):patch('CuratedPlasterLoss',x+r.uniform(-.09,.09),z+r.uniform(-.08,.08),r.uniform(.018,.060))
 # Crooked small station plaque, original operational identity rather than new puzzle text.
 plaque=cube('StationServicePlaque',(-2.6,-.025,1.52),(.46,.035,.27),olive,.008)
 text('StationServicePlaqueText','1420 AM\nSERVICE',(-2.6,-.047,1.52),.06,paper)
 for x in [-5.9,5.9]:
  for z in [.65,1.65,2.65]:
   cube('PipeSaddle',(x,-.046,z),(.14,.045,.032),dark,.005)
 # Leave the microphone and original mixing console anchors exactly where gameplay expects them.
 namespace['current']='Interior'
 # The shallow working return makes a lived-in layer visible above the street-side sill.
 cube('StreetVisibleDeskReturn',(1.25,.74,.81),(4.7,.64,.095),woodlight,.025)
 for x in [-.85,3.35]:cube('DeskReturnBracket',(x,.82,.65),(.09,.24,.32),wood,.01)
 # Phone is still one original prop, now usable to the operator at the window-side return.
 for o in groups['Interior']:
  if o.name.startswith(('Phone_','Phone_coil')):o.location+=Vector((-.85,-.95,0))
 # A handful of records chosen from the shelf, with uneven edges and worn printed sleeves.
 sleeve_mats=[paper,red,olive,woodlight]
 for i in range(11):
  x=-.35+r.uniform(-.05,.05);y=.70+r.uniform(-.04,.04);z=.88+i*.012
  o=cube('SelectedLPJacket',(x,y,z),(.34,.34,.009),sleeve_mats[i%4]);o.rotation_euler.z=r.uniform(-.20,.20)
  if i==10:
   cyl('LPLabelOnJacket',(x,y,z+.006),(x,y,z+.009),.09,dark,24)
   cyl('LPCentreLabel',(x,y,z+.010),(x,y,z+.012),.026,paper,16)
 # Rehearsal scripts and engineering logs: overlap them deliberately rather than a grid.
 for i in range(5):
  x=.65+i*.14;y=.65+r.uniform(-.07,.07);z=.871+i*.004
  o=cube('MarkedCueSheet',(x,y,z),(.29,.21,.0025),paper);o.rotation_euler.z=r.uniform(-.25,.25)
  for j in range(5):cube('CueSheetInk',(x,y-.075+j*.028,z+.002),(.20-r.random()*.04,.003,.001),ink)
 text('CueSheetHeading','NIGHT LINE',(.88,.70,.893),.028,ink,rot=(0,0,0))
 # Two tape boxes, one opened, and a loose reel: work in progress, not random floor clutter.
 for i,(x,y) in enumerate([(-.92,.71),(2.35,.72)]):
  box=cube('QuarterInchTapeBox',(x,y,.91),(.30,.28,.075),paper,.005);box.rotation_euler.z=(-.09 if i==0 else .13)
  text('TapeBoxHandLabel','1420\nMASTER',(x,y,.950),.033,red,rot=(0,0,0))
 torus('LooseTapeReel',(-.91,.71,.97),.103,.012,metal)
 cyl('LooseTapeReelHub',(-.91,.71,.97),(-.91,.71,.985),.025,dark,16)
 for a in [0,2.094,4.188]:cyl('LooseTapeSpoke',(-.91,.71,.982),(-.91+.09*math.cos(a),.71+.09*math.sin(a),.982),.008,metal,6)
 # Headphones left on the working return; ear pads and lead have an understandable silhouette.
 torus('HeadphoneBand',(1.75,.61,1.00),.16,.016,dark,rot=(math.pi/2,0,0))
 for x in [1.59,1.91]:
  cyl('HeadphoneEarpad',(x,.58,.92),(x,.65,.92),.068,dark,16)
  cyl('HeadphoneEarShell',(x,.65,.92),(x,.68,.92),.058,metal,16)
 for a,b in [((1.93,.67,.92),(2.03,.62,.87)),((2.03,.62,.87),(2.15,.43,.78)),((2.15,.43,.78),(2.11,.44,.42))]:cyl('HeadphoneLead',a,b,.005,dark,6)
 # Coffee ring, pencil cup, short pencils and a torn pad complete the operator's working area.
 cyl('UsedPencilCup',(3.13,.77,.86),(3.13,.77,.97),.045,metal,16)
 for i in range(5):
  a=i*1.3;x=3.13+.025*math.cos(a);y=.77+.025*math.sin(a)
  cyl('OperatorPencil',(x,y,.92),(x+r.uniform(-.018,.018),y+r.uniform(-.018,.018),1.13+r.uniform(-.03,.03)),.004,red if i%2 else paper,6)
 torus('CoffeeRingOnReturn',(2.05,.84,.860),.044,.0025,wood)
 # Pinboard on the side wall remains outside the original photo/microphone relationship.
 cube('EngineeringPinboard',(5.57,2.55,1.8),(.055,1.25,.83),woodlight,.015)
 for i in range(5):
  y=2.09+i*.22;z=1.75+r.uniform(-.12,.13)
  cube('PinnedEngineeringSheet',(5.532,y,z),(.006,.19,.34),paper)
  cyl('PinboardTack',(5.525,y,z+.13),(5.515,y,z+.13),.007,red,8)
 # Bind two small stacks together, rotate sleeves in-place for a less machine-perfect archive.
 for o in groups['Interior']:
  if o.name.startswith('Record_sleeve'):
   o.rotation_euler.x=r.uniform(-.035,.035)
  if o.name.startswith('Rack_unit'):
   o.location.z+=r.uniform(-.007,.007)
 namespace['current']='Mast'
 return rows
