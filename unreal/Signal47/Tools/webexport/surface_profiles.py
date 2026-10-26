"""Opt-in surface detail for the Unreal import, independent of material export IDs.

These are authored micro-surfaces, not replacements for the source art.  The
packed tile is deterministic and uses only Python's standard library.  Source
PBR maps, characters, paper, flora and emissive surfaces keep their own shading.
"""
import math
import struct
import zlib


# Centimetres in Unreal; normal strength is the slope of the small surface.
# Detail roughness is an additive range around the exported roughness.  Macro
# colour is a neutral multiplier, so the original painted palette survives.
PROFILES = {
    'paint': dict(DetailWorldSize=45., DetailNormalStrength=.10, RoughnessVariation=.065,
                  MacroWorldSize=1200., MacroColorVariation=.018, MacroRoughnessVariation=.025, VertexDirtRoughness=.08),
    'plaster': dict(DetailWorldSize=65., DetailNormalStrength=.18, RoughnessVariation=.10,
                    MacroWorldSize=1600., MacroColorVariation=.035, MacroRoughnessVariation=.035, VertexDirtRoughness=.10),
    'concrete': dict(DetailWorldSize=65., DetailNormalStrength=.20, RoughnessVariation=.12,
                     MacroWorldSize=2000., MacroColorVariation=.045, MacroRoughnessVariation=.05, VertexDirtRoughness=.12),
    'ground': dict(DetailWorldSize=95., DetailNormalStrength=.30, RoughnessVariation=.13,
                   MacroWorldSize=3600., MacroColorVariation=.18, MacroRoughnessVariation=.07, VertexDirtRoughness=.12),
    'asphalt': dict(DetailWorldSize=80., DetailNormalStrength=.16, RoughnessVariation=.12,
                    MacroWorldSize=2800., MacroColorVariation=.08, MacroRoughnessVariation=.06, VertexDirtRoughness=.12),
    'road': dict(DetailWorldSize=80., DetailNormalStrength=.12, RoughnessVariation=.10,
                 MacroWorldSize=3600., MacroColorVariation=.045, MacroRoughnessVariation=.045,
                 VertexDirtRoughness=.10, DampSheen=.55),
    'floor': dict(DetailWorldSize=40., DetailNormalStrength=.055, RoughnessVariation=.115,
                  MacroWorldSize=1600., MacroColorVariation=.015, MacroRoughnessVariation=.045, VertexDirtRoughness=.16),
    'wood': dict(DetailWorldSize=55., DetailNormalStrength=.10, RoughnessVariation=.10,
                 MacroWorldSize=1500., MacroColorVariation=.025, MacroRoughnessVariation=.035, VertexDirtRoughness=.08),
    'metal': dict(DetailWorldSize=32., DetailNormalStrength=.035, RoughnessVariation=.09,
                  MacroWorldSize=1200., MacroColorVariation=.01, MacroRoughnessVariation=.025, VertexDirtRoughness=.10),
    'vinyl': dict(DetailWorldSize=28., DetailNormalStrength=.07, RoughnessVariation=.075,
                  MacroWorldSize=1200., MacroColorVariation=.01, MacroRoughnessVariation=.025, VertexDirtRoughness=.10),
}

ART_PROFILES = {
    'art/desert': 'ground', 'art/concrete': 'concrete', 'art/oldConcrete': 'concrete',
    'art/wall': 'paint', 'art/motelWall': 'plaster', 'art/stucco': 'plaster',
    'art/ceiling': 'paint', 'art/floor': 'floor', 'art/vinyl': 'floor',
    'art/floorboards': 'wood', 'art/weatheredWood': 'wood', 'art/desk': 'wood',
    'art/cabinet': 'metal', 'art/asphalt': 'asphalt',
    'surface/highway': 'road',
    'art/dinerWall': 'paint', 'art/dinerCounter': 'floor',
    'art/dinerFloor': 'floor', 'art/dinerBooth': 'vinyl',
}

# Names exported by World.ts are stable when export traversal order changes.
SITE_PROFILES = {
    'stucco': 'plaster', 'concrete': 'concrete', 'wood': 'wood', 'woodDark': 'wood',
    'paint': 'paint', 'steel': 'metal', 'darkMetal': 'metal', 'corrugated': 'metal',
    'rust': 'metal', 'fadedPaint': 'paint', 'galv': 'metal', 'plaster': 'plaster',
    'dado': 'paint', 'boards': 'wood', 'ceiling': 'wood', 'woodIn': 'wood',
    'drawer': 'wood', 'crinkle': 'metal', 'cabinet': 'metal', 'doorIn': 'wood',
}
TOWN_PROFILES = {
    'cream': 'plaster', 'tan': 'plaster', 'pink': 'plaster', 'white': 'plaster',
    'block': 'concrete', 'tin': 'metal', 'roof': 'asphalt', 'trim': 'wood',
    'metal': 'metal', 'concrete': 'concrete', 'pave': 'concrete', 'wood': 'wood',
    'red': 'paint', 'paint': 'paint', 'brown': 'wood',
}
DINER_PROFILES = {
    'panel': 'paint', 'vinyl': 'vinyl', 'floor': 'floor', 'counter': 'floor',
    'gravel': 'ground', 'asphalt': 'asphalt', 'cream': 'paint', 'red': 'vinyl',
    'mint': 'paint', 'steel': 'metal', 'wood': 'wood',
}
TRUCK_PROFILES = {
    'paint': 'paint', 'dash': 'vinyl', 'vinyl': 'vinyl', 'doorCard': 'vinyl',
    'liner': 'vinyl', 'mat': 'vinyl', 'cabTrim': 'paint', 'cabChrome': 'metal',
}
CONTROL_CRT_HANDLES = frozenset('room.%s.%s' % (screen, kind)
                              for screen in ('crtLeft', 'crtCenter', 'crtRight', 'crtOps')
                              for kind in ('mat', 'tex'))


def material_labels(manifest):
    """Collect handles once; never rely on the current material-number layout."""
    labels = {}
    for label, handle in manifest.get('handles', {}).items():
        mid = handle.get('mat', -1) if isinstance(handle, dict) else -1
        if mid >= 0:
            labels.setdefault(mid, set()).add(label)
    return labels


def texture_labels(manifest):
    labels = {t['id']: t.get('name', '') for t in manifest['textures']}
    # roadTextures.highwayTex() is unnamed in the historical export.  Its unique
    # canvas format is 512 x 2048 with repeat in both directions.  This also finds
    # the same highway material used on the old road, without export IDs.
    for t in manifest['textures']:
        if (not t.get('name') and t.get('canvas') and t.get('w') == 512 and t.get('h') == 2048
                and t.get('wrapS') == 'repeat' and t.get('wrapT') == 'repeat'):
            labels[t['id']] = 'surface/highway'
    return labels


def profile_for(material, texture_names, labels):
    if (not material.get('lit') or material.get('flora') or material.get('shader')
            or material.get('additive') or material.get('transparent') or material.get('alphaTest', 0) > 0
            or 'emissiveMap' in material or max(material.get('emissive', [0, 0, 0])) > 0
            or any(key in material for key in ('normalMap', 'bumpMap', 'roughnessMap', 'metalnessMap', 'aoMap'))):
        return None
    for label in sorted(labels):
        table = None
        if label.startswith('world.site.m.'):
            table = SITE_PROFILES
        elif label.startswith('world.town.m.'):
            table = TOWN_PROFILES
        elif label.startswith('world.diner.m.'):
            table = DINER_PROFILES
        elif '.truck.m.' in label or label.startswith(('world.saroTruck.m.', 'world.oldTruck.m.')):
            table = TRUCK_PROFILES
        if table:
            key = table.get(label.rsplit('.', 1)[-1])
            if key:
                return key
    return ART_PROFILES.get(texture_names.get(material.get('map'), ''))


def is_control_crt(material, labels):
    """The four existing SARO picture planes, identified by exported handles.

    These keep their unlit image path as phosphor emission, but gain a separate
    dark dielectric surface.  Do not adopt other screens or Kelvora TV panes:
    their film shading and power contracts are owned by those systems.
    """
    return (not material.get('lit') and not material.get('shader')
            and not material.get('transparent') and not material.get('additive')
            and material.get('alphaTest', 0) <= 0 and bool(CONTROL_CRT_HANDLES.intersection(labels)))


def glass_parameters(material, labels):
    """Only existing lit glass: keep power-driven unlit panes and CRTs intact."""
    if not material.get('lit') or any(key in material for key in ('normalMap', 'roughnessMap', 'metalnessMap')):
        return None
    glass = material.get('name') in ('town/glass', 'diner/glass')
    glass |= any(label.endswith('.m.glass') and ('Truck.' in label or '.truck.' in label) for label in labels)
    if not glass:
        return None
    return dict(Metallic=0., Specular=.35, Roughness=max(.14, float(material.get('roughness', .14))))


def write_detail_tile(path, size=256):
    """RGBA = signed world-plane slopes, roughness noise, broad macro noise.

    Each lattice wraps exactly.  Sampling this at two unrelated world sizes
    gives fine pores and metre-scale breakup without following albedo UV tiles.
    Slopes are precomputed so ray-hit shaders need no pixel derivatives.
    """
    def lattice(x, y, cells, seed):
        value = ((x % cells) * 0x1f123bb5 ^ (y % cells) * 0x5f356495 ^ seed) & 0xffffffff
        value = ((value ^ (value >> 16)) * 0x7feb352d) & 0xffffffff
        value = ((value ^ (value >> 15)) * 0x846ca68b) & 0xffffffff
        return ((value ^ (value >> 16)) & 0xffff) / 65535.

    def noise(x, y, cells, seed):
        x, y = x * cells / size, y * cells / size
        ix, iy = math.floor(x), math.floor(y)
        fx, fy = x - ix, y - iy
        fx, fy = fx * fx * (3. - 2. * fx), fy * fy * (3. - 2. * fy)
        a = lattice(ix, iy, cells, seed) * (1. - fx) + lattice(ix + 1, iy, cells, seed) * fx
        b = lattice(ix, iy + 1, cells, seed) * (1. - fx) + lattice(ix + 1, iy + 1, cells, seed) * fx
        return a * (1. - fy) + b * fy

    heights = [sum(weight * noise(x, y, cells, seed) for cells, seed, weight in
                   ((32, 4701, .5), (64, 1986, .35), (128, 4703, .15)))
               for y in range(size) for x in range(size)]
    def h(x, y):
        return heights[(y % size) * size + (x % size)]
    def byte(value):
        return max(0, min(255, round(value * 255.)))
    rows = []
    for y in range(size):
        row = bytearray(b'\x00')
        for x in range(size):
            dx, dy = (h(x + 1, y) - h(x - 1, y)) * 2., (h(x, y + 1) - h(x, y - 1)) * 2.
            rough = noise(x, y, 16, 4733) * .55 + noise(x, y, 64, 4741) * .45
            macro = noise(x, y, 4, 4799) * .75 + noise(x, y, 8, 4721) * .25
            row.extend((byte(.5 + dx), byte(.5 + dy), byte(rough), byte(macro)))
        rows.append(bytes(row))
    def chunk(kind, data):
        return struct.pack('>I', len(data)) + kind + data + struct.pack('>I', zlib.crc32(kind + data) & 0xffffffff)
    with open(path, 'wb') as out:
        out.write(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', size, size, 8, 6, 0, 0, 0))
                  + chunk(b'IDAT', zlib.compress(b''.join(rows))) + chunk(b'IEND', b''))
