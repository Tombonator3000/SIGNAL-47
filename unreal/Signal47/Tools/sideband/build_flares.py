#!/usr/bin/env python3
"""SIDEBAND-20261113: FLARES' baked sounds (nine stingers, three beds), made from the bought bundle and staged out of Git.

  build_flares.py [--bundle DIR] [--content DIR] [--check]

Writes DIR/Audio/Raw/media/sideband/<name>.wav (PCM16; stingers at 48 kHz, beds at 12 kHz: they never pass more than 1 kHz in
the game), where FS47SidebandBus::LoadFlares reads them at first use (S47Sideband.cpp). Without them the colour bus still works
and FLARES is silent with no error. --check: bake and measure, write nothing.

The game never plays a bundle file as it is. A stinger is trimmed to its front, layered where the table says, narrowed (mono-safe),
put through the same bus recipe as the game's own mixes (30 Hz highpass, 9.5 kHz lowpass, the tape curve), shaped, shortened and
set to the score's staged level at its loudest 400 ms; a bed becomes a 13.8 s loop that meets itself, mono under 150 Hz, -27 LUFS.
The palette and the numbers are the prototype's that Tom heard on 13 November 2026 (Docs/Design/Audio/SIDEBAND_20261113.md).

Licence: Function Loops, "Ultimate SFX for Games" bundle. Tom confirmed on 13 November 2026 that the licence allows commercial use.
The raw bundle files never go into Git and are never staged; only these baked forms ship (Content/Audio is ignored by Git).
Needs python3 with numpy and scipy, and ffmpeg (on this machine: /home/tom/stable-diffusion-webui-forge/venv/bin/python3).
"""
import argparse
import json
import os
import subprocess
import sys

import numpy as np
from scipy.signal import lfilter, resample_poly

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import s47dsp as D  # noqa: E402  (the offline stand-ins for the engine's nodes, and the four colours)

HERE = os.path.dirname(os.path.abspath(__file__))
PROJECT = os.path.dirname(os.path.dirname(HERE))
BUNDLE = os.path.expanduser('~/Desktop/SIGNAL 47/bt25_ultimatesfxforgames_softwarebundle')
SR = D.SR
SCORE_LUFS = -21.5  # the staged level of every piece of the score: the reference for every level
BED_RATE = 12000

# ------------------------------------------------------------------ meters (BS.1770 K-weighting at 48 kHz)
_K1 = ([1.53512485958697, -2.69169618940638, 1.19839281085285], [1.0, -1.69065929318241, 0.73248077421585])
_K2 = ([1.0, -2.0, 1.0], [1.0, -1.99004745483398, 0.99007225036621])


def kweight(x):
    return lfilter(_K2[0], _K2[1], lfilter(_K1[0], _K1[1], x, axis=0), axis=0)


def max_momentary(x, win=0.4, hop=0.05):
    k = kweight(x if x.ndim == 2 else np.stack([x, x], axis=1))
    p = (k ** 2).sum(axis=1)
    c = np.concatenate([[0.0], np.cumsum(p)])
    w, h = int(win * SR), int(hop * SR)
    if len(p) < w:
        return float(-0.691 + 10 * np.log10(p.mean() + 1e-12))
    e = np.arange(w, len(p) + 1, h)
    return float((-0.691 + 10 * np.log10((c[e] - c[e - w]) / w + 1e-12)).max())


def corr(x):
    d = np.sqrt((x[:, 0] ** 2).sum() * (x[:, 1] ** 2).sum())
    return float((x[:, 0] * x[:, 1]).sum() / d) if d > 0 else 1.0


# ------------------------------------------------------------------ the bakes
def load(bundle, rel):
    p = os.path.join(bundle, rel)
    if not os.path.isfile(p):
        sys.exit('S47 sideband: not in the bundle: ' + rel)
    return D.load(p)


def narrow(x, want=0.3, low_mono_hz=150.0):
    """Mono-safe: nothing on the sides under low_mono_hz, and the sides turned down until left and right are alike by `want`."""
    mid, side = x.mean(axis=1), (x[:, 0] - x[:, 1]) / 2
    side = D.biquad(D.biquad(side, 'highpass', low_mono_hz, -3.0), 'highpass', low_mono_hz, -3.0)
    M, S = (mid ** 2).sum(), (side ** 2).sum()
    k = 1.0 if S <= 0 else min(1.0, np.sqrt(max(0.0, M * (1 - want) / (S * (1 + want)))))
    return np.stack([mid + k * side, mid - k * side], axis=1)


def bus_recipe(x, work_db=-18.0):
    """What every one of the game's own mixes went through (Tools/music/build_music.py): 30 Hz highpass, 9.5 kHz lowpass, the tape
    curve at a working level (the full tier's stingers are baked 6 dB hotter into it, which rounds their fronts)."""
    y = D.biquad(x, 'highpass', 30.0, -3.0)
    y = D.biquad(D.biquad(y, 'lowpass', 9500.0, -3.0), 'lowpass', 9500.0, -3.0)
    g = D.db(work_db - max_momentary(y))
    y = y * g
    return np.tanh(y * 1.6) / 1.6 / g


def trim_start(x, under_db=-45.0, lead=0.004):
    a = np.abs(x).max(axis=1)
    first = int(np.argmax(a > a.max() * 10 ** (under_db / 20)))
    return x[max(0, first - int(lead * SR)):]


def shape(x, length, fade_in=0.003, fade_out=1.5, decay_from=None, decay_db_per_s=0.0):
    """Length, edges (raised cosine: no click at either end) and, if asked, a faster dying away from decay_from seconds."""
    n = int(length * SR)
    y = np.zeros((n, 2))
    m = min(n, len(x))
    y[:m] = x[:m]
    t = np.arange(n) / SR
    g = np.ones(n)
    if decay_from is not None:
        g *= 10 ** (-decay_db_per_s * np.maximum(t - decay_from, 0) / 20)
    g *= np.sin(np.clip(t / fade_in, 0, 1) * np.pi / 2) ** 2
    g *= np.sin(np.clip((length - t) / fade_out, 0, 1) * np.pi / 2) ** 2
    return y * g[:, None]


def bake_stinger(bundle, parts, length, fade_in=0.003, fade_out=1.5, decay_from=None, decay_db_per_s=0.0, work_db=-18.0):
    """parts: [(file in the bundle, dB relative to the first part at their loudest, seconds late)]. The stinger at SCORE_LUFS at
    its loudest 400 ms: a gain of 0 dB in the game = as loud, at its loudest, as the score is on average."""
    out = np.zeros((int(length * SR) + SR, 2))
    ref = None
    for rel, rel_db, late in parts:
        x = trim_start(load(bundle, rel))
        mm = max_momentary(x)
        ref = mm if ref is None else ref
        x = x * D.db(ref + rel_db - mm)
        s = int(late * SR)
        m = min(len(x), len(out) - s)
        out[s:s + m] += x[:m]
    y = bus_recipe(narrow(out, 0.3), work_db)
    y = shape(y, length, fade_in, fade_out, decay_from, decay_db_per_s)
    return y * D.db(SCORE_LUFS - max_momentary(y))


def bake_bed(bundle, rel, xfade=2.0, lufs=-27.0):
    """A loop that meets itself: the last xfade seconds are crossfaded into the first (equal power). 35 Hz highpass, mono under
    150 Hz, sides down until left/right likeness is 0.5, level `lufs`."""
    x = load(bundle, rel)
    x = D.biquad(x, 'highpass', 35.0, -3.0)[int(0.2 * SR):]
    x = narrow(x, 0.5)
    m = int(xfade * SR)
    n = len(x) - m
    u = np.linspace(0, np.pi / 2, m)[:, None]
    y = x[:n].copy()
    y[:m] = x[:m] * np.sin(u) + x[n:] * np.cos(u)
    i, _ = D.measure(np.concatenate([y, y]))
    return y * D.db(lufs - i)


# ------------------------------------------------------------------ the palette (names are ours; S47Sideband.cpp has the same table)
HS = 'Horror Screamers/FL_GFX_Horror_Screamers_%s.wav'
BR = 'Orchestral Braams/FL_GFX_Orchestral_Braams_%s.wav'
IM = 'Orchestral Impacts/FL_GFX_Orchestral_Impact_%s.wav'
HA = 'Horror Ambience/FL_GFX_Horror_Ambience_%s.wav'
STINGERS = {
    # full tier: a clear front, then made to die much faster than the source: the three seconds of near silence after it stay so
    'full_braam_dsharp': dict(work_db=-12.0, parts=[(BR % '05_D#m', 0, 0)], length=3.6, decay_from=0.55, decay_db_per_s=13.0),
    'full_cluster_cold': dict(work_db=-12.0, parts=[(HS % '26', 0, 0), (IM % '01', -3.0, 0)], length=3.6, decay_from=0.5, decay_db_per_s=7.0),
    'full_glass_low': dict(work_db=-12.0, parts=[(HS % '15', 0, 0)], length=3.6, decay_from=0.5, decay_db_per_s=13.0),
    'full_cluster_c': dict(work_db=-12.0, parts=[(HS % '20', 0, 0), (IM % '19', -3.0, 0)], length=3.6, decay_from=0.5, decay_db_per_s=9.0),
    # glimpse tier: slow or soft fronts with their weight between 300 and 3000 Hz (what FAR's send lets through)
    'glimpse_glass_swell': dict(parts=[(HS % '25', 0, 0)], length=7.0, fade_in=0.3, fade_out=3.0),
    'glimpse_struck_tone': dict(parts=[(HS % '07', 0, 0)], length=6.5, fade_in=0.2, fade_out=3.0),
    'glimpse_far_horn': dict(parts=[(BR % '01_D#m', 0, 0)], length=6.0, fade_in=0.15, fade_out=2.5),
    # hint tier: one low note, never a front
    'hint_low_note': dict(parts=[(BR % '05_D#m', 0, 0)], length=5.0, fade_in=0.45, fade_out=3.0, decay_from=1.0, decay_db_per_s=4.0),
    'hint_low_swell': dict(parts=[(IM % '05', 0, 0)], length=5.5, fade_in=0.3, fade_out=3.0),
}
BEDS = {'bed_low_breath': HA % '18', 'bed_still': HA % '20', 'bed_cold_tone': HA % '26'}


def write_wav(x, path, rate, loop=False):
    """PCM16 with triangular dither, written beside its final name and moved into place. loop: resampled as a loop (its own
    end before it and its own start after it), so that it still meets itself at the lower rate."""
    if rate != SR:
        if loop:
            pad = SR // 2
            k = pad * rate // SR
            x = resample_poly(np.concatenate([x[-pad:], x, x[:pad]]), rate, SR, axis=0)[k:-k]
        else:
            x = resample_poly(x, rate, SR, axis=0)
    peak = float(np.abs(x).max())
    if peak >= 1.0:
        sys.exit('S47 sideband: %s would clip (%.2f)' % (path, peak))
    tmp = path + '.tmp-%d.wav' % os.getpid()
    r = subprocess.run(['ffmpeg', '-v', 'error', '-y', '-f', 'f32le', '-ar', str(rate), '-ac', '2', '-i', '-', '-af',
                        'aresample=osf=s16:dither_method=triangular', '-c:a', 'pcm_s16le', '-f', 'wav', tmp],
                       input=x.astype('<f4').tobytes(), capture_output=True)
    if r.returncode != 0:
        sys.exit('S47 sideband: ffmpeg failed writing %s: %s' % (path, r.stderr.decode('utf-8', 'replace')[-300:]))
    os.replace(tmp, path)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--bundle', default=BUNDLE)
    ap.add_argument('--content', default=os.path.join(PROJECT, 'Content'))
    ap.add_argument('--check', action='store_true')
    args = ap.parse_args()
    if not os.path.isdir(os.path.join(args.bundle, 'Horror Screamers')):
        print('S47 sideband: no bought bundle at %s: FLARES is left out (the game plays without it)' % args.bundle)
        return
    out = os.path.join(args.content, 'Audio', 'Raw', 'media', 'sideband')
    if not args.check:
        if not os.path.isdir(os.path.join(args.content, 'Audio', 'Raw')):
            sys.exit('S47 sideband: no %s (run Scripts/import-audio.sh first)' % os.path.join(args.content, 'Audio', 'Raw'))
        os.makedirs(out, exist_ok=True)
    rows = {}
    for name, spec in STINGERS.items():
        y = bake_stinger(args.bundle, **spec)
        rows[name] = dict(secs=round(len(y) / SR, 2), loudest_400ms_lufs=round(max_momentary(y), 1), peak_dbfs=round(float(20 * np.log10(np.abs(y).max())), 1),
                          likeness=round(corr(y), 2), starts_at_zero=bool(abs(y[0]).max() < 1e-4 and abs(y[-1]).max() < 1e-4),
                          parts=len(spec['parts']))
        if abs(rows[name]['loudest_400ms_lufs'] - SCORE_LUFS) > 0.2 or not rows[name]['starts_at_zero']:
            sys.exit('S47 sideband: %s is not at the staged level or does not start and end at zero: %s' % (name, rows[name]))
        if not args.check:
            write_wav(y, os.path.join(out, name + '.wav'), SR)
        print('S47 sideband: %-22s %4.1f s  %6.1f LUFS at its loudest  peak %6.1f dBFS  likeness %.2f' % (
            name, rows[name]['secs'], rows[name]['loudest_400ms_lufs'], rows[name]['peak_dbfs'], rows[name]['likeness']))
    for name, rel in BEDS.items():
        y = bake_bed(args.bundle, rel)
        i, pk = D.measure(np.concatenate([y, y]))
        seam = float(np.abs(y[0] - y[-1]).max() / (np.percentile(np.abs(np.diff(y, axis=0)), 99.9) + 1e-12))
        rows[name] = dict(secs=round(len(y) / SR, 2), lufs=round(i, 1), peak_dbfs=round(pk, 1), likeness=round(corr(y), 2), seam=round(seam, 2))
        if abs(i + 27.0) > 0.3 or seam > 3:
            sys.exit('S47 sideband: %s is not at -27 LUFS or would click where it wraps: %s' % (name, rows[name]))
        if not args.check:
            write_wav(y, os.path.join(out, name + '.wav'), BED_RATE, loop=True)
        print('S47 sideband: %-22s %4.1f s  %6.1f LUFS  peak %6.1f dBFS  likeness %.2f  seam %.2f (loop)' % (
            name, rows[name]['secs'], i, pk, rows[name]['likeness'], seam))
    if not args.check:
        with open(os.path.join(out, 'sideband.json'), 'w', encoding='utf-8') as f:
            json.dump(dict(made_by='Tools/sideband/build_flares.py', staged_lufs=SCORE_LUFS, sounds=rows), f, indent=1, sort_keys=True)
        total = sum(os.path.getsize(os.path.join(out, n)) for n in os.listdir(out))
        print('S47 sideband: %d files in %s (%.1f MB), none of them in Git' % (len(os.listdir(out)), out, total / 1e6))


if __name__ == '__main__':
    main()
