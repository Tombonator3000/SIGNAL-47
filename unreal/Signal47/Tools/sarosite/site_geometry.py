"""Original SARO wing and single service-transformer equipment, metres/Blender Z-up.
No process launch. Utility pole itself belongs to the existing native HighwayPower HISM.
"""
from meshkit import Mesh as Base
ROOT=[0,0,0]
PARTS={
 'SM_SaroSite_Facade':{'offset':[29.6,0,-.6],'world_root':[29.6,-.6,0],'role':'Solid sign wall, actual recessed window apertures and original quiet room depth'},
 'SM_SaroSite_Glass':{'offset':[29.6,0,-.6],'world_root':[29.6,-.6,0],'role':'Separate recessed panes, no use/floor collision'},
 'SM_SaroSite_ServiceEquipment':{'offset':[3.6,-18.6,-.6],'world_root':[3.6,-.6,18.6],'role':'One pole-mounted transformer and the existing meter/weatherhead at exact original coordinates'},
}
class Mesh(Base):
 def face(self,ps,mat,uv=None,smooth=False):
  super().face(ps,mat,uv,smooth);self.mat[-1]=self.mat[-1].replace('M_Substation_','M_SaroSite_',1)
def block(m,name,x0,x1,y0,y1,z0,z1,mat='Stucco'):
 m.box(name,((x0+x1)/2,(y0+y1)/2,(z0+z1)/2),(x1-x0,y1-y0,z1-z0),mat)
def build(lod=0):
 f=Mesh('SM_SaroSite_Facade');g=Mesh('SM_SaroSite_Glass');e=Mesh('SM_SaroSite_ServiceEquipment')
 front=[(2.05,4.05,1.25,2.90),(5.35,7.35,1.25,2.90)]
 back=[(x-.73,x+.73,1.25,2.90) for x in [-7.6,-5.4,-3.2,-1,1.2,3.4,5.6,7.8]]
 def side(y,windows,frontside):
  xs=sorted(set([-9,9]+[x for w in windows for x in w[:2]]));ys=(y,y+.24) if frontside else (y-.24,y)
  for a,b in zip(xs,xs[1:]):
   holes=[w for w in windows if w[0]<b-1e-5 and w[1]>a+1e-5]
   if not holes:block(f,'Solid_facade_wall',a,b,*ys,0,4.2)
   else:
    block(f,'Window_sill_wall',a,b,*ys,0,holes[0][2]);block(f,'Window_head_wall',a,b,*ys,holes[0][3],4.2)
  for k,(a,b,z0,z1) in enumerate(windows):
   inward=.27 if frontside else -.27;gy=y+inward
   block(g,'Real_recessed_glass',a+.035,b-.035,gy-.006,gy+.006,z0+.035,z1-.035,'Glass')
   # Four deep light-painted jambs, an outward drip sill, and a central mullion.
   block(f,'Reveal_left',a,a+.055,min(y,gy),max(y,gy),z0,z1,'Trim')
   block(f,'Reveal_right',b-.055,b,min(y,gy),max(y,gy),z0,z1,'Trim')
   block(f,'Reveal_head',a,b,min(y,gy),max(y,gy),z1-.055,z1,'Trim')
   block(f,'Sloped_drip_sill',a-.06,b+.06,y-.10 if frontside else y-.24,y+.24 if frontside else y+.10,z0-.065,z0+.02,'Concrete')
   block(f,'Window_mullion',(a+b)/2-.022,(a+b)/2+.022,gy-.026,gy+.026,z0,z1,'Trim')
   # A physical shallow room behind the pane, with ceiling, wall and furniture silhouettes.
   rear=y+(1.38 if frontside else -1.38);lo,hi=sorted([gy,rear])
   block(f,'Room_back',a,b,rear-.025,rear+.025,z0,z1,'Interior')
   block(f,'Room_floor',a,b,lo,hi,z0-.02,z0,'Interior')
   block(f,'Room_ceiling',a,b,lo,hi,z1,z1+.02,'Interior')
   block(f,'Room_side',a,a+.025,lo,hi,z0,z1,'Interior');block(f,'Room_side',b-.025,b,lo,hi,z0,z1,'Interior')
   desk=y+(.86 if frontside else -.86)
   block(f,'Quiet_office_desk',a+.16,b-.15,desk-.22,desk+.22,z0+.31,z0+.36,'Wood')
   block(f,'Desk_cabinet',a+.23,a+.57,desk-.17,desk+.17,z0,z0+.31,'Interior')
   if lod<2:
    for i in range(6 if lod==0 else 3):
     zz=z1-.12-i*.075;block(f,'Half_raised_blind',a+.07,b-.07,gy+(.045 if frontside else -.065),gy+(.065 if frontside else -.045),zz,zz+.018,'Trim')
 side(-5.5,front,True);side(5.5,back,False)
 block(f,'West_end_wall',-9,-8.76,-5.5,5.5,0,4.2)
 block(f,'East_end_wall',8.76,9,-5.5,5.5,0,4.2)
 block(f,'Roof_below_preserved_equipment',-9,9,-5.5,5.5,4.12,4.2,'Roof')
 for y0,y1 in [(-5.54,-5.5),(5.5,5.54)]:block(f,'Weathered_dado',-9,9,y0,y1,0,.32,'Concrete')
 # The entire wordmark is on uninterrupted masonry; no sign-over-fake-window illusion.
 block(f,'Solid_sign_mount',-7.75,.95,-5.547,-5.50,1.62,3.76,'SignBack')
 f.tag='Original_SARO_wordmark_on_solid_wall'
 f.face([(-7.65,-5.552,1.70),(.85,-5.552,1.70),(.85,-5.552,3.68),(-7.65,-5.552,3.68)],'Sign',[(0,0),(1,0),(1,1),(0,1)])
 n=[32,20,12][lod]
 e.lathe('Single_can_distribution_transformer',(-.36,0,6.85),[(0,.31),(.035,.34),(.97,.34),(1.01,.36),(1.06,.36)],'Metal',n)
 for z in [6.88,7.79]:e.torus('Transformer_seam',(-.36,0,z),.34,.014,'DarkMetal',n,6)
 for x,y in [(-.53,-.16),(-.19,.16)]:
  e.lathe('Ceramic_HV_bushing',(x,y,7.91),[(0,.045),(.04,.073),(.07,.043),(.11,.068),(.14,.04),(.21,.035)],'Ceramic',max(8,n//2))
  e.tube('Bushing_terminal',(x,y,8.11),(x,y,8.20),.012,'DarkMetal',8)
 for z in [7.03,7.55]:e.box('Pole_mounting_bracket',(-.12,0,z),(.48,.12,.10),'DarkMetal')
 e.box('Transformer_ID_plate',(-.36,-.347,7.28),(.18,.012,.25),'Label')
 # Original meter box/conduit coordinates, reconstructed when old g441 is removed.
 e.box('Original_meter_case',(-2,7.72,2.1),(.30,.14,.42),'Metal')
 e.tube('Original_weatherhead_conduit',(-2,7.74,2.35),(-2,7.74,4.2),.03,'Metal',max(6,n//2))
 # Old g442 remains the exact glass meter face at world(1.6,1.55,10.96).
 return {f.name:f,g.name:g,e.name:e}
