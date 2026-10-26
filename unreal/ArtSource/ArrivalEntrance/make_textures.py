"""Original enamel, brushed metal and sign artwork. No external source images."""
from pathlib import Path
import json, math, random
from PIL import Image, ImageDraw, ImageFont

HERE=Path(__file__).resolve().parent
TEX=HERE/'textures'; TEX.mkdir(parents=True,exist_ok=True)
RNG=random.Random(471026)
ROWS=[]
FONT='/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf'

def material(name,color,rough,metal):
    n=256; colors=[]; roughness=[]; normals=[]
    for y in range(n):
        for x in range(n):
            noise=RNG.gauss(0,1);grain=noise*.012+math.sin(x*.24+y*.013)*.006
            rgb=[max(0,min(1,c*(1+grain))) for c in color]
            colors.append(tuple(round((c*12.92 if c<=.0031308 else 1.055*c**(1/2.4)-.055)*255) for c in rgb))
            r=round(max(0,min(1,rough+grain*2))*255);roughness.append((r,r,r))
            normals.append((round(max(0,min(1,.5-noise*.006))*255),round(max(0,min(1,.5+RNG.gauss(0,1)*.006))*255),255))
    for suffix,data in [('_color',colors),('_rough',roughness),('_normal',normals)]:
        im=Image.new('RGB',(n,n));im.putdata(data);im.save(TEX/(name+suffix+'.png'))
    ROWS.append({'name':name,'base_color_linear':[1,1,1,1],'roughness':rough,'metallic':metal,'specular':.25,'normal_strength':.18,
                 'textures':{'base_color':f'textures/{name}_color.png','roughness':f'textures/{name}_rough.png','normal':f'textures/{name}_normal.png'}})

def sign(name,size,lines):
    w,h=size
    # Sun-warmed cream enamel, sparse edge wear and slight lower-edge dust.
    data=[]
    for y in range(h):
        for x in range(w):
            noise=RNG.gauss(0,1);dust=(y/h)**6*8
            data.append(tuple(round(max(0,min(255,c+noise*1.2-dust))) for c in (219,216,195)))
    image=Image.new('RGB',(w,h));image.putdata(data);d=ImageDraw.Draw(image)
    ink=(43,62,53);border=max(3,int(min(w,h)*.015));pad=int(min(w,h)*.045)
    d.rounded_rectangle((pad,pad,w-pad-1,h-pad-1),radius=int(w*.03),outline=ink,width=border)
    for text,y,sizefont in lines:
        f=ImageFont.truetype(FONT,sizefont);d.text((w/2,h*y),text,fill=ink,font=f,anchor='mm',align='center')
    for i in range(60):
        x=int(RNG.randrange(pad,w-pad));y=int(RNG.choice([pad,h-pad-1]))+int(RNG.randrange(-2,3))
        d.line((x,y,x+int(RNG.randrange(1,6)),y),fill=(143,143,127),width=1)
    image.save(TEX/(name+'.png'))
    ROWS.append({'name':name,'base_color_linear':[1,1,1,1],'roughness':.56,'metallic':.12,'specular':.23,'textures':{'base_color':f'textures/{name}.png'}})

material('M_Arrival_PaintedMetal',(.10,.13,.12),.62,.45)
material('M_Arrival_Aluminium',(.28,.30,.29),.43,.78)
material('M_Arrival_Rubber',(.018,.021,.019),.9,0)
material('M_Arrival_Enamel',(.63,.62,.53),.55,.12)
material('M_Arrival_WeatheredSteel',(.16,.18,.17),.67,.7)
sign('M_Arrival_VisitorPrint',(768,1000),[('P',.29,260),('VISITOR',.57,91),('PARKING',.72,85)])
sign('M_Arrival_EntrancePrint',(1536,256),[('ENTRANCE',.5,157)])
sign('M_Arrival_ReceptionPrint',(1024,560),[('SARO',.32,140),('RECEPTION',.67,105)])
(HERE/'materials.json').write_text(json.dumps(ROWS,indent=2)+'\n')
print('Original arrival PBR artwork:',len(ROWS),'materials')
