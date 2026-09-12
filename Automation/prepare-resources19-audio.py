#!/usr/bin/env python3
"""Prepare the downloaded Resources19 audition set without changing the live mix."""
import json,subprocess,hashlib,math,struct,html
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
IN=ROOT/'Artifacts/Resources19/Incoming';OUT=ROOT/'Docs/Resources19/Audio';OUT.mkdir(parents=True,exist_ok=True)
tracks=[]
sources=[(IN/'echoes_of_the_past.wav','Echoes of the Past','isaiah658','CC0','https://opengameart.org/content/echoes-of-the-past','Arkiv / etterforskning'),(IN/'outpost_loop.flac','Outpost (loop)','Tsorthan Grove','CC0','https://opengameart.org/content/the-world-fell-silent','Avsides stasjon / natt'),(IN/'dirty_rain_loop.flac','Dirty Rain (loop)','Tsorthan Grove','CC0','https://opengameart.org/content/the-world-fell-silent','Uro / overgang')]
for p in sorted((IN/'machinery').rglob('*.wav')):sources.append((p,p.stem,'Michael Brigida and students','CC-BY-3.0','https://opengameart.org/content/12-ambient-machine-sounds','Maskinlyd; vurder som romlyd eller mekanikk'))
for p,title,author,license,url,role in sources:
 name=p.stem.lower().replace(' ','_')+'.ogg';dest=OUT/name
 subprocess.run(['ffmpeg','-y','-v','error','-i',str(p),'-vn','-af','volume=-3dB','-c:a','libvorbis','-q:a','5',str(dest)],check=True)
 info=json.loads(subprocess.check_output(['ffprobe','-v','error','-show_streams','-show_format','-of','json',str(dest)]));s=info['streams'][0]
 raw=subprocess.check_output(['ffmpeg','-v','error','-i',str(dest),'-f','f32le','-acodec','pcm_f32le','-'])
 samples=struct.unpack('<'+'f'*(len(raw)//4),raw);peak=max(abs(x) for x in samples);rms=math.sqrt(sum(x*x for x in samples)/len(samples))
 tracks.append(dict(file='Audio/'+name,title=title,author=author,license=license,source=url,proposed_use=role,duration_s=float(info['format']['duration']),channels=s['channels'],sample_rate=int(s['sample_rate']),peak_dbfs=round(20*math.log10(max(peak,1e-12)),2),rms_dbfs=round(20*math.log10(max(rms,1e-12)),2),source_sha256=hashlib.sha256(p.read_bytes()).hexdigest(),sha256=hashlib.sha256(dest.read_bytes()).hexdigest(),processing='Full-length Vorbis q5 transcode; -3 dB headroom; no cut, loop edits or denoising',status='audition only; not assigned in Unity; subjective balance unverified'))
(OUT.parent/'audio-manifest.json').write_text(json.dumps(tracks,indent=2,ensure_ascii=False)+'\n')
cards='\n'.join(f'<article><h2>{html.escape(t["title"])}</h2><p>{html.escape(t["proposed_use"])} · {t["duration_s"]:.1f} s</p><audio controls preload="none" src="{t["file"]}"></audio><p><a href="{t["source"]}">{html.escape(t["author"])}</a> · {t["license"]}</p></article>' for t in tracks)
(OUT.parent/'lydprove.html').write_text('<!doctype html><html lang="nb"><meta charset="utf-8"><meta name="viewport" content="width=device-width"><title>SIGNAL / 47 – lydkandidater</title><style>body{background:#111c1a;color:#e8dfc5;font:17px system-ui;margin:40px auto;max-width:850px;padding:0 20px}h1{font-size:32px}article{border-top:1px solid #4a5b50;padding:18px 0}h2{font-size:21px}a{color:#b2cfac}audio{width:100%}p{line-height:1.6}</style><h1>SIGNAL / 47 · lydprøve</h1><p>15 gratis kandidater. Foreslått bruk bygger på kildebeskrivelsene. Spillmiks og subjektiv lytteprøve gjenstår. Ingen automatisk avspilling.</p><p><a href="CREDITS.md">Kilder og kreditering</a></p>'+cards+'</html>')
print(json.dumps({'tracks':len(tracks),'bytes':sum((OUT.parent/t['file']).stat().st_size for t in tracks),'peaks_over_0_dbfs':[t['title'] for t in tracks if t['peak_dbfs']>0]}))
