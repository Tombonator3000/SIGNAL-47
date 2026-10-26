"""Original tiny raised seven-segment numerals for physical motel room plaques."""
from geometry import add,mul,cross,unit
DIGITS={'0':'abcedf','1':'bc','2':'abged','3':'abgcd','4':'fgbc','5':'afgcd','6':'afgecd','7':'abc','8':'abcdefg','9':'abfgcd'}
def text_mesh(m,text,center,height,mat='Paper',normal=(1,0,0)):
 right=unit(cross((0,0,1),normal));up=(0,0,1);n=normal;th=height*.09;w=height*.49
 segments={'a':(-w/2,.5*height,w/2,.5*height),'g':(-w/2,0,w/2,0),'d':(-w/2,-.5*height,w/2,-.5*height),'f':(-w/2,0,-w/2,.5*height),'b':(w/2,0,w/2,.5*height),'e':(-w/2,-.5*height,-w/2,0),'c':(w/2,-.5*height,w/2,0)}
 for j,ch in enumerate(text):
  cx=(j-(len(text)-1)/2)*height*.72
  for k in DIGITS.get(ch,''):
   x0,z0,x1,z1=segments[k];p=add(center,add(mul(right,cx+(x0+x1)/2),mul(up,(z0+z1)/2)))
   a=(x1-x0)/2+th/2;b=(z1-z0)/2+th/2;m.tag='Raised_original_room_numeral'
   m.face([add(p,add(mul(right,x*a),mul(up,z*b)))for x,z in [(-1,-1),(1,-1),(1,1),(-1,1)]],mat)
