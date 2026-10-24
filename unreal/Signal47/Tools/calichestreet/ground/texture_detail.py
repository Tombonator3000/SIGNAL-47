"""Original multi-frequency mineral/plant atlas authoring, revision 4.

NumPy and Pillow from the bundled workspace runtime. No downloaded pixels.
Albedo: up to 2048; roughness/normal: 1024. All height detail is visual only.
"""
import json
import math
from pathlib import Path
import random
import numpy as np
from PIL import Image, ImageDraw, ImageFilter

ROOT=Path(__file__).resolve().parent

def smooth_image(rng,size,cells):
    data=(rng.random((cells,cells))*255).astype(np.uint8)
    return np.asarray(Image.fromarray(data).resize((size,size),Image.Resampling.BICUBIC),dtype=np.float32)/255

def save_maps(name,color,height,rough,path):
    color.save(path/(name+'_color.png'))
    h=Image.fromarray(np.clip(height*255,0,255).astype(np.uint8)).resize((1024,1024),Image.Resampling.LANCZOS)
    hh=np.asarray(h,dtype=np.float32)/255
    dx=(np.roll(hh,-1,1)-np.roll(hh,1,1))*3.2
    dy=(np.roll(hh,-1,0)-np.roll(hh,1,0))*3.2
    normal=np.dstack((-dx,dy,np.ones_like(dx)));normal/=np.linalg.norm(normal,axis=2)[:,:,None]
    Image.fromarray(np.clip((normal*.5+.5)*255,0,255).astype(np.uint8)).save(path/(name+'_normal.png'))
    Image.fromarray(np.clip(rough*255,0,255).astype(np.uint8)).resize((1024,1024),Image.Resampling.LANCZOS).save(path/(name+'_roughness.png'))

def mineral(name,rgb,seed,path,size=2048):
    rng=np.random.default_rng(seed);rr=random.Random(seed)
    macro=smooth_image(rng,size,7);mid=smooth_image(rng,size,61);fine=rng.random((size,size),dtype=np.float32)
    tone=.76+.28*macro+.16*(mid-.5)+.22*(fine-.5)
    col=np.clip(np.asarray(rgb)[None,None,:]*tone[:,:,None],0,255).astype(np.uint8)
    image=Image.fromarray(col);height=Image.fromarray(np.clip((.21+.13*mid+.19*fine)*255,0,255).astype(np.uint8))
    draw=ImageDraw.Draw(image);hd=ImageDraw.Draw(height)
    count=26000 if name=='PeaGravel' else 14000 if name in ['OldAsphalt','TarRepair'] else 8000
    for _ in range(count):
        x=rr.randrange(size);y=rr.randrange(size)
        r=rr.uniform(1.2,8.5) if name=='PeaGravel' else rr.uniform(.6,2.8)
        if name=='Limestone':r*=2.0
        shade=rr.uniform(.67,1.42);tint=rr.choice([(1,.99,.94),(.92,.95,1),(1.03,.98,.89)])
        c=tuple(max(0,min(255,int(rgb[k]*shade*tint[k]))) for k in range(3))
        pts=[]
        for k in range(5):
            a=k*math.tau/5;rad=r*rr.uniform(.7,1.2)
            pts.append((x+math.cos(a)*rad,y+math.sin(a)*rad*.73))
        # Thin dark rims under angular mineral chips, rather than smooth noise.
        draw.polygon([(a+.7,b+1.1) for a,b in pts],fill=tuple(int(v*.62) for v in c))
        draw.polygon(pts,fill=c);hd.polygon(pts,fill=rr.randint(100,205))
    if name in ['OldAsphalt','TarRepair','WornConcrete','CalicheSoil']:
        # Branching fracture network, no large repair polygons or sine waves.
        crack=Image.new('L',(size,size),0);cd=ImageDraw.Draw(crack)
        for i in range(24 if name=='OldAsphalt' else 10):
            x=rr.uniform(0,size);y=rr.uniform(0,size);a=rr.random()*math.tau
            points=[(x,y)]
            for j in range(rr.randint(9,21)):
                a+=rr.uniform(-.55,.55);length=rr.uniform(8,37)
                x+=math.cos(a)*length;y+=math.sin(a)*length;points.append((x,y))
                if j in [4,9,14] and rr.random()<.65:
                    bx=x+rr.uniform(-90,90);by=y+rr.uniform(-90,90)
                    cd.line([(x,y),((x+bx)/2+rr.uniform(-14,14),(y+by)/2),(bx,by)],fill=rr.randint(90,160),width=1)
            cd.line(points,fill=rr.randint(140,220),width=rr.choice([1,2,3]))
        arr=np.asarray(image,dtype=np.float32);mask=np.asarray(crack.filter(ImageFilter.GaussianBlur(.6)),dtype=np.float32)/255
        arr*=1-mask[:,:,None]*.69
        image=Image.fromarray(np.clip(arr,0,255).astype(np.uint8))
        hh=np.asarray(height,dtype=np.float32)/255-mask*.16
    else:hh=np.asarray(height,dtype=np.float32)/255
    rough=np.clip(.81+.10*mid+.06*(fine-.5),0,1)
    save_maps(name,image,hh,rough,path)

def blend_patch(name,path,seed,shoulder=False):
    base=Image.open(path/'PeaGravel_color.png').convert('RGB').resize((1024,1024))
    rng=np.random.default_rng(seed);size=1024
    noise=smooth_image(rng,size,31);macro=smooth_image(rng,size,6)
    y,x=np.mgrid[0:size,0:size]/(size-1)
    if shoulder:
        # ALL four edges die to zero. The previous long-edge-only feather
        # left bright straight patch ends in the real shallow street camera.
        alpha=(np.sin(np.pi*x)**1.45)*(np.sin(np.pi*y)**1.3)*np.clip((noise-.24)*2,0,1)*.67
    else:
        r=np.sqrt(((x-.5)*2.05)**2+((y-.5)*2.05)**2)
        alpha=np.clip((1-r)*3,0,1)*np.clip((noise-.18)*1.8,0,1)*(.25+.45*macro)
    rgba=np.dstack((np.asarray(base),np.clip(alpha*255,0,255).astype(np.uint8)))
    image=Image.fromarray(rgba,'RGBA')
    save_maps(name,image,noise*.24,np.full((size,size),.94,dtype=np.float32),path)

def plant_atlas(name,path,sage=False):
    rr=random.Random(4801 if sage else 4800);size=1024
    image=Image.new('RGBA',(size,size),(55,52,39,0));d=ImageDraw.Draw(image)
    for tile in range(4):
        ox=(tile%2)*512;oy=(tile//2)*512;root=(ox+256,oy+499)
        # Broader, leafy sage rather than isolated wire stems at street scale.
        for stem in range(72 if sage else 66):
            endx=ox+rr.uniform(38,474);endy=oy+rr.uniform(24,330 if sage else 260)
            bend=rr.uniform(-75,75);pts=[]
            for k in range(13):
                t=k/12;pts.append((root[0]+(endx-root[0])*t+bend*math.sin(math.pi*t),root[1]+(endy-root[1])*t))
            c=rr.choice([(111,103,73,255),(135,125,86,255),(94,99,75,255),(153,139,99,255),(122,117,91,255)])
            d.line(pts,fill=c,width=rr.choice([2,3,4]) if sage else rr.choice([1,2,3]))
            if sage:
                for p in pts[3::2]:
                    for sign in [-1,1]:
                        ex=p[0]+sign*rr.uniform(5,18);ey=p[1]-rr.uniform(3,15);r=rr.uniform(4.5,9)
                        leaf=rr.choice([(93,105,84,255),(118,128,101,255),(139,141,113,255),(103,115,92,255)])
                        d.ellipse((ex-r,ey-r*.45,ex+r,ey+r*.45),fill=leaf)
            else:
                for p in pts[-5::2]:
                    d.line([p,(p[0]+rr.uniform(-9,9),p[1]-rr.uniform(6,18))],fill=c,width=1)
    image.save(path/(name+'_color.png'))
    Image.new('RGB',(1024,1024),(128,128,255)).save(path/(name+'_normal.png'))
    Image.new('L',(1024,1024),235).save(path/(name+'_roughness.png'))

def generate(root=ROOT):
    path=root/'textures';path.mkdir(exist_ok=True)
    # Native R3 revealed that the neutral grey soil reads like a separate
    # concrete plate. Warm dusty tan stays closer to the existing desert;
    # stronger gravel remains confined to small shoulder/plant patches.
    specs=[('CalicheSoil',(126,111,89),.93,.22),('PeaGravel',(130,119,99),.96,.32),('OldAsphalt',(67,68,65),.87,.32),('DrySilt',(131,119,99),.98,.22),('Limestone',(144,135,115),.91,.34),('DryPlant',(128,119,88),.91,.15),('TarRepair',(59,60,57),.85,.24),('WornConcrete',(140,136,124),.95,.25)]
    for seed,(name,color,_,_) in enumerate(specs):mineral(name,color,4900+seed,path,2048 if name in ['CalicheSoil','PeaGravel','OldAsphalt','WornConcrete'] else 1024)
    blend_patch('ShoulderBlend',path,5101,True);blend_patch('DustFeather',path,5102)
    plant_atlas('GrassAtlas',path);plant_atlas('SageAtlas',path,True)
    mineral('Yucca',(101,111,82),5120,path,1024)
    specs += [('ShoulderBlend',(127,123,111),.96,.30),('DustFeather',(127,123,111),.96,.18),('GrassAtlas',(128,119,88),.94,.12),('SageAtlas',(110,120,95),.94,.12),('Yucca',(101,111,82),.94,.18)]
    rows=[]
    for name,_,rough,strength in specs:
        blend=name in ['ShoulderBlend','DustFeather'];plant=name in ['GrassAtlas','SageAtlas'];edge=name in ['CalicheSoil','OldAsphalt']
        rows.append({'name':'M_CS_'+name,'base_color_linear':[1,1,1,1],'metallic':0,'roughness':rough,'emission':[0,0,0,1],'emission_strength':0,'alpha':1,'two_sided':plant or name in ['DryPlant','Yucca'],'normal_strength':strength,'uv_scale':1.0,'opacity_from_base_color':blend or plant,'vertex_alpha_opacity':edge,'alpha_mode':'dithered_masked' if edge else 'translucent' if blend else 'masked' if plant else 'opaque','normal_convention':'DirectX','texture_color_space':'sRGB','textures':{k:'textures/'+name+'_'+s+'.png' for k,s in [('base_color','color'),('normal','normal'),('roughness','roughness')]}})
        if blend:
            # Native R4: translucent ground can retain a glass-like specular
            # response beyond its atlas mask. Use deferred dithered coverage.
            rows[-1].update(alpha_mode='dithered_masked',opacity_from_base_color=True,vertex_alpha_opacity=False,specular=0.0)
    (root/'materials.json').write_text(json.dumps(rows,indent=2)+'\n')
    print('DETAIL_TEXTURES_GENERATED',len(rows),'materials',len(rows)*3,'maps')

if __name__=='__main__':generate()
