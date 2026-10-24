"""Original Mesa Diner 2k PBR. Run only in parent's shared heavy-job slot."""
from pathlib import Path
import importlib.util,json,math
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
def main():
 (R/'textures').mkdir(exist_ok=True);rng=np.random.default_rng(19864786)
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
 atlas();rows={row['name']:row for row in material_rows()}
 if (R/'make_exterior_textures.py').exists():
  import make_exterior_textures
  make_exterior_textures.main()
  for row in make_exterior_textures.material_rows_extra():rows[row['name']]=row
 (R/'materials.json').write_text(json.dumps(list(rows.values()),indent=2)+'\n');print('MESA_DINER_ORIGINAL_PBR_READY',len(rows),'materials /8original diner sets plus exterior')
if __name__=='__main__':main()
