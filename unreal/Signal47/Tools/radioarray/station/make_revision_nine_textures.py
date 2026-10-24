"""Original English 1986 station paperwork. No copied document or external bitmap."""
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
import random
ROOT=Path(__file__).resolve().parent;OUT=ROOT/'textures'/'revision9'
FONT='/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf'
BOLD='/usr/share/fonts/truetype/dejavu/DejaVuSansMono-Bold.ttf'
PAGES=[
 ('1420 AM','PROGRAM BOARD',[
  'APRIL 1986', '', 'NIGHT LINE / EARL', 'MIDNIGHT TO FOUR', '',
  'OPENING CHECK', '  Station identification', '  County road report', '  Weather / local notices', '',
  'BEFORE TAKING A CALL', '  Write the caller name.', '  Mark reel and counter.', '  Cue the next record.', '',
  'STUDIO IN USE', 'Please use the buzzer.', 'Leave recordings at the desk.']),
 ('TELEPHONE','MESSAGE SLIPS',[
  'DATE: APRIL ____ 1986', '', 'CALLER: __________________', '', 'LINE: ____   TIME: _______', '',
  'RETURN NUMBER:', '__________________________', '', 'MESSAGE:', '__________________________',
  '__________________________', '__________________________', '', 'FOR THE PRESENTER',
  'Take a name before transfer.', 'Do not interrupt a live call.', '', 'INITIALS: ________________']),
 ('TAPE ROOM','REEL HANDLING',[
  '1/4 INCH / 7 1/2 IPS', '', 'MASTER: __________________', '', 'DATE: ____________________', '',
  'REEL: ______   SIDE: ______', '', 'LEADER / SPLICE CHECK', '  Label both box and reel.', '  Keep the original take.',
  '  Log every splice.', '  Return tape to its box.', '', 'ARCHIVE RETURN', 'Shelf: ____  Initials: ____', '',
  'Keep magnets off the desk.']),
 ('1420 AM','ENGINEERING LOG',[
  'DATE: APRIL ____ 1986', '', 'SOURCE / LEVEL / CUE', '', 'REEL _____________________',
  'COUNTER __________________', '', 'LINE CHECK _______________', 'LEVEL ____________________', '',
  'NOTES', '__________________________', '__________________________', '__________________________', '',
  'HEADS CLEANED: ___________', 'CHECKED BY: ______________', '', 'Keep with the program log.'])]
def generate():
 OUT.mkdir(parents=True,exist_ok=True);im=Image.new('RGB',(2048,1024));r=random.Random(14201986)
 title=ImageFont.truetype(BOLD,37);sub=ImageFont.truetype(BOLD,25);body=ImageFont.truetype(FONT,20)
 for page,(heading,label,lines) in enumerate(PAGES):
  x0=page*512;d=ImageDraw.Draw(im)
  d.rectangle((x0,0,x0+511,1023),fill=(219-page*3,209-page*3,181-page*3))
  for i in range(650):
   x=x0+r.randrange(512);y=r.randrange(1024);d.point((x,y),fill=(205,193,164))
  d.rectangle((x0+22,22,x0+490,1002),outline=(177,165,137),width=2)
  d.text((x0+37,50),heading,font=title,fill=(48,43,32))
  d.text((x0+37,105),label,font=sub,fill=(64,52,37))
  d.line((x0+35,151,x0+477,151),fill=(98,83,60),width=3)
  for i,line in enumerate(lines):d.text((x0+35,181+i*38),line,font=body,fill=(54,48,35))
  # Pencil check marks and a hand-completed margin make the copy used, but readable.
  for y in [383,459,535]:d.line((x0+464,y,x0+469,y+5,x0+480,y-8),fill=(115,92,65),width=2)
  d.text((x0+37,956),'1420 AM / CALICHE',font=ImageFont.truetype(FONT,17),fill=(113,97,73))
 im.save(OUT/'station_notices_color.png')
 Image.new('RGB',im.size,(233,233,233)).save(OUT/'station_notices_roughness.png')
 Image.new('RGB',im.size,(128,128,255)).save(OUT/'station_notices_normal_gl.png')
 Image.new('RGB',im.size,(128,127,255)).save(OUT/'station_notices_normal_dx.png')
 print('R9_PRINT_ATLAS_READY',OUT)
if __name__=='__main__':generate()
