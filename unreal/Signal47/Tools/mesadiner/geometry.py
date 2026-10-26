"""Original diner modelling primitives; metres/Zup. No Blender process here."""
from pathlib import Path
import importlib.util,math
_path=Path(__file__).resolve().parent.parent/'substation/meshkit.py'
_spec=importlib.util.spec_from_file_location('s47_mesa_original_meshkit',_path)
_original=importlib.util.module_from_spec(_spec);_spec.loader.exec_module(_original)
add,sub,mul,dot,cross,unit=(_original.add,_original.sub,_original.mul,_original.dot,_original.cross,_original.unit)
class Mesh(_original.Mesh):
 def face(self,ps,mat,uv=None,smooth=False):
  super().face(ps,mat,uv,smooth);self.mat[-1]=self.mat[-1].replace('M_Substation_','M_Mesa_',1)
 def panel(self,name,center,width,height,cell,normal=(1,0,0)):
  self.tag=name;normal=unit(normal);right=unit(cross((0,0,1),normal));up=cross(normal,right)
  ps=[add(center,add(mul(right,x*width/2),mul(up,y*height/2))) for x,y in [(-1,-1),(1,-1),(1,1),(-1,1)]]
  col=cell%2;row=cell//2;u0=(col+.025)/2;u1=(col+.975)/2;v0=1-(row+.975)/4;v1=1-(row+.025)/4
  self.face(ps,'PrintAtlas',[(u0,v0),(u1,v0),(u1,v1),(u0,v1)])
 def merge(self,other,offset=(0,0,0),yaw=0):
  co,si=math.cos(yaw),math.sin(yaw);start=len(self.v)
  self.v.extend((offset[0]+p[0]*co-p[1]*si,offset[1]+p[0]*si+p[1]*co,offset[2]+p[2]) for p in other.v)
  self.f.extend([[i+start for i in f] for f in other.f]);self.uv.extend(other.uv);self.mat.extend(other.mat);self.smooth.extend(other.smooth);self.tags.extend(other.tags)
