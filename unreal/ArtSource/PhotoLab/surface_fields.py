"""Original, tileable PhotoLab surface fields. Heights are metres per 1m UV tile.

No Blender import or file output. A CPU caller can inspect the generated arrays.
"""
import numpy as np


def surface_channels(kind, u, v, noise, roughness):
    """Return albedo multiplier, roughness and metric relief for one real material.

    Wear lives mainly in the specular response. It must not turn a maintained
    1986 laboratory into mottled concrete, rust or heavily embossed plastic.
    """
    tau = np.pi * 2
    # Low frequency handling marks and cleaning variation, with no baked light.
    broad = np.sin(tau*(u*3+np.sin(tau*v)*.12))*np.cos(tau*v*5)
    wipe = np.sin(tau*(u*11+v*2))*.55 + np.cos(tau*(u*7-v*3))*.45
    fine = (noise + np.roll(noise,1,0) + np.roll(noise,-1,0) +
            np.roll(noise,1,1) + np.roll(noise,-1,1)) / 5
    if kind == 'metal':
        brush = (np.sin(tau*u*233)*.65 + np.sin(tau*u*367)*.35)
        color = 1 + brush*.012 + broad*.008
        rough = roughness + brush*.026 + wipe*.035 + broad*.014
        height = brush*.000004 + fine*.000001
    elif kind == 'paper':
        fibers = np.sin(tau*(u*197+v*13))*.4 + fine*.6
        color = 1 + fibers*.022 + broad*.016
        rough = roughness + fibers*.021 + broad*.012
        height = fibers*.000006
    elif kind == 'rubber':
        grain = np.sin(tau*u*149)*np.sin(tau*v*137)*.5 + fine*.5
        color = 1 + grain*.018
        rough = roughness + grain*.025 + wipe*.012
        height = grain*.000012
    elif kind in ('bakelite', 'plastic'):
        polish = np.maximum(wipe-.35,0)
        color = 1 + fine*.008 + broad*.009
        rough = roughness + fine*.016 + broad*.018 - polish*.055
        height = fine*.000002 + np.sin(tau*u*89)*np.sin(tau*v*83)*.000001
    else:
        # Shallow enamel orange peel, distinct from brushed steel or fibres.
        peel = np.sin(tau*u*113)*np.sin(tau*v*107)*.6 + fine*.4
        color = 1 + broad*.016 + fine*.007
        rough = roughness + peel*.018 + wipe*.026 + broad*.012
        height = peel*.000005
    return np.clip(color,.88,1.10), np.clip(rough,.12,.98), height


def normal_map(height):
    """Encoded DirectX tangent normals; periodic central differences at 1m/tile."""
    dy = (np.roll(height,-1,axis=0)-np.roll(height,1,axis=0))*(height.shape[0]*.5)
    dx = (np.roll(height,-1,axis=1)-np.roll(height,1,axis=1))*(height.shape[1]*.5)
    normal = np.stack((-dx,dy,np.ones_like(dx)),axis=2)
    normal /= np.linalg.norm(normal,axis=2,keepdims=True)
    rgba = np.ones((*height.shape,4))
    rgba[:,:,:3] = normal*.5+.5
    return rgba
