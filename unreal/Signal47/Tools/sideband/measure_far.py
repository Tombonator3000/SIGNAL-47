#!/usr/bin/env python3
"""SIDEBAND-20261113: FAR's wet trim per piece of the score, measured.

  measure_far.py [--content DIR] [--write]

A long room rings up far more on a held pad than on a plucked guitar, so FAR's send carries one number per piece (dB), as the
score's table has a gain per piece. For every staged piece (DIR/Audio/Raw/media/music/*.wav) this takes 24 s from two fifths in,
runs the offline stand-in of FAR at full (s47dsp.py: the same chain and numbers as S47Sideband.cpp) and finds the trim at which
the whole is TARGET LU under the dry passage. The four pieces of the prototype keep the numbers Tom heard.
--write: Source/Signal47/Private/S47SidebandTrim.inl (FS47SidebandBus reads it through S47Sideband::FarTrimDb).
"""
import argparse
import glob
import os
import sys

import numpy as np
from scipy.signal import lfilter

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import s47dsp as D  # noqa: E402

HERE = os.path.dirname(os.path.abspath(__file__))
PROJECT = os.path.dirname(os.path.dirname(HERE))
TARGET = -5.5
HEARD = {'s47_cold_dish': -10.0, 'look_back_steelguitar': -6.5, 's47_night_freight': -3.0, 's47_carrier_lock': -6.0}
_K1 = ([1.53512485958697, -2.69169618940638, 1.19839281085285], [1.0, -1.69065929318241, 0.73248077421585])
_K2 = ([1.0, -2.0, 1.0], [1.0, -1.99004745483398, 0.99007225036621])


def lufs(x):
    k = lfilter(_K2[0], _K2[1], lfilter(_K1[0], _K1[1], x, axis=0), axis=0)
    return float(-0.691 + 10 * np.log10((k ** 2).sum(axis=1).mean() + 1e-12))


def far_parts(x, P=D.FAR):
    """FAR at full as (what is left dry, everything wet at a trim of 0 dB)."""
    n = len(x)
    f = np.ones(n)
    dry = D.biquad(x * (1 - (1 - P['dry_left']) * f ** 2)[:, None], 'lowpass', P['dry_lp'][1], -3.0)
    dry = D.panner(dry, P['dry_pan'][1] * f * D.sine(n, P['dry_pan'][0]))
    send = D.biquad(D.biquad(x.mean(axis=1), 'highpass', P['send_hp'], -3.0), 'lowpass', P['send_lp'], -3.0)
    tA, tB, fb, lp, pos, lvl = P['pp']
    A, B = D.pingpong(send, tA, tB, fb, lp)
    wet = (D.place(A, -pos) + D.place(B, pos)) * lvl
    r = D.delay(send + P['pp_to_rev'] * (A + B), P['predelay'])
    for secs, g in P['ap']:
        r = D.allpass(r, secs, g)
    o = D.fdn(r, P['lines'], P['rt60'], P['shelf'], P['mod'])
    sg = np.array([1, 1, -1, -1, 1, 1, -1, -1])[:, None]
    rev = D.place((o[0::2] * sg[0::2]).sum(axis=0), -0.8) + D.place((o[1::2] * sg[1::2]).sum(axis=0), 0.8)
    return dry, wet + D.biquad(rev, 'lowpass', P['rev_lp'], -3.0) * P['rev']


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--content', default=os.path.join(PROJECT, 'Content'))
    ap.add_argument('--write', action='store_true')
    args = ap.parse_args()
    files = sorted(glob.glob(os.path.join(args.content, 'Audio', 'Raw', 'media', 'music', '*.wav')))
    if not files:
        sys.exit('S47 sideband: no staged music under %s' % args.content)
    rows = []
    for path in files:
        name = os.path.splitext(os.path.basename(path))[0]
        secs = float(os.popen('ffprobe -v error -show_entries format=duration -of csv=p=0 "%s"' % path).read().strip() or 0)
        if secs < 40:
            continue
        x = D.load(path, secs * 0.4, 24.0)
        dry, wet = far_parts(x)
        skip = 6 * D.SR  # past the room's rise
        ref = lufs(x[skip:])
        lo, hi = -20.0, 6.0
        for _ in range(24):
            mid = (lo + hi) / 2
            if lufs((dry + wet * D.db(mid))[skip:]) - ref > TARGET:
                hi = mid
            else:
                lo = mid
        found = round((lo + hi) / 2 * 2) / 2
        trim = HEARD.get(name, float(np.clip(found, -14.0, 0.0)))
        got = lufs((dry + wet * D.db(trim))[skip:]) - ref
        rows.append((name, trim))
        print('S47 sideband: %-32s trim %5.1f dB  (measured %5.1f)  FAR at full %5.1f LU against dry%s' % (
            name, trim, found, got, '  [as heard]' if name in HEARD else ''), flush=True)
    if args.write:
        dst = os.path.join(PROJECT, 'Source', 'Signal47', 'Private', 'S47SidebandTrim.inl')
        with open(dst, 'w', encoding='utf-8') as f:
            f.write('// FAR\'s wet trim per staged piece, dB (written by Tools/sideband/measure_far.py; the file\'s base name)\n')
            for name, trim in rows:
                f.write('{ TEXT("%s"), %.1f },\n' % (name, trim))
        print('S47 sideband: %d rows in %s' % (len(rows), dst))


if __name__ == '__main__':
    main()
