#!/usr/bin/env python3
"""Deterministiske datakart for Runde 9. Krever bare Pillow.

Eksempel (kjøres fra web):
  python3 tools/round9_maps.py --albedo src/assets/art/room/tex_floor_hextile.jpg \
    --height-guide /path/height.png --profile hextile \
    --output-prefix /tmp/hextile --qa-dir /tmp/hextile-qa

Albedo og guide leses, aldri endres. Guiden er supplerende mikrorelieff, ikke
geometrisk fasit. Fugene følger albedo-masken. Ingen luminans-til-høyde-kopi.
Rapportens tekniske PASS er ikke visuell aksept eller bevis på sømløs albedo.
"""
from __future__ import annotations

import argparse
from array import array
from collections import deque
import hashlib
import json
import math
from pathlib import Path

from PIL import Image, ImageChops, ImageFilter, ImageOps, __version__ as PIL_VERSION

SIZE = 512
PROFILES = {
    'hextile':       dict(kind='hex', rough=(.45,.55), feature_rough=.90, depth=2.2, guide=.018),
    'wall_paint':    dict(kind='mortar', rough=(.70,.80), feature_rough=.90, depth=1.5, guide=.035),
    'concrete':      dict(kind='cracks', rough=(.85,.95), depth=.65, guide=.05),
    'desert':        dict(kind='granular', rough=(.95,1.), depth=.8, guide=.05),
    'asphalt':       dict(kind='asphalt', rough=(.75,.85), depth=.50, guide=.04),
    'vinyl':         dict(kind='grid4', rough=(.45,.60), depth=.75, guide=.015),
    'cabinet':       dict(kind='metal', rough=(.50,.65), feature_rough=.90, depth=.35, guide=.025),
    'desk':          dict(kind='flat', rough=(.35,.50), depth=.08, guide=.018),
    'ceiling':       dict(kind='porous', rough=(.95,.95), depth=.50, guide=.025),
    'stucco':        dict(kind='cracks', rough=(.90,.90), depth=.50, guide=.05),
    'concrete_old':  dict(kind='granular', rough=(.90,1.), depth=.8, guide=.05),
    'weathered_wood':dict(kind='wood_local_v', rough=(.85,.85), depth=1.1, guide=.03),
    'floorboards':   dict(kind='wood_h', rough=(.60,.75), depth=1.4, guide=.025, joint_count=10),
    'gravel':        dict(kind='tracks', rough=(.95,.95), depth=.6, guide=.035),
    'motel_wall':    dict(kind='cracks', rough=(.85,.85), depth=.35, guide=.04),
    'checker':       dict(kind='grid8', rough=(.35,.50), depth=.75, guide=.012),
    'wall_panel':    dict(kind='wood_v', rough=(.55,.65), depth=.9, guide=.025),
    'booth':         dict(kind='tuft', rough=(.30,.45), depth=1.7, guide=.055),
    'counter':       dict(kind='flat', rough=(.30,.40), depth=.045, guide=.012),
}


def clamp(v, lo=0., hi=1.):
    return max(lo, min(hi, v))


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def values(im):
    return list(im.get_flattened_data() if hasattr(im,'get_flattened_data') else im.getdata())


def gray(vals, size=(SIZE,SIZE)):
    out = Image.new('L', size)
    out.putdata([round(clamp(v)*255) for v in vals])
    return out


def wrapped_filter(im, filt):
    """Alle analysefiltre ser periodiske naboer, aldri svarte bildekanter."""
    w,h = im.size
    tiled = Image.new(im.mode, (3*w,3*h))
    for y in range(3):
        for x in range(3):
            tiled.paste(im,(x*w,y*h))
    return tiled.filter(filt).crop((w,h,2*w,2*h))


def blur(im, radius):
    return wrapped_filter(im, ImageFilter.GaussianBlur(radius))


def contrast_features(luma, radius=5, threshold=.024):
    """Smale mørke søkk: lukking fjerner små spor, bevarer brede pigmentfelt."""
    closed = wrapped_filter(luma, ImageFilter.MaxFilter(radius))
    closed = wrapped_filter(closed, ImageFilter.MinFilter(radius))
    diff = ImageChops.subtract(closed, luma)
    return diff.point(lambda p: round(255*clamp((p/255-threshold)/.12)))


def connected(mask, minimum=120):
    """Forkast isolerte malingsflekker; behold sammenhengende fugenett."""
    w,h = mask.size; data=values(mask); seen=bytearray(w*h); out=bytearray(w*h)
    for start,v in enumerate(data):
        if v<70 or seen[start]:
            continue
        seen[start]=1; q=deque([start]); component=[]
        while q:
            i=q.popleft(); component.append(i); x=i%w; y=i//w
            for j in (y*w+(x-1)%w,y*w+(x+1)%w,((y-1)%h)*w+x,((y+1)%h)*w+x):
                if not seen[j] and data[j]>=70:
                    seen[j]=1; q.append(j)
        if len(component)>=minimum:
            for i in component: out[i]=255
    return Image.frombytes('L',(w,h),bytes(out))


def axis_seams(luma, vertical, joint_count=None):
    """Lange bordfuger fra smale daler i albedoens faktiske rad-/kolonneprofil."""
    line=luma.resize((SIZE,1) if vertical else (1,SIZE),Image.Resampling.BOX)
    v=[p/255 for p in values(line)]
    ridge=[]
    for i,p in enumerate(v):
        neighbour=max(v[(i+d)%SIZE] for d in (-4,-3,-2,2,3,4))
        ridge.append(clamp((neighbour-p-.009)/.080))
    if joint_count:
        peaks=[];spacing=SIZE/joint_count*.58
        for i in sorted(range(SIZE),key=lambda j:ridge[j],reverse=True):
            if ridge[i]<.12:break
            if all(min(abs(i-j),SIZE-abs(i-j))>=spacing for j in peaks):peaks.append(i)
            if len(peaks)==joint_count:break
        # Smale kildedaler, ingen jevnt plasserte erstatningsbord.
        ridge=[max((ridge[j]*math.exp(-min(abs(i-j),SIZE-abs(i-j))**2/(2*.8**2)) for j in peaks),default=0) for i in range(SIZE)]
    a=gray(ridge,(SIZE,1) if vertical else (1,SIZE))
    return a.resize((SIZE,SIZE),Image.Resampling.NEAREST)


def distance_to_features(mask):
    """Avstand til kildesøm gir putehøyde uten fotografiske lysgradienter."""
    data=values(mask);distance=[80]*len(data);queue=deque()
    for i,v in enumerate(data):
        if v>=80:distance[i]=0;queue.append(i)
    while queue:
        i=queue.popleft();x=i%SIZE;y=i//SIZE;d=distance[i]+1
        if d>=80:continue
        for j in (y*SIZE+(x-1)%SIZE,y*SIZE+(x+1)%SIZE,((y-1)%SIZE)*SIZE+x,((y+1)%SIZE)*SIZE+x):
            if d<distance[j]:distance[j]=d;queue.append(j)
    return distance


def grid_seams(rgb, count):
    """Finn fasen fra fargekantene. Flisenes svarte/hvite pigment gir ikke høyde."""
    energy=[]
    for axis in ('x','y'):
        shifted=ImageChops.offset(rgb,1,0) if axis=='x' else ImageChops.offset(rgb,0,1)
        diff=ImageChops.difference(rgb,shifted).convert('L')
        line=diff.resize((SIZE,1) if axis=='x' else (1,SIZE),Image.Resampling.BOX)
        energy.append(values(line))
    period=SIZE/count
    phases=[]
    for e in energy:
        phases.append(max(range(round(period)),key=lambda p:sum(e[round(p+k*period)%SIZE] for k in range(count))))
    out=[]
    for y in range(SIZE):
        for x in range(SIZE):
            dx=abs((x-phases[0]+period/2)%period-period/2)
            dy=abs((y-phases[1]+period/2)%period-period/2)
            out.append(math.exp(-min(dx,dy)**2/(2*.75**2)))
    return gray(out),{'count_per_axis':count,'phase_pixels':phases,'period_pixels':period}


def feature_masks(rgb, profile):
    luma=rgb.convert('L'); kind=profile['kind']; notes={}
    dark=contrast_features(blur(luma,.6))
    aux=Image.new('L',(SIZE,SIZE))
    if kind=='hex':
        # Fargede fliser og nøytrale fuger skilles før komponentfilteret.
        mask=gray([clamp((110-(.299*r+.587*g+.114*b))/35)*clamp((55-(max(r,g,b)-min(r,g,b)))/25)
                   for r,g,b in values(rgb)])
        mask=connected(mask)
        notes['geometry']='dark neutral connected grout; orange/brown tile pigment excluded'
    elif kind in ('grid4','grid8'):
        mask,notes['grid']=grid_seams(rgb,4 if kind=='grid4' else 8)
        notes['geometry']='source-edge fitted regular joints, no height from tile colour'
    elif kind in ('wood_v','wood_h'):
        mask=axis_seams(luma,kind=='wood_v',profile.get('joint_count'))
        # Fiber er begrensede smale spor, ikke hele mørke bord eller kvister.
        aux=dark.point(lambda p:round(p*.20))
        notes['geometry']='narrow source row/column valleys for board joints; weak narrow grain'
    elif kind=='wood_local_v':
        # Oppsprukket, bølgende fiber følger lokale kildepiksler. Ingen dal
        # ekstrapoleres fra en kolonnes gjennomsnitt gjennom hele bildet.
        local=values(contrast_features(blur(luma,.65),7,.026))
        supported=[]
        for y in range(SIZE):
            for x in range(SIZE):
                vertical=sum(max(local[((y+d)%SIZE)*SIZE+(x+dx)%SIZE] for dx in (-1,0,1)) for d in (-2,-1,0,1,2))/(5*255)
                supported.append(local[y*SIZE+x]/255*clamp((vertical-.04)/.20))
        supported=gray(supported)
        mask=connected(supported.point(lambda v:255 if v>=150 else 0),minimum=80)
        aux=dark.point(lambda v:round(v*.10))
        notes['geometry']='connected local high-contrast fragmented cracks with five-row vertical support plus very weak local fibre; no full-column extrusion'
    elif kind=='tracks':
        columns=[p/255 for p in values(blur(luma,4).resize((SIZE,1),Image.Resampling.BOX))]
        # Bare de to observerte sporene. Et ekstra mørkt steinbånd nær x=.56
        # er ikke et tredje hjulspor. Finn sentrum/bredde lokalt i albedoen.
        filtered=[sum(columns[(i+d)%SIZE] for d in range(-6,7))/13 for i in range(SIZE)]
        ruts=[]
        for lo,hi in ((.20,.42),(.60,.80)):
            centre=min(range(round(lo*SIZE),round(hi*SIZE)),key=lambda x:filtered[x])
            shoulder=(filtered[centre-40]+filtered[centre+40])/2
            halfway=(shoulder+filtered[centre])/2
            left=centre;right=centre
            while left>round(lo*SIZE) and filtered[left]<halfway:left-=1
            while right<round(hi*SIZE) and filtered[right]<halfway:right+=1
            sigma=max(9.,min(24.,(right-left)/2.355))
            ruts.append((centre,sigma))
        smooth=[max(math.exp(-.5*((i-c)/s)**2) for c,s in ruts) for i in range(SIZE)]
        aux=gray(smooth,(SIZE,1)).resize((SIZE,SIZE))
        mask=dark
        notes['rut_centres_source_512']=[c for c,s in ruts]
        notes['rut_sigma_pixels']=[s for c,s in ruts]
        notes['geometry']='exactly two source-fitted main rut valleys near x=.3/.7 plus unchanged local stone relief; image height is track direction'
    elif kind=='metal':
        mask=dark.point(lambda p:round(p*.35))
        colors=values(rgb)
        aux=gray([1. if r-g>12 and g-b>3 else .5*clamp((r-g-7)/6)*clamp((g-b-1)/4) for r,g,b in colors])
        notes['rust_core_pixel_count']=sum(r-g>12 and g-b>3 for r,g,b in colors)
        notes['geometry']='weak narrow scratches and source-supported red-brown rust; no broad tonal dents'
    elif kind=='flat':
        mask=dark.point(lambda p:round(p*.1))
        notes['geometry']='almost flat; only weak narrow source scratches'
    elif kind=='tuft':
        ridge=contrast_features(blur(luma,.8),9,.018)
        binary=ridge.point(lambda v:255 if v>=70 else 0)
        binary=wrapped_filter(wrapped_filter(binary,ImageFilter.MaxFilter(3)),ImageFilter.MinFilter(3))
        # Sammenhengende sømnett, ikke lærpigment eller isolert fotografisk korn.
        binary=connected(binary,minimum=500)
        # Lukk lyse knappsentre; store puteflater beholdes mellom sømmene.
        mask=ImageOps.invert(connected(ImageOps.invert(binary),minimum=1500))
        # Målte sentre i den godkjente 512px-kilden. Fotoets lyse knapper ser hevet
        # ut, men briefen krever at knappene ligger nede i trekket. Kildens
        # seam-nett alene lukker ikke alle de fotograferte knapp-ringene.
        centres=[(x,y) for xs,y in [((0,256),4),((128,384),131),((0,256),258),((128,384),386)] for x in xs]
        button=[]
        for y in range(SIZE):
            for x in range(SIZE):
                dist=min(math.hypot(min(abs(x-cx),SIZE-abs(x-cx)),min(abs(y-cy),SIZE-abs(y-cy))) for cx,cy in centres)
                button.append(clamp((15-dist)/2))
        mask=ImageChops.lighter(mask,gray(button))
        notes['button_centres_source_512']=centres
        notes['geometry']='source stitched valleys, measured source button centres depressed, distance-derived raised cushions; no lighting-to-height conversion'
    elif kind=='granular':
        mask=dark
        aux=contrast_features(ImageOps.invert(blur(luma,.9)),5,.025)
        notes['geometry']='small source aggregate peaks and narrow depressions; broad colour/shading excluded'
    else:
        mask=dark
        notes['geometry']='small narrow local valleys only; broad albedo colour/shading excluded'
    # Små rustkjerner skal beholde .9 i ruhet, ikke vaskes bort av kantfilteret.
    aux_out=ImageChops.lighter(aux,blur(aux,.7)) if kind=='metal' else blur(aux,1.1)
    return blur(mask,.7),aux_out,notes


def condition_edges(field, band=8, size=SIZE):
    """Bare datakart: flat 2px kantkrage med lik motsatt kant og glatt overgang.

    Endrer ingen albedo. Overgangsbåndet er eksplisitt dokumentert i rapporten.
    Første/to siste høydeprøver er like, så periodisk sentraldifferanse er null
    normalt på sømmen og lik tangentialt på begge sider.
    """
    out=array('f',field)
    for y in range(size):
        edge=(field[y*size]+field[y*size+size-1])/2
        for d in range(band):
            t=clamp((d-1)/(band-2)); t=t*t*t*(t*(t*6-15)+10)
            for x in (d,size-1-d): out[y*size+x]=edge*(1-t)+field[y*size+x]*t
    before=array('f',out)
    for x in range(size):
        edge=(before[x]+before[(size-1)*size+x])/2
        for d in range(band):
            t=clamp((d-1)/(band-2)); t=t*t*t*(t*(t*6-15)+10)
            for y in (d,size-1-d):out[y*size+x]=edge*(1-t)+before[y*size+x]*t
    return out


def normal_bytes(height, support, size=SIZE):
    """OpenGL +Y: bilde-y går ned, tangent-y går opp. N=(-dh/dx,+dh/drow,1)."""
    data=bytearray(size*size*3); calm=math.tan(math.radians(14.5)); strong=math.tan(math.radians(39.5))
    clipped=0
    for y in range(size):
        row=y*size; prev=((y-1)%size)*size; nxt=((y+1)%size)*size
        for x in range(size):
            i=row+x
            nx=-(height[row+(x+1)%size]-height[row+(x-1)%size])/2
            ny=(height[nxt+x]-height[prev+x])/2
            slope=math.hypot(nx,ny); maximum=strong if support[i]>=.08 else calm
            if slope>maximum:
                scale=maximum/slope; nx*=scale; ny*=scale;clipped+=1
            inv=1/math.sqrt(nx*nx+ny*ny+1)
            data[i*3:i*3+3]=bytes((round((nx*inv+1)*127.5),round((ny*inv+1)*127.5),round((inv+1)*127.5)))
    return bytes(data),clipped


def make_fields(rgb, guide, profile):
    mask,aux,details=feature_masks(rgb,profile)
    mv=[v/255 for v in values(mask)]; av=[v/255 for v in values(aux)]
    # Bandbegrenset guide fjerner store AI-lysgradienter. Styrken er i høydepiksler.
    gsmall=blur(guide.convert('L'),2); glarge=blur(guide.convert('L'),14)
    gv=[(a-b)/255 for a,b in zip(values(gsmall),values(glarge))]
    raw=[]; rough=[]; low,high=profile['rough']; kind=profile['kind']
    luma=rgb.convert('L'); broad=[v/255 for v in values(blur(luma,10))]
    fine=[abs(a-b)/255 for a,b in zip(values(luma),values(blur(luma,3)))]
    texture_energy=[v/255 for v in values(blur(gray(fine),9))]
    wear_values=None
    if kind=='wood_h':
        # Lokal slitasje fra tapt mikrokontrast, ikke lyse/mørke bordpigmenter.
        quiet=sorted(e for e,m in zip(texture_energy,mv) if m<.08)
        q10=quiet[int(len(quiet)*.10)];q70=quiet[int(len(quiet)*.70)]
        wear_image=blur(gray([clamp((q70-e)/max(.001,q70-q10)) for e in texture_energy]),12)
        blurred=[v/255 for v in values(wear_image)];ordered=sorted(blurred)
        w10=ordered[int(len(ordered)*.10)];w95=ordered[int(len(ordered)*.95)]
        wear_values=[clamp((v-w10)/max(.001,w95-w10)) for v in blurred]
        details['wear_method']='source low microcontrast, 9px energy and 12px spatial smoothing, percentile normalized; excludes grout'
        details['wear_interpretation']='UNVERIFIED walking path: only local smoother source regions, no invented continuous traffic stripe'
    wet_values=[0.]*len(broad)
    if kind=='asphalt':
        # Mørke, rolige felt kan leses som fuktig asfalt. Absolutt terskel på
        # sRGB-pikselverdi overser denne kildens dempede kontrast. Bruk kildens
        # egen fordeling; sprekkene holdes ru og blir aldri blanke vannstriper.
        tones=sorted(broad);energy=sorted(texture_energy)
        b10=tones[int(len(tones)*.10)];b55=tones[int(len(tones)*.55)]
        e15=energy[int(len(energy)*.15)];e70=energy[int(len(energy)*.70)]
        candidate=[clamp((b55-v)/max(.005,b55-b10))*clamp((e70-e)/max(.002,e70-e15)) for v,e in zip(broad,texture_energy)]
        wet_mask=blur(gray(candidate),12)
        filtered=[v/255 for v in values(wet_mask)];strongest=max(filtered)
        wet_values=[clamp(v/max(.10,strongest))*(1-m) for v,m in zip(filtered,mv)]
        details['wet_method']='source-relative dark low-frequency regions with low local texture energy, 12px smoothing; narrow cracks excluded'
        details['wet_interpretation']='UNVERIFIED: damp-looking smooth dark regions, not photographic proof of standing water'
        details['wet_thresholds']={'tone_p10':b10,'tone_p55':b55,'texture_energy_p15':e15,'texture_energy_p70':e70,'max_candidate_before_normalization':strongest}
    cushion_distance=distance_to_features(mask) if kind=='tuft' else None
    puddle=[]
    for i,(m,a,g) in enumerate(zip(mv,av,gv)):
        h=-m*profile['depth'] + g*profile['guide']*(1-m)
        if kind=='tracks':h-=a*3
        if kind=='metal':h+=a*.3
        if kind=='granular':h+=a*.65
        if kind in ('wood_v','wood_h','wood_local_v'):h-=a*.15
        if cushion_distance is not None:h+=1.4*(1-math.exp(-cushion_distance[i]/13))
        # Aggregat bruker små kantgroper som svak struktur; ikke tilfeldig ekstra støy.
        raw.append(h)
        r=(low+high)/2
        if 'feature_rough' in profile:r=r*(1-m)+profile['feature_rough']*m
        elif kind in ('wood_v','wood_h','grid4','grid8'):r+=m*(high-r)
        if kind=='metal':r=r*(1-a)+.9*a
        if kind=='grid4':
            # Slitasje er lavere lokal strukturenergi, ikke lysere fargepigment.
            wear=clamp((.008-texture_energy[i])/.008)*(1-m)
            r=r*(1-wear)+low*wear
        if wear_values is not None:
            wear=wear_values[i]*(1-m)
            r=r*(1-wear)+low*wear
        wet=0.
        if kind=='asphalt':
            wet=wet_values[i]
            r=r*(1-wet)+.23*wet
        puddle.append(wet)
        rough.append(clamp(r))
    # Fugekantens skrå flisefas ligger utenfor selve fugens sentrum.
    support=[v/255 for v in values(wrapped_filter(mask,ImageFilter.MaxFilter(9)))]
    if kind=='tracks':support=[max(a,b) for a,b in zip(support,av)]
    details.update(guide_weight_height_pixels=profile['guide'],guide_method='periodic Gaussian 2px minus 14px, gated outside source features',
                   feature_mask_fraction=sum(v>.08 for v in mv)/len(mv),
                   strong_slope_support_fraction=sum(v>=.08 for v in support)/len(support),
                   puddle_candidate_fraction=sum(v>.25 for v in puddle)/len(puddle))
    return condition_edges(raw),condition_edges(rough),condition_edges(support),mask,details


def light_normal(normal):
    """Ortografisk flate, lys mot øvre kant (+tangent-Y) og kameraet."""
    out=[]; inv=1/math.sqrt(2)
    for r,g,b in values(normal):
        ny=g/127.5-1;nz=b/127.5-1
        v=round(255*clamp(.14+.86*max(0,(ny+nz)*inv)))
        out.append((v,v,v))
    image=Image.new('RGB',normal.size);image.putdata(out);return image


def info(path):
    with Image.open(path) as im:
        return {'path':str(path),'sha256':sha(path),'bytes':Path(path).stat().st_size,'size':list(im.size),'mode':im.mode,'format':im.format}


def edge_stats(im):
    rgb=im.convert('RGB');w,h=rgb.size;p=rgb.load()
    differences=[abs(a-b) for y in range(h) for a,b in zip(p[0,y],p[w-1,y])]
    differences += [abs(a-b) for x in range(w) for a,b in zip(p[x,0],p[x,h-1])]
    differences.sort()
    return {'mean_abs_byte_delta':sum(differences)/len(differences),'p95_abs_byte_delta':differences[int(.95*(len(differences)-1))],'max_abs_byte_delta':max(differences)}


def make_qa(rgb,normal,mask,height,qa,prefix):
    qa.mkdir(parents=True,exist_ok=True)
    repeated=Image.new('RGB',(SIZE*2,SIZE*2))
    for y in range(2):
        for x in range(2):repeated.paste(normal,(x*SIZE,y*SIZE))
    outputs={
        'normal_2x2':repeated,'overlay':Image.blend(rgb,normal,.5),
        'light_top':light_normal(normal),'mask':mask,
    }
    lo=min(height);hi=max(height)
    outputs['derived_height']=gray([(v-lo)/(hi-lo) if hi>lo else .5 for v in height])
    files={}
    for key,im in outputs.items():
        path=qa/f'{prefix}_{key}.png';im.save(path,compress_level=9);files[key]=info(path)
    return files


def check_material(normal,support,rough,profile):
    """Kontroll etter kvantisering, med samme eksplisitte maskekontrakt som generatoren."""
    calm=[];strong=[]
    for (r,g,b),mask in zip(values(normal),support):
        x,y,z=r/127.5-1,g/127.5-1,b/127.5-1
        slope=math.degrees(math.acos(clamp(z/math.sqrt(x*x+y*y+z*z),-1,1)))
        (strong if mask>=.08 else calm).append(slope)
    lo,hi=profile['rough'];hi=max(hi,profile.get('feature_rough',hi))
    if profile['kind']=='asphalt':lo=.15
    if profile['kind']=='metal':hi=.90
    stats={'calm_max_degrees':max(calm,default=0),'feature_max_degrees':max(strong,default=0),
           'calm_pixel_count':len(calm),'feature_pixel_count':len(strong),
           'roughness_intended_range':[lo,hi],'roughness_pre_jpeg_range':[min(rough),max(rough)]}
    stats['status']='PASS' if stats['calm_max_degrees']<=15 and stats['feature_max_degrees']<=40 and min(rough)>=lo-1e-6 and max(rough)<=hi+1e-6 else 'FAIL'
    return stats


def convert(albedo,guide_path,profile_name,output_prefix,qa_dir,report_path=None):
    if profile_name not in PROFILES:raise ValueError(f'Unknown profile {profile_name}')
    if Path(albedo).resolve()==Path(guide_path).resolve():raise ValueError('Guide must be a separate observed height-guide, not albedo')
    with Image.open(albedo) as source:
        if source.width!=source.height:raise ValueError('Albedo must be square; no crop or geometric warp is allowed')
        rgb=source.convert('RGB').resize((SIZE,SIZE),Image.Resampling.LANCZOS)
    with Image.open(guide_path) as source:
        if source.width!=source.height:raise ValueError('Guide must be square; no crop or geometric warp is allowed')
        guide=source.convert('L').resize((SIZE,SIZE),Image.Resampling.LANCZOS)
    profile=PROFILES[profile_name]
    height,rough,support,mask,details=make_fields(rgb,guide,profile)
    normal_data,clipped=normal_bytes(height,support)
    normal=Image.frombytes('RGB',(SIZE,SIZE),normal_data)
    prefix=Path(output_prefix); prefix.parent.mkdir(parents=True,exist_ok=True)
    npath=Path(str(prefix)+'_n.png');rpath=Path(str(prefix)+'_r.jpg')
    for target in (npath,rpath):
        if target.resolve() in (Path(albedo).resolve(),Path(guide_path).resolve()):raise ValueError('Output would overwrite input')
    normal.save(npath,compress_level=9)
    gray(rough).save(rpath,quality=90,optimize=True)
    from round9_check import check_pair,analytical_hill
    validation=check_pair(npath,rpath)
    material=check_material(normal,support,rough,profile)
    qa=make_qa(rgb,normal,mask,height,Path(qa_dir),prefix.name)
    if profile['kind']=='asphalt':
        wet_qa=Path(qa_dir)/(prefix.name+'_roughness.png')
        with Image.open(rpath) as rough_image:rough_image.save(wet_qa)
        qa['roughness']=info(wet_qa)
    report={
        'schema':'signal47-round9-map-report-v1','tool':{'file':str(Path(__file__).resolve()),'sha256':sha(__file__),'pillow':PIL_VERSION,
                                                     'checker_sha256':sha(Path(__file__).with_name('round9_check.py'))},
        'albedo':info(albedo),'height_guide':info(guide_path),'profile':profile_name,'parameters':profile,
        'method':details,'outputs':{'normal':info(npath),'roughness':info(rpath)},'qa':qa,
        'normal_convention':'OpenGL +Y; image row increases downward; N=(-dh/dx,+dh/drow,1)',
        'roughness_convention':'absolute linear grayscale values; integration chooses material multiplier',
        'seam_conditioning':{'data_only':True,'band_pixels':8,'flat_boundary_pixels':2,'normal_edges':edge_stats(normal),'albedo_edges_unmodified':edge_stats(rgb)},
        'slope_clamped_pixel_count':clipped,'technical':validation,'material_contract':material,'analytic_positive_y':analytical_hill(),
        'visual_status':'UNVERIFIED','runtime_status':'UNVERIFIED',
        'limitations':['Semantic feature masks require visual review per source.','The guide does not replace source geometry.','Normal direction test verifies the converter, not semantic correctness of every bump.',
                      'Equal data-map borders do not fix or certify albedo seams.','PNG QA lighting is diagnostic, not an in-game screenshot.','No exact image-generator model version is inferred by this tool.'],
    }
    target=Path(report_path) if report_path else Path(qa_dir)/(prefix.name+'_report.json')
    target.parent.mkdir(parents=True,exist_ok=True);target.write_text(json.dumps(report,indent=2)+'\n')
    return report,target


def main():
    p=argparse.ArgumentParser(description=__doc__,formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('--list-profiles',action='store_true')
    p.add_argument('--albedo',type=Path);p.add_argument('--height-guide',type=Path)
    p.add_argument('--profile',choices=PROFILES);p.add_argument('--output-prefix',type=Path)
    p.add_argument('--qa-dir',type=Path);p.add_argument('--report',type=Path)
    args=p.parse_args()
    if args.list_profiles:print(json.dumps(PROFILES,indent=2));return
    for name in ('albedo','height_guide','profile','output_prefix','qa_dir'):
        if getattr(args,name) is None:p.error('--'+name.replace('_','-')+' is required')
    report,path=convert(args.albedo,args.height_guide,args.profile,args.output_prefix,args.qa_dir,args.report)
    print(json.dumps({'report':str(path),'technical':report['technical']['status'],'visual':'UNVERIFIED','outputs':report['outputs']}))
    if any(report[k]['status']!='PASS' for k in ('technical','material_contract','analytic_positive_y')):raise SystemExit(1)


if __name__=='__main__':main()
