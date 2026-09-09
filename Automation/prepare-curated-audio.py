#!/usr/bin/env python3
import array,json,math,subprocess,wave,zipfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
SRC=ROOT/'Unity/Assets/Signal47/Art/ThirdParty'
OUT=SRC/'PreparedAudio';OUT.mkdir(exist_ok=True)
RATE=44100
jobs=[('Printer','printer',.2,1.2,.65),('PhoneRing','phone',.55,5.95,.7),('CeramicBreak','cup',.19,1.45,.9),('DesertWind','wind',4,24,.3)]
records=[]
for name,source,start,duration,peak in jobs:
 inp=SRC/'Freesound'/f'{source}-hq-preview.mp3'
 raw=subprocess.check_output(['ffmpeg','-v','error','-ss',str(start),'-i',str(inp),'-t',str(duration),'-ac','1','-ar',str(RATE),'-f','f32le','-'])
 samples=array.array('f');samples.frombytes(raw)
 gain=peak/max(max(abs(x) for x in samples),.001)
 for i in range(len(samples)):samples[i]*=gain
 if name=='DesertWind':
  n=int(.75*RATE)
  # Overlap the loop's head and tail, retaining the transition into the remainder.
  for i in range(n):samples[len(samples)-n+i]=samples[len(samples)-n+i]*(1-i/n)+samples[i]*(i/n)
  samples=samples[n:]
 else:
  fade=int((.002 if name=='CeramicBreak' else .015)*RATE)
  for i in range(fade):samples[i]*=i/fade;samples[-1-i]*=i/fade
 pcm=array.array('h',(int(max(-1,min(1,x))*32767) for x in samples))
 path=OUT/(name+'.wav')
 with wave.open(str(path),'wb') as w:w.setnchannels(1);w.setsampwidth(2);w.setframerate(RATE);w.writeframes(pcm.tobytes())
 records.append(dict(file=str(path.relative_to(SRC)),source=str(inp.relative_to(SRC)),start_seconds=start,length_seconds=len(samples)/RATE,peak=peak,processing='Mono 44.1kHz PCM; peak normalization and edge fades; wind crossfades its loop.'))
with zipfile.ZipFile(SRC/'Kenney/interface-sounds.zip') as z:
 for name in ['Audio/click_001.ogg','Audio/switch_003.ogg','License.txt']:(SRC/'Kenney'/Path(name).name).write_bytes(z.read(name))
subprocess.run(['ffmpeg','-y','-v','error','-i',str(SRC/'ScottBuckley/SignalToNoise-NoPiano.mp3'),'-t','45','-af','afade=t=in:d=2,afade=t=out:st=40:d=5','-c:a','libvorbis','-q:a','5',str(OUT/'TitleMusic.ogg')],check=True)
(SRC/'audio-processing.json').write_text(json.dumps(records,indent=2)+'\n')
print('Prepared',len(jobs),'effects and title music')
