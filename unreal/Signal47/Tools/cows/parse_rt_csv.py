#!/usr/bin/env python3
"""Read completed UE boot CSV, using its final summary header and startup READY log.

No game/API calls. A PASS covers this finite engine-memory sample only.
Example: python3 parse_rt_csv.py <Profile.csv[.gz]> --log <run.log> --out <new.json>
"""
import argparse
import csv
import gzip
import hashlib
import io
import json
import math
from pathlib import Path
import re
import statistics
import sys

# UE's EVENTS and metadata fields may exceed Python's 128 KiB default.
# Use the largest C long accepted by this Python build without truncating data.
csv_field_limit = sys.maxsize
while True:
    try:
        csv.field_size_limit(csv_field_limit)
        break
    except OverflowError:
        csv_field_limit //= 10

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('csv',type=Path)
p.add_argument('--log',type=Path,required=True)
p.add_argument('--out',type=Path,required=True)
p.add_argument('--min-samples',type=int,default=120)
p.add_argument('--margin-frames',type=int,default=30,help='Conservative frames after startup READY before sampling')
p.add_argument('--pool-mib',type=float,default=400.)
p.add_argument('--warning-percent',type=float,default=20.)
a=p.parse_args()
assert not a.out.exists(), 'Refusing evidence overwrite: '+str(a.out)
result={'status':'FAIL','csv':str(a.csv),'log':str(a.log),'errors':[],
        'limits':'Finite world-active engine-tracked RT geometry memory sample. Not total GPU VRAM, full-game stability or pixel acceptance.'}
try:
    raw=a.csv.read_bytes()
    text=(gzip.decompress(raw)if a.csv.suffix=='.gz'else raw).decode('utf-8-sig')
    rows=list(csv.reader(io.StringIO(text)))
    headers=[(i,row)for i,row in enumerate(rows)if row and row[0]=='EVENTS']
    assert headers,'No EVENTS header; not a completed UE CSV'
    summary_index,header=headers[-1]
    footer=','.join(','.join(row)for row in rows[summary_index+1:])
    assert '[HasHeaderRowAtEnd],1' in footer,'No final header/footer marker; capture may be partial'
    def column(suffix,required=True):
        found=[(i,name)for i,name in enumerate(header)if name.lower().endswith(('raytracinggeometry/'+suffix).lower())]
        assert len(found)==1 or not required,('Missing/ambiguous column',suffix,found)
        return found[0]if found else (None,None)
    ai,an=column('TotalAlwaysResidentSizeMB')
    ri,rn=column('TotalResidentSizeMB')
    qi,qn=column('RequestedSizeMB',False)
    log=a.log.read_text(errors='replace')
    complete=[line for line in log.splitlines()if 'Capture Ended. Writing CSV to file' in line and a.csv.name in line]
    assert complete,'No matching completed-write message in log'
    ready=[int(match.group(1))for line in log.splitlines()if 'LogS47Startup' in line and 'startup READY' in line
           for match in [re.search(r'\[\s*(\d+)\].*startup READY',line)]if match]
    assert len(ready)==1,('Expected one startup READY marker',ready)
    assert 'raytracing="1"' in log or re.search(r'RT residency dump dispatched=1 rayTracing=1',log),'Log does not confirm RT enabled'
    # Boot capture includes a dummy init frame. Leaving an additional full margin
    # after READY makes the possible one-frame difference immaterial to this sample.
    first_world_row=ready[0]+a.margin_frames
    data=[]
    for row in rows[:summary_index]:
        if not row or row[0]=='EVENTS' or row[0].startswith('['):continue
        # Dynamic series append columns. Very early rows may not yet have RT data.
        def number(index):
            if index is None or index>=len(row)or not row[index]:return None
            try:return float(row[index])
            except ValueError:return None
        data.append({'always':number(ai),'resident':number(ri),'requested':number(qi)})
    active=[dict(row,capture_row=i)for i,row in enumerate(data)if i>=first_world_row and row['always']is not None and row['resident']is not None]
    assert len(active)>=a.min_samples,('Insufficient world-active RT samples',len(active),a.min_samples,'READY',ready[0],'totalRows',len(data))
    assert all(math.isfinite(x['always'])and math.isfinite(x['resident'])for x in active),'Non-finite memory value'
    assert max(x['resident']for x in active)>0 and max(x['always']for x in active)>0,'Zero-memory capture cannot validate active RT residency'
    always=[x['always']for x in active]
    threshold=a.pool_mib*a.warning_percent/100.
    result.update(csv_sha256=hashlib.sha256(raw).hexdigest(),log_sha256=hashlib.sha256(a.log.read_bytes()).hexdigest(),
                  columns={'always':an,'resident':rn,'requested':qn},startup_ready_frame=ready[0],margin_frames=a.margin_frames,
                  total_capture_rows=len(data),world_active_samples=len(active),first_sample_row=active[0]['capture_row'],last_sample_row=active[-1]['capture_row'],
                  always_resident_mib={'min':min(always),'max':max(always),'last':always[-1],'median':statistics.median(always)},
                  total_resident_mib={'min':min(x['resident']for x in active),'max':max(x['resident']for x in active)},
                  policy={'pool_mib':a.pool_mib,'warning_percent':a.warning_percent,'threshold_mib':threshold,'basis':'Caller must confirm unchanged real runtime cvars; no setting is changed by this parser.'},
                  capture_logged_complete=True,footer_complete=True)
    assert max(always)<threshold,('AlwaysResident still reaches warning threshold',max(always),threshold)
    result['status']='PASS_ENGINE_MEMORY_SAMPLE_ONLY'
except BaseException as error:
    result['errors'].append(repr(error))
finally:
    a.out.parent.mkdir(parents=True,exist_ok=True)
    a.out.write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
raise SystemExit(0 if result['status'].startswith('PASS')else 1)
