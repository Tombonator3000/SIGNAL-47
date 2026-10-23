#!/usr/bin/env python3
"""Record the packaged graphics tour, expand stable Vulkan PSOs, or summarize startup CSV.

Workflow (run sequentially, after the first cook with stable shader keys enabled):
  pso-cache.py record --build ~/scratch-bm/Signal47-build --out /path/to/evidence/pso
  pso-cache.py expand --recordings /path/to/evidence/pso
  Scripts/package-linux.sh

The .spc in Build/Linux/PipelineCaches is an input to the next cook. Recorded caches and
shader-key files stay in the evidence directory. No machine/driver-specific cache is shipped.
"""
import argparse
import csv
import hashlib
import json
import math
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys


PROJECT = Path(__file__).resolve().parents[1]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def no_unreal_job():
    # Keep the required pgrep evidence in stdout. pgrep excludes its own process.
    check = subprocess.run(['pgrep', '-fa', r'AutomationTool|UnrealEditor|UnrealBuildTool|/Signal47( |$|-Linux)'],
                           text=True, capture_output=True, check=False)
    if check.returncode not in (0, 1):
        raise RuntimeError(check.stderr)
    if check.stdout.strip():
        raise RuntimeError('Another Unreal job is running:\n' + check.stdout)
    print('pgrep: no AutomationTool, UnrealEditor, UnrealBuildTool or Signal47 job', flush=True)


def record(args):
    out = args.out.expanduser().resolve()
    out.mkdir(parents=True, exist_ok=True)
    recorded = out / 'recorded'
    recorded.mkdir(exist_ok=True)
    metadata = []
    for quality in args.qualities:
        no_unreal_job()
        run_out = out / quality
        command = [sys.executable, str(PROJECT / 'Scripts/graphics-review.py'), '--build', str(args.build.expanduser().resolve()),
                   '--out', str(run_out), '--quality', quality, '--seconds', str(args.seconds), '--', '-logPSO']
        subprocess.run(command, check=True)
        text = (run_out / 'run.log').read_text(errors='replace')
        if not re.search(r'Vulkan PSO Precaching = 1, PipelineFileCache = 1', text):
            raise RuntimeError('Vulkan did not enable BOTH precaching and the pipeline file cache: ' + str(run_out / 'run.log'))
        caches = sorted((run_out / 'profile').rglob('*.rec.upipelinecache'))
        if not caches:
            raise RuntimeError('No recorded PSOs. Check -logPSO and the run log: ' + str(run_out))
        for source in caches:
            target = recorded / (quality + '_' + source.name)
            shutil.copy2(source, target)
            metadata.append({'quality': quality, 'source': str(source), 'copy': str(target),
                             'bytes': target.stat().st_size, 'sha256': digest(target)})
    (out / 'recordings.json').write_text(json.dumps(metadata, indent=2) + '\n')
    print(f'Recorded {len(metadata)} cache files. Expand with keys from the SAME cook before packaging again.', flush=True)


def expand(args):
    no_unreal_job()
    recordings = args.recordings.expanduser().resolve()
    source = recordings / 'recorded' if (recordings / 'recorded').is_dir() else recordings
    caches = sorted(source.glob('*.rec.upipelinecache'))
    keys_dir = args.keys.expanduser().resolve()
    keys = sorted(keys_dir.glob('*SF_VULKAN_SM6.shk'))
    if not caches or not keys:
        raise RuntimeError(f'Need recordings and stable shader keys from the recorded cook; got {len(caches)} caches, {len(keys)} keys in {keys_dir}')
    keys_copy = recordings / 'keys'
    keys_copy.mkdir(exist_ok=True)
    for path in keys:
        shutil.copy2(path, keys_copy / path.name)
    output = args.output.expanduser().resolve()
    if not output.name.endswith('Signal47_SF_VULKAN_SM6.spc'):
        raise RuntimeError('Output must end in Signal47_SF_VULKAN_SM6.spc so the Linux cooker finds it')
    output.parent.mkdir(parents=True, exist_ok=True)
    engine = Path(os.environ.get('UE_ROOT', str(Path.home() / '.local/opt/UnrealEngine-5.8.4')))
    command = [str(engine / 'Engine/Binaries/Linux/UnrealEditor-Cmd'), str(PROJECT / 'Signal47.uproject'),
               '-run=ShaderPipelineCacheTools', 'expand', str(source / '*.rec.upipelinecache'),
               str(keys_copy / '*.shk'), str(output), '-unattended', '-nullrhi', '-nosplash',
               '-ddc=InstalledNoZenLocalFallback']
    log_path = recordings / 'expand.log'
    with log_path.open('w') as log:
        result = subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=False)
    text = log_path.read_text(errors='replace')
    counts = re.findall(r'Generated (\d+) stable PSOs total', text)
    if result.returncode or not output.is_file() or output.stat().st_size < 64 or not counts or int(counts[-1]) == 0:
        raise RuntimeError(f'Expansion failed or produced no stable PSOs: {log_path}')
    manifest = {'command': command, 'stable_psos': int(counts[-1]), 'output': str(output),
                'bytes': output.stat().st_size, 'sha256': digest(output),
                'inputs': [{'path': str(p), 'bytes': p.stat().st_size, 'sha256': digest(p)} for p in caches + keys]}
    (recordings / 'expanded.json').write_text(json.dumps(manifest, indent=2) + '\n')
    print(f'Expanded {counts[-1]} stable PSOs to {output}. Repackage; verify cook log and runtime cache-open log.', flush=True)


def summary(args):
    with args.csv.expanduser().open(newline='') as stream:
        rows = [{key: float(value) for key, value in row.items()} for row in csv.DictReader(stream)]
    result = {}
    for name, key in [('initial_10s', 'wall_s'), ('visible_10s', 'visible_s')]:
        chosen = [r for r in rows if 0 <= r[key] < 10]
        frames = sorted(r['frame_ms'] for r in chosen)
        bins = [sum(1 for r in chosen if second <= r[key] < second + 1) for second in range(10)]
        result[name] = {'frames': len(chosen), 'frames_each_second': bins,
                        'complete': bool(chosen and max(r[key] for r in chosen) >= 9.9)}
        if frames:
            result[name].update({'mean_ms': sum(frames) / len(frames),
                                 'p95_ms': frames[min(len(frames) - 1, math.ceil(len(frames) * .95) - 1)],
                                 'max_ms': max(frames), 'over_16_667ms': sum(x > 1000 / 60 for x in frames),
                                 'over_33_333ms': sum(x > 1000 / 30 for x in frames),
                                 'over_50ms': sum(x > 50 for x in frames)})
    print(json.dumps(result, indent=2))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest='command', required=True)
    rec = commands.add_parser('record')
    rec.add_argument('--build', type=Path, required=True)
    rec.add_argument('--out', type=Path, required=True)
    rec.add_argument('--qualities', nargs='+', choices=['low', 'high', 'ultra'], default=['low', 'high', 'ultra'])
    rec.add_argument('--seconds', type=int, default=169)
    rec.set_defaults(action=record)
    exp = commands.add_parser('expand')
    exp.add_argument('--recordings', type=Path, required=True)
    exp.add_argument('--keys', type=Path, default=PROJECT / 'Saved/Cooked/Linux/Signal47/Metadata/PipelineCaches')
    exp.add_argument('--output', type=Path, default=PROJECT / 'Build/Linux/PipelineCaches/S47_Signal47_SF_VULKAN_SM6.spc')
    exp.set_defaults(action=expand)
    summ = commands.add_parser('summary')
    summ.add_argument('csv', type=Path)
    summ.set_defaults(action=summary)
    args = parser.parse_args()
    args.action(args)


if __name__ == '__main__':
    main()
