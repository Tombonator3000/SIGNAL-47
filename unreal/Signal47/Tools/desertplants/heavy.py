#!/usr/bin/env python3
"""Kjør Blender gjennom prosjektets reservasjon og vakt. Stopper aldri andre prosesser.

  python3 ~/.claude/skills/signal47-nattskift/scripts/q.py python3 heavy.py <modus> [...] [+ <modus> [...]]
Flere jobber skilles med '+' og kjøres under samme reservasjon. Er maskinen opptatt, feiler dette med
ReservationBusy eller opptatt-meldingen fra vakten, og q.py prøver igjen. Bryter noen inn, stopper bare vår egen kjøring.
"""
import os
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parents[1] / 'Scripts'))
from production_lease import HeavyReservation  # noqa: E402
from unreal_guard import run_owned  # noqa: E402

ROOT = Path(os.environ.get('DESERTPLANTS_OUT', Path.home() / 'Documents/ChatGPT/Signal 47/modeller/desertplants-2026-11-07'))


def main():
    jobs, cur = [], []
    for a in sys.argv[1:]:
        if a == '+':
            jobs.append(cur)
            cur = []
        else:
            cur.append(a)
    jobs.append(cur)
    logs = ROOT / 'logs'
    logs.mkdir(parents=True, exist_ok=True)
    with HeavyReservation('desertplants: ' + ' '.join(sys.argv[1:3])) as lease:
        os.environ.update(lease.environment())  # require_idle leser egen prosess.
        env = dict(os.environ, OMP_NUM_THREADS='4', OPENBLAS_NUM_THREADS='4', PYTHONDONTWRITEBYTECODE='1')
        for job in jobs:
            cmd = ['blender', '-b', '-t', '4', '--python-exit-code', '1', '--python', str(HERE / 'run_blender.py'), '--', str(ROOT)] + job
            log = logs / ('_'.join(job[:2]) + '.log')
            print('START', job, flush=True)
            res = run_owned(cmd, log, timeout=1800, cwd=str(HERE), env=env)
            print('DONE', job, res['returncode'], f"{res['wall_seconds']:.0f}s", res['stop_reason'], flush=True)
            if res['stop_reason'] and 'contention' in res['stop_reason']:
                print('contention: noen andre startet. Prøv igjen.', flush=True)
                return 1
            if res['returncode'] != 0:
                print('FEILET, se', log, flush=True)
                print(log.read_text()[-3000:], flush=True)
                return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
