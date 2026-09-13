#!/usr/bin/env python3
"""Reuse the verified isolated Unity package setup, then install the trained-scan probe."""
import argparse,json,shutil,subprocess,sys
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--crop',required=True,type=Path);p.add_argument('--output',required=True,type=Path)
p.add_argument('--package-repo',required=True,type=Path);p.add_argument('--webp-repo',required=True,type=Path)
a=p.parse_args();repo=Path(__file__).resolve().parents[2];d=json.loads((a.crop/'crop.json').read_text())
subprocess.run([sys.executable,str(repo/'Automation/Hybrid23/prepare.py'),'--package-repo',str(a.package_repo),
 '--webp-repo',str(a.webp_repo),'--webp-revision','9818db6327e09d399bf4fb84da4ed19f03b565b4',
 '--output',str(a.output),'--ply',str(a.crop/'rock-trained.ply'),'--data-origin','Loop CE / rock / trained photo reconstruction / CC BY 4.0',
 '--source-coordinates','RUB','--max-splats','800000','--max-input-mib','200'],check=True)
config=a.output/'Assets/Resources/Hybrid23Config.json';c=json.loads(config.read_text())
c.update({'basis':[v for row in d['worldToLocalRUF'] for v in row],'sourceCenter':dict(zip('xyz',d['center']))})
(config.parent/'Scan25Config.json').write_text(json.dumps(c,indent=2)+'\n');config.unlink()
for name,target in [('Build','Editor'),('Runtime','Probe')]:
 (a.output/'Assets'/target/('Hybrid23'+name+'.cs')).unlink()
 shutil.copy2(Path(__file__).with_name('Scan25'+name+'.cs'),a.output/'Assets'/target)
preparation=a.output/'preparation.json';data=json.loads(preparation.read_text());data['config']=c;data['trained_scan']=d
preparation.write_text(json.dumps(data,indent=2)+'\n')
