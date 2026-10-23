#!/usr/bin/env python3
"""Fail closed before a heavy Unreal job. Never signal a pre-existing process."""
import argparse
from contextlib import contextmanager
from datetime import datetime, timedelta, timezone
import json
import os
from pathlib import Path
import signal
import subprocess
import time


PATTERN = 'UnrealEditor|AutomationTool|UnrealBuildTool|Signal47'
INSTALLED_EXE = str(Path.home() / 'Games/Signal47/Linux/Signal47/Binaries/Linux/Signal47')
GRACE_ENV = 'S47_INSTALLED_GAME_GRACE'
WAIT_SECONDS = 7200


def utc(value):
    stamp = datetime.fromisoformat(value.replace('Z', '+00:00'))
    if stamp.tzinfo is None:
        raise ValueError('Grace timestamps must include a timezone')
    return stamp.astimezone(timezone.utc)


def grace_policy(now=None):
    """Opt-in exemption for exactly the installed game, only after a fixed two-hour wait."""
    source = os.environ.get(GRACE_ENV)
    if not source:
        return {'enabled': False, 'eligible': False, 'executable': INSTALLED_EXE}
    path = Path(source).expanduser()
    data = json.loads(path.read_text())
    start, deadline = utc(data['started_at']), utc(data['deadline'])
    now = now or datetime.now(timezone.utc)
    if (data.get('version') != 1 or data.get('installed_executable') != INSTALLED_EXE or
            data.get('wait_seconds') != WAIT_SECONDS or deadline - start != timedelta(seconds=WAIT_SECONDS)):
        raise RuntimeError('Invalid installed-game grace policy; cannot exempt any process')
    return {'enabled': True, 'eligible': now >= deadline, 'executable': INSTALLED_EXE,
            'started_at': start.isoformat(), 'deadline': deadline.isoformat(),
            'remaining_seconds': max(0, (deadline - now).total_seconds()), 'source': str(path)}


def split_jobs(jobs, policy):
    allowed, blocked = [], []
    for job in jobs:
        (allowed if policy['eligible'] and job['exe'] == INSTALLED_EXE else blocked).append(job)
    return allowed, blocked


def heavy_command(executable, argv):
    """Classify the executable or interpreter's actual tool, never later wrapper args."""
    name = Path(executable).name
    if name.startswith('UnrealEditor') or name in (
            'Signal47', 'Signal47-Linux-Shipping', 'Signal47-Linux-Development',
            'UnrealBuildTool', 'AutomationTool', 'UnrealBuildTool.exe', 'AutomationTool.exe'):
        return True
    arguments = [os.fsdecode(value) for value in argv if value][1:]
    shell = name in ('sh', 'bash', 'dash', 'zsh', 'ksh', 'fish')
    managed = name in ('dotnet', 'mono', 'mono-sgen')
    if not shell and not managed:
        return False
    if managed and arguments[:1] == ['exec']:
        arguments = arguments[1:]
    value_options = ({'-o', '-O', '--rcfile', '--init-file'} if shell else
                     {'--depsfile', '--runtimeconfig', '--additionalprobingpath', '--fx-version',
                      '--roll-forward', '--roll-forward-on-no-candidate-fx', '--additional-deps',
                      '--config', '--runtime', '--profile'})
    index = 0
    while index < len(arguments):
        value = arguments[index]
        if value == '--':
            index += 1
            break
        if shell and (value == '--command' or (value.startswith('-') and not value.startswith('--') and 'c' in value[1:])):
            return False  # Shell command text is not a script path; inspect the actual child.
        if not value.startswith('-'):
            break
        index += 2 if value in value_options else 1
    if index >= len(arguments):
        return False
    target = Path(arguments[index]).name
    return target == 'RunUAT.sh' if shell else target in (
        'UnrealBuildTool.dll', 'AutomationTool.dll', 'UnrealBuildTool.exe', 'AutomationTool.exe')


def heavy_jobs():
    jobs = []
    for directory in Path('/proc').iterdir():
        if not directory.name.isdigit():
            continue
        try:
            executable = os.readlink(directory / 'exe').removesuffix(' (deleted)')
            argv = (directory / 'cmdline').read_bytes().split(b'\0')
            if heavy_command(executable, argv):
                jobs.append({'pid': int(directory.name), 'exe': executable,
                             'command': ' '.join(os.fsdecode(x) for x in argv if x)})
        except (OSError, ProcessLookupError):
            pass
    return sorted(jobs, key=lambda x: x['pid'])


def available_gib():
    values = dict(line.split(':', 1) for line in Path('/proc/meminfo').read_text().splitlines())
    return int(values['MemAvailable'].split()[0]) * 1024 / 1024**3


def owned_process(pid, root_pid):
    """Accept our private session OR a proven PPID chain back to our launch.

    UAT's cook/editor children may create another session; session membership alone
    does not prove that they are external. An unrelated process never enters this tree.
    """
    if os.getsid(pid) == root_pid:
        return True
    seen = set()
    while pid > 1 and pid not in seen:
        if pid == root_pid:
            return True
        seen.add(pid)
        try:
            fields = (Path('/proc') / str(pid) / 'stat').read_text().rsplit(')', 1)[1].split()
            pid = int(fields[1])  # field 4: PPID; comm may itself contain ')'.
        except FileNotFoundError:
            raise ProcessLookupError(pid)
        except (OSError, ValueError, IndexError):
            return False
    return False


def require_idle():
    memory = subprocess.run(['free', '-g'], text=True, capture_output=True, check=True)
    print(memory.stdout.rstrip(), flush=True)
    check = subprocess.run(['pgrep', '-fa', PATTERN], text=True, capture_output=True, check=False)
    if check.returncode not in (0, 1):
        raise RuntimeError('pgrep failed: ' + check.stderr)
    print('pgrep -fa "' + PATTERN + '":\n' + (check.stdout.rstrip() or '(no matches)'), flush=True)
    available = available_gib()
    jobs = heavy_jobs()
    policy = grace_policy()
    exempted, blocked = split_jobs(jobs, policy)
    if blocked:
        raise RuntimeError('Heavy Unreal job already running; no process was stopped: ' +
                           ', '.join(f"{x['pid']} {x['exe']}" for x in blocked))
    if available < 10:
        raise RuntimeError(f'Need at least 10 GiB MemAvailable; found {available:.3f} GiB. No job started.')
    state = 'CONTENDED: installed game allowed after two-hour grace; FPS UNVERIFIED' if exempted else 'PASS: no heavy Unreal job'
    print(f'Guard {state}; {available:.3f} GiB available.', flush=True)
    return {'available_gib': available, 'heavy_jobs': jobs, 'pgrep': check.stdout,
            'exempted_installed_game': exempted, 'grace_policy': policy,
            'performance_status': 'UNVERIFIED' if exempted else 'UNCONTENDED'}


@contextmanager
def parent_sigterm_as_interrupt():
    previous = signal.getsignal(signal.SIGTERM)
    def interrupted(signum, frame):
        raise InterruptedError('Owned-run wrapper received SIGTERM; stopping only its private child session')
    signal.signal(signal.SIGTERM, interrupted)
    try:
        yield
    finally:
        signal.signal(signal.SIGTERM, previous)


def run_owned(command, log_path, *, timeout, env=None, cwd=None):
    """Run in a private session. On contention/timeout stop only this session's processes.

    A launch script's descendants remain ours even if the immediate parent exits. A new
    external Unreal process ends our test, except for the exact installed game after an
    explicitly enabled two-hour grace. That exception always invalidates FPS evidence.
    """
    before = require_idle()
    began = time.monotonic()
    contention = []
    if before.get('exempted_installed_game'):
        contention.append({'seconds': 0, 'jobs': before['exempted_installed_game'],
                           'allowed_grace': True})
    reason = None
    last_report = began
    process = None
    with parent_sigterm_as_interrupt(), Path(log_path).open('w') as log:
        try:
            process = subprocess.Popen(command, stdout=log, stderr=subprocess.STDOUT,
                                       env=env, cwd=cwd, start_new_session=True)
            while process.poll() is None:
                other = []
                for job in heavy_jobs():
                    try:
                        if not owned_process(job['pid'], process.pid):
                            other.append(job)
                    except ProcessLookupError:
                        pass
                now = time.monotonic()
                exempted, blocked = split_jobs(other, grace_policy())
                if exempted:
                    contention.append({'seconds': round(now - began, 3), 'jobs': exempted, 'allowed_grace': True})
                if blocked:
                    contention.append({'seconds': round(now - began, 3), 'jobs': blocked, 'allowed_grace': False})
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
            if reason and process is not None:
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
            'contention': contention, 'stop_reason': reason, 'preflight': before,
            'performance_status': 'UNVERIFIED' if contention else 'UNCONTENDED'}


def watch_installed(args):
    """Read-only minute polling. Finishing this watcher never launches an Unreal job."""
    path = args.policy.expanduser().resolve()
    path.parent.mkdir(parents=True, exist_ok=True)
    start = utc(args.started_at) if args.started_at else datetime.now(timezone.utc)
    data = {'version': 1, 'started_at': start.isoformat(),
            'deadline': (start + timedelta(seconds=WAIT_SECONDS)).isoformat(),
            'wait_seconds': WAIT_SECONDS, 'installed_executable': INSTALLED_EXE,
            'authorization': 'Explicit user two-hour installed-game exception; never stop installed game'}
    if path.exists():
        existing = json.loads(path.read_text())
        if args.started_at and existing != data:
            raise RuntimeError('Existing policy has a different fixed start; refusing to reset the wait')
    else:
        with path.open('x') as stream:
            stream.write(json.dumps(data, indent=2) + '\n')
    os.environ[GRACE_ENV] = str(path)
    log_path = path.with_suffix('.watch.jsonl')
    while True:
        policy = grace_policy()
        probe = subprocess.run(['pgrep', '-f', 'Games/Signal47/Linux/Signal47/Binaries'],
                               capture_output=True, text=True, check=False)
        if probe.returncode not in (0, 1):
            raise RuntimeError('Installed-game watcher pgrep failed: ' + probe.stderr)
        installed = [job for job in heavy_jobs() if job['exe'] == INSTALLED_EXE]
        event = {'at': datetime.now(timezone.utc).isoformat(), 'pgrep_pattern': 'Games/Signal47/Linux/Signal47/Binaries',
                 'pgrep_pids': probe.stdout.split(), 'installed_pids': [job['pid'] for job in installed],
                 'deadline': policy['deadline'], 'remaining_seconds': round(policy['remaining_seconds'], 3),
                 'status': 'IDLE' if not installed else ('GRACE_EXPIRED' if policy['eligible'] else 'WAITING')}
        with log_path.open('a') as stream:
            stream.write(json.dumps(event) + '\n')
        print(json.dumps(event), flush=True)
        if not installed or policy['eligible']:
            return
        time.sleep(min(60, policy['remaining_seconds']))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest='command', required=True)
    commands.add_parser('require-idle')
    watch = commands.add_parser('watch-installed')
    watch.add_argument('--policy', type=Path, required=True)
    watch.add_argument('--started-at', help='fixed original wait start in ISO8601 with timezone; default now')
    args = parser.parse_args()
    try:
        watch_installed(args) if args.command == 'watch-installed' else require_idle()
    except (RuntimeError, OSError, ValueError, KeyError, subprocess.SubprocessError) as error:
        parser.exit(1, 'Guard BLOCKED: ' + str(error) + '\n')


if __name__ == '__main__':
    main()
