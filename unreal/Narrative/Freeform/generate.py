#!/usr/bin/env python3
"""Validate authored Freeform event definitions and embed them for packaged builds."""
from pathlib import Path
import json
root=Path(__file__).resolve().parent
data=json.loads((root/'events.json').read_text())
assert data['version']==1
ids=set()
for event in data['events']:
 assert event['id'] not in ids and event['day']>=1 and event['duration']>0
 assert event['family'] in ('minor','instrument','maintenance','investigation','threat','major')
 assert event['action'] in ('compare','field','service','photo','feeder','escape','telex','control','sample')
 ids.add(event['id'])
assert len(ids)==16
(root/'generated/FreeformEvents.inl').write_text('// Generated from ../events.json. Rebuild with Narrative/Freeform/generate.py.\nstatic const TCHAR* FreeformEventsJson=TEXT(R"S47JSON('+(root/'events.json').read_text()+')S47JSON");\n')
print('16 Freeform events validated and embedded')
