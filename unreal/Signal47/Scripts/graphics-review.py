#!/usr/bin/env python3
"""Sequential packaged graphics proof. Saved games, driver cache and logs stay in OUT.

Tour: 20 fixed views at night/dawn; startup: capture first engine frames and the ready trace.
Never overlaps another Unreal job. A contention record invalidates performance evidence.
"""
import argparse
import json
import os
from pathlib import Path
import subprocess
import time


def heavy():
    found = []
    for d in Path('/proc').iterdir():
        if not d.name.isdigit():
            continue
        try:
            exe = Path(os.readlink(d / 'exe')).name
            cmd = (d / 'cmdline').read_bytes()
            if exe in ('UnrealEditor', 'UnrealEditor-Cmd', 'Signal47', 'Signal47-Linux-Shipping', 'Signal47-Linux-Development') or (exe == 'dotnet' and any(x in cmd for x in (b'UnrealBuildTool.dll', b'AutomationTool.dll'))):
                found.append(int(d.name))
        except (OSError, ProcessLookupError):
            pass
    return found


def idle():
    deadline = time.monotonic() + 2400
    quiet = None
    last = None
    while time.monotonic() < deadline:
        pids = heavy()
        if pids != last:
            print('waiting for Unreal jobs:' if pids else 'Unreal jobs idle:', pids, flush=True)
            last = pids
        if pids:
            quiet = None
        else:
            quiet = quiet or time.monotonic()
            if time.monotonic() - quiet >= 4:
                return
        time.sleep(1)
    raise RuntimeError('Other Unreal job did not finish within 40 minutes')


def run(args):
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    profile = out / 'profile'
    start = args.build.expanduser().resolve() / 'Linux/Signal47.sh'
    duration = args.seconds or (55 if args.mode == 'startup' else (49 if args.view else 169))
    cmd = [str(start), '-UserDir=' + str(profile), '-unattended', '-NoCrashDialog', '-RenderOffscreen', '-windowed',
           '-ResX=' + str(args.width), '-ResY=' + str(args.height), '-S47SmokeTest', '-S47Jump=chapter1',
           '-S47Quality=' + args.quality, '-S47Seconds=' + str(duration), '-S47Face=room',
           '-ExecCmds=r.VSync 0,t.MaxFPS 0', '-S47Shot=' + str(out / 'final.png')]
    if args.mode == 'tour':
        cmd += ['-S47GraphicsReview=' + str(out), '-S47GraphicsClock=' + args.clock, '-S47GraphicsHold=7']
        if args.view:
            cmd += ['-S47GraphicsView=' + args.view]
    else:
        cmd += ['-csvCaptureFrames=1800', '-S47StartupTrace=' + str(out / 'startup.csv')]
        if args.cold:
            cmd += ['-clearPSODriverCache']
    cmd += args.extra
    env = os.environ.copy()
    env['__GL_SHADER_DISK_CACHE_PATH'] = str(out / 'driver-cache')
    idle()
    begin = time.monotonic()
    contention = []
    print('START', str(out), flush=True)
    with (out / 'run.log').open('w') as log:
        p = subprocess.Popen(cmd, stdout=log, stderr=subprocess.STDOUT, env=env)
        while p.poll() is None:
            other = [x for x in heavy() if x != p.pid]
            # Launcher has a short-lived shell; the heavy executable PID is its child.
            for pid in other[:]:
                try:
                    parent = int((Path('/proc') / str(pid) / 'stat').read_text().split(')')[1].split()[1])
                    if parent == p.pid:
                        other.remove(pid)
                except OSError:
                    pass
            if other:
                contention.append({'seconds': round(time.monotonic() - begin, 3), 'pids': other})
            if time.monotonic() - begin > duration + 240:
                p.terminate()
                p.wait(timeout=30)
                raise RuntimeError('graphics run timed out')
            time.sleep(.5)
    meta = {'command': cmd, 'returncode': p.returncode, 'wall_seconds': time.monotonic() - begin,
            'contention': contention, 'images': sorted(x.name for x in out.glob('*.png'))}
    (out / 'meta.json').write_text(json.dumps(meta, indent=2))
    log = (out / 'run.log').read_text(errors='replace')
    ok = p.returncode == 0 and 'smoke test OK' in log and not any(x in log for x in ('Critical error:', 'Assertion failed:', 'Signal 11 caught'))
    if args.mode == 'tour':
        ok &= 'graphics review complete' in log and (out / 'frames.csv').exists()
    print('END', 'PASS' if ok else 'FAIL', 'contention', len(contention), flush=True)
    return 0 if ok else 1


if __name__ == '__main__':
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--build', type=Path, required=True)
    ap.add_argument('--out', type=Path, required=True)
    ap.add_argument('--mode', choices=['tour', 'startup'], default='tour')
    ap.add_argument('--clock', default='02:15')
    ap.add_argument('--quality', choices=['low', 'high', 'ultra'], default='high')
    ap.add_argument('--view')
    ap.add_argument('--seconds', type=int)
    ap.add_argument('--width', type=int, default=2560)
    ap.add_argument('--height', type=int, default=1440)
    ap.add_argument('--cold', action='store_true')
    ap.add_argument('extra', nargs='*')
    raise SystemExit(run(ap.parse_args()))
