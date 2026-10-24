"""Technical RGBA/MASK encoding checks only. This is not a visual-density verdict."""
import hashlib,io,json,struct
from pathlib import Path
from PIL import Image
ROOT=Path(__file__).resolve().parent

def inspect_png(data):
 image=Image.open(io.BytesIO(data));assert image.mode=='RGBA',image.mode
 w,h=image.size;assert w==h*2 and w%4==0,(w,h);n=w//4;rows={}
 for tile in range(8):
  x,y=(tile%4)*n,(tile//4)*n;hist=image.getchannel('A').crop((x,y,x+n,y+n)).histogram()
  fraction=sum(hist[85:])/(n*n);lo=next(i for i,c in enumerate(hist) if c);hi=max(i for i,c in enumerate(hist) if c)
  if tile in (0,3,4):assert lo==0 and hi==255 and .20<fraction<.85,(tile,lo,hi,fraction)
  else:assert hist[255]==n*n,(tile,hist[255],n*n)
  rows[str(tile)]={'alpha_min':lo,'alpha_max':hi,'opaque_at_cutoff_fraction':fraction}
 return {'dimensions':[w,h],'mode':image.mode,'tiles':rows}

def run():
 contract=json.loads((ROOT/'model_contract.json').read_text());materials=json.loads((ROOT/'materials.json').read_text())
 report={'source_revision':contract['source_revision'],'status':'PASS','scope':'Technical alpha channels and glTF MASK encoding only. No botanical density, shape or native visual acceptance claimed.','source':inspect_png((ROOT/materials['M_CN_Foliage']['textures']['base_color']).read_bytes()),'glb':{},'unique_embedded_pngs':{}}
 for row in contract['parts'].values():
  for lod in row['lods']:
   raw=(ROOT/lod['glb']).read_bytes();js=struct.unpack_from('<I',raw,12)[0];g=json.loads(raw[20:20+js]);binary=raw[28+js:]
   entries=[]
   for mat in g.get('materials',[]):
    if mat['name']=='M_CN_Foliage':
     assert mat.get('alphaMode')=='MASK' and abs(mat.get('alphaCutoff',0)-.333)<1e-6 and mat.get('doubleSided') is True,mat
     texture=g['textures'][mat['pbrMetallicRoughness']['baseColorTexture']['index']];image=g['images'][texture['source']];view=g['bufferViews'][image['bufferView']]
     start=view.get('byteOffset',0);data=binary[start:start+view['byteLength']];sha=hashlib.sha256(data).hexdigest()
     if sha not in report['unique_embedded_pngs']:report['unique_embedded_pngs'][sha]=inspect_png(data)
     entries.append({'name':mat['name'],'alphaMode':'MASK','alphaCutoff':mat['alphaCutoff'],'rgba_sha256':sha})
    else:assert mat.get('alphaMode','OPAQUE')=='OPAQUE',mat
   assert bool(entries)==('M_CN_Foliage' in lod['materials']),lod['name']
   report['glb'][lod['name']]=entries
 (ROOT/'alpha_texture_verification.json').write_text(json.dumps(report,indent=2));print('MASK_ENCODING_VERIFY',len(report['glb']),'GLBs,',len(report['unique_embedded_pngs']),'unique RGBA images, PASS')
if __name__=='__main__':run()
