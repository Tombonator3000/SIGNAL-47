#!/usr/bin/env python3
"""Exercise launcher failure boundaries with harmless fake players, never Unity."""
import json
import os
from pathlib import Path
import subprocess
import tempfile
import time

here = Path(__file__).resolve().parent
results = []
def run(label, args, env=None):
    start = time.monotonic()
    p = subprocess.run(args, capture_output=True, text=True, env=env, timeout=15)
    results.append(dict(test=label, exit=p.returncode, seconds=time.monotonic()-start,
                        stdout=p.stdout, stderr=p.stderr))
    return p.returncode

with tempfile.TemporaryDirectory(prefix='signal47-splat21-') as directory:
    root = Path(directory)
    marker = root/'launched'
    player = root/'fake-player.sh'
    player.write_text('#!/bin/bash\ntouch "'+str(marker)+'"\nexit 7\n')
    player.chmod(0o700)
    def bounded(*args): return ['bash', str(here/'bounded.sh'), *args]
    def launch(backend, out): return ['bash', str(here/'run.sh'), str(player), backend, str(out)]
    assert run('effective-cgroup-limits', bounded('67108864','10','/bin/true')) == 0
    assert run('time-limit', bounded('67108864','2','/bin/sleep','10')) != 0
    assert results[-1]['seconds'] < 9
    assert run('reject-glcore-without-launch', launch('glcore',root/'glcore')) == 2
    assert not marker.exists() and not (root/'glcore').exists()
    env = dict(os.environ, DBUS_SESSION_BUS_ADDRESS='unix:path=/nonexistent/splat21-bus')
    assert run('no-unbounded-fallback', bounded('67108864','10',str(player)),env) != 0
    assert not marker.exists()
    assert run('player-failure-recorded', launch('vulkan',root/'failed')) == 7
    assert marker.exists() and (root/'failed/exit-code.txt').read_text().strip() == '7'
    marker.unlink()
    assert run('preserve-existing-evidence', launch('vulkan',root/'failed')) == 2
    assert not marker.exists() and (root/'failed/exit-code.txt').read_text().strip() == '7'
print(json.dumps({'passed':len(results),'tests':results},indent=2))
