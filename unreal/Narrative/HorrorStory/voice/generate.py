#!/usr/bin/env python3
"""Produce only SC01's five original takes through the existing local pipeline.

All synthesis/cleaning code is reused from EveningShift/voice/generate.py.
The text stays exact; finished WAVs and raw model output remain outside Git.
"""
from pathlib import Path
import importlib.util
import sys

HERE = Path(__file__).resolve().parent
PACK = HERE.parent
HOME_DATA = Path.home() / '.cache/signal47-horror/sc01'
CUE_IDS = ('H01_BRIEF_001', 'H01_BRIEF_002', 'H01_BRIEF_003',
           'SC01_FIRST_REPORT_001', 'SC01_FIRST_REPORT_002')
BASE_PATH = PACK.parent / 'EveningShift/voice/generate.py'
spec = importlib.util.spec_from_file_location('signal47_existing_voice_pipeline', BASE_PATH)
base = importlib.util.module_from_spec(spec)
spec.loader.exec_module(base)
original_cues = base.cues
base.HERE = HERE
base.PACK = PACK
base.HOME_DATA = HOME_DATA
base.__file__ = str(Path(__file__).resolve())


def cues():
    rows = {r['line_id']: r for r in original_cues()}
    missing = set(CUE_IDS) - rows.keys()
    if missing:
        raise ValueError(f'SC01 original recording cues missing: {sorted(missing)}')
    return [rows[k] for k in CUE_IDS]


base.cues = cues
for name in ('RATE', 'PEAK_CAP_DBFS', 'TARGET_ACTIVE_RMS_DBFS', 'metrics',
             'atomic_json', 'integrated_lufs', 'api_generate', 'bind_profiles'):
    globals()[name] = getattr(base, name)

if __name__ == '__main__':
    base.main()
