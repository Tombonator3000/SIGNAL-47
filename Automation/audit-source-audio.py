#!/usr/bin/env python3
"""Measure supplied game source clips. Does NOT claim a listener-mix or listening test.
Requires NumPy and soundfile in the analysis environment, not the Unity player.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path

import numpy as np
import soundfile as sf

ROOT = Path(__file__).resolve().parents[1]
CLIPS = [
    ('Kenney/click_001.ogg', .35, .22),
    ('Kenney/switch_003.ogg', .35, .22),
    ('PreparedAudio/CeramicBreak.wav', .85, .70),
    ('PreparedAudio/DesertWind.wav', .24, .16),
    ('PreparedAudio/PhoneRing.wav', 1., 1.),
    ('PreparedAudio/Printer.wav', .25, .25),
    ('PreparedAudio/TitleMusic.ogg', .28, .20),
]
METHOD = ('Decoded source audio files, all interleaved samples incl silence; sample peak and unweighted RMS; '
          'constant source gains applied mathematically. No Unity distance attenuation, concurrent-source sum, '
          'OS output, LUFS, true-peak or subjective listening evaluated. Vorbis decode may differ slightly from Unity FMOD.')


def dbfs(value):
    return round(20 * math.log10(value), 3) if value > 0 else None


def measure():
    rows = []
    for name, old, new in CLIPS:
        path = ROOT / 'Unity/Assets/Signal47/Art/ThirdParty' / name
        data, rate = sf.read(path, dtype='float64', always_2d=True)
        if not data.size or not np.isfinite(data).all():
            raise ValueError(f'Invalid audio samples in {path}')
        peak = float(np.max(np.abs(data)))
        rms = float(np.sqrt(np.mean(data * data)))
        rows.append(dict(file=str(path.relative_to(ROOT)), sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
                         sample_rate=rate, channels=data.shape[1], sample_frames=data.shape[0], duration_s=data.shape[0]/rate,
                         peak=peak, rms=rms, source_peak_dbfs=dbfs(peak), source_rms_dbfs=dbfs(rms),
                         samples_at_or_above_one=int(np.count_nonzero(np.abs(data) >= 1)), old_gain=old, new_gain=new,
                         gain_change_db=dbfs(new/old), old_isolated_peak_dbfs=dbfs(peak*old),
                         new_isolated_peak_dbfs=dbfs(peak*new), new_isolated_rms_dbfs=dbfs(rms*new)))
    return dict(method=METHOD, clips=rows, status='SOURCE_LEVELS_MEASURED; RUNTIME_MIX_AND_LISTENING_UNVERIFIED')


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--output', type=Path, default=ROOT/'Docs/Evidence/ControlRoomPass06/Audio/source-levels.json')
    args = parser.parse_args()
    result = measure()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2)+'\n')
    print('Measured', len(result['clips']), 'unchanged source clips:', args.output)
