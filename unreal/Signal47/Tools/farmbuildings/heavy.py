#!/usr/bin/env python3
"""Kjør Blender-jobber gjennom prosjektets reservasjon og vakt. Venter når maskinen er opptatt.

  python3 heavy.py <bygg> <clay|mat|back> [visninger] [lodvisning] ...
Flere jobber skilles med '+', og alle kjøres under samme reservasjon:
  python3 heavy.py barn clay + house clay

Stopper aldri andre prosesser. Bryter noen inn i en kjøring, stopper bare vår egen, og vi prøver igjen.
"""
import os
import sys
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parents[1] / 'Scripts'))
from production_lease import HeavyReservation, ReservationBusy  # noqa: E402
from unreal_guard import run_owned  # noqa: E402

ROOT = Path(os.environ.get('FARMB_OUT', Path.home() / 'Documents/ChatGPT/Signal 47/modeller/farmbuildings-2026-11-07'))
WAIT = 4


def jobs(argv):
    out, cur = [], []
    for a in argv:
        if a == '+':
            out.append(cur)
            cur = []
        else:
            cur.append(a)
    out.append(cur)
    return [j for j in out if j]


def main():
    todo = jobs(sys.argv[1:])
    env_extra = {'OMP_NUM_THREADS': '4', 'OPENBLAS_NUM_THREADS': '4', 'PYTHONDONTWRITEBYTECODE': '1'}
    logs = ROOT / 'logs'
    logs.mkdir(parents=True, exist_ok=True)
    started = time.time()
    while todo:
        try:
            with HeavyReservation('farmbuildings: ' + ' '.join(todo[0])) as lease:
                os.environ.update(lease.environment())  # require_idle leser egen prosess.
                env = dict(os.environ, **env_extra)
                while todo:
                    job = todo[0]
                    cmd = ['blender', '-b', '-t', '4', '--python-exit-code', '1', '--python', str(HERE / 'run_blender.py'), '--', str(ROOT)] + job
                    log = logs / ('_'.join(job[:2]) + '.log')
                    print('START', job, flush=True)
                    result = run_owned(cmd, log, timeout=1500, cwd=str(HERE), env=env)
                    print('DONE', job, result['returncode'], f"{result['wall_seconds']:.0f}s", result['stop_reason'], flush=True)
                    if result['stop_reason'] and 'contention' in result['stop_reason']:
                        break  # Noen andre startet. Slipp reservasjonen og prøv igjen.
                    if result['returncode'] != 0:
                        print('FAILED, see', log, flush=True)
                        print(log.read_text()[-3000:], flush=True)
                        return 1
                    todo.pop(0)
        except ReservationBusy:
            pass
        except RuntimeError as e:
            print('opptatt:', str(e)[:200], flush=True)
            time.sleep(30)  # En fremmed prosess uten reservasjon, f.eks. spillet. Ikke maser på låsen.
        finally:
            os.environ.pop('S47_PRODUCTION_TOKEN', None)
            os.environ.pop('S47_PRODUCTION_LOCK_PATH', None)
        if todo:
            if time.time() - started > 3 * 3600:
                print('GAVE UP after 3 h waiting', flush=True)
                return 2
            time.sleep(WAIT)
    print('ALL DONE', flush=True)
    return 0


if __name__ == '__main__':
    sys.exit(main())
