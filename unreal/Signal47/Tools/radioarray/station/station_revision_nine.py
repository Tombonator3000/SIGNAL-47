"""Targeted lobby print/veneer UV correction and one period opal ceiling fitting."""
import bpy
from pathlib import Path
from mathutils import Vector
from station_revision_two import textured
ROOT=Path(__file__).resolve().parent
def apply(groups,mat,cube,cyl,namespace):
 namespace['current']='Interior';bpy.context.view_layer.update()
 ink=bpy.data.materials['M_1420_Ink'];paper=bpy.data.materials['M_1420_Paper']
 dark=bpy.data.materials['M_1420_Black_enamel'];metal=bpy.data.materials['M_1420_Brushed_aluminium']
 ivory=bpy.data.materials['M_1420_Ivory_plastic'];glow=bpy.data.materials['M_1420_Lamp_glow']
 printmat=mat('M_1420_StationNotices',(.70,.65,.53),rough=.91)
 row=textured(printmat,'station_notices',(.70,.65,.53),1,.05,folder='revision9')
 printmat.node_tree.nodes.get('Principled BSDF').inputs['Roughness'].default_value=.91;row['roughness']=.91
 walls=[];notices=[]
 for o in groups['Interior']:
  if o.name.startswith(('Walnut_side_lining','Walnut_back_lining','Studio_corridor_partition','Lobby_back_wall')):
   uv=o.data.uv_layers.active
   for p in o.data.polygons:
    if abs(p.normal.x)<.9 and abs(p.normal.y)<.9:continue
    for li in p.loop_indices:
     co=o.matrix_world@o.data.vertices[o.data.loops[li].vertex_index].co
     uv.data[li].uv=(co.z/2.4,(co.y if abs(p.normal.x)>.9 else co.x)/.30)
   walls.append(o.name)
  if o.name=='Lobby_notice' or o.name.startswith('Lobby_notice.'):
   page=len(notices);idx=len(o.data.materials);o.data.materials.append(printmat)
   for p in o.data.polygons:
    if p.normal.x<.9:continue
    p.material_index=idx
    for li in p.loop_indices:
     co=o.data.vertices[o.data.loops[li].vertex_index].co
     o.data.uv_layers.active.data[li].uv=(page*.25+(co.y/.27+.5)*.25,co.z/.48+.5)
   notices.append(o.name)
 def top_print(o,page):
  o.data.materials.append(printmat);idx=len(o.data.materials)-1
  for p in o.data.polygons:
   if p.normal.z<.9:continue
   p.material_index=idx
   for li in p.loop_indices:
    co=o.data.vertices[o.data.loops[li].vertex_index].co
    o.data.uv_layers.active.data[li].uv=(page*.25+(co.x/.24+.5)*.25,co.y/.33+.5)
 # The existing reception table gains a single useful log, not a new furnishing or obstacle.
 for i in range(3):
  o=cube('LobbyProgramLogSheet',(-4.78,5.80,.827+i*.0025),(.24,.33,.002),paper)
  top_print(o,3)
 for i in range(2):
  x=-4.55+i*.025;cyl('LobbyDeskPencil',(x,5.72,.834),(x+.018,5.88,.834),.003,ivory,8)
  cyl('LobbyPencilTip',(x+.018,5.88,.834),(x+.019,5.89,.834),.0017,ink,6)
 # Discreet original 1980s surface-mounted opal diffuser, below the existing ceiling.
 cube('StudioCeilingDiffuserFrame',(1.3,6.3,3.095),(1.18,.53,.06),ivory,.012)
 cube('StudioCeilingOpalDiffuser',(1.3,6.3,3.07),(1.1,.45,.045),glow,.008)
 for x in [.79,1.81]:cyl('DiffuserCaptiveScrew',(x,6.3,3.046),(x,6.3,3.044),.009,metal,12)
 # Interior camera sees these equipment backs directly. Only add a small service vent.
 cube('TapeRearServicePanel',(.14,2.529,1.34),(.32,.006,.17),metal)
 for i in range(5):cube('TapeRearVent',(.14,2.533,1.295+i*.023),(.25,.003,.007),dark)
 namespace['current']='Mast'
 return {printmat.name:row},{'printed_notice_objects':notices,'metric_vertical_wood_objects':walls,
  'wall_uv_rule':'U=worldZ/2.4, V=worldY/.30 for X-facing walls; V=worldX/.30 for Y-facing walls. Long grain follows vertical U.',
  'ceiling_diffuser_source_m':[1.3,6.3,3.07],'ceiling_diffuser_size_m':[1.1,.45,.045],
  'preserved':'All previous anchors, dimensions, clocks, proxies, printed canonical Night Line hours, old material slot order, footprint and walking surfaces.'}

INTERIOR_SLOT_PREFIX = ['M_1420_Walnut','M_1420_Desk_walnut','M_1420_Acoustic_fabric','M_1420_Black_enamel','M_1420_Galvanized_metal','M_1420_Brushed_aluminium','M_1420_VU_backlight','M_1420_Ink','M_1420_Ivory_plastic','M_1420_Brick_red','M_1420_Painted_olive','M_1420_Paper','M_1420_Lamp_glow','M_1420_OliveWornTimber','M_1420_Vinyl_green','M_1420_LeatherStitch','M_1420_ProgramPanelPrint']
def preserve_interior_slots(obj):
 old=[m.name for m in obj.data.materials]
 desired=INTERIOR_SLOT_PREFIX+[n for n in old if n not in INTERIOR_SLOT_PREFIX]
 assert set(old)==set(desired), (old,desired)
 face_slots=[desired.index(old[p.material_index]) for p in obj.data.polygons]
 obj.data.materials.clear()
 for name in desired:obj.data.materials.append(bpy.data.materials[name])
 for face,index in zip(obj.data.polygons,face_slots):face.material_index=index

def restore_original_uvs(groups):
 import json
 rows=json.loads((ROOT/'r9_original_uv.json').read_text())
 changed_prefix=('Walnut_side_lining','Walnut_back_lining','Studio_corridor_partition','Lobby_back_wall')
 for objects in groups.values():
  for ob in objects:
   if ob.name not in rows or ob.name.startswith(changed_prefix) or ob.name=='Lobby_notice' or ob.name.startswith('Lobby_notice.'):continue
   layers=rows[ob.name]
   assert len(layers)==len(ob.data.uv_layers),ob.name
   for layer,saved in zip(ob.data.uv_layers,layers):
    assert len(layer.data)==len(saved),ob.name
    for datum,uv in zip(layer.data,saved):datum.uv=uv
