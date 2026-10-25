"""Read-only reuse of our accepted original substation mesh primitives, Town prefix."""
import importlib.util,math
from pathlib import Path
p=Path(__file__).resolve().parent.parent/'substation/meshkit.py';s=importlib.util.spec_from_file_location('s47_town_base_meshkit',p);base=importlib.util.module_from_spec(s);s.loader.exec_module(base)
add,sub,mul,dot,cross,unit=base.add,base.sub,base.mul,base.dot,base.cross,base.unit
class Mesh(base.Mesh):
 def face(self,ps,mat,uv=None,smooth=False):
  super().face(ps,mat,uv,smooth);self.mat[-1]=self.mat[-1].replace('M_Substation_','M_Town_',1)
 def merge(self,other,offset=(0,0,0),yaw=0):
  c,s=math.cos(yaw),math.sin(yaw);start=len(self.v)
  self.v.extend((offset[0]+p[0]*c-p[1]*s,offset[1]+p[0]*s+p[1]*c,offset[2]+p[2])for p in other.v)
  self.f.extend([[i+start for i in f]for f in other.f]);self.mat.extend(other.mat);self.uv.extend(other.uv);self.smooth.extend(other.smooth);self.tags.extend(other.tags)
 def panel(self,name,center,width,height,cell,normal=(1,0,0)):
  self.tag=name;n=unit(normal);right=unit(cross((0,0,1),n));up=cross(n,right)
  ps=[add(center,add(mul(right,x*width/2),mul(up,y*height/2)))for x,y in [(-1,-1),(1,-1),(1,1),(-1,1)]]
  # 4x4 original atlas; guarded gutters avoid neighboring labels at lower mips.
  col,row=cell%4,cell//4;u0=(col+.035)/4;u1=(col+.965)/4;v0=1-(row+.965)/4;v1=1-(row+.035)/4
  self.face(ps,'PrintAtlas',[(u0,v0),(u1,v0),(u1,v1),(u0,v1)])

def seg(lod):return [20,12,8][lod]
def box_bounds(m,name,x0,x1,y0,y1,z0,z1,mat):
 if min(x1-x0,y1-y0,z1-z0)<=1e-7:return
 m.box(name,((x0+x1)/2,(y0+y1)/2,(z0+z1)/2),(x1-x0,y1-y0,z1-z0),mat)
def front_wall(m,x,y0,y1,height,holes,mat='Stucco',thick=.22):
 # Actual wall voids, partitioned at each aperture; no facade or wall behind the glass.
 cuts=sorted(set([y0,y1]+[max(y0,min(y1,a))for h in holes for a in h[:2]]))
 for a,b in zip(cuts,cuts[1:]):
  intervals=sorted((h[2],h[3])for h in holes if h[0]<(a+b)/2<h[1]);last=0
  for lo,hi in intervals:
   box_bounds(m,'Mineral_aperture_return',x-thick/2,x+thick/2,a,b,last,lo,mat);last=max(last,hi)
  box_bounds(m,'Mineral_aperture_return',x-thick/2,x+thick/2,a,b,last,height,mat)
def frame(m,x,y,z,w,h,lod,mat='Timber',depth=.22):
 for a in [-1,1]:
  m.box('Deep_window_jamb',(x,y+a*(w/2+.035),z),(depth,.07,h+.14),mat)
  m.box('Deep_window_rail',(x,y,z+a*(h/2+.035)),(depth,w+.14,.07),mat)
 if lod<2:m.box('Drip_sill',(x+.015,y,z-h/2-.045),(depth+.08,w+.22,.06),'Concrete')
def pane(m,x,y,z,w,h,mat='Glass',normal=1):
 m.tag='Thin_actual_glazing';ps=[(x,y-w/2,z-h/2),(x,y+w/2,z-h/2),(x,y+w/2,z+h/2),(x,y-w/2,z+h/2)]
 if normal<0:ps=ps[::-1]
 m.face(ps,mat,[(0,0),(1,0),(1,1),(0,1)])
def table(m,xy,size=(1.4,.8),top=.78,mat='Timber'):
 x,y=xy;m.rounded_box('Bullnosed_tabletop',(x,y,top-.035),(size[0],size[1],.07),.035,mat,2)
 for dx in [-size[0]/2+.12,size[0]/2-.12]:
  for dy in [-size[1]/2+.10,size[1]/2-.10]:m.box('Table_tapered_leg',(x+dx,y+dy,(top-.07)/2),(.065,.065,top-.07),'DarkWood')
def chair(m,x,y,lod,metal=False,yaw=0):
 c=Mesh('chair');mat='DarkMetal'if metal else 'Timber'
 c.rounded_box('Shaped_vinyl_seat',(0,0,.46),(.45,.46,.075),.05,'Upholstery',2)
 for dx in [-.18,.18]:
  for dy in [-.18,.18]:c.tube('Chair_tubular_leg',(dx,dy,.025),(dx,dy,.44),.022,mat,max(5,seg(lod)//2))
 for dx in [-.20,.20]:c.tube('Chair_back_support',(dx,.18,.40),(dx,.23,.91),.021,mat,6)
 c.rounded_box('Chair_contoured_back',(0,.23,.80),(.44,.075,.26),.035,'Upholstery',2);m.merge(c,(x,y,0),yaw)
def cabinet(m,x,y,w=.80,d=.48,h=1.4,drawers=4,lod=0,mat='OliveEnamel'):
 m.rounded_box('Rolled_steel_filing_cabinet',(x,y,h/2),(w,d,h),.025,mat,2)
 for k in range(drawers):
  z=.12+(k+.5)*(h-.15)/drawers;m.box('Independent_drawer_front',(x+d*.0+w/2+.014,y,z),(.026,d-.05,(h-.15)/drawers-.025),mat)
  m.box('Drawer_label_card',(x+w/2+.030,y,z+.055),(.007,.13,.045),'Paper')
  if lod<2:m.tube('Drawer_pull',(x+w/2+.05,y-.065,z-.025),(x+w/2+.05,y+.065,z-.025),.012,'Metal',6)
def book(m,x,y,z,w=.13,h=.25,mat='Paper'):
 m.box('Bound_record_volume',(x,y,z+h/2),(.24,w,h),mat);m.box('Cloth_book_spine',(x+.122,y,z+h/2),(.016,w,h),'BookCloth')
def lamp(m,x,y,z,lod,label='WarmGlow'):
 n=seg(lod);m.lathe('Turned_lamp_base',(x,y,z),[(0,.12),(.035,.14),(.07,.10),(.27,.035)],'DarkMetal',n)
 m.lathe('Fabric_lamp_shade',(x,y,z+.25),[(0,.22),(.30,.13),(.32,.13)],'LampFabric',n,caps=False)
 m.lathe('Lamp_visible_opal',(x,y,z+.27),[(0,.12),(.035,.12)],label,n)
def pendant(m,x,y,z,lod,label='WarmGlow'):
 m.tube('Fixture_stem',(x,y,z),(x,y,z+.4),.014,'DarkMetal',6)
 m.lathe('Spun_enamel_shade',(x,y,z-.14),[(0,.25),(.05,.26),(.15,.10),(.2,.04)],'DarkMetal',seg(lod),caps=False)
 m.lathe('Opal_bulb',(x,y,z-.14),[(0,.07),(.07,.09),(.12,.05)],label,seg(lod))
def tube_light(m,x,y,z,length=1.2,label='TubeGlow'):
 m.box('Fluorescent_folded_housing',(x,y,z),(.22,length,.09),'Metal')
 for dx in [-.055,.055]:m.tube('Frosted_fluorescent_tube',(x+dx,y-length/2+.06,z-.055),(x+dx,y+length/2-.06,z-.055),.019,label,8)
def trim_room(m,x0,x1,y0,y1,h,lod,floor='Linoleum',ceiling=True):
 m.box('Original_floor_authority',(sum([x0,x1])/2,(y0+y1)/2,-.055),(x1-x0,y1-y0,.11),floor)
 if ceiling:m.box('Acoustic_ceiling',((x0+x1)/2,(y0+y1)/2,h+.035),(x1-x0,y1-y0,.07),'Ceiling')
 for x in [x0+.035,x1-.035]:m.box('Perimeter_skirting',(x,(y0+y1)/2,.065),(.06,y1-y0,.13),'DarkWood')
 for y in [y0+.035,y1-.035]:m.box('Perimeter_skirting',((x0+x1)/2,y,.065),(x1-x0,.06,.13),'DarkWood')
def roofplant(m,x,y,z,lod):
 m.box('Roof_mechanical_plinth',(x,y,z+.08),(1.5,1.2,.16),'Concrete');m.box('Galvanized_HVAC',(x,y,z+.46),(1.25,1.0,.62),'Metal')
 for k in range([11,7,4][lod]):m.box('HVAC_louvre',(x+.637,y,z+.22+k*.041),(.018,.82,.018),'DarkMetal')
 m.lathe('Roof_vent_stack',(x-1.7,y+.8,z),[(0,.13),(.65,.13),(.69,.19),(.75,.19)],'Metal',seg(lod))
