"""Original 2k painted PBR plates: timber fibres/knots, lime loss and rust.

Shapes describe material structure rather than a sine/noise substitute for wear.
No downloaded or concept-image pixels are embedded. Pillow is authoring-only.
"""
import math, random, json, subprocess
from pathlib import Path

STONE_SETTINGS = {
    'stucco': {'uv_scale': .42, 'normal_strength': .12, 'resolution': 2048},
    'concrete': {'uv_scale': .65, 'normal_strength': .20, 'resolution': 2048},
}
SURFACE_SETTINGS = {**STONE_SETTINGS,
    'walnut': {'uv_scale': 1, 'normal_strength': .28, 'resolution': 2048},
    'desk_walnut': {'uv_scale': 1, 'normal_strength': .25, 'resolution': 2048},
    'weathered_timber': {'uv_scale': 1, 'normal_strength': .35, 'resolution': 2048},
    'rust_metal': {'uv_scale': .7, 'normal_strength': .25, 'resolution': 2048},
}

def generate(folder, only=None):
    try:
        from PIL import Image, ImageDraw, ImageFilter, ImageChops
    except ImportError:
        # The project's Blender Python intentionally has no Pillow. Use the
        # existing system authoring dependency; never install into Blender.
        subprocess.run(['python3', str(Path(__file__).resolve())], check=True)
        return
    folder = Path(folder); folder.mkdir(parents=True, exist_ok=True)
    evidence = {'author': 'SIGNAL / 47 original raster painting', 'resolution': 2048,
                'method': 'drawn fibre paths, growth knots, flaked lime, capillary cracks, oxide blooms; no sine field', 'surfaces': {}}
    for index, name in enumerate(SURFACE_SETTINGS):
        if only is not None and name not in only: continue
        rng = random.Random(194700 + index); n = 2048
        bases = {'stucco': (184,173,148), 'concrete': (125,117,99),
                 'walnut': (76,46,28), 'desk_walnut': (99,63,36),
                 'weathered_timber': (115,103,83), 'rust_metal': (109,95,72)}
        base = bases[name]; col = Image.new('RGB', (n,n), base)
        height = Image.new('L', (n,n),128); rough = Image.new('L',(n,n),220)
        cd, hd, rd = ImageDraw.Draw(col), ImageDraw.Draw(height), ImageDraw.Draw(rough)
        def pigment(delta): return tuple(max(0,min(255,c+delta)) for c in base)
        if name in ('walnut','desk_walnut','weathered_timber'):
            # Long crooked longitudinal vessels, with a different latewood width.
            for i in range(1250):
                x = rng.uniform(-100,n+100); points=[]; drift=rng.uniform(-.4,.4)
                for y in range(-80,n+81,32):
                    drift = .88*drift + rng.uniform(-.9,.9); x += drift*6
                    points.append((round(x),y))
                w = rng.choices([1,2,3,6,10],[3,6,5,2,1])[0]
                cd.line(points, fill=pigment(rng.randint(-24,15)), width=w)
                hd.line(points, fill=rng.randint(108,140), width=w)
                rd.line(points, fill=rng.randint(160,235), width=w)
            # Flattened ellipses and wandering rings are actual growth knots.
            for i in range(13):
                x,y = rng.randrange(n),rng.randrange(n); rw=rng.randint(12,38); rh=rw*rng.uniform(2,3.8)
                for k in range(12,0,-1):
                    a=rw*k/10; b=rh*k/10
                    cd.ellipse((x-a,y-b,x+a,y+b),outline=pigment(-10-k%3*6),width=2)
                    hd.ellipse((x-a,y-b,x+a,y+b),outline=113+k%3*3,width=2)
                cd.ellipse((x-rw*.18,y-rh*.3,x+rw*.18,y+rh*.3),fill=pigment(-35))
            # Local abrasion follows traffic/grain instead of washing everything out.
            for i in range(65 if name=='weathered_timber' else 24):
                x,y=rng.randrange(n),rng.randrange(n); w=rng.randint(3,15); h=rng.randint(60,450)
                cd.line([(x,y),(x+w*.4,y+h*.45),(x+w*.15,y+h)],fill=pigment(rng.randint(16,34)),width=w)
                rd.line([(x,y),(x,y+h)],fill=235,width=w)
            rough = rough.filter(ImageFilter.GaussianBlur(.45))
        else:
            # Uneven trowel strokes, chipped lime, mineral cavities and larger oxide islands.
            for i in range(6200):
                x,y=rng.randint(-30,n),rng.randint(-30,n); r=rng.choices([2,4,8,16] if name=='stucco' else [2,5,12,34],[6,7,3,1])[0]
                points=[(x+math.cos(k*math.tau/7)*r*rng.uniform(.6,1.4),
                         y+math.sin(k*math.tau/7)*r*rng.uniform(.5,1.2)) for k in range(7)]
                d=rng.randint(-9,7) if name=='stucco' else rng.randint(-21,17)
                cd.polygon(points,fill=pigment(d)); hd.polygon(points,fill=128+rng.randint(-4,4) if name=='stucco' else 128+rng.randint(-8,8))
                rd.polygon(points,fill=rng.randint(208,242))
            if name=='rust_metal':
                for i in range(350):
                    x,y=rng.randrange(n),rng.randrange(n); r=rng.randint(8,90)
                    points=[(x+math.cos(k*math.tau/11)*r*rng.uniform(.55,1.3),y+math.sin(k*math.tau/11)*r*rng.uniform(.4,1)) for k in range(11)]
                    cd.polygon(points,fill=rng.choice([(105,54,25),(133,75,32),(74,51,35)])); hd.polygon(points,fill=137); rd.polygon(points,fill=236)
            else:
                for i in range(26):
                    x,y=rng.randrange(n),rng.randrange(n); points=[(x,y)]
                    for j in range(rng.randint(5,18)):
                        x+=rng.uniform(-23,23); y+=rng.uniform(8,30); points.append((x,y))
                    cd.line(points,fill=pigment(-38),width=1); hd.line(points,fill=108,width=2)
        # Small grit/pinholes sit on top of the structural paint, never replace it.
        for i in range(18000):
            x,y=rng.randrange(n),rng.randrange(n); r=rng.choice([0,0,1,2])
            cd.ellipse((x-r,y-r,x+r,y+r),fill=pigment(rng.randint(-15,14)))
            hd.point((x,y),fill=rng.randint(112,145))
        height=height.filter(ImageFilter.GaussianBlur(.45))
        # Central differences; small tangent slopes keep the texture from embossing.
        nx=ImageChops.subtract(ImageChops.offset(height,-1,0),ImageChops.offset(height,1,0),scale=1.5,offset=128)
        ny=ImageChops.subtract(ImageChops.offset(height,0,-1),ImageChops.offset(height,0,1),scale=1.5,offset=128)
        gl=Image.merge('RGB',(nx,ny,Image.new('L',(n,n),254)))
        dx=Image.merge('RGB',(nx,ImageChops.invert(ny),Image.new('L',(n,n),254)))
        for suffix,img in [('color',col),('height',height),('roughness',rough.convert('RGB')),('normal',gl),('normal_gl',gl),('normal_dx',dx)]:
            img.save(folder/(name+'_'+suffix+'.png'),compress_level=7)
        evidence['surfaces'][name]={'size':[n,n], 'structural_marks':True,'directx_normal':'green complement of OpenGL',**SURFACE_SETTINGS[name]}
    (folder.parent/'texture_provenance.json').write_text(json.dumps(evidence,indent=2))

if __name__=='__main__': generate(Path(__file__).parent/'textures')
