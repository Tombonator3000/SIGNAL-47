"""Original dry mineral PBR; deterministic, no photo/reference pixels or baked light."""
from pathlib import Path
import json
import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parent
OUT = ROOT / 'textures'


def main():
    OUT.mkdir(exist_ok=True)
    size = 2048
    rng = np.random.default_rng(1947047)

    def noise(cells):
        small = rng.random((cells+1, cells+1)).astype(np.float32)
        small[-1, :] = small[0, :]
        small[:, -1] = small[:, 0]
        x = np.arange(size, dtype=np.float32)*cells/size
        ix = x.astype(int)
        f = x-ix
        f = f*f*(3-2*f)
        return ((small[ix[:,None],ix[None,:]]*(1-f)[None,:]+small[ix[:,None],ix[None,:]+1]*f[None,:])*(1-f)[:,None]
                +(small[ix[:,None]+1,ix[None,:]]*(1-f)[None,:]+small[ix[:,None]+1,ix[None,:]+1]*f[None,:])*f[:,None])

    grain = noise(512)
    aggregate = noise(183)
    fine = noise(1001)
    patches = noise(11)*.6+noise(39)*.4
    # In a 6m physical tile most grains are millimetres, larger chips centimetres.
    chips = np.maximum(0, (aggregate-.64)/.36)**1.5
    pits = np.maximum(0, (.27-grain)/.27)
    variation = (grain-.5)*.11+(fine-.5)*.05+(patches-.5)*.045+chips*.14-pits*.075
    base = np.array([.48,.44,.38],np.float32)[None,None,:]+variation[:,:,None]
    base += chips[:,:,None]*np.array([.035,.04,.055],np.float32)[None,None,:]
    base = np.clip(base, .18, .8)
    height = grain*.0015+chips*.007+pits*-.002
    dx = (np.roll(height,-1,1)-np.roll(height,1,1))/(12/size)
    dy = (np.roll(height,-1,0)-np.roll(height,1,0))/(12/size)
    normals = np.stack((-dx,dy,np.ones_like(height)),axis=2)
    normals /= np.linalg.norm(normals,axis=2)[:,:,None]
    Image.fromarray(np.uint8(np.clip(base*255,0,255))).save(OUT/'MineralSoil_color.png')
    Image.fromarray(np.uint8(np.clip((normals*.5+.5)*255,0,255))).save(OUT/'MineralSoil_normal_dx.png')
    Image.fromarray(np.uint8(np.clip((.86+grain*.11)*255,0,255))).save(OUT/'MineralSoil_roughness.png')
    rows=[]
    for name, tint in [('M_CN_MineralSoil',[1,1,1]),('M_CN_GravelShoulder',[1.04,1.025,1.0])]:
        rows.append({'name':name,'base_color_linear':[.25,.22,.18,1], 'roughness':.92,
                     'metallic':0,'specular':.18,'normal_strength':.38,'uv_scale':1,
                     'vertex_rgb_tint':True,'vertex_alpha_opacity':True,'alpha_mode':'dithered_masked',
                     'texture_tint_linear':tint,
                     'textures':{'base_color':'textures/MineralSoil_color.png',
                                 'normal':'textures/MineralSoil_normal_dx.png',
                                 'roughness':'textures/MineralSoil_roughness.png'}})
    (ROOT/'materials.json').write_text(json.dumps(rows,indent=2)+'\n')
    (ROOT/'texture_provenance.json').write_text(json.dumps({'author':'Original deterministic mineral synthesis',
        'seed':1947047,'physical_tile_metres':6,'normal_convention':'DirectX tangent space',
        'reference_pixels_used':False,'baked_illumination':False,'size':[size,size],
        'roles':{'base_color':'sRGB','normal':'linear','roughness':'linear'}},indent=2)+'\n')
    print('Nature mineral PBR prepared',size)


if __name__ == '__main__':
    main()
