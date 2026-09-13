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
shutil.copytree(repo/'Docs/Research/Hybrid23/License',out/'License')
shutil.copy2(repo/'Automation/Splat21/bounded.sh',out/'bounded.sh')
shutil.copy2(Path(__file__).with_name('start-player.sh'),out/'Start-Hybrid23.sh')
(out/'Start-Hybrid23.sh').chmod(0o755)
shutil.copy2(a.identity,out/'build-identity.json')
(out/'LES-MEG.txt').write_text('''SIGNAL / 47 — HYBRID23, ISOLERT MILJØPRØVE

Start: dobbeltklikk Start-Hybrid23.sh eller kjør den i terminalen.
Linux med systemd-brukersesjon, Vulkan og compute-støtte kreves.
Prøven har 2 GiB minnegrense og avsluttes senest etter 15 minutter.

WASD: gå. Mus: se. E: åpne/lukke døren når du ser på den nær nok.
F: lokal lampe. G: bytt generert 3D-mesh / avledede splats.
C: lagre et diagnostisk kamerabilde. Escape: frigjør/lås musepekeren.
Lukk vinduet for å avslutte.

Bildene er prøvens kamerafiler, ikke bevis i SIGNAL47s historie.
Modellen kommer fra GPT2.5-forlegg og Magnific/Tripo v3.1.
Splatsene er samplet fra modellens overflate, ikke trent fra flere
fotografier og ikke et World Labs/Marble-miljø. Interiør og dør er
vanlig Unity-geometri. Dette er ingen ny spillutgave eller standardstarter.

Automatisk prøve: ./Start-Hybrid23.sh --checks /absolutt/ny/resultatmappe
Den styrer spilleren via API; dette verifiserer ikke ekte tastatur/mus.

Pluginens MIT/BSD-lisenser følger i License/. Tjenester/innhold har egne vilkår.
''')
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
