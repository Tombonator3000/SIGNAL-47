"""Original Mesa Diner 2k PBR. Run only in parent's shared heavy-job slot."""
from pathlib import Path
import importlib.util,json,math,hashlib
import numpy as np
from PIL import Image,ImageDraw
from materials import material_rows
R=Path(__file__).resolve().parent
_spec=importlib.util.spec_from_file_location('mesa_original_texture_helpers',R.parent/'substation/make_textures.py');base=importlib.util.module_from_spec(_spec);_spec.loader.exec_module(base);base.R=R
N=2048

def atlas():
 im=Image.new('RGB',(N,N),(207,195,160));d=ImageDraw.Draw(im);ink=(37,36,27)
 def panel(i,title,lines):
  x=i%2*1024;y=i//2*512;d.rectangle((x+8,y+8,x+1016,y+504),fill=(218,207,175),outline=ink,width=8);d.text((x+512,y+28),title,font=base.font(65,True),anchor='mt',fill=ink)
  for k,text in enumerate(lines):d.text((x+55,y+133+k*72),text,font=base.font(37,True),fill=ink)
 panel(0,'MESA DINER',['COFFEE  .35     PIE  .95','HOTCAKES             1.75','GREEN CHILE STEW     2.95','TWO EGGS ANY STYLE   1.95'])
 panel(1,'SHORT ORDERS',['HAMBURGER           2.25','CHEESEBURGER        2.45','HAM SANDWICH        1.95','SOUP OF THE DAY     1.10'])
 panel(2,'RESTROOM',['CUSTOMERS ONLY','PLEASE LEAVE KEY','AT THE COUNTER','THANK YOU'])
 panel(3,'MESA DINER',['OPEN ALL NIGHT','CALICHE, NEW MEXICO','COFFEE ALWAYS FRESH','EST. 1954'])
 panel(4,'TELEPHONE',['LOCAL CALLS  25 CENTS','PLEASE LIMIT CALLS','TO THREE MINUTES','COIN RETURN BELOW'])
 panel(5,'DAILY SPECIAL',['GREEN CHILE STEW','COFFEE AND PIE','HOMEMADE EACH DAY','ASK YOUR SERVER'])
 panel(6,'PUSH',['WELCOME','OPEN ALL NIGHT','MESA DINER','CASH ONLY'])
 panel(7,'CALICHE COUNTY',['COMMUNITY NOTICES','FRIDAY FISH DINNER','REPAIR SERVICE','APRIL 1986'])
 out=R/'textures';im.save(out/'PrintAtlas_base_color.png');Image.new('RGB',(1024,1024),(128,128,255)).save(out/'PrintAtlas_normal.png');Image.new('L',(1024,1024),218).save(out/'PrintAtlas_roughness.png')
def facade_maps():
 # Independent RNG: the six retained interior texture families stay byte-identical.
 rng=np.random.default_rng(1986478602);broad=base.noise(rng,5);grain=base.noise(rng,160);fine=rng.random((N,N),dtype=np.float32)
 stains=Image.new('L',(N,N));d=ImageDraw.Draw(stains)
 for _ in range(95):
  x=int(rng.integers(0,N));y=int(rng.choice([rng.integers(0,160),rng.integers(600,1500)]));length=int(rng.integers(80,540));width=int(rng.integers(4,24));strength=int(rng.integers(10,46))
  # Thin gravity runs are gently feathered and taper toward their lower end.
  d.polygon([(x-width,y),(x+width,y),(x+width//3,y+length),(x-width//4,y+length)],fill=strength)
 from PIL import ImageFilter
 streak=np.asarray(stains.filter(ImageFilter.GaussianBlur(6)),dtype=np.float32)/255
 chips=Image.new('L',(N,N));d=ImageDraw.Draw(chips)
 for _ in range(210):
  x=int(rng.integers(0,N));y=int(rng.integers(0,N));r=int(rng.integers(2,16))
  d.polygon([(x-r,y),(x+r,y-2),(x+r//2,y+r),(x-r,y+r//2)],fill=int(rng.integers(80,175)))
 edge=np.asarray(chips,dtype=np.float32)/255
 tone=.95+.035*(broad-.5)+.026*(grain-.5)+.012*(fine-.5)-streak*.24
 color=np.array([216,205,181])[None,None,:]*tone[:,:,None]-edge[:,:,None]*np.array([40,42,31])
 base.maps('Stucco',color,grain*.10+fine*.035-edge*.13,.87+.07*grain)
 # Exfoliation is sparse, hard-edged and elongated; no broad green/brown blotches.
 peel=Image.new('L',(N,N));d=ImageDraw.Draw(peel)
 for _ in range(290):
  x=int(rng.integers(0,N));y=int(rng.integers(0,N));w=int(rng.integers(4,44));h=int(rng.integers(2,10))
  d.polygon([(x-w,y),(x+w//2,y-h),(x+w,y),(x+w//4,y+h),(x-w//2,y+h//2)],fill=int(rng.integers(150,255)))
 wear=np.asarray(peel,dtype=np.float32)/255
 red=np.array([150,63,52])[None,None,:]*(.96+.035*(broad-.5)+.035*(grain-.5)-streak*.15)[:,:,None]
 color=red*(1-wear[:,:,None])+np.array([205,187,151])[None,None,:]*wear[:,:,None]
 base.maps('PaintRed',color,grain*.020-wear*.09,.70+.12*wear)


def main():
 (R/'textures').mkdir(exist_ok=True);rng=np.random.default_rng(19864786)
 protected=['TealTile','CheckerFloor','Vinyl','Formica','Wood','PrintAtlas'];before={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in (R/'textures').glob('*.png') if p.name.split('_')[0] in protected}
 a=base.noise(rng,7);b=base.noise(rng,92);c=rng.random((N,N),dtype=np.float32)
 chipped=np.clip((.20-a)*3.0,0,.24);color=np.array([214,203,177])[None,None,:]*(.91+.12*a+.045*(b-.5))[:,:,None]-chipped[:,:,None]*np.array([65,61,45])
 base.maps('Stucco',color,b*.14+c*.06-chipped*.10,.85+.12*a)
 a=base.noise(rng,9);b=base.noise(rng,151);wear=np.clip((.28-a)*3,0,.5)
 color=np.array([153,64,51])[None,None,:]*(.90+.15*a)[:,:,None];color=color*(1-wear[:,:,None])+np.array([182,165,130])[None,None,:]*wear[:,:,None]
 base.maps('PaintRed',color,b*.035+wear*.07,.64+.22*a)
 yy,xx=np.indices((N,N),dtype=np.float32);tx=(xx%512)/512;ty=(yy%512)/512;grout=(tx<.024)|(ty<.024);a=base.noise(rng,4);b=base.noise(rng,130)
 tile=np.array([80,128,116])[None,None,:]*(.87+.20*a+.04*(b-.5))[:,:,None];tile[grout]=[129,125,105]
 base.maps('TealTile',tile,np.where(grout,0,.12)+b*.008,np.where(grout,.93,.34+.10*a))
 gx=(xx.astype(int)//1024);gy=(yy.astype(int)//1024);seam=(xx%1024<11)|(yy%1024<11);a=base.noise(rng,11);b=base.noise(rng,180);dark=((gx+gy)%2==0)
 color=np.where(dark[:,:,None],np.array([58,57,49]),np.array([177,168,142]))*(.90+.17*a+.04*(b-.5))[:,:,None];color[seam]=[101,94,78]
 base.maps('CheckerFloor',color,np.where(seam,0,.055)+b*.015,.65+.20*a)
 a=base.noise(rng,8);b=base.noise(rng,240);color=np.array([111,32,29])[None,None,:]*(.91+.16*a+.055*(b-.5))[:,:,None]
 base.maps('Vinyl',color,b*.045,.29+.16*a)
 a=base.noise(rng,16);b=base.noise(rng,110);color=np.array([194,182,153])[None,None,:]*(.91+.14*a+.05*(b-.5))[:,:,None]
 base.maps('Formica',color,b*.01,.31+.10*a)
 a=base.noise(rng,35);b=base.noise(rng,150);grain=np.sin(xx*.16+a*5)*.5+.5;color=np.array([89,62,39])[None,None,:]*(.83+.19*a+.13*grain)[:,:,None]
 base.maps('Wood',color,b*.025+grain*.035,.70+.15*a)
 facade_maps();atlas();after={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in (R/'textures').glob('*.png') if p.name.split('_')[0] in protected}
 assert not before or before==after,'Retained interior texture changed'
 (R/'facade_texture_proof.json').write_text(json.dumps({'revision':'R2-concept-fascia','protected_interior_PNG_byte_identical':not before or before==after,'compared_count':len(before),'sha256':after},indent=2)+'\n')
 rows={row['name']:row for row in material_rows()}
 if (R/'make_exterior_textures.py').exists():
  import make_exterior_textures
  make_exterior_textures.main()
  for row in make_exterior_textures.material_rows_extra():rows[row['name']]=row
 (R/'materials.json').write_text(json.dumps(list(rows.values()),indent=2)+'\n');print('MESA_DINER_ORIGINAL_PBR_READY',len(rows),'materials /8original diner sets plus exterior')
if __name__=='__main__':main()
