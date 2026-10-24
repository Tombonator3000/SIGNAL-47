"""Original 2048px mineral PBR authoring. No downloaded images or external data.

Run only in the granted heavy slot with NumPy/Pillow workspace runtime.
"""
from pathlib import Path
import json
import numpy as np
from PIL import Image,ImageFilter
R=Path(__file__).resolve().parent
SIZE=2048

def noise(rng,n):
    # Periodic value noise: shared PBR maps repeat without macro tile seams.
    grid=rng.random((n,n),dtype=np.float32);grid=np.pad(grid,((0,1),(0,1)),mode='wrap')
    t=np.arange(SIZE,dtype=np.float32)*n/SIZE;i=t.astype(np.int32);f=t-i;f=f*f*(3-2*f)
    return ((grid[i[:,None],i[None,:]]*(1-f)[None,:]+grid[i[:,None],i[None,:]+1]*f[None,:])*(1-f)[:,None]+
            (grid[i[:,None]+1,i[None,:]]*(1-f)[None,:]+grid[i[:,None]+1,i[None,:]+1]*f[None,:])*f[:,None])
def stripe(rng,n,vertical=True):
    data=rng.random(n+1).astype(np.float32);data[-1]=data[0]
    t=np.arange(SIZE,dtype=np.float32)*n/SIZE;i=t.astype(int);f=t-i;f=f*f*(3-2*f)
    values=data[i]*(1-f)+data[i+1]*f
    return values[:,None] if vertical else values[None,:]

def write_maps(label,color,height,rough):
    out=R/'textures';out.mkdir(exist_ok=True)
    Image.fromarray(np.clip(color,0,255).astype(np.uint8),'RGB').save(out/(label+'_base_color.png'))
    small=np.asarray(Image.fromarray(height.astype(np.float32),'F').resize((1024,1024),Image.Resampling.BILINEAR))
    dx=np.gradient(small,axis=1)*8.0;dy=np.gradient(small,axis=0)*8.0
    normal=np.stack([-dx,dy,np.ones_like(dx)],axis=2);normal/=np.linalg.norm(normal,axis=2)[:,:,None]
    Image.fromarray(np.clip((normal*.5+.5)*255,0,255).astype(np.uint8),'RGB').save(out/(label+'_normal.png'))
    Image.fromarray(np.clip(rough*255,0,255).astype(np.uint8),'L').resize((1024,1024),Image.Resampling.LANCZOS).save(out/(label+'_roughness.png'))

def generate():
    rng=np.random.default_rng(198624)
    macro=noise(rng,9);medium=noise(rng,56);fine=rng.random((SIZE,SIZE),dtype=np.float32)
    # Wide, irregular mineral beds and narrow angular joint stains. Geometry
    # owns the five macro ledges, while these2–5m beds remain legible at distance.
    beds=stripe(rng,17)
    joints=noise(rng,22);bands=stripe(rng,43,False)
    fissure=np.clip((.19-bands)*5.5,0,.70)*(.30+.70*joints)
    bedding=np.clip((.38-beds)*1.7,0,.46)
    tone=.79+.16*macro+.12*(medium-.5)+.045*(fine-.5)-bedding*.28-fissure*.23
    color=np.array([142,122,111])[None,None,:]*tone[:,:,None]
    color[:,:,0]+=(macro-.5)*8;color[:,:,2]+=(medium-.5)*5
    height=medium*.18+fine*.085+beds*.12-fissure*.32-bedding*.18
    write_maps('MesaRock',color,height,.84+.14*medium)
    # Cap is a separate original mineral image, not the same pale map across
    # every vertical face. Six stable native material names remain unchanged.
    capcolor=np.array([144,123,111])[None,None,:]*(.81+.16*macro+.10*(medium-.5))[:,:,None]
    write_maps('MesaCap',capcolor,height*.62,.88+.10*medium)
    rng=np.random.default_rng(198625)
    broad=noise(rng,11);aggregate=noise(rng,150);fine=rng.random((SIZE,SIZE),dtype=np.float32)
    pebble=np.clip((aggregate-.57)*3.0,0,.9)
    tone=.83+.18*broad+.22*(aggregate-.5)+.08*(fine-.5)
    color=np.array([126,113,104])[None,None,:]*tone[:,:,None]+pebble[:,:,None]*20
    height=aggregate*.25+pebble*.41+fine*.08
    write_maps('MesaTalus',color,height,.88+.10*aggregate)
    rows=[]
    # Stable names/slots; original images and light physical tints only.
    roles=[('talus_fines','MesaTalus',[1,1,1],.42),
           ('sandstone_beds','MesaRock',[1,.99,.98],.66),
           ('resistant_pale_cap','MesaCap',[1,1,1],.40),
           ('thin_recessive_beds','MesaRock',[.86,.90,.94],.54),
           ('eroded_talus','MesaTalus',[.92,.94,.96],.62),
           ('distant_ridges','MesaRock',[.84,.91,1],.18)]
    for i,(role,label,tint,strength) in enumerate(roles):
        rows.append({'name':'M_Caliche_Mesa_'+str(i),'role':role,'base_color_linear':[1,1,1,1],
          'texture_tint_linear':tint,'metallic':0,'roughness':.93,'specular':.18,
          'normal_strength':strength,'uv_scale':1,'normal_convention':'DirectX','texture_color_space':'sRGB',
          'textures':{role:'textures/'+label+'_'+role+'.png' for role in ['base_color','normal','roughness']}})
    (R/'materials.json').write_text(json.dumps(rows,indent=2)+'\n')
    print('ORIGINAL_GEOLOGY_PBR_READY',len(rows),'stable materials,3original texture sets')
if __name__=='__main__':generate()
