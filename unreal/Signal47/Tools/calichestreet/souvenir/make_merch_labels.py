"""Original period shop price/packaging cards, a 4x4 UV atlas, no external art."""
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
import random,json
ROOT=Path(__file__).resolve().parent
labels=[('POSTCARDS','35c EACH'),('DESERT MUGS','$2.50'),('TOY SAUCERS','$3.00'),('ROAD MAPS','$1.95'),
 ('GENUINE DEBRIS','$1.00'),('MINERAL SAMPLES','$1.50'),('DESERT TEA TOWELS','$2.00'),('TRAVEL GUIDES','$1.95'),
 ('CALICHE SOUVENIRS','HAND PACKED'),('NEW MEXICO','DESERT TRADING CO.'),('TOURS','ASK INSIDE'),('GIFT SET','CALICHE  NM'),
 ('COLLECTOR SERIES','1947'),('CAREFULLY WRAPPED','FRAGILE'),('LOCAL MINERALS','KEEP DRY'),('POSTCARD SELECTION','NEW MEXICO')]
im=Image.new('RGB',(2048,1024),(207,188,145));d=ImageDraw.Draw(im);rng=random.Random(194703)
font='/usr/share/fonts/truetype/dejavu/DejaVuSerif-Bold.ttf';small='/usr/share/fonts/truetype/dejavu/DejaVuSerif.ttf'
for idx,(name,price) in enumerate(labels):
 x=(idx%4)*512;y=(idx//4)*256
 d.rectangle((x+9,y+9,x+502,y+246),fill=(225,209,174),outline=(109,87,57),width=3)
 for k in range(600):
  xx=x+rng.randrange(15,496);yy=y+rng.randrange(15,241)
  d.point((xx,yy),fill=(rng.randrange(184,213),rng.randrange(170,196),rng.randrange(140,162)))
 f=ImageFont.truetype(font,25 if len(name)>18 else 29);pf=ImageFont.truetype(font,43 if '$' in price else 31)
 d.text((x+256,y+62),name,fill=(53,43,31),font=f,anchor='mm')
 d.line((x+45,y+104,x+467,y+104),fill=(130,83,51),width=2)
 d.text((x+256,y+155),price,fill=(77,40,27),font=pf,anchor='mm')
 d.text((x+256,y+213),'SAUCER CRASH 1947',fill=(112,89,57),font=ImageFont.truetype(small,17),anchor='mm')
im.save(ROOT/'textures/souvenir_labels_v3.png')
(ROOT/'label_atlas.json').write_text(json.dumps({'size':[2048,1024],'grid':[4,4],'labels':labels,'author':'original SIGNAL / 47 typography','font':'DejaVu Serif, system font'},indent=2))
