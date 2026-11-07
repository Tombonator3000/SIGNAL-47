#!/usr/bin/env python3
"""Isolert SARO/AUTO HOLD-prøve for PLAY-31-16. Rører verken Toms spill eller lagringer.

Kjører -S47SaroResumeTest (vanlig Continue fra en kopi av krasjøktens auto1, fire pauser, tre
OPS-handlinger, minst 120 s etter AUTO HOLD) i High/Vulkan med egen -UserDir under /tmp, gjennom
HeavyReservation og run_owned. Samler GPU-minne og -belastning hvert sekund utenfra prosessen,
bildetider, Fatal/DEVICE_LOST/VK-linjer og kjernens NVRM/Xid, og gir PASS, FAIL eller INVALID.

  python3 ~/.claude/skills/signal47-nattskift/scripts/q.py \\
    python3 saro_hold_soak.py --build /abs/Linux --execute [--focus-cycles 3] [--video-driver x11]

Uten --execute skrives bare planen. Etterpå: python3 saro_hold_soak.py --analyze /tmp/s47-saro-soak-...
Utgang 0 PASS, 1 FAIL (renderfeil), 2 INVALID (prøven sa ingenting om renderfeilen).

--focus-cycles åpner og lukker et eget Spectacle-vindu i holdvinduet, for det krasjøkten hadde og
den rene prøven mangler: fokus bort og tilbake med ny swapchain. Bruk det bare når Tom ikke sitter ved maskinen.
"""
import argparse
import contextlib
import csv
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import signal
import statistics
import subprocess
import sys
import threading
import time

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parents[1] / 'Scripts'))

FIXTURE = (Path.home() / 'Documents/Signal47-kildearkiv-20261105/scratch-bm/Signal47-saro-crash-20261031'
           / 'Saved/SaveGames/save-1-auto-1.json')
INSTALLED = Path.home() / 'Games/Signal47'
HOLD_SECONDS = 120
STALL_SECONDS = 25      # Testen logger hvert 10. sekund; motoren selv gir opp etter 60 s.
FRAME_FAIL_MS = 10000
FRAME_WARN_MS = 1000
MIN_GPU_SAMPLES = 60
VRAM_WARN_FREE_MIB = 256
GPU_QUERY = ('timestamp,memory.used,memory.total,utilization.gpu,utilization.memory,pstate,'
             'clocks.gr,clocks.mem,power.draw,temperature.gpu')

STAMP = re.compile(r'^\[(\d{4})\.(\d\d)\.(\d\d)-(\d\d)\.(\d\d)\.(\d\d):(\d{3})\]\[\s*\d+\]')
HEARTBEAT = re.compile(r'SARO resume heartbeat ([\d.]+)s afterterminal ([\d.]+)s frame(\d+) stage(\d+)')
FATAL = re.compile(r'Fatal error|appError called|=== Critical error|Unhandled Exception|Assertion failed|'
                   r'timed out waiting for RenderThread|Caught signal')
DEVICE = re.compile(r'VK_ERROR_\w+|DEVICE_LOST|DEVICE FAULT REPORT|VkResult=-\d+|Swapchain present failed|'
                    r'Out Of Memory, trying to allocate|Swapchain surface lost')
HANG = re.compile(r'timed out waiting for RenderThread after ([\d.]+) secs')
XID = re.compile(r'Xid \([^)]*\): (\d+)(?:, pid=(\d+), name=([^,]+))?')


def log_time(line):
    match = STAMP.match(line)
    if not match:
        return None
    y, mo, d, h, mi, s, ms = map(int, match.groups())
    return datetime(y, mo, d, h, mi, s, ms * 1000, tzinfo=timezone.utc).timestamp()  # UE logger i UTC.


def scan_log(text):
    facts = dict(sdl_driver=None, fatal=[], device=[], vk_warnings=[], slow_frames=[], heartbeats=[],
                 swapchains=[], outdated=0, pauses=0, last_time=None, hang_seconds=None, hang_at=None,
                 test_ran=False)
    now = None
    for line in text.splitlines():
        now = log_time(line) or now
        if now is not None:
            facts['last_time'] = now
        if 'SARO resume' in line:
            facts['test_ran'] = True
        match = HEARTBEAT.search(line)
        if match and now is not None:
            facts['heartbeats'].append(dict(t=now, play=float(match[1]), after=float(match[2]),
                                            frame=int(match[3]), stage=int(match[4])))
        if 'Using SDL video driver' in line:
            found = re.search(r"driver '(\w+)'", line)
            facts['sdl_driver'] = found[1] if found else None
        if FATAL.search(line) and len(facts['fatal']) < 12:
            facts['fatal'].append(line.strip()[:240])
        hang = HANG.search(line)
        if hang and facts['hang_at'] is None:
            facts['hang_seconds'], facts['hang_at'] = float(hang[1]), now
        if DEVICE.search(line) and len(facts['device']) < 12:
            facts['device'].append(line.strip()[:240])
        if re.search(r'LogVulkanRHI: (Warning|Error)', line) and len(facts['vk_warnings']) < 20:
            facts['vk_warnings'].append(line.strip()[:240])
        if 'Creating new VK swapchain' in line:
            facts['swapchains'].append(now)
        if 'AcquireNextImage() failed' in line:
            facts['outdated'] += 1
        if 'S47: pause opened' in line:
            facts['pauses'] += 1
        if 'S47: slow frames' in line and len(facts['slow_frames']) < 20:
            facts['slow_frames'].append(line.strip()[:240])
    return facts


def heartbeat_gap(facts):
    """Størst opphold mellom hjerteslag, med loggens siste linje som endepunkt."""
    times = [beat['t'] for beat in facts['heartbeats']]
    if not times:
        return None
    times.append(max(times[-1], facts['last_time'] or times[-1]))
    return max(b - a for a, b in zip(times, times[1:])) if len(times) > 1 else 0


def stall_start(facts):
    if facts['hang_at'] is not None and facts['hang_seconds'] is not None:
        return facts['hang_at'] - facts['hang_seconds']
    return facts['heartbeats'][-1]['t'] if facts['heartbeats'] else None


def number(value):
    try:
        return float(value)
    except (TypeError, ValueError):
        return None


def parse_gpu(text):
    rows = []
    for row in csv.reader(text.splitlines(), skipinitialspace=True):
        if len(row) < 10:
            continue
        try:
            stamp = datetime.strptime(row[0].strip(), '%Y/%m/%d %H:%M:%S.%f').timestamp()  # Lokal tid.
        except ValueError:
            continue
        rows.append(dict(t=stamp, used=number(row[1]), total=number(row[2]), gpu=number(row[3]), mem=number(row[4]),
                         pstate=row[5].strip(), clock=number(row[6]), power=number(row[8]), temp=number(row[9])))
    return rows


def summarize_gpu(rows, baseline=None):
    used = [r['used'] for r in rows if r['used'] is not None]
    if not used:
        return dict(samples=0)
    total = next((r['total'] for r in rows if r['total']), None)
    busy = [r['gpu'] for r in rows if r['gpu'] is not None]
    return dict(samples=len(rows), used_peak_mib=max(used), used_median_mib=statistics.median(used), total_mib=total,
                free_min_mib=(total - max(used)) if total else None, baseline_mib=baseline,
                util_median=statistics.median(busy) if busy else None,
                temp_peak=max((r['temp'] for r in rows if r['temp'] is not None), default=None))


def classify_stall(rows, start):
    """Skiller 'GPU regner' fra 'GPU venter'. Andre klienter (KWin) trekker litt, derfor grove grenser."""
    if start is None:
        return None
    during = [r['gpu'] for r in rows if r['t'] > start + 3 and r['gpu'] is not None]
    if len(during) < 5:
        return dict(kind='UKJENT', samples=len(during))
    median = statistics.median(during)
    kind = ('GPU_REGNER: lang eller ikke-terminerende GPU-jobb' if median >= 85 else
            'GPU_VENTER: venter på semafor/fence/compositor, ikke på regning' if median <= 15 else 'BLANDET')
    return dict(kind=kind, util_median=median, samples=len(during))


def parse_pmon(text, pid):
    peak = None
    for line in text.splitlines():
        fields = line.split()
        if line.startswith('#') or len(fields) < 11 or not fields[2].isdigit() or int(fields[2]) != pid:
            continue
        value = number(fields[10])
        if value is not None:
            peak = max(peak or 0, value)
    return peak


def parse_frames(path):
    csv.field_size_limit(1 << 30)  # Unreal skriver svært lange metadatafelt i CSV-profilene.
    with path.open(newline='', errors='replace') as stream:
        reader = csv.reader(stream)
        header = next(reader, [])
        if 'FrameTime' not in header:
            return None
        columns = {name: header.index(name) for name in ('FrameTime', 'GPUTime', 'RenderThreadTime', 'GameThreadTime', 'RHIThreadTime') if name in header}
        values = {name: [] for name in columns}
        for row in reader:
            if row and (row[0].startswith('[') or row[0] == 'EVENTS'):   # Halen er overskrift og metadata.
                continue
            frame = number(row[columns['FrameTime']]) if len(row) > columns['FrameTime'] else None
            if frame is None or frame <= 0:
                continue
            for name, index in columns.items():
                value = number(row[index]) if len(row) > index else None
                if value is not None:
                    values[name].append(value)
    frames = sorted(values['FrameTime'])
    if not frames:
        return None
    def at(share):
        return frames[min(len(frames) - 1, int(len(frames) * share))]
    result = dict(frames=len(frames), mean_ms=statistics.fmean(frames), p50_ms=at(.5), p95_ms=at(.95), p99_ms=at(.99),
                  max_ms=frames[-1], over_35ms=sum(f > 35 for f in frames), over_100ms=sum(f > 100 for f in frames))
    for name in ('GPUTime', 'RenderThreadTime', 'GameThreadTime', 'RHIThreadTime'):
        if values.get(name):
            result[name + '_p95'] = sorted(values[name])[int(len(values[name]) * .95)]
    return result


def scan_kernel(text, game_pid=None):
    result = dict(xid_game=[], xid_other=[], gsp_alloc=0, no_memory=0, oom_kill=[], other_nvrm=0)
    for line in text.splitlines():
        match = XID.search(line)
        if match:
            ours = (match[2] and game_pid and int(match[2]) == game_pid) or (match[3] or '').startswith('Signal47')
            (result['xid_game'] if ours else result['xid_other']).append(line.strip()[:220])
        elif 'GspRmAlloc' in line:
            result['gsp_alloc'] += 1
        elif 'NV_ERR_NO_MEMORY' in line:
            result['no_memory'] += 1
        elif 'oom-kill' in line or 'Out of memory: Killed' in line:
            result['oom_kill'].append(line.strip()[:220])
        elif 'NVRM' in line:
            result['other_nvrm'] += 1
    return result


def crash_reports(profile):
    reports = []
    for context in sorted((profile / 'Saved/Crashes').glob('*/CrashContext.runtime-xml')):
        text = context.read_text(errors='replace')
        found = re.search(r'<ErrorMessage>(.*?)</ErrorMessage>', text, re.S)
        reports.append(dict(dir=str(context.parent), error=(found[1].strip()[:400] if found else 'ukjent')))
    return reports


def verdict(meta, log, result, gpu, frames, kernel, crashes):
    """Ren beslutning. FAIL krever et spor av renderfeil; alt annet uklart er INVALID, aldri PASS."""
    fail, invalid, warn = [], [], []
    run = meta.get('run') or {}
    stop = run.get('stop_reason')
    gap = heartbeat_gap(log)
    focus = meta.get('focus_cycles', 0)

    if log['hang_at'] is not None:
        fail.append(f"renderstans: motoren ga opp etter {log['hang_seconds']:g} s (samme signatur som krasjøkten)")
    if log['fatal']:
        fail.append('Fatal i loggen: ' + log['fatal'][0])
    if log['device']:
        fail.append('Vulkan-feil: ' + log['device'][0])
    if crashes:
        fail.append('krasjmappe: ' + crashes[0]['error'])
    if kernel['xid_game']:
        fail.append('Xid for spillet: ' + kernel['xid_game'][0])
    if kernel['oom_kill']:
        fail.append('OOM-killer: ' + kernel['oom_kill'][0])
    if gap is not None and gap >= STALL_SECONDS:
        fail.append(f'bildestans: {gap:.0f} s uten hjerteslag')
    if frames and frames['max_ms'] >= FRAME_FAIL_MS:
        fail.append(f"enkeltbilde på {frames['max_ms']:.0f} ms")
    code = run.get('returncode')
    if code is not None and code not in (0, 1) and not stop:
        fail.append(f'prosessen endte med kode {code}')

    if stop and 'contention' in stop:
        invalid.append(stop)
    elif stop and not fail:
        invalid.append('stoppet uten renderspor: ' + stop)
    if not log['test_ran']:
        invalid.append('-S47SaroResumeTest ble aldri kjørt; pakken mangler testen eller loggen')
    wanted = meta.get('video_driver')
    if wanted and log['sdl_driver'] and log['sdl_driver'] != wanted:
        invalid.append(f"SDL brukte {log['sdl_driver']}, bestilt {wanted}")
    if wanted and not log['sdl_driver']:
        warn.append('fant ikke SDL-driveren i loggen')
    if gpu.get('samples', 0) < MIN_GPU_SAMPLES:
        invalid.append(f"bare {gpu.get('samples', 0)} GPU-målinger")
    status = (result or {}).get('status')
    after = (result or {}).get('afterTerminalWallSeconds') or 0
    reason = (result or {}).get('reason', '')
    if status == 'PASS':
        if after < HOLD_SECONDS or (result or {}).get('terminalUses', 0) < 1:
            invalid.append(f'testen sa PASS med bare {after:.0f} s etter AUTO HOLD')
        if code != 0:
            invalid.append(f'testen sa PASS, men prosessen endte med kode {code}')
    elif focus and status == 'FAIL' and after >= HOLD_SECONDS and 'remains interactive' in reason:
        warn.append('testens sluttsjekk feilet som ventet: fokustap åpner pausemenyen (S47Flow::Activation)')
    elif not fail:
        invalid.append(f"testen ga {status or 'ingen result.json'}: {reason or 'uten grunn'}")
    if focus and len(meta.get('focus_events', [])) < focus:
        invalid.append(f"bare {len(meta.get('focus_events', []))} av {focus} fokusrunder ble kjørt")
    if focus and not fail:
        hold = meta.get('hold_began')
        fresh = [t for t in log['swapchains'] if t and hold and t > hold]
        if not fresh:
            invalid.append('fokusrundene ga ingen ny swapchain; stimulansen virket ikke')
    if meta.get('performance_status') == 'UNVERIFIED':
        warn.append('annen tung prosess under prøven; ytelsestall gjelder ikke')
    if log['slow_frames']:
        warn.append(f"{len(log['slow_frames'])} 'slow frames'-linjer: " + log['slow_frames'][0])
    if frames and frames['max_ms'] >= FRAME_WARN_MS:
        warn.append(f"enkeltbilde på {frames['max_ms']:.0f} ms")
    if not frames:
        warn.append('ingen CSV-bildetider; hjerteslag er eneste bildedata')
    free = gpu.get('free_min_mib')
    if free is not None and free < VRAM_WARN_FREE_MIB:
        warn.append(f'bare {free:.0f} MiB VRAM ledig på det laveste')
    if kernel['xid_other']:
        warn.append('Xid i annen prosess: ' + kernel['xid_other'][0])
    if kernel['gsp_alloc']:
        warn.append(f"{kernel['gsp_alloc']} GspRmAlloc-linjer under prøven; samme tilstand som i krasjets oppetid")
    if fail:
        return 'FAIL', fail, warn
    return ('INVALID' if invalid else 'PASS'), invalid, warn


def report(session):
    session = Path(session)
    profile = session / 'profile'
    meta = json.loads((session / 'session.json').read_text())
    logs = list((profile / 'Saved/Logs').glob('Signal47.log')) or list((profile / 'Saved/Crashes').glob('*/Signal47.log'))
    source = logs[0] if logs else session / 'run.log'
    log = scan_log(source.read_text(errors='replace') if source.exists() else '')
    result_file = profile / 'Saved/SaroResumeTest/result.json'
    result = None
    with contextlib.suppress(OSError, ValueError):
        result = json.loads(result_file.read_text())
    rows = parse_gpu((session / 'gpu.csv').read_text(errors='replace')) if (session / 'gpu.csv').exists() else []
    gpu = summarize_gpu(rows, meta.get('vram_before_mib'))
    pid = meta.get('game_pid')
    if pid and (session / 'pmon.txt').exists():
        gpu['game_peak_mib'] = parse_pmon((session / 'pmon.txt').read_text(errors='replace'), pid)
    csvs = sorted((profile / 'Saved/Profiling/CSV').glob('*.csv'))
    frames = parse_frames(csvs[-1]) if csvs else None
    kernel = scan_kernel((session / 'kernel.txt').read_text(errors='replace') if (session / 'kernel.txt').exists() else '', pid)
    crashes = crash_reports(profile)
    status, reasons, warnings = verdict(meta, log, result, gpu, frames, kernel, crashes)
    beats = log['heartbeats']
    fps = None
    if result and result.get('afterTerminalWallSeconds'):
        fps = result.get('nativeFramesAfterTerminal', 0) / result['afterTerminalWallSeconds']
    summary = dict(status=status, reasons=reasons, warnings=warnings, sdl_driver=log['sdl_driver'], gpu=gpu,
                   frames=frames, fps_after_hold=fps, stall=classify_stall(rows, stall_start(log)) if status == 'FAIL' else None,
                   heartbeat_max_gap_s=heartbeat_gap(log), heartbeats=len(beats), swapchains=len(log['swapchains']),
                   outdated_swapchains=log['outdated'], pauses=log['pauses'], fatal=log['fatal'], device=log['device'],
                   vk_warnings=log['vk_warnings'], slow_frames=log['slow_frames'], kernel=kernel, crashes=crashes,
                   driver_before=meta.get('driver_before'), test=dict(
                       status=(result or {}).get('status'), reason=(result or {}).get('reason'),
                       after_hold_s=(result or {}).get('afterTerminalWallSeconds'),
                       terminal_uses=(result or {}).get('terminalUses'), pauses=(result or {}).get('pauseOpens'),
                       RHI=(result or {}).get('RHI'), hwrt=(result or {}).get('lumenHardwareRayTracing')),
                   log=str(source), session=str(session),
                   limits='PASS gjelder denne prøven. Den er ikke den 21 minutter lange økten, og lukker ikke PLAY-31-16 alene.')
    (session / 'summary.json').write_text(json.dumps(summary, indent=2, ensure_ascii=False) + '\n')
    text = render(summary)
    (session / 'summary.txt').write_text(text + '\n')
    print(text, flush=True)
    return {'PASS': 0, 'FAIL': 1}.get(status, 2)


def render(s):
    g, f, t = s['gpu'], s['frames'] or {}, s['test']
    lines = [f"=== SARO HOLD SOAK: {s['status']} ===", *('  ' + reason for reason in s['reasons']),
             f"Test: {t['status']} {t['reason'] or ''} | etter AUTO HOLD {t['after_hold_s'] or 0:.0f} s, "
             f"OPS {t['terminal_uses']}, pauser {t['pauses']}, RHI {t['RHI']}, HWRT {t['hwrt']}, SDL {s['sdl_driver']}",
             f"GPU-minne: topp {g.get('used_peak_mib')} / {g.get('total_mib')} MiB, minst ledig {g.get('free_min_mib')} MiB, "
             f"spillet {g.get('game_peak_mib')} MiB, før start {g.get('baseline_mib')} MiB, {g.get('samples')} målinger, GPU-median {g.get('util_median')} %",
             f"Bilder: {s['fps_after_hold'] and round(s['fps_after_hold'], 1)} FPS etter AUTO HOLD, CSV {f.get('frames')} bilder "
             f"p50 {f.get('p50_ms')} p95 {f.get('p95_ms')} p99 {f.get('p99_ms')} maks {f.get('max_ms')} ms, "
             f"over 100 ms {f.get('over_100ms')}, største hjerteslagopphold {s['heartbeat_max_gap_s']} s",
             f"Swapchain: {s['swapchains']} nye, {s['outdated_swapchains']} utdaterte",
             f"Kjerne: Xid spillet {len(s['kernel']['xid_game'])}, andre {len(s['kernel']['xid_other'])}, GspRmAlloc {s['kernel']['gsp_alloc']}, "
             f"NO_MEMORY {s['kernel']['no_memory']} | før start denne oppetiden: {s['driver_before']}"]
    if s['stall']:
        lines.append(f"Under stans: {s['stall']}")
    lines += [f'Merk: {warning}' for warning in s['warnings']]
    lines += ['Fatal: ' + line for line in s['fatal'][:4]] + ['VK: ' + line for line in s['device'][:4]]
    lines += [f"Krasj: {crash['dir']}" for crash in s['crashes']]
    lines.append(f"Sammendrag: {s['session']}/summary.json (i /tmp: kopier ut ved FAIL)")
    if s['status'] == 'PASS':
        lines.append(s['limits'])
    return '\n'.join(lines)


class Watcher(threading.Thread):
    """Følger loggen under kjøringen: tar øyeblikksbilde ved stans og driver eventuelle fokusrunder."""

    def __init__(self, session, executable, focus_cycles, guard):
        super().__init__(daemon=True)
        self.session, self.executable, self.focus_cycles, self.guard = session, str(executable), focus_cycles, guard
        self.halt = threading.Event()
        self.game_pid = None
        self.hold_began = None
        self.focus_events = []
        self.snapshots = 0

    def find_game(self):
        for job in self.guard.heavy_jobs():
            if job['exe'] == self.executable:
                return job['pid']
        return None

    def snapshot(self, silent):
        """Bare lesing. Motorens egen tidsgrense får virke, så krasjrapporten blir komplett."""
        self.snapshots += 1
        lines = [f'stille i {silent:.0f} s, pid {self.game_pid}']
        with contextlib.suppress(OSError, subprocess.SubprocessError):
            lines.append(subprocess.run(['nvidia-smi', '-q', '-d', 'UTILIZATION,MEMORY,CLOCK,PERFORMANCE'],
                                        capture_output=True, text=True, timeout=15).stdout)
        tasks = Path('/proc') / str(self.game_pid) / 'task'
        with contextlib.suppress(OSError):
            for task in sorted(tasks.iterdir()):
                with contextlib.suppress(OSError):
                    state = (task / 'stat').read_text().rsplit(')', 1)[1].split()[0]
                    lines.append(f"{(task / 'comm').read_text().strip():24} {state} {(task / 'wchan').read_text().strip()}")
        with (self.session / f'stall-{self.snapshots}.txt').open('w') as stream:
            stream.write('\n'.join(lines) + '\n')

    def spectacle(self):
        """Eget Spectacle-vindu, bare vår egen prosess stoppes. I krasjøkten fulgte ny swapchain når Spectacle lukket."""
        began = time.time()
        with (self.session / 'spectacle.log').open('a') as log:
            process = subprocess.Popen(['spectacle', '--new-instance', '--fullscreen'],
                                       stdout=log, stderr=subprocess.STDOUT, start_new_session=True)
        self.halt.wait(8)
        with contextlib.suppress(ProcessLookupError):
            os.killpg(process.pid, signal.SIGTERM)
        try:
            process.wait(timeout=8)
        except subprocess.TimeoutExpired:
            with contextlib.suppress(ProcessLookupError):
                os.killpg(process.pid, signal.SIGKILL)
            process.wait(timeout=5)
        self.focus_events.append(dict(began=began, ended=time.time()))

    def run(self):
        log_path = self.session / 'profile/Saved/Logs/Signal47.log'
        next_focus = None
        while not self.halt.wait(1):
            self.game_pid = self.game_pid or self.find_game()
            if not log_path.exists():
                continue
            facts = scan_log(log_path.read_text(errors='replace'))
            beats = facts['heartbeats']
            if not beats:
                continue
            if self.hold_began is None and beats[-1]['after'] > 0:
                self.hold_began = beats[-1]['t'] - beats[-1]['after']
                # Fokusrundene må ligge i holdvinduet, med god margin til sluttsjekken.
                next_focus = time.time() + 12
            silent = time.time() - beats[-1]['t']
            if self.game_pid and ((silent >= STALL_SECONDS and self.snapshots == 0) or (silent >= STALL_SECONDS * 2 and self.snapshots == 1)):
                self.snapshot(silent)
            elif next_focus and len(self.focus_events) < self.focus_cycles and time.time() >= next_focus and silent < STALL_SECONDS:
                self.spectacle()
                next_focus = time.time() + max(4, 90 / max(1, self.focus_cycles) - 8)


def validate_fixture(path):
    data = json.loads(path.read_text())
    state = data.get('state') or {}
    pose = state.get('pose') or {}
    schedule = ((state.get('spine') or {}).get('sc04d') or {}).get('schedule')
    if not (data.get('caseId') == 1 and state.get('checkpoint') == 'nightService' and state.get('area') == 'saro'
            and schedule is False and 600 < pose.get('z', 0) < 800):
        raise ValueError(f'{path} er ikke krasjøktens auto1 (sak 1, nightService, saro, schedule=false, veipose)')
    return hashlib.sha256(path.read_bytes()).hexdigest()


def plan(args):
    build = args.build.resolve()
    executable = build / 'Signal47/Binaries/Linux/Signal47'
    if not executable.is_file():
        raise ValueError('--build må være Linux-mappa med Signal47/Binaries/Linux/Signal47')
    if not args.allow_installed and build.is_relative_to(INSTALLED):
        raise ValueError('--build er Toms installerte spill; bruk en pakkekandidat, eller --allow-installed for en ren leseprøve')
    session = (args.out or Path('/tmp') / ('s47-saro-soak-' + datetime.now().strftime('%Y%m%d-%H%M%S'))).absolute()
    if not session.is_relative_to('/tmp') or session.exists() or any(p.is_symlink() for p in session.parents):
        raise ValueError('Økten må være en ny mappe under /tmp uten symlenker')
    profile = session / 'profile'
    command = [str(executable), 'Signal47', '-UserDir=' + str(profile), '-S47Title', '-S47Open=continue',
               '-S47SaroResumeTest', '-S47Quality=high', '-log', '-NoSplash', '-csvCaptureFrames=20000', '-csvCompression=0',
               '-LogCmds=LogVulkanRHI Verbose']
    if args.windowed:
        width, height = args.windowed.split('x')
        command += ['-windowed', '-ResX=' + width, '-ResY=' + height]
    if args.video_driver != 'auto':
        command.append('-sdlvideodriver=' + args.video_driver)
    return dict(build=str(build), executable=str(executable), session=str(session), profile=str(profile), command=command,
                fixture=str(args.fixture), focus_cycles=args.focus_cycles,
                video_driver=None if args.video_driver == 'auto' else args.video_driver, timeout=args.timeout)


def vram_used():
    with contextlib.suppress(OSError, ValueError, subprocess.SubprocessError):
        out = subprocess.run(['nvidia-smi', '--query-gpu=memory.used', '--format=csv,noheader,nounits'],
                             capture_output=True, text=True, timeout=15).stdout
        return float(out.split()[0])
    return None


def driver_state():
    """Krasjet kom i en oppetid der driveren allerede ga GspRmAlloc-feil. Er tilstanden her nå?"""
    counts = {}
    for name, pattern in (('GspRmAlloc', 'GspRmAlloc failed'), ('NO_MEMORY', 'NV_ERR_NO_MEMORY'), ('Xid', 'NVRM: Xid')):
        out = subprocess.run(['journalctl', '-k', '-b', '--no-pager', '-q', '--grep', pattern], capture_output=True, text=True, timeout=30).stdout
        counts[name] = len(out.splitlines())
    counts['oppetid_timer'] = round(float(Path('/proc/uptime').read_text().split()[0]) / 3600, 1)
    return counts


def execute(args, details):
    from production_lease import HeavyReservation, TOKEN_ENV
    import unreal_guard as guard
    from inhibit_lock import ScreenSaverInhibitor

    sha = validate_fixture(args.fixture)
    if os.environ.get(guard.GRACE_ENV):
        raise RuntimeError('Prøven godtar ikke unntak for installert spill; målingene ville vært forurenset')
    if args.focus_cycles and not shutil.which('spectacle'):
        raise RuntimeError('--focus-cycles krever spectacle')
    if shutil.disk_usage('/tmp').free < 3 * 1024**3:
        raise RuntimeError('Under 3 GiB ledig i /tmp')
    session, profile = Path(details['session']), Path(details['profile'])
    reservation = contextlib.nullcontext() if os.environ.get(TOKEN_ENV) else HeavyReservation('SARO hold soak')
    with reservation as held:
        if held:
            os.environ.update(held.environment())
        guard.require_idle()
        session.mkdir(mode=0o700, parents=True)
        saves = profile / 'Saved/SaveGames'
        saves.mkdir(parents=True)
        shutil.copyfile(args.fixture, saves / 'save-1-auto-1.json')
        if hashlib.sha256((saves / 'save-1-auto-1.json').read_bytes()).hexdigest() != sha:
            raise RuntimeError('Kopien av lagringen avviker fra kilden')
        meta = dict(details, fixture_sha256=sha, vram_before_mib=vram_used(), driver_before=driver_state(),
                    started=datetime.now().astimezone().isoformat())
        with Path(details['executable']).open('rb') as stream:
            meta['binary_sha256'] = hashlib.file_digest(stream, 'sha256').hexdigest()
        env = dict(os.environ)
        if details['video_driver']:
            env.update(SDL_VIDEO_DRIVER=details['video_driver'], SDL_VIDEODRIVER=details['video_driver'])
        began = time.time()
        watcher = Watcher(session, details['executable'], details['focus_cycles'], guard)
        samplers = []
        try:
            with (session / 'gpu.csv').open('w') as gpu:
                samplers.append(subprocess.Popen(['nvidia-smi', '--query-gpu=' + GPU_QUERY, '--format=csv,noheader,nounits', '-l', '1'], stdout=gpu, stderr=subprocess.DEVNULL))
            with (session / 'pmon.txt').open('w') as pmon:
                samplers.append(subprocess.Popen(['nvidia-smi', 'pmon', '-s', 'um', '-d', '1', '-o', 'T'], stdout=pmon, stderr=subprocess.DEVNULL))
            watcher.start()
            with ScreenSaverInhibitor('SARO hold soak'):
                run = guard.run_owned(details['command'], session / 'run.log', timeout=details['timeout'], env=env, cwd=details['build'])
            meta['run'] = {k: run[k] for k in ('returncode', 'wall_seconds', 'stop_reason')}
            meta['performance_status'] = run['performance_status']
            if run['stop_reason'] and 'contention' in run['stop_reason']:
                meta['interrupted_by'] = run['contention']
        finally:
            watcher.halt.set()
            watcher.join(timeout=25)
            for sampler in samplers:   # Bare våre egne nvidia-smi.
                sampler.terminate()
                with contextlib.suppress(subprocess.TimeoutExpired):
                    sampler.wait(timeout=5)
            meta.update(game_pid=watcher.game_pid, hold_began=watcher.hold_began, focus_events=watcher.focus_events, ended=datetime.now().astimezone().isoformat())
            (session / 'session.json').write_text(json.dumps(meta, indent=2) + '\n')
        since = '@' + str(int(began) - 2)
        kernel = subprocess.run(['journalctl', '-k', '--since', since, '--no-pager', '-o', 'short-iso'], capture_output=True, text=True, timeout=60).stdout
        (session / 'kernel.txt').write_text(''.join(line + '\n' for line in kernel.splitlines() if re.search(r'NVRM|oom-kill|Out of memory', line)))
        desktop = subprocess.run(['journalctl', '--since', since, '--no-pager', '-o', 'short-iso', '-t', 'kwin_wayland_wrapper'], capture_output=True, text=True, timeout=60).stdout
        (session / 'kwin.txt').write_text(desktop)
    return report(session)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--build', type=Path, help='Pakket Linux-mappe')
    parser.add_argument('--fixture', type=Path, default=FIXTURE, help='Krasjøktens auto1; leses og kopieres bare')
    parser.add_argument('--out', type=Path, help='Ny øktmappe under /tmp')
    parser.add_argument('--execute', action='store_true', help='Start motoren; ellers bare plan')
    parser.add_argument('--analyze', type=Path, help='Regn sammendraget på nytt for en ferdig økt')
    parser.add_argument('--video-driver', choices=('wayland', 'x11', 'auto'), default='wayland', help='Krasjet var på wayland; x11 er sammenligningen')
    parser.add_argument('--focus-cycles', type=int, default=0, help='Åpne og lukk Spectacle så mange ganger i holdvinduet')
    parser.add_argument('--windowed', help='BxH, f.eks. 2560x1440. Standard er spillets vanlige visning, som i krasjet')
    parser.add_argument('--timeout', type=float, default=480, help='Grense for hele kjøringen, sekunder')
    parser.add_argument('--allow-installed', action='store_true', help='Tillat Toms installerte binær. Lagringene røres uansett ikke')
    args = parser.parse_args()
    if args.analyze:
        return report(args.analyze)
    if not args.build:
        parser.error('--build kreves')
    if not 0 <= args.focus_cycles <= 6:
        parser.error('--focus-cycles må være 0..6')
    details = plan(args)
    if not args.execute:
        validate_fixture(args.fixture)
        print(json.dumps(details, indent=2))
        return 0
    return execute(args, details)


if __name__ == '__main__':
    sys.exit(main())
