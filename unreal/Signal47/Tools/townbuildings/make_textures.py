"""Original deterministic shared PBR. Root invokes in its exclusive production slot.
No downloads, no photographic/source concept pixels, no external generation API.
"""
from pathlib import Path
import importlib.util,json,math
import numpy as np
from PIL import Image,ImageDraw,ImageFilter
from materials import material_rows
R=Path(__file__).resolve().parent
s=importlib.util.spec_from_file_location('town_original_pbr_helpers',R.parent/'substation/make_textures.py');base=importlib.util.module_from_spec(s);s.loader.exec_module(base);base.R=R
N=2048
LABELS=[('CALICHE COUNTY',['SHERIFF','SUBSTATION']),('LAUNDROMAT',['OPEN 24 HOURS','COIN OPERATED']),('FEED & SEED',['FARM SUPPLIES','CALICHE, NEW MEXICO']),('PLAZA',['PICTURE HOUSE']),('CLOSED',['PLAZA THEATRE']),('SIERRA',['MOTOR COURT','VACANCY']),('6',[]),('OFFICE',['REGISTER HERE']),('SIERRA MOTOR COURT',['SINGLE     $18.00','DOUBLE     $24.00']),('WASH 75c',['DRY 25c','PLEASE REMOVE LINT']),('NEW MEXICO',['COUNTY ROAD MAP']),('THE DESERT SKY',['A PLAZA PRESENTATION']),('FRIDAY MATINEE',['PLAZA THEATRE']),('CALICHE CHAPEL',['SUNDAY SERVICE 10 AM']),('CALICHE FEED',['NET WT 50 LB','GRAIN & SEED']),('BATHROOM',[])]
def atlas():
 im=Image.new('RGB',(N,N),(212,200,167));d=ImageDraw.Draw(im)
 for i,(title,lines)in enumerate(LABELS):
  x=i%4*512;y=i//4*512;dark=i in [0,1,2,3,5,6,13];bg=(42,51,46)if dark else(218,204,167);ink=(223,211,173)if dark else(49,48,36)
  d.rectangle((x+6,y+6,x+506,y+506),fill=bg,outline=ink,width=5)
  if i==10:
   d.rectangle((x+27,y+77,x+485,y+474),fill=(194,185,153),outline=ink,width=2)
   for k in range(17):
    xx=x+30+k*27;d.line([(xx,y+80),(xx+int(9*math.sin(k)),y+470)],fill=(147,145,115),width=1)
   for k in range(15):d.line([(x+30,y+80+k*27),(x+485,y+78+k*27)],fill=(147,145,115),width=1)
   for k in range(6):
    pts=[(x+35+j*56,y+100+k*54+int(17*math.sin(j*.9+k)))for j in range(9)];d.line(pts,fill=(103,90,69),width=2)
   for px,py in [(140,210),(280,330),(370,180),(230,390)]:d.ellipse((x+px-4,y+py-4,x+px+4,y+py+4),fill=(137,47,31))
   d.text((x+256,y+24),'NEW MEXICO',font=base.font(33,True),anchor='mt',fill=ink);continue
  if i in [11,12]:
   d.polygon([(x+25,y+340),(x+160,y+180),(x+305,y+320),(x+420,y+215),(x+487,y+340)],fill=(127,97,67));d.ellipse((x+310,y+95,x+405,y+190),fill=(181,126,70))
  size=142 if title=='6'else min(60,int(720/max(6,len(title))))
  d.text((x+256,y+100 if title!='6'else y+148),title,font=base.font(size,True),anchor='mt',fill=ink)
  for k,t in enumerate(lines):d.text((x+256,y+245+k*74),t,font=base.font(min(38,int(650/max(6,len(t)))),True),anchor='mt',fill=ink)
 im.save(R/'textures/PrintAtlas_base_color.png');Image.new('RGB',(N,N),(128,128,255)).save(R/'textures/PrintAtlas_normal.png');Image.new('L',(N,N),220).save(R/'textures/PrintAtlas_roughness.png')
def main():
 (R/'textures').mkdir(exist_ok=True);rng=np.random.default_rng(19864725);yy,xx=np.indices((N,N),dtype=np.float32)
 broad=base.noise(rng,7);fine=base.noise(rng,180);grain=rng.random((N,N),dtype=np.float32)
 # Quiet mineral multiscale surface with thin gravity weathering, no blobby mold.
 streak=Image.new('L',(N,N));d=ImageDraw.Draw(streak)
 for _ in range(90):
  x=int(rng.integers(0,N));y=int(rng.integers(0,N));w=int(rng.integers(3,13));h=int(rng.integers(100,650));d.polygon([(x-w,y),(x+w,y),(x+w//3,y+h),(x-w//4,y+h)],fill=int(rng.integers(9,35)))
 st=np.asarray(streak.filter(ImageFilter.GaussianBlur(5)),dtype=np.float32)/255
 a=.91+.055*broad+.025*fine-st*.22
 base.maps('Stucco',np.ones((N,N,3))*245*a[:,:,None],fine*.11+grain*.035,.87+.08*broad)
 g=np.sin(xx*.25+base.noise(rng,32)*5)*.5+.5;wood=.82+.12*broad+.08*g
 base.maps('Wood',np.ones((N,N,3))*235*wood[:,:,None],g*.055+fine*.015,.67+.18*broad)
 peel=np.clip((.20-base.noise(rng,28))*4,0,.43);v=.95+.022*broad-.18*peel-st*.10
 base.maps('MetalPaint',np.ones((N,N,3))*248*v[:,:,None],fine*.012-peel*.07,.56+.25*peel+.08*broad)
 # Long connected rain streaks over predominantly galvanized material.
 rust=np.clip((base.noise(rng,10)*.65+base.noise(rng,39)*.35-.57)*3.6,0,.85)
 rust*=.45+.55*(np.sin(xx*.035+base.noise(rng,20)*4)*.5+.5)
 rgb=np.array([200,197,185])[None,None,:]*(1-rust[:,:,None])+np.array([116,65,34])[None,None,:]*rust[:,:,None]
 base.maps('RustTin',rgb*(.94+.05*fine)[:,:,None],fine*.018+rust*.07,.64+.28*rust)
 tile=(xx%512<9)|(yy%512<9);check=(xx.astype(int)//512+yy.astype(int)//512)%2
 tone=.82+.13*check+.04*broad;base.maps('FloorTile',np.ones((N,N,3))*255*np.where(tile,.53,tone)[:,:,None],np.where(tile,0,.04)+fine*.005,.62+.18*tile+.06*broad)
 weave=(np.sin(xx*math.pi/2)*np.sin(yy*math.pi/2)+1)/2
 base.maps('Carpet',np.ones((N,N,3))*230*(.80+.08*broad+.09*weave)[:,:,None],weave*.055+fine*.02,.94+fine*.03)
 cloth=.85+.07*broad+.08*weave;base.maps('Cloth',np.ones((N,N,3))*245*cloth[:,:,None],weave*.045+fine*.02,.81+.13*broad)
 sy=yy%128;offset=((yy//128).astype(int)%2)*128;seam=((xx+offset)%256<6)|(sy<9);h=np.where(seam,0,.065+(sy/128)*.025)
 base.maps('RoofShingle',np.ones((N,N,3))*240*(.76+.16*broad-.23*seam+.04*fine)[:,:,None],h+fine*.02,.89+.07*broad)
 atlas();(R/'materials.json').write_text(json.dumps(material_rows(),indent=2)+'\n');print('TOWN_ORIGINAL_PBR_READY',len(material_rows()),'materials /9 shared original 2k sets')
if __name__=='__main__':main()
