"""Prepare original shared PBR maps and legible station labels, no Blender launch."""
import hashlib
import json
import shutil
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

R=Path(__file__).resolve().parent
TEXTURES=R/'textures'
TEXTURES.mkdir(exist_ok=True)
SHARED=R.parent/'townbuildings'/'textures'

def linear(rgb):return [c/3294.6 if c<=10.31475 else ((c/255+.055)/1.055)**2.4 for c in rgb]
def rows():
    out=[]
    def row(name,rgb,rough=.8,metal=0,tex=None,normal=.2,**kw):
        r={'name':'M_FieldStation_'+name,'base_color_linear':[*linear(rgb),1],
           'roughness':rough,'metallic':metal,'specular':.28,'normal_strength':normal,
           'normal_convention':'DirectX','uv_scale':1,'emission':[0,0,0,1],'emission_strength':0,**kw}
        if tex:r.update(textures={k:'textures/'+tex+'_'+k+'.png' for k in ['base_color','roughness','normal']},texture_tint_linear=[*linear(rgb),1])
        out.append(r)
    row('Stucco',[198,183,157],.95,tex='Stucco',normal=.38)
    row('Plaster',[204,195,169],.94,tex='Stucco',normal=.16)
    row('PlasterRepair',[187,177,151],.97,tex='Stucco',normal=.2)
    row('Dado',[78,87,69],.84,tex='MetalPaint',normal=.14)
    row('Concrete',[150,141,122],.96,tex='Stucco',normal=.25)
    row('Masonry',[155,126,91],.96,tex='Stucco',normal=.28)
    row('WeatherStain',[106,98,81],.99)
    row('Timber',[127,95,59],.76,tex='Wood',normal=.28)
    row('FloorTimber',[126,113,89],.82,tex='Wood',normal=.24)
    row('DoorTimber',[68,92,72],.86,tex='Wood',normal=.24)
    row('DarkWood',[70,54,35],.84,tex='Wood',normal=.20)
    row('TealPaint',[71,97,80],.79,tex='MetalPaint',normal=.18)
    row('WhitePaint',[195,188,162],.85,tex='MetalPaint',normal=.16)
    row('RoofFelt',[54,51,44],.96,tex='RoofShingle',normal=.3)
    row('RustTin',[157,139,111],.79,.65,'RustTin',.26)
    row('Metal',[150,156,148],.39,.85)
    row('DarkMetal',[41,43,36],.72,.65)
    row('OliveEnamel',[103,109,85],.65,.30,'MetalPaint',.15)
    row('Rubber',[22,23,19],.95)
    row('Ceramic',[200,193,165],.42)
    row('Ceiling',[189,180,153],.96,tex='Stucco',normal=.10)
    row('Cardboard',[165,144,106],.97,tex='Cloth',normal=.15)
    row('BookCloth',[88,105,81],.94,tex='Cloth',normal=.20)
    row('PrintAtlas',[255,255,255],.86,tex='PrintAtlas',normal=.01)
    row('Glass',[163,184,175],.12,alpha=.055,alpha_mode='translucent',two_sided=True)
    return out

def create():
    material_rows=rows()
    families={Path(p).name.split('_base_color')[0] for r in material_rows for k,p in r.get('textures',{}).items() if k=='base_color'}-{'PrintAtlas'}
    for family in sorted(families):
        for role in ['base_color','roughness','normal']:
            shutil.copy2(SHARED/(family+'_'+role+'.png'),TEXTURES/(family+'_'+role+'.png'))
    # These labels are original period-appropriate text, not copied logos or art.
    size=2048;im=Image.new('RGB',(size,size),(26,33,27));draw=ImageDraw.Draw(im)
    font=Path('/usr/share/fonts/truetype/dejavu/DejaVuSansCondensed-Bold.ttf')
    if not font.exists():font=Path('/usr/share/fonts/truetype/ubuntu/UbuntuMono[wght].ttf')
    labels=[['STATION 01','FIELD SURVEY'],['ELECTRICAL','120 V AC'],['SURVEY EQUIPMENT','RETURN AFTER USE'],['SPARE VALVES','FIELD SERVICE'],['SOUTHWEST SURVEY','ESTABLISHED 1947'],['RECORDS','1947 - 1986'],['PROPERTY OF SARO','STATION 01'],['KEEP DRY','SERVICE STORES']]
    for i,lines in enumerate(labels):
        x=(i%2)*1024;y=(i//2)*512
        draw.rectangle([x+18,y+14,x+1006,y+498],fill=(35,49,40),outline=(188,180,146),width=9)
        for j,line in enumerate(lines):
            fs=110 if j==0 else 61
            f=ImageFont.truetype(str(font),fs)
            while draw.textlength(line,font=f)>900:
                fs-=2;f=ImageFont.truetype(str(font),fs)
            draw.text((x+512,y+170+j*155),line,font=f,fill=(219,211,175),anchor='mm')
    im.save(TEXTURES/'PrintAtlas_base_color.png')
    Image.new('L',(size,size),220).save(TEXTURES/'PrintAtlas_roughness.png')
    Image.new('RGB',(size,size),(128,128,255)).save(TEXTURES/'PrintAtlas_normal.png')
    (R/'materials.json').write_text(json.dumps(material_rows,indent=2)+'\n')
    provenance={'originality':'All geometry and labels authored for SIGNAL47. Six procedural PBR texture families are copied byte-for-byte from our own townbuildings source; no purchased or third-party artwork. Font rasterized using locally available open font, no font file distributed.',
                'texture_families':sorted(families),'shared_source':'../townbuildings/textures',
                'files':[{'path':str(p.relative_to(R)),'sha256':hashlib.sha256(p.read_bytes()).hexdigest()} for p in sorted(TEXTURES.glob('*.png'))]}
    (R/'materials_provenance.json').write_text(json.dumps(provenance,indent=2)+'\n')
    print('FIELDSTATION_MATERIAL_SOURCE_READY',len(material_rows),len(provenance['files']))

if __name__=='__main__':create()
