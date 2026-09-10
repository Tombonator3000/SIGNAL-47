#!/usr/bin/env python3
"""Verify physical photo acquisition, saved image, negative cases and comparison."""
import json,shutil
from pathlib import Path
from PIL import Image
root=Path(__file__).resolve().parents[1]/'Artifacts/Pass08'
required={'camera-on-shelf','camera-collected','camera-rejects-before-motor-log','camera-rejects-wrong-subject','camera-valid-viewfinder','photo-captured-and-saved','photograph-deduplicated','photograph-in-notebook','comparison-requires-control-room','photo-log-compared','photo-reopened-after-comparison','restart-clears-camera-state'}
checks={}
for kind in ('Performance','Journey'):
    report=json.loads((root/kind/'journey-result.json').read_text());events={e['checkpoint']:e for e in report['events']}
    checks[kind]=report.get('field_camera') is True and report['outcome']=='PASS' and required<=events.keys()
    if not checks[kind]:continue
    captured=events['photo-captured-and-saved']['state'];photo=Path(captured['photoPath']);saved=json.loads(photo.with_suffix('.json').read_text())
    with Image.open(photo) as image:
        checks[kind+'_jpg']=image.size==(saved['width'],saved['height']) and image.width==960 and max(image.convert('L').getextrema())>40
    checks[kind+'_negative']=not events['camera-rejects-wrong-subject']['state']['photoTaken'] and not events['comparison-requires-control-room']['state']['photoCompared']
    checks[kind+'_compared']=events['photo-log-compared']['state']['photoCompared'] and not events['restart-clears-camera-state']['state']['cameraAcquired']
    folder=root/kind/'ExportedPhoto';folder.mkdir(exist_ok=True);shutil.copy2(photo,folder/'S03.jpg');shutil.copy2(photo.with_suffix('.json'),folder/'S03.json')
    if kind=='Journey':checks['screenshots']=all((root/kind/events[n]['screenshot']).is_file() for n in required)
p=root/'quality-gates.json';g=json.loads(p.read_text());g['field_camera_checks']=checks;g['field_camera']='PASS' if all(checks.values()) else 'FAIL';p.write_text(json.dumps(g,indent=2)+'\n');print(json.dumps(g,indent=2))
assert g['field_camera']=='PASS' and g['native_journey']=='PASS' and g['performance']=='PASS' and g['same_release_build_and_preset'],'A required field-camera gate failed'
