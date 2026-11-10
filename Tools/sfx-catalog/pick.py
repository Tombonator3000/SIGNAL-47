import csv,os,sys,re
rows=list(csv.DictReader(open(os.path.join(os.path.dirname(os.path.abspath(__file__)),"..","..","Docs","Research","Assets","sfx_catalog.csv"))))
for r in rows:
    for k in ["dur","peak_db","lufs","rms_db","crest_db","centroid_hz","low_pct","mid_pct","high_pct","flatness","onset_frac","attack_ms","decay_s","silence_pct","lvl_std_db","rise_db","headtail_db","tail_db"]:
        try: r[k]=float(r[k])
        except: r[k]=0.0
def show(title,rs,cols,n=15):
    print("##",title,len(rs))
    for r in rs[:n]:
        print(r["path"].replace("1984 Retro Music Production Bundle","1984")[-88:]," ".join(f"{c}={r[c] if isinstance(r[c],str) else format(r[c],"g")}" for c in cols))
def top(f): return [r for r in rows if f(r)]
