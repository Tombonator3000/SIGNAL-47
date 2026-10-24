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
LABELS=[('CALICHE COUNTY',['SHERIFF','SUBSTATION']),('LAUNDROMAT',['OPEN 24 HOURS','COIN OPERATED']),('FEED & SEED',['FARM SUPPLIES','CALICHE, NEW MEXICO']),('PLAZA',['PICTURE HOUSE']),('CLOSED',['PLAZA THEATRE']),('CLOSED',['PLAZA THEATRE']),('6',[]),('OFFICE',['REGISTER HERE']),('SIERRA MOTOR COURT',['SINGLE     $18.00','DOUBLE     $24.00']),('WASH 75c',['DRY 25c','PLEASE REMOVE LINT']),('NEW MEXICO',['COUNTY ROAD MAP']),('THE DESERT SKY',['A PLAZA PRESENTATION']),('FRIDAY MATINEE',['PLAZA THEATRE']),('CALICHE CHAPEL',['SUNDAY SERVICE 10 AM']),('CALICHE FEED',['NET WT 50 LB','GRAIN & SEED']),('CLOSED',[])]
# Physical panel dimensions. Separate cells4/5/15 keep the front marquee,
# its shorter returns and the ticket notice aspect-correct independently.
# Only the two secondary Cinema sign UV-cell assignments change.
PANEL_SIZE_M=[(3.5,.62),(4.5,.67),(6.7,1.1),(.78,3.21),(9.6,.99),
 (2.42,.99),(.19,.27),(1.15,.29),(.60,.44),(.60,.74),(2.83,1.68),
 (1.13,1.77),(1.13,1.77),(3.5,.62),(.60,.80),(.50,.27)]
def fitted_text(draw,text,rect,ink,proof):
 # Measure the actual font glyph bbox, including bearings, rather than guessing
 # from string length. No title can spill into an adjacent atlas cell.
 x0,y0,x1,y1=rect;available_w=x1-x0;available_h=y1-y0
 chosen=None
 for size in range(max(1,int(available_h*1.6)),0,-1):
  font=base.font(size,True);bb=draw.textbbox((0,0),text,font=font)
  if bb[2]-bb[0]<=available_w and bb[3]-bb[1]<=available_h:
   chosen=(font,bb);break
 assert chosen is not None,('text cannot fit',text,rect)
 font,bb=chosen;xx=(x0+x1-bb[2]-bb[0])/2;yy=(y0+y1-bb[3]-bb[1])/2
 actual=draw.textbbox((xx,yy),text,font=font)
 assert actual[0]>=x0-1 and actual[1]>=y0-1 and actual[2]<=x1+1 and actual[3]<=y1+1,(text,actual,rect)
 draw.text((xx,yy),text,font=font,fill=ink)
 proof.append({'text':text,'actual_bbox_canvas_px':list(actual),'allowed_bbox_canvas_px':list(rect),'font_size':font.size})
def atlas():
 im=Image.new('RGB',(N,N),(212,200,167));layout=[]
 for i,(title,lines)in enumerate(LABELS):
  x=i%4*512;y=i//4*512;dark=i in [0,1,2,3,6,13]
  bg=(42,51,46)if dark else(218,204,167);ink=(223,211,173)if dark else(49,48,36)
  pw,ph=PANEL_SIZE_M[i];aspect=pw/ph
  # Work at the real sign's aspect, then pre-distort to its square atlas tile.
  # The unchanged square UV mapping to the physical panel reverses that resize.
  w=max(256,round(1024*aspect))if aspect<1 else 2048
  h=round(w/aspect);canvas=Image.new('RGB',(w,h),bg);d=ImageDraw.Draw(canvas);proof=[]
  def box(a,b,c,e):return(a*w,b*h,c*w,e*h)
  def label(t,a,b,c,e):fitted_text(d,t,box(a,b,c,e),ink,proof)
  # Keep border and all glyphs inside existing.035..965 guarded UV limits.
  d.rectangle(box(.045,.045,.955,.955),outline=ink,width=max(1,round(min(w,h)*.008)))
  if i==10:
   d.rectangle(box(.07,.24,.93,.91),fill=(194,185,153),outline=ink,width=2)
   for k in range(17):
    xx=w*(.08+k*.052);d.line([(xx,h*.25),(xx+w*.008*math.sin(k),h*.90)],fill=(147,145,115),width=1)
   for k in range(15):d.line([(w*.08,h*(.25+k*.046)),(w*.92,h*(.25+k*.046))],fill=(147,145,115),width=1)
   for k in range(6):
    pts=[(w*(.09+j*.10),h*(.30+k*.10+.025*math.sin(j*.9+k)))for j in range(9)];d.line(pts,fill=(103,90,69),width=2)
   for px,py in[(.25,.44),(.56,.67),(.74,.38),(.46,.78)]:d.ellipse((w*px-4,h*py-4,w*px+4,h*py+4),fill=(137,47,31))
   label(title,.08,.075,.92,.20)
  elif i==3:
   # The tall blade sign uses vertical lettering, not a horizontal word stretched.
   for k,ch in enumerate(title):label(ch,.20,.085+k*.143,.80,.205+k*.143)
   label('PICTURE',.10,.825,.90,.88);label('HOUSE',.10,.89,.90,.94)
  elif i in [11,12]:
   d.polygon([(w*.07,h*.72),(w*.30,h*.36),(w*.58,h*.67),(w*.82,h*.42),(w*.93,h*.72)],fill=(127,97,67))
   d.ellipse((w*.58,h*.28,w*.82,h*.44),fill=(181,126,70))
   label(title,.07,.09,.93,.22)
   for k,t in enumerate(lines):label(t,.08,.79+k*.07,.92,.85+k*.07)
  elif i==6:label(title,.13,.13,.87,.87)
  else:
   label(title,.07,.12,.93,.47 if lines else .88)
   for k,t in enumerate(lines):
    lo=.56+k*(.30/max(1,len(lines)));hi=lo+.27/max(1,len(lines))
    label(t,.08,lo,.92,hi)
  layout.append({'cell':i,'title':title,'primary_panel_m':[pw,ph],'canvas_px':[w,h],
   'glyphs':proof,'guarded_uv_cell_fraction':[.035,.965]})
  im.paste(canvas.resize((512,512),Image.Resampling.LANCZOS),(x,y))
 im.save(R/'textures/PrintAtlas_base_color.png')
 Image.new('RGB',(N,N),(128,128,255)).save(R/'textures/PrintAtlas_normal.png')
 Image.new('L',(N,N),220).save(R/'textures/PrintAtlas_roughness.png')
 (R/'atlas_layout.json').write_text(json.dumps({'status':'PASS','scope':'Measured glyph bounds on physical-aspect canvases; unchanged geometry and guarded atlas UVs','cells':layout},indent=2)+'\n')
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
