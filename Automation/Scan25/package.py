#!/usr/bin/env python3
"""Package and verify the exact successfully built experiment; no publication or promotion."""
import argparse
import json
from pathlib import Path
import shutil
import tarfile
from identity import aggregate, digest, files

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--player',required=True,type=Path)
p.add_argument('--identity',required=True,type=Path)
p.add_argument('--output',required=True,type=Path)
a=p.parse_args();repo=Path(__file__).resolve().parents[2]
identity=json.loads(a.identity.read_text())
actual=files(a.player.resolve(),['.'])
if actual!=identity['player_files']:raise SystemExit('Player bytes differ from successful build stamp')
out=a.output.resolve()
if out.exists() or out.with_suffix('.tar.gz').exists():raise SystemExit('Refusing to replace a package')
shutil.copytree(a.player,out)
shutil.copytree(repo/'Docs/Research/Scan25/License',out/'License')
shutil.copy2(repo/'Automation/Splat21/bounded.sh',out/'bounded.sh')
shutil.copy2(Path(__file__).with_name('start-player.sh'),out/'Start-Scan25.sh')
(out/'Start-Scan25.sh').chmod(0o755)
shutil.copy2(a.identity,out/'build-identity.json')
(out/'LES-MEG.txt').write_text("""SIGNAL / 47 — SCAN25, TRENT STEINSKANN VED STATION 01

Start Start-Scan25.sh. Prøven åpner ved steinen. Linux/Vulkan kreves.
WASD: gå. Mus: se. E: åpne/lukke døren når du ser på den nær nok.
F: lokal lampe. G: vis/skjul steinskannen. C: diagnostisk kamerabilde.
Escape frigjør/låser musen. Lukk vinduet for å avslutte.

Steinen er Loop CEs fotograferte og trente «rock», CC BY 4.0.
Den er beskåret og renset; opprinnelige parametre i beholdte splats er bevart.
Originalt innbakt lys er beholdt. Dør, gulv og kollisjon er vanlig 3D.
Dette er en isolert miljøprøve, ingen ny standardutgave eller ferdig historie.
Bildene lagres i prøvens egen mappe og er ikke bevis i SIGNAL47s historie.

Automatiske kontroller: ./Start-Scan25.sh --checks /absolutt/ny/resultatmappe
Ytelsesprøve: ./Start-Scan25.sh --benchmark /absolutt/ny/resultatmappe
API-kontrollene verifiserer ikke ekte tastatur/mus. Minnegrense: 2 GiB.
Ytelsesprøven bruker 10 sekunder oppvarming og 60 sekunder kamerabevegelse.
Vanlig manuell prøve avsluttes senest etter 15 minutter.

Opphav og bruksrettigheter: License/ATTRIBUTION.md og CC BY 4.0-teksten.
Pluginens MIT/BSD-notiser følger i samme mappe.
""")
payload=files(out,['.'])
(out/'package-manifest.json').write_text(json.dumps({'files':payload,'sha256':aggregate(payload)},indent=2)+'\n')
archive=out.with_suffix('.tar.gz')
with tarfile.open(archive,'w:gz') as t:t.add(out,arcname=out.name)
# Read and hash every file through the archive without executing anything.
with tarfile.open(archive) as t:
    for name,expected in payload.items():
        import hashlib
        f=t.extractfile(out.name+'/'+name)
        if f is None or hashlib.file_digest(f,'sha256').hexdigest()!=expected:raise SystemExit('Archive payload mismatch: '+name)
print(json.dumps({'directory':str(out),'archive':str(archive),'archive_sha256':digest(archive),'files':len(payload),'payload_sha256':aggregate(payload)},indent=2))
