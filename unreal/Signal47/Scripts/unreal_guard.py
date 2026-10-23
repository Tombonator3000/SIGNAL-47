#!/usr/bin/env python3
"""Fail closed before a heavy Unreal job. Never signal a pre-existing process."""
import argparse
import os
from pathlib import Path
import signal
import subprocess
import time


PATTERN = 'UnrealEditor|AutomationTool|UnrealBuildTool|Signal47'


def heavy_jobs():
    jobs = []
    for directory in Path('/proc').iterdir():
        if not directory.name.isdigit():
            continue
        try:
            executable = os.readlink(directory / 'exe').removesuffix(' (deleted)')
            argv = (directory / 'cmdline').read_bytes().split(b'\0')
            names = [os.path.basename(os.fsdecode(x)) for x in argv if x]
            name = Path(executable).name
            # Match executable / actual tool arguments, not Python scripts under Signal47/.
            is_heavy = (name.startswith('UnrealEditor') or name in
                        ('Signal47', 'Signal47-Linux-Shipping', 'Signal47-Linux-Development') or
                        any(x in ('UnrealBuildTool.dll', 'AutomationTool.dll', 'UnrealBuildTool.exe',
                                  'AutomationTool.exe', 'RunUAT.sh') for x in names))
            if is_heavy:
                jobs.append({'pid': int(directory.name), 'exe': executable,
                             'command': ' '.join(os.fsdecode(x) for x in argv if x)})
        except (OSError, ProcessLookupError):
            pass
    return sorted(jobs, key=lambda x: x['pid'])


def available_gib():
    values = dict(line.split(':', 1) for line in Path('/proc/meminfo').read_text().splitlines())
    return int(values['MemAvailable'].split()[0]) * 1024 / 1024**3


def require_idle():
    memory = subprocess.run(['free', '-g'], text=True, capture_output=True, check=True)
    print(memory.stdout.rstrip(), flush=True)
    check = subprocess.run(['pgrep', '-fa', PATTERN], text=True, capture_output=True, check=False)
    if check.returncode not in (0, 1):
        raise RuntimeError('pgrep failed: ' + check.stderr)
    print('pgrep -fa "' + PATTERN + '":\n' + (check.stdout.rstrip() or '(no matches)'), flush=True)
    available = available_gib()
    jobs = heavy_jobs()
    if jobs:
        raise RuntimeError('Heavy Unreal job already running; no process was stopped: ' +
                           ', '.join(f"{x['pid']} {x['exe']}" for x in jobs))
    if available < 10:
        raise RuntimeError(f'Need at least 10 GiB MemAvailable; found {available:.3f} GiB. No job started.')
    print(f'Guard PASS: {available:.3f} GiB available; no heavy Unreal job.', flush=True)
    return {'available_gib': available, 'heavy_jobs': jobs, 'pgrep': check.stdout}


def run_owned(command, log_path, *, timeout, env=None, cwd=None):
    """Run in a private session. On contention/timeout stop only this session's processes.

    A launch script's descendants remain ours even if the immediate parent exits. A new
    external Unreal process invalidates all evidence and ends our test to free the GPU.
    """
    before = require_idle()
    began = time.monotonic()
    contention = []
    reason = None
    last_report = began
    with Path(log_path).open('w') as log:
        process = subprocess.Popen(command, stdout=log, stderr=subprocess.STDOUT,
                                   env=env, cwd=cwd, start_new_session=True)
        try:
            while process.poll() is None:
                other = []
                for job in heavy_jobs():
                    try:
                        if os.getsid(job['pid']) != process.pid:
                            other.append(job)
                    except ProcessLookupError:
                        pass
                now = time.monotonic()
                if other:
                    contention.append({'seconds': round(now - began, 3), 'jobs': other})
                    reason = 'contention: another Unreal process started; evidence invalid'
                    break
                if available_gib() < 2:
                    reason = 'less than 2 GiB available during our test; stopping our session'
                    break
                if now - began > timeout:
                    reason = f'our test exceeded {timeout:g}s timeout'
                    break
                if now - last_report >= 15:
                    print(f'RUN {now-began:.0f}s pid={process.pid} log={log_path}', flush=True)
                    last_report = now
                time.sleep(.5)
        except BaseException:
            reason = 'interrupted; stopped only the session created by this tool'
            raise
        finally:
            if reason:
                try:
                    os.killpg(process.pid, signal.SIGTERM)
                except ProcessLookupError:
                    pass
                try:
                    process.wait(timeout=10)
                except subprocess.TimeoutExpired:
                    try:
                        os.killpg(process.pid, signal.SIGKILL)
                    except ProcessLookupError:
                        pass
                    process.wait(timeout=5)
    return {'returncode': process.returncode, 'wall_seconds': time.monotonic() - began,
            'contention': contention, 'stop_reason': reason, 'preflight': before}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('command', choices=['require-idle'])
    parser.parse_args()
    try:
        require_idle()
    except (RuntimeError, OSError, subprocess.SubprocessError) as error:
        parser.exit(1, 'Guard BLOCKED: ' + str(error) + '\n')


if __name__ == '__main__':
    main()
