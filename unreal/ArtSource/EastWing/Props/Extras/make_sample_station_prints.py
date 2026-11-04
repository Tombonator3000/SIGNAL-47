"""Original sample-index and checkout desk-sign artwork; existing Pillow/font stack.
Run plain Python before build_sample_station.py. No existing print is overwritten.
S47_SAMPLE_STATION_ONLY selects the same asset names as the Blender builder;
unselected print files are untouched. Default selects only the original two assets.
"""
from pathlib import Path
import os
from PIL import Image, ImageDraw, ImageFont
HERE=Path(__file__).resolve().parent
OUT=Path(os.environ.get('S47_SAMPLE_STATION_OUT',str(HERE)))/'textures'
FONT=Path('/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf')

def line(draw,words,y,width,size,fill):
    font=ImageFont.truetype(str(FONT),size)
    bounds=draw.textbbox((0,0),words,font=font)
    while bounds[2]-bounds[0]>width-60:
        size-=1;font=ImageFont.truetype(str(FONT),size);bounds=draw.textbbox((0,0),words,font=font)
    draw.text(((width-bounds[2]+bounds[0])/2,y-bounds[1]),words,font=font,fill=fill)

def main():
    signs={
        'SM_EW_DepotCheckoutSign':('EW_Print_DepotCheckout','PARTS STORAGE','EQUIPMENT CHECKOUT','RETURN LOGGER + ORIGINAL TAPE'),
        'SM_EW_CalibrationSign':('EW_Print_Calibration','CALIBRATION / REPAIR','DONOR / LOGGER TEST',''),
        'SM_EW_SampleAnalysisSign':('EW_Print_SampleAnalysis','GENERAL TEST LAB','FIELD SAMPLE ANALYSIS',''),
        'SM_EW_ReceivingSign':('EW_Print_Receiving','GARAGE RECEIVING','UNPACK CARRIED SUPPLIES',''),
    }
    selection=os.environ.get('S47_SAMPLE_STATION_ONLY','SM_EW_SampleHolder,SM_EW_DepotCheckoutSign')
    selected={name.strip() for name in selection.split(',') if name.strip()}
    assert selected and selected<=set(signs)|{'SM_EW_SampleHolder'},('Unknown/empty sample station asset filter',selection)
    OUT.mkdir(parents=True,exist_ok=True)
    for asset,(name,title,subtitle,footer) in signs.items():
        if asset not in selected:continue
        im=Image.new('RGB',(1024,512),(206,201,176));d=ImageDraw.Draw(im)
        d.rectangle((12,12,1011,499),outline=(50,65,53),width=8)
        d.rectangle((24,24,999,205),fill=(35,55,44))
        line(d,title,66,1024,88,(231,225,201))
        line(d,subtitle,252 if footer else 300,1024,65,(30,43,35))
        if footer:line(d,footer,398,1024,31,(55,62,48))
        im.save(OUT/(name+'_BaseColor.png'))
    if 'SM_EW_SampleHolder' not in selected:
        print('SAMPLE_STATION_PRINTS',OUT,sorted(selected))
        return
    im=Image.new('RGB',(1024,256),(211,205,179));d=ImageDraw.Draw(im)
    d.rectangle((10,10,1013,245),outline=(50,61,52),width=5)
    line(d,'SARO  /  FIELD SAMPLE',30,1024,67,(34,44,37))
    line(d,'RETAIN ORIGINAL  -  DO NOT DISCARD',151,1024,39,(34,44,37))
    im.save(OUT/'EW_Print_SampleIndex_BaseColor.png')
    print('SAMPLE_STATION_PRINTS',OUT)

if __name__=='__main__':main()
