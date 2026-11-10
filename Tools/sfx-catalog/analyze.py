import os, sys, json, subprocess, csv, math
import numpy as np
from scipy.signal import lfilter
from concurrent.futures import ProcessPoolExecutor
LIB=os.environ.get("S47_SFX_LIB",os.path.expanduser("~/Desktop/SIGNAL 47/bt25_ultimatesfxforgames_softwarebundle")) # read only; never copied into Git
OUT=os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))),"Docs","Research","Assets")
SR=48000
def kw(x):
    # BS.1770 K-weighting at 48k
    b1=[1.53512485958697,-2.69169618940638,1.19839281085285]; a1=[1,-1.69065929318241,0.73248077421585]
    b2=[1,-2,1]; a2=[1,-1.99004745483398,0.99007225036621]
    return lfilter(b2,a2,lfilter(b1,a1,x,axis=0),axis=0)
def db(x): return 20*math.log10(max(x,1e-9))
def probe(p):
    r=subprocess.run(["ffprobe","-v","error","-print_format","json","-show_format","-show_streams","-select_streams","a:0",p],capture_output=True,text=True)
    j=json.loads(r.stdout); s=j["streams"][0]
    bits=s.get("bits_per_raw_sample") or s.get("bits_per_sample") or ""
    if bits in ("0",0): bits=""
    return dict(codec=s.get("codec_name"),sr=int(s.get("sample_rate",0)),ch=int(s.get("channels",0)),bits=bits,dur_probe=float(j["format"].get("duration",0) or 0))
def analyze(rel):
    p=os.path.join(LIB,rel); d=dict(path=rel)
    parts=rel.split("/"); d["top"]=parts[0]; d["sub"]="/".join(parts[1:-1])
    try:
        pr=probe(p); d.update(codec=pr["codec"],sr=pr["sr"],ch=pr["ch"],bits=pr["bits"])
        r=subprocess.run(["ffmpeg","-v","error","-nostdin","-i",p,"-vn","-ac","2","-ar",str(SR),"-f","f32le","-"],capture_output=True)
        raw=np.frombuffer(r.stdout,dtype=np.float32)
        if raw.size<2: raise RuntimeError("no samples "+r.stderr.decode()[:100])
        x=raw[:raw.size//2*2].reshape(-1,2).astype(np.float64)
        dur=len(x)/SR; d["dur"]=round(dur,3)
        peak=np.abs(x).max(); d["peak_db"]=round(db(peak),1)
        m=x.mean(axis=1)
        # integrated loudness (gated)
        k=kw(x); step=int(0.1*SR); blk=int(0.4*SR)
        if len(k)>=blk:
            ms=[]
            for i in range(0,len(k)-blk+1,step):
                ms.append((k[i:i+blk]**2).mean(axis=0).sum())
            ms=np.array(ms); l=-0.691+10*np.log10(np.maximum(ms,1e-12))
            g=ms[l>-70]
            if len(g):
                rel_t=-0.691+10*np.log10(g.mean())-10
                g2=ms[(l>-70)&(l>rel_t)]
                lufs=-0.691+10*np.log10(g2.mean()) if len(g2) else -99
            else: lufs=-99
        else:
            ms_=(k**2).mean(axis=0).sum(); lufs=-0.691+10*np.log10(max(ms_,1e-12))
        d["lufs"]=round(float(lufs),1)
        # envelope 5ms RMS
        h=int(0.005*SR); n=len(m)//h
        if n<2: m=np.pad(m,(0,h*2-len(m))); n=2
        fr=m[:n*h].reshape(n,h); env=np.sqrt((fr**2).mean(axis=1)); envdb=20*np.log10(np.maximum(env,1e-9))
        im=int(np.argmax(env)); d["onset_frac"]=round(im/n,3)
        # attack: time from (peakenv-20dB) up to peak
        j=im
        while j>0 and im-j<200 and envdb[j]>envdb[im]-20: j-=1
        att=(im-j)*5.0
        d["attack_ms"]=round(att,0)
        k30=im
        while k30<n-1 and envdb[k30]>envdb[im]-30: k30+=1
        d["decay_s"]=round((k30-im)*0.005,2)
        d["silence_pct"]=round(100*float((envdb<-55).mean()),1)
        # 100ms level series
        h2=int(0.1*SR); n2=len(m)//h2
        if n2>=2:
            e2=20*np.log10(np.maximum(np.sqrt((m[:n2*h2].reshape(n2,h2)**2).mean(axis=1)),1e-9))
        else: e2=envdb
        act=e2[e2>-55]
        d["rms_db"]=round(float(db(np.sqrt((m**2).mean()))),1)
        d["crest_db"]=round(float(peak and db(peak)-db(np.sqrt((m**2).mean()))),1)
        d["lvl_std_db"]=round(float(act.std()) if len(act)>2 else 0,1)
        t=max(1,len(e2)//3)
        d["rise_db"]=round(float(np.clip(np.mean(e2[-t:])-np.mean(e2[:t]),-60,60)),1)
        # head/tail diff over 0.5s windows
        w=max(1,min(len(e2)//5,5))
        d["headtail_db"]=round(float(min(60,abs(np.mean(e2[:w])-np.mean(e2[-w:])))),1)
        # spectrum: up to 40 sampled 8192 windows
        N=8192
        if len(m)>=N:
            starts=np.linspace(0,len(m)-N,min(40,max(1,(len(m)-N)//N+1))).astype(int)
            win=np.hanning(N); S=np.zeros(N//2+1)
            for s in starts: S+=np.abs(np.fft.rfft(m[s:s+N]*win))**2
        else:
            S=np.abs(np.fft.rfft(np.pad(m,(0,N-len(m)))*np.hanning(N)))**2
        f=np.fft.rfftfreq(N,1/SR); tot=S.sum()+1e-20
        d["centroid_hz"]=int(round(float((f*S).sum()/tot)))
        d["low_pct"]=round(100*float(S[f<250].sum()/tot),0)
        d["mid_pct"]=round(100*float(S[(f>=250)&(f<2500)].sum()/tot),0)
        d["high_pct"]=round(100*float(S[f>=2500].sum()/tot),0)
        Sp=S[1:]+1e-20; d["flatness"]=round(float(np.exp(np.log(Sp).mean())/Sp.mean()),4)
        # tonality: peak share of strongest bin region
        d["tonal_peak"]=round(float(S.max()/tot),3)
        # fade-out / start-from-silence flags
        d["fade_in"]=int(envdb[0]<envdb[im]-30 and im>n*0.0 and (envdb[:max(1,int(0.1*n))].mean()<envdb[im]-25))
        d["tail_db"]=round(float(max(-90,envdb[-max(1,n//20):].mean()-envdb[im])),1)
        d["err"]=""
    except Exception as e:
        d["err"]=str(e)[:150]
    return d
def classify(d):
    if d["err"]: return "decode_fail",""
    dur=d["dur"]; top=d["top"].lower(); path=d["path"].lower()
    musicish=("music" in path or "1984" in top or d["path"].lower().endswith(".mp3") or "kit" in path or "loop" in path and "bpm" in path)
    steady=d["lvl_std_db"]<4 and d["silence_pct"]<10
    loop=dur>=3 and steady and d["headtail_db"]<2.5 and abs(d["rise_db"])<4 and d["tail_db"]>-25
    pulsed=dur>=3 and d["silence_pct"]<10 and d["headtail_db"]<3 and d["tail_db"]>-25 and not steady
    if dur<0.05 or d["silence_pct"]>95: return "near_silent",""
    if "_stm_" in path or "stems" in path: return "music_stem", ("maybe" if d["headtail_db"]<4 else "no")
    if "demo" in path and dur>40: return "music_track",""
    att=d["attack_ms"]; of=d["onset_frac"]
    if musicish and dur>=20: kind="music_track" if dur>60 else "music_loop"
    elif dur>=6 and steady and d["crest_db"]<18 and not musicish:
        kind="bed_tonal_drone" if d["flatness"]<0.003 else "bed_texture"
    elif d["rise_db"]>8 and of>0.55 and dur>=1.5: kind="riser"
    elif att<=40 and d["decay_s"]<=10 and d["crest_db"]>=14 and of<0.25 and dur<=20:
        kind="stinger_hard" if att<=15 else "stinger"
    elif dur<=1.0: kind="short_hit" if att<=30 else "short_sound"
    elif 0.2<of<0.8 and d["rise_db"]>-6 and dur<=8 and att>=40: kind="whoosh_swell"
    elif dur<=10 and of<0.3 and d["tail_db"]<-20: kind="impact_decay" if att<=40 else "one_shot"
    elif dur>=6 and steady: kind="bed_texture"
    elif dur>=6: kind="long_evolving"
    else: kind="one_shot"
    return kind,("yes" if loop else ("maybe" if (pulsed or (dur>=3 and steady and d["headtail_db"]<5)) else "no"))
FIELDS=["path","top","sub","dur","ch","sr","bits","codec","peak_db","lufs","rms_db","crest_db","centroid_hz","low_pct","mid_pct","high_pct","flatness","tonal_peak","onset_frac","attack_ms","decay_s","silence_pct","lvl_std_db","rise_db","headtail_db","tail_db","kind","loopable","err"]
if __name__=="__main__":
    files=[]
    for r,_,fs in os.walk(LIB):
        for f in fs:
            if f.lower().endswith((".wav",".mp3")): files.append(os.path.relpath(os.path.join(r,f),LIB))
    files.sort()
    lim=int(sys.argv[1]) if len(sys.argv)>1 else None
    if lim: files=files[::max(1,len(files)//lim)][:lim]
    rows=[]
    with ProcessPoolExecutor(10) as ex:
        for i,d in enumerate(ex.map(analyze,files,chunksize=8)):
            d["kind"],d["loopable"]=classify(d); rows.append(d)
            if i%200==0: print(i,len(files),flush=True)
    out=os.path.join(OUT,"sfx_catalog.csv" if not lim else "sfx_catalog_test.csv")
    with open(out,"w",newline="") as fh:
        w=csv.DictWriter(fh,FIELDS,extrasaction="ignore"); w.writeheader(); [w.writerow(r) for r in rows]
    print("done",len(rows),"errors",sum(1 for r in rows if r["err"]))
