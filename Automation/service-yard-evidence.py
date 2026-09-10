#!/usr/bin/env python3
"""Gate the actual exterior journey on both measured and screenshot runs."""
import json
from pathlib import Path

root=Path(__file__).resolve().parents[1]/'Artifacts/Pass07'
required={'service-investigation-start','service-door-open','walked-outside','service-path','motor-log-filed','motor-log-deduplicated','motor-log-in-notebook','returned-to-control-room','restart-clears-yard'}
checks={}
for kind in ('Performance','Journey'):
    report=json.loads((root/kind/'journey-result.json').read_text())
    events={event['checkpoint']:event for event in report['events']}
    checks[kind]=report.get('service_yard') is True and report['outcome']=='PASS' and required<=events.keys()
    if checks[kind]:
        filed=events['motor-log-filed']['state']
        start=events['service-investigation-start']['state']
        reset=events['restart-clears-yard']['state']
        checks[kind]=filed['evidence']==start['evidence']+1 and events['returned-to-control-room']['state']['yardReturned'] and reset['evidence']==0 and not reset['yardActive'] and not reset['doorOpen']
    if kind=='Journey':checks['exterior_screenshots']=all((root/kind/events[name].get('screenshot','MISSING')).is_file() for name in required if name in events) and required<=events.keys()
p=root/'quality-gates.json';gates=json.loads(p.read_text())
gates['service_yard_checks']=checks;gates['service_yard']='PASS' if all(checks.values()) else 'FAIL'
p.write_text(json.dumps(gates,indent=2)+'\n')
print(json.dumps(gates,indent=2))
assert gates['service_yard']=='PASS' and gates['performance']=='PASS' and gates['native_journey']=='PASS' and gates['same_release_build_and_preset'] and gates['sampled_audio']=='PASS','A required gate failed; preserve evidence and correct before merge'
