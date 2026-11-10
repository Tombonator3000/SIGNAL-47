import csv,sys,os
rows=list(csv.DictReader(open(os.path.join(os.path.dirname(os.path.abspath(__file__)),"..","..","Docs","Research","Assets","sfx_catalog.csv"))))
top=sys.argv[1]; cols=sys.argv[2].split(",") if len(sys.argv)>2 else ["dur","peak_db","lufs","centroid_hz","low_pct","high_pct","onset_frac","attack_ms","silence_pct","lvl_std_db","rise_db","headtail_db","crest_db","kind","loopable"]
for r in rows:
    if r["top"]==top or (top.startswith("~") and top[1:] in r["path"]):
        print(r["path"].split("/",1)[1][-70:]," ".join(f"{c[:6]}={r[c]}" for c in cols))
