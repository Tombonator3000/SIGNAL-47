#!/usr/bin/env python3
"""Produce HorrorStory takes for one scenario through the existing local pipeline.

All synthesis/cleaning code is reused from EveningShift/voice/generate.py.
The text stays exact; finished WAVs and raw model output remain outside Git.
Without --scenario the original SC01 delivery (five takes) is selected, so the
documented SC01 command keeps producing the same set.
"""
from pathlib import Path
import importlib.util
import json
import sys

HERE = Path(__file__).resolve().parent
PACK = HERE.parent
SC01_CUE_IDS = ('H01_BRIEF_001', 'H01_BRIEF_002', 'H01_BRIEF_003',
                'SC01_FIRST_REPORT_001', 'SC01_FIRST_REPORT_002')
BASE_PATH = PACK.parent / 'EveningShift/voice/generate.py'


def take_scenario(argv):
    """Remove --scenario SCnn from argv; the base parser does not know it."""
    if '--scenario' not in argv:
        return 'SC01', argv
    at = argv.index('--scenario')
    if at + 1 >= len(argv) or not argv[at + 1].startswith('SC'):
        raise SystemExit('--scenario needs an id such as SC02')
    return argv[at + 1], argv[:at] + argv[at + 2:]


SCENARIO, sys.argv = take_scenario(sys.argv)
HOME_DATA = Path.home() / f'.cache/signal47-horror/{SCENARIO.lower()}'
spec = importlib.util.spec_from_file_location('signal47_existing_voice_pipeline', BASE_PATH)
base = importlib.util.module_from_spec(spec)
spec.loader.exec_module(base)
original_cues = base.cues
base.HERE = HERE
base.PACK = PACK
base.HOME_DATA = HOME_DATA
base.__file__ = str(Path(__file__).resolve())


def scenario_cue_ids(scenario):
    """Cue order of the manifest, limited to scenes the registry gives this SC first."""
    registry = json.loads((PACK / 'generated/runtime_registry.json').read_text())
    scenes = {row['id'] for row in registry['scenes'] if row['scenario_ids'][0] == scenario}
    if not scenes:
        raise ValueError(f'unknown scenario {scenario}')
    manifest = json.loads((PACK / 'generated/recording_manifest.json').read_text())
    return tuple(r['line_id'] for r in manifest['recordings']
                 if r['scene_id'] in scenes and r['recording_required'])


def cues():
    wanted = SC01_CUE_IDS if SCENARIO == 'SC01' else scenario_cue_ids(SCENARIO)
    rows = {r['line_id']: r for r in original_cues()}
    missing = set(wanted) - rows.keys()
    if missing:
        raise ValueError(f'{SCENARIO} recording cues missing: {sorted(missing)}')
    casting = json.loads((HERE / 'casting.json').read_text())['profiles']
    uncast = sorted({rows[k]['voice_profile'] for k in wanted} - casting.keys())
    if uncast:
        # A named NPC is never recast silently; the profile must be chosen first.
        raise ValueError(f'{SCENARIO} needs casting.json profiles for: {", ".join(uncast)}')
    return [rows[k] for k in wanted]


base.cues = cues
for name in ('RATE', 'PEAK_CAP_DBFS', 'TARGET_ACTIVE_RMS_DBFS', 'metrics',
             'atomic_json', 'integrated_lufs', 'api_generate', 'bind_profiles'):
    globals()[name] = getattr(base, name)

if __name__ == '__main__':
    base.main()
