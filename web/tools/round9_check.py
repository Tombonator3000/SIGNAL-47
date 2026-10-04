#!/usr/bin/env python3
"""Uavhengig readback av Runde 9-datafiler og analytisk +Y-kontroll. Pillow."""
from __future__ import annotations
import argparse
import json
import math
from pathlib import Path
import struct
from PIL import Image


def values(im):
    return list(im.get_flattened_data() if hasattr(im,'get_flattened_data') else im.getdata())


def chunks(path):
    data=Path(path).read_bytes();out=[];offset=8
    if data[:8]!=b'\x89PNG\r\n\x1a\n':return []
    while offset+12<=len(data):
        size=struct.unpack('>I',data[offset:offset+4])[0]
        out.append(data[offset+4:offset+8].decode('ascii'));offset+=size+12
    return out


def quantization90():
    # ISO/IEC JPEG luminance base table, scaled by quality 90 (scale=20).
    base=[16,11,10,16,24,40,51,61,12,12,14,19,26,58,60,55,14,13,16,24,40,57,69,56,14,17,22,29,51,87,80,62,
          18,22,37,56,68,109,103,77,24,35,55,64,81,104,113,92,49,64,78,87,103,121,120,101,72,92,95,98,112,100,103,99]
    return [max(1,min(255,(v*20+50)//100)) for v in base]


def check_pair(normal_path,rough_path):
    checks=[]
    def check(name,ok,detail=None):checks.append({'check':name,'status':'PASS' if ok else 'FAIL','detail':detail})
    with Image.open(normal_path) as n:
        n.load();check('normal format RGB 512x512',n.format=='PNG' and n.mode=='RGB' and n.size==(512,512),{'format':n.format,'mode':n.mode,'size':list(n.size)})
        png_chunks=chunks(normal_path);check('normal no gamma/profile/alpha',not set(png_chunks)&{'gAMA','sRGB','iCCP','cHRM','tRNS'},png_chunks)
        raw=values(n.convert('RGB'));lengths=[];angles=[]
        for r,g,b in raw:
            x,y,z=r/127.5-1,g/127.5-1,b/127.5-1
            length=math.sqrt(x*x+y*y+z*z);lengths.append(length)
            angles.append(math.degrees(math.acos(max(-1,min(1,z/length)))))
        check('normal B >= 128',min(p[2] for p in raw)>=128,min(p[2] for p in raw))
        check('normal vector length 0.95..1.05',min(lengths)>=.95 and max(lengths)<=1.05,{'min':min(lengths),'max':max(lengths)})
        check('normal slope <= 40 degrees',max(angles)<=40,{'max_degrees':max(angles),'pixels_over15':sum(a>15 for a in angles)})
        w,h=n.size;p=n.load();same=all(p[0,y]==p[w-1,y] for y in range(h)) and all(p[x,0]==p[x,h-1] for x in range(w))
        check('normal opposite border pixels identical',same)
    with Image.open(rough_path) as r:
        r.load();check('roughness JPEG L 512x512',r.format=='JPEG' and r.mode=='L' and r.size==(512,512),{'format':r.format,'mode':r.mode,'size':list(r.size)})
        check('roughness no profile/EXIF/gamma',not any(k in r.info for k in ('icc_profile','exif','gamma','srgb')),sorted(r.info))
        check('roughness JPEG quality90 table',r.quantization=={0:quantization90()},r.quantization)
        vals=values(r.convert('L'));stats={'min':min(vals)/255,'max':max(vals)/255,'mean':sum(vals)/(len(vals)*255)}
    return {'status':'PASS' if all(c['status']=='PASS' for c in checks) else 'FAIL','checks':checks,'roughness_readback':stats}


def analytical_hill():
    """Kontroller fortegn etter PNG-kvantisering, uavhengig av albedo/guide."""
    from round9_maps import normal_bytes
    size=65;c=(size-1)/2;sigma=10
    height=[8*math.exp(-((x-c)**2+(y-c)**2)/(2*sigma*sigma)) for y in range(size) for x in range(size)]
    data,_=normal_bytes(height,[1.]*(size*size),size)
    def normal(x,y):return tuple(v/127.5-1 for v in data[(y*size+x)*3:(y*size+x)*3+3])
    upper=normal(32,22);lower=normal(32,42);left=normal(22,32);right=normal(42,32)
    def lit(n):return (n[1]+n[2])/math.sqrt(2)
    ok=upper[1]>0 and lower[1]<0 and left[0]<0 and right[0]>0 and lit(upper)>lit(lower)
    return {'status':'PASS' if ok else 'FAIL','upper_normal':upper,'lower_normal':lower,'left_normal':left,'right_normal':right,
            'upper_light':lit(upper),'lower_light':lit(lower),'light_direction':[0,1/math.sqrt(2),1/math.sqrt(2)]}


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--normal',type=Path);p.add_argument('--roughness',type=Path);p.add_argument('--self-test',action='store_true')
    args=p.parse_args()
    if args.self_test:
        result=analytical_hill()
    else:
        if not args.normal or not args.roughness:p.error('--normal and --roughness are required')
        result=check_pair(args.normal,args.roughness)
    print(json.dumps(result,indent=2))
    if result['status']!='PASS':raise SystemExit(1)


if __name__=='__main__':main()
