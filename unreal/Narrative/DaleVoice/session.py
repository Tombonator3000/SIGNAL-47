#!/usr/bin/env python3
"""One GPU session for Dale's takes, entirely inside the heavy production reservation.

VoiceStudio holds about 5 GB of VRAM while it runs, which is enough to make Unreal run out
of video memory (14:26, 9 November). So the server runs only while this script holds the
HeavyReservation (Signal47/Scripts/production_lease.py): take the lock, start VoiceStudio
headless, generate the missing takes, check every take with ASR, retake the ones that
differ (--until-asr), check again, sync the takes for the game, then stop the server and
its children. Only after that is the lock released. The raw bank is imported last, on the CPU.

  python3 ~/.claude/skills/signal47-nattskift/scripts/q.py python3 session.py

While another job holds the lock, or the game or an Unreal editor runs, it prints
ReservationBusy and exits 75, so q.py tries again later. It refuses to use or stop a
VoiceStudio that is already listening on the port: that one is not ours to stop.
"""
from pathlib import Path
import argparse
import json
import os
import signal
import subprocess
import sys
import time
import urllib.request

HERE = Path(__file__).resolve().parent
UNREAL = HERE.parent.parent
sys.path.insert(0, str(UNREAL / 'Signal47/Scripts'))
import production_lease  # noqa: E402

BACKEND = Path.home() / '.config/VoiceStudio/runtime/project/backend'
PYTHON = BACKEND.parent / '.venv/bin/python'
ENDPOINT = 'http://127.0.0.1:3900'
LOG = Path.home() / '.cache/signal47-dale-voice/voicestudio-session.log'


def say(**value):
    print(json.dumps(value, ensure_ascii=False), flush=True)


def listening():
    try:
        urllib.request.urlopen(ENDPOINT + '/profiles', timeout=3).read()
        return True
    except OSError:
        return False


def busy_process():
    sys.argv = [sys.argv[0]]
    import generate  # noqa: E402  (DaleVoice/generate.py: the game or a live Unreal process)
    return generate.busy_process()


def run(env, *args, check=True):
    say(step=' '.join(args))
    result = subprocess.run([sys.executable, '-B', *args], cwd=HERE, env=env, text=True, capture_output=True)
    tail = (result.stdout + result.stderr).strip().splitlines()[-4:]
    say(returncode=result.returncode, tail=tail)
    if check and result.returncode:
        raise RuntimeError(f'{args[0]} failed ({result.returncode})')
    return result


def asr_failures():
    report = json.loads((HERE / 'verification.json').read_text())
    return report, [k for k in report['failures'] if any(p.startswith('ASR differs') for p in report['takes'][k]['problems'])]


def stop(server):
    if server.poll() is None:
        os.killpg(server.pid, signal.SIGTERM)
        try:
            server.wait(timeout=40)
        except subprocess.TimeoutExpired:
            os.killpg(server.pid, signal.SIGKILL)
            server.wait(timeout=20)
    for _ in range(20):
        if not listening():
            break
        time.sleep(1)
    used = subprocess.run(['nvidia-smi', '--query-gpu=memory.used', '--format=csv,noheader'],
                          capture_output=True, text=True).stdout.strip()
    say(server='stopped', listening=listening(), vram_used=used)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--attempts', type=int, default=6)
    args = parser.parse_args()
    reservation = production_lease.HeavyReservation('Dale voice: VoiceStudio session (DaleVoice/session.py)')
    try:
        reservation.acquire()
    except production_lease.ReservationBusy as error:
        print(f'ReservationBusy: {error}', file=sys.stderr)
        return 75
    server = None
    try:
        name = busy_process()
        if name:
            print(f'ReservationBusy: {name} is running', file=sys.stderr)
            return 75
        if listening():
            raise SystemExit('A VoiceStudio is already listening on 3900; not ours to use or stop')
        env = {**os.environ, **reservation.environment()}
        env.pop('SIGNAL47_DALE_VOICE', None)
        LOG.parent.mkdir(parents=True, exist_ok=True)
        server = subprocess.Popen([str(PYTHON), 'main.py'], cwd=BACKEND, env=env, stdout=LOG.open('ab'),
                                  stderr=subprocess.STDOUT, start_new_session=True)
        for _ in range(180):
            if listening():
                break
            if server.poll() is not None:
                raise RuntimeError('VoiceStudio exited during start; see ' + str(LOG))
            time.sleep(1)
        else:
            raise RuntimeError('VoiceStudio did not answer within 180 s')
        say(server='listening', pid=server.pid)
        run(env, 'generate.py')
        run(env, 'verify.py', check=False)
        report, failing = asr_failures()
        if failing:
            run(env, 'generate.py', '--only', *failing, '--until-asr', *failing, '--attempts', str(args.attempts), check=False)
            run(env, 'verify.py', check=False)
            report, failing = asr_failures()
        say(status=report['status'], present=report['present_takes'], missing=report['missing'],
            failures=report['failures'], asr=report['asr_summary'])
        run(env, 'generate.py', '--sync', str(UNREAL / 'Signal47/Content/Audio/DaleVoice'))
    finally:
        if server is not None:
            stop(server)
        reservation.release()
        say(reservation='released')
    # CPU only, after the GPU is free again.
    run(os.environ, 'catalog.py')
    run(os.environ, 'catalog.py', '--import-raw', str(UNREAL / 'Signal47/Content/Audio/Raw'))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
