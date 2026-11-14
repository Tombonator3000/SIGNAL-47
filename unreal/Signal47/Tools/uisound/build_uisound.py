#!/usr/bin/env python3
"""FIX10-7 (14 Nov 2026): the station's key clicks and the TVs' static, baked from the bought bundle and staged out of Git.

  build_uisound.py [--bundle DIR] [--content DIR] [--check] [--verify] [--listen DIR]

Writes DIR/Audio/Raw/media/uisound/<name>.wav (mono PCM16, 48 kHz) and uisound.json (the manifest: every file's source in the
bundle, what was done to it, its measured peak and loudest 50 ms), where S47UiSound.cpp reads them at first use. Without them the
game plays today's sounds (the free click, the switches, the synthesised hiss) and says nothing. --content defaults to the MAIN
checkout's Content (as Tools/rxacquire/make_search.py), since Content/Audio is ignored by Git and only exists there.
  --check   bake and measure, write nothing
  --verify  read the staged files back: levels per kind, loops that meet themselves, nothing staged is tracked by Git
  --listen  also write every final sound and three "in use" demos (made here from the baked files, no engine) with a README

The game never plays a bundle file as it is. A click is cut to its own front (2 ms before the first sample over 5 % of its
peak), shortened to its kind's length with a raised-cosine tail, band-limited (a 2nd-order high-pass and low-pass, the Web Audio
biquad of S47WebAudio.h) so it sits in a 1980s room and not on a hi-fi, played back faster or slower for its round-robin variants,
set to a common loudest-50-ms RMS of BAKE_RMS dBFS with its peak held under BAKE_PEAK dBFS. Each kind's playback gain
(S47UiSound.cpp, the same table as KINDS here) puts it at or under the sound it replaces at the same call (REF: measured from the
free click and the bought switches at their call gains). The static bed is level-matched through each TV's own filter chain so
that it is never louder than the white noise it stands in for.

Licence: Function Loops, "Ultimate SFX for Games" bundle; Tom confirmed commercial use (10 and 13 November 2026). The raw files
never go into Git and are never staged or copied; they are read here (with ffmpeg, as data) and only these baked forms ship.
Needs python3 with numpy and scipy, and ffmpeg (on this machine: /home/tom/stable-diffusion-webui-forge/venv/bin/python3 -I).
"""
import argparse
import json
import os
import subprocess
import sys
import wave

import numpy as np

sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'sideband'))
import s47dsp as D  # noqa: E402  (the Web Audio biquad, the loader)

HERE = os.path.dirname(os.path.abspath(__file__))
PROJECT = os.path.dirname(os.path.dirname(HERE))
MAIN_CONTENT = '/home/tom/SIGNAL-47/unreal/Signal47/Content'
BUNDLE = os.path.expanduser('~/Desktop/SIGNAL 47/bt25_ultimatesfxforgames_softwarebundle')
SR = D.SR  # 48000, the context's own rate: nothing is resampled in the game
SUB = os.path.join('Audio', 'Raw', 'media', 'uisound')
BAKE_RMS, BAKE_PEAK = -24.0, -6.0  # every one-shot in its file (S47UiSound.cpp BakedRms / BakedPeak)

UC = 'Ui Clicks/FL_GFX_UI_Click_%s.wav'
HF = 'Household Foley & Devices/FL_GFX_Household_Foley_Devices_%s.wav'
GL = 'Glitches/FL_GFX_Motion_Glitch_%s.wav'
CM = 'Cozy Mechanics/FL_GFX_Cozy_Mechanic_%s.wav'

# What each kind replaces, at the call that plays it (measured 14 Nov from Content/Audio/Raw/sfx/click.wav and
# Content/Licensed/Raw/{small,rack}_switch.wav at their gains, x LicGain): (peak dBFS, loudest 50 ms RMS dBFS), before the master.
REF = {
    'terminal key, click .30': (-12.1, -34.5),
    'terminal action, click .42': (-9.2, -31.6),
    # (a square has no crest: a click's peak is held to the terminal key's instead, its loudest 50 ms to the beep's)
    'knob detent, square beep .028 for 7 ms': (-12.1, -39.6),
    'deck key, small_switch .38 x 2.2': (-3.6, -18.6),
    'deck primary, rack_switch .42 x 1.6': (-5.4, -19.3),
}
# kind: (files as (source, playback rate), length s, high-pass Hz, low-pass Hz, in-game loudest-50-ms target dBFS, what it may not
# pass (a REF key, or None where nothing played before), where it plays). The gain in the game is target - BAKE_RMS.
KINDS = {
    'key': ([(UC % '10', 1.0), (UC % '02', 1.0), (UC % '10', 0.94), (UC % '02', 1.06)], 0.07, 150, 6500, -36.0,
            'terminal key, click .30', 'a key on the three SARO PCs, the receiver\'s step keys'),
    'tab': ([(UC % '09', 1.0), (UC % '09', 0.95), (UC % '09', 1.06)], 0.09, 150, 6500, -35.5,
            'terminal key, click .30', 'a tab or a menu line on the three SARO PCs'),
    'action': ([(HF % 'Interrupteur_01', 1.0), (HF % 'Interrupteur_01', 0.95), (HF % 'Interrupteur_01', 1.05)], 0.16, 120, 7000, -33.0,
               'terminal action, click .42', 'the primary key on a PC: RETAIN TAKE, START RECORDING, SEND'),
    'detent': ([(UC % '04', 1.0), (UC % '04', 1.08), (UC % '04', 0.93)], 0.035, 200, 6500, -41.0,
               'knob detent, square beep .028 for 7 ms', 'a TUNE dial or slider step'),
    'transport': ([(HF % 'Button_02', 1.0), (HF % 'Cartridge_01', 1.0), (HF % 'Button_02', 0.94), (HF % 'Cartridge_01', 1.05)], 0.14, 100, 7000, -27.0,
                  'deck key, small_switch .38 x 2.2', 'the reel deck: ORIGINAL, REFERENCE, REWIND, PLAY/STOP, the reference keys'),
    'mark': ([(UC % '13', 1.0), (UC % '13', 1.07), (UC % '13', 0.94)], 0.06, 150, 7000, -30.0,
             'deck key, small_switch .38 x 2.2', 'the reel deck: MARK IN, MARK OUT'),
    'page': ([(CM % 'Map_01', 1.0), (CM % 'Map_01', 0.92), (CM % 'Map_01', 1.07)], 0.30, 250, 8000, -37.0,
             None, 'a page of the notebook or the binder turned'),
    'menu_move': ([(UC % '05', 1.0), (UC % '05', 1.06), (UC % '05', 0.95)], 0.08, 150, 6000, -38.0,
                  None, 'the pause menu: the selection moved'),
    'menu_ok': ([('menu_ok', 1.0), ('menu_ok', 0.95), ('menu_ok', 1.05)], 0.12, 120, 6000, -35.0,
                None, 'the pause menu: a line chosen'),
}
# The deck's RETAIN plays the action's files at its own gain (no files of its own): heavier than a PC's key, under the old rack_switch.
RETAIN = (-26.0, 'deck primary, rack_switch .42 x 1.6', 'the reel deck: RETAIN')
# The TVs. The bed's level is set by MatchBed, the one-shots by their peak (BAKE_PEAK) and S47UiSound.cpp's TV gains.
TV_SOURCES = {'tv_static': [GL % '10'], 'tv_thunk': [GL % '25', GL % '10'], 'tv_power_on': [HF % 'Turn_On', GL % '03'],
              'tv_power_off': [HF % 'Turn_Off', GL % '03']}
# The chains the bed goes through in the game (white noise there today, uniform -1..1): S47OfficeTv / US47Audio::TvHiss, S47MotelTv.
CHAINS = {'office and room 6': [('highpass', 450, 1.0), ('bandpass', 2600, 0.5)], 'motel': [('highpass', 900, 1.0)]}  # (Q unset there: 1 dB)


# ------------------------------------------------------------------ reading, measuring, writing
def load(bundle, rel):
    p = os.path.join(bundle, rel)
    if not os.path.isfile(p):
        sys.exit('S47 uisound: not in the bundle: ' + rel)
    return D.load(p).mean(axis=1)


def rate(x, r):
    """Played r times as fast (pitch up for r > 1), linear interpolation as the buffer source does."""
    if r == 1.0:
        return x
    t = np.arange(0, len(x) - 1, r)
    return np.interp(t, np.arange(len(x)), x)


def glide(x, r0, r1, secs):
    """Played at a rate that slides from r0 to r1 over secs (then stays): a set running down."""
    out, pos, n = [], 0.0, int(secs * SR)
    i = 0
    while pos < len(x) - 1:
        r = r0 + (r1 - r0) * min(1.0, i / max(1, n))
        out.append(pos)
        pos += r
        i += 1
    return np.interp(np.array(out), np.arange(len(x)), x)


def front(x, lead=0.002, under=0.05):
    a = np.abs(x)
    first = int(np.argmax(a > a.max() * under))
    return x[max(0, first - int(lead * SR)):]


def edges(x, fin=0.0015, fout=None):
    n = len(x)
    t = np.arange(n) / SR
    fout = fout if fout is not None else 0.35 * n / SR
    g = np.sin(np.clip(t / fin, 0, 1) * np.pi / 2) ** 2 * np.sin(np.clip((n / SR - t) / fout, 0, 1) * np.pi / 2) ** 2
    y = x * g
    y[0] = y[-1] = 0.0
    return y


def band(x, hp, lp):
    return D.biquad(D.biquad(x, 'highpass', hp, -3.0), 'lowpass', lp, -3.0)


def rms50(x):
    w = int(0.05 * SR)
    if len(x) <= w:
        return float(10 * np.log10(np.sum(x ** 2) / w + 1e-20))
    c = np.concatenate([[0.0], np.cumsum(x ** 2)])
    return float(10 * np.log10((c[w:] - c[:-w]).max() / w + 1e-20))


def peak(x):
    return float(20 * np.log10(np.abs(x).max() + 1e-20))


def rms(x):
    return float(10 * np.log10(np.mean(x ** 2) + 1e-20))


def chain(x, steps):
    for kind, f, q in steps:
        x = D.biquad(x, kind, f, q)
    return x


def write_wav(x, path):
    pk = float(np.abs(x).max())
    if pk >= 1.0:
        sys.exit('S47 uisound: %s would clip (%.2f)' % (path, pk))
    d = np.round(x * 32767).astype('<i2')
    tmp = path + '.tmp-%d' % os.getpid()
    with wave.open(tmp, 'wb') as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(SR); w.writeframes(d.tobytes())
    os.replace(tmp, path)


def read_wav(path):
    with wave.open(path) as w:
        if w.getnchannels() != 1 or w.getsampwidth() != 2 or w.getframerate() != SR:
            sys.exit('S47 uisound: %s is not mono PCM16 at %d Hz' % (path, SR))
        return np.frombuffer(w.readframes(w.getnframes()), dtype='<i2').astype(np.float64) / 32767


# ------------------------------------------------------------------ the bakes
def to_level(x):
    """The common quiet level: loudest 50 ms at BAKE_RMS, the peak held under BAKE_PEAK (then quieter, never louder)."""
    x = x * D.db(BAKE_RMS - rms50(x))
    if peak(x) > BAKE_PEAK:
        x = x * D.db(BAKE_PEAK - peak(x))
    return x


def menu_ok_source(bundle):
    """Two parts: Click 05 a little slower, Click 09 14 ms after it (a key that goes down and seats)."""
    a = front(rate(load(bundle, UC % '05'), 0.82))
    b = front(rate(load(bundle, UC % '09'), 0.90)) * D.db(-4.0)
    n = max(len(a), len(b) + int(0.014 * SR))
    y = np.zeros(n)
    y[:len(a)] += a
    y[int(0.014 * SR):int(0.014 * SR) + len(b)] += b
    return y


def bake_click(bundle, rel, r, length, hp, lp, cache):
    if rel not in cache:
        cache[rel] = menu_ok_source(bundle) if rel == 'menu_ok' else load(bundle, rel)
    x = front(rate(cache[rel], r))[:int(length * SR)]
    x = np.concatenate([x, np.zeros(max(0, int(length * SR) - len(x)))])
    x = band(x, hp, lp)
    x = edges(x)
    return to_level(x)


def loop_join(parts, xfade):
    """Parts joined by equal-power crossfades, then the end folded into the head: a loop that meets itself."""
    k = int(xfade * SR)
    u = np.linspace(0, np.pi / 2, k)
    y = parts[0]
    for p in parts[1:]:
        y = np.concatenate([y[:-k], y[-k:] * np.cos(u) + p[:k] * np.sin(u), p[k:]])
    out = y[:-k].copy()
    out[:k] = y[:k] * np.sin(u) + y[-k:] * np.cos(u)
    return out


def bake_bed(bundle):
    """Glitch 10 (the steady 2 s of hiss: 100 ms RMS within 3 dB end to end) as it is, backwards, and 4 % slower, joined into one
    loop of about 5 s so that no two-second pattern repeats; 200 Hz to 10 kHz (the TVs' own filters make the small speaker). Its
    level comes from match_bed."""
    g = load(bundle, GL % '10')[int(0.05 * SR):int(1.95 * SR)]
    y = loop_join([g, g[::-1].copy(), rate(g, 0.96)], 0.25)
    y = y - y.mean()
    y = D.biquad(D.biquad(y, 'highpass', 200, -3.0), 'lowpass', 10000, -3.0)
    return y


def match_bed(y):
    """The file at a peak of -1 dBFS (a recording has more crest than uniform noise, so it cannot be as loud in the file). Per TV
    chain: its level against the white noise it replaces there (uniform -1..1 through the same filters), and the lift the game
    gives it (S47UiSound::TvBedLift reads it from the manifest, clamped to 0..12 dB) so that its RMS there equals today's hiss:
    as loud, never louder. Also the share of each band after the chain, the bed's and the noise's, so the spectra can be compared."""
    rng = np.random.default_rng(47)
    white = rng.uniform(-1, 1, len(y))
    y = y * D.db(-1.0 - peak(y))
    vs = {name: round(rms(chain(y, c)) - rms(chain(white, c)), 2) for name, c in CHAINS.items()}
    lift = {name: round(max(0.0, -v), 2) for name, v in vs.items()}

    def share(x):
        S = np.abs(np.fft.rfft(x[:SR * 4])) ** 2
        f = np.fft.rfftfreq(SR * 4, 1 / SR)
        return [round(100 * S[(f >= a) & (f < b)].sum() / S.sum()) for a, b in ((0, 1000), (1000, 2000), (2000, 4000), (4000, 8000), (8000, 24000))]
    spectra = {name: dict(bed=share(chain(y, c)), noise=share(chain(white, c))) for name, c in CHAINS.items()}
    return y, vs, lift, spectra


def bake_thunk(bundle, bed):
    """Glitch 25's thump (the first 0.32 s: a relay and the picture dropping), and 0.12 s of the bed over its front: the set
    changing channel. Peak at BAKE_PEAK."""
    t = front(load(bundle, GL % '25'))[:int(0.32 * SR)]
    t = band(t, 60, 4000)
    t = edges(t, 0.001, 0.18)
    n = len(t)
    s = np.zeros(n)
    m = int(0.12 * SR)
    s[int(0.01 * SR):int(0.01 * SR) + m] = edges(bed[:m] / (np.abs(bed[:m]).max() + 1e-9), 0.004, 0.06)
    y = t / (np.abs(t).max() + 1e-9) + 0.35 * s
    y = edges(y, 0.001, 0.15)
    return y * D.db(BAKE_PEAK - peak(y))


def bake_power_on(bundle):
    """The switch going on (Turn_On, its front), then the tube's buzz swelling up (Glitch 03's first 0.7 s backwards: its own decay
    turned into a rise), cut short as the picture comes and the set's hiss takes over. 0.75 s, peak at BAKE_PEAK."""
    c = band(front(load(bundle, HF % 'Turn_On'))[:int(0.09 * SR)], 120, 7000)
    c = edges(c, 0.001, 0.05)
    b = load(bundle, GL % '03')
    b = front(b)[:int(0.7 * SR)][::-1].copy()
    b = band(b, 150, 5000)
    n = int(0.75 * SR)
    y = np.zeros(n)
    y[:len(c)] += c / (np.abs(c).max() + 1e-9)
    s = int(0.04 * SR)
    m = min(len(b), n - s)
    y[s:s + m] += 0.55 * b[:m] / (np.abs(b).max() + 1e-9)
    y = edges(y, 0.001, 0.08)
    return y * D.db(BAKE_PEAK - peak(y))


def bake_power_off(bundle):
    """The switch going off (Turn_Off, its front), then the buzz running down: Glitch 03 played at a rate that slides from 1 to
    0.4 over half a second (the picture collapsing to its dot), dying out. 0.8 s, peak at BAKE_PEAK."""
    c = band(front(load(bundle, HF % 'Turn_Off'))[:int(0.09 * SR)], 120, 7000)
    c = edges(c, 0.001, 0.05)
    b = glide(front(load(bundle, GL % '03'))[:int(1.0 * SR)], 1.0, 0.4, 0.5)
    b = band(b, 80, 4000)
    n = int(0.8 * SR)
    b = b[:n - int(0.02 * SR)]
    b = b * np.exp(-np.arange(len(b)) / SR / 0.22)
    y = np.zeros(n)
    y[:len(c)] += c / (np.abs(c).max() + 1e-9)
    s = int(0.02 * SR)
    y[s:s + len(b)] += 0.6 * b / (np.abs(b).max() + 1e-9)
    y = edges(y, 0.001, 0.2)
    return y * D.db(BAKE_PEAK - peak(y))


def seam(y):
    """How a loop meets itself: the jump at the wrap against the 99.9th percentile of the steps inside it (under 3: no click), and
    the slope at the end against the slope at the start (as a share of a typical step)."""
    d = np.abs(np.diff(y))
    p = np.percentile(d, 99.9) + 1e-12
    return float(abs(y[0] - y[-1]) / p), float(abs((y[1] - y[0]) - (y[-1] - y[-2])) / p)


def bake_all(bundle):
    out, rows, cache = {}, {}, {}
    for kind, (files, length, hp, lp, target, ref, where) in KINDS.items():
        for i, (rel, r) in enumerate(files):
            name = 'ui_%s_%d' % (kind, i)
            y = bake_click(bundle, rel, r, length, hp, lp, cache)
            out[name] = y
            src = [UC % '05', UC % '09'] if rel == 'menu_ok' else [rel]
            rows[name] = dict(kind=kind, sources=[os.path.basename(s) for s in src], rate=r, secs=round(len(y) / SR, 3), highpass=hp,
                              lowpass=lp, peak_dbfs=round(peak(y), 1), loudest_50ms_dbfs=round(rms50(y), 1))
    bed, vs, lift, spectra = match_bed(bake_bed(bundle))
    out['tv_static'] = bed
    s0, s1 = seam(bed)
    rows['tv_static'] = dict(kind='tv', sources=[os.path.basename(GL % '10')], secs=round(len(bed) / SR, 3), peak_dbfs=round(peak(bed), 1),
                             rms_dbfs=round(rms(bed), 1), against_white_noise_db=vs, lift_db=lift, band_shares_pct_0_1_2_4_8_24k=spectra, seam=round(s0, 2), slope=round(s1, 2), loop=True,
                             done='forward + backwards + 4 % slower, 0.25 s equal-power joins, folded; 200 Hz-10 kHz')
    for name, fn, what in (('tv_thunk', lambda: bake_thunk(bundle, bed), 'Glitch 25 front 0.32 s (60 Hz-4 kHz) + 0.12 s of the bed'),
                           ('tv_power_on', lambda: bake_power_on(bundle), 'Turn_On front + Glitch 03 first 0.7 s reversed (a swell)'),
                           ('tv_power_off', lambda: bake_power_off(bundle), 'Turn_Off front + Glitch 03 gliding from rate 1 to 0.4, decaying')):
        y = fn()
        out[name] = y
        rows[name] = dict(kind='tv', sources=[os.path.basename(s) for s in TV_SOURCES[name]], secs=round(len(y) / SR, 3),
                          peak_dbfs=round(peak(y), 1), loudest_50ms_dbfs=round(rms50(y), 1), done=what)
    return out, rows


# ------------------------------------------------------------------ checks
def in_game(kind):
    """(gain dB, target, the REF it may not pass) for a kind as S47UiSound.cpp plays it."""
    if kind == 'retain':
        return RETAIN[0] - BAKE_RMS, RETAIN[0], RETAIN[1]
    t = KINDS[kind][4]
    return t - BAKE_RMS, t, KINDS[kind][5]


def check_levels(rows):
    bad = []
    for kind in list(KINDS) + ['retain']:
        files = [n for n, r in rows.items() if r['kind'] == (kind if kind != 'retain' else 'action')]
        if len(files) < 3:
            bad.append('%s has %d variants (needs 3 to 5)' % (kind, len(files)))
        g, target, ref = in_game(kind)
        for n in files:
            pk, lo = rows[n]['peak_dbfs'] + g, rows[n]['loudest_50ms_dbfs'] + g
            if rows[n]['peak_dbfs'] > BAKE_PEAK + 0.05 or rows[n]['loudest_50ms_dbfs'] > BAKE_RMS + 0.05:
                bad.append('%s is over the baked level' % n)
            if ref and (pk > REF[ref][0] + 0.05 or lo > REF[ref][1] + 0.05):
                bad.append('%s (%s) in the game: peak %.1f, loudest 50 ms %.1f: over %s %s' % (n, kind, pk, lo, ref, REF[ref]))
    for n, r in rows.items():
        if r['kind'] == 'tv':
            if r['peak_dbfs'] > -0.95:
                bad.append('%s peaks at %.1f dBFS' % (n, r['peak_dbfs']))
    v = rows['tv_static']
    if any(abs(v['against_white_noise_db'][k] + v['lift_db'][k]) > 0.05 or v['lift_db'][k] > 12 for k in CHAINS) or v['seam'] > 3 or v['slope'] > 3:
        bad.append('tv_static lifted is not as loud as the noise it replaces, or clicks where it wraps: %s' % v)
    return bad


def same_gains():
    """The game's gain table (S47UiSound.cpp Rows, in EKind's order) is this script's: what was measured is what plays."""
    import re
    src = os.path.join(PROJECT, 'Source', 'Signal47', 'Private', 'S47UiSound.cpp')
    try:
        text = open(src, encoding='utf-8').read()
    except OSError:
        return []
    got = [float(g) for g in re.findall(r'\{ TEXT\("[a-z_]+"\), (-?[0-9.]+), -?[0-9.]+, -?[0-9.]+ \}', text)]
    order = ['key', 'tab', 'action', 'detent', 'transport', 'mark', 'retain', 'page', 'menu_move', 'menu_ok']
    want = [in_game(k)[0] for k in order]
    return [] if len(got) == len(want) and all(abs(a - b) < 1e-6 for a, b in zip(got, want)) else ['S47UiSound.cpp gains %s are not the script\'s %s' % (got, want)]


def tracked(content):
    """Files under the staging folder that Git tracks (in the checkout that holds content): must be none."""
    stage = os.path.join(content, SUB)
    top = subprocess.run(['git', '-C', content, 'rev-parse', '--show-toplevel'], capture_output=True, text=True)
    if top.returncode != 0:
        return []
    r = subprocess.run(['git', '-C', top.stdout.strip(), 'ls-files', '--', os.path.relpath(stage, top.stdout.strip())], capture_output=True, text=True)
    return [l for l in r.stdout.splitlines() if l.strip()]


# ------------------------------------------------------------------ for Tom's ears
def square(freq, dur, gain, n_at, n):
    """US47Audio::Beep: a square through a 3 kHz low-pass, 5 ms up, held, 20 ms down."""
    t = np.arange(int((dur + 0.05) * SR)) / SR
    x = np.sign(np.sin(2 * np.pi * freq * t))
    e = np.clip(t / 0.005, 0, 1) * np.where(t <= dur, 1.0, np.clip(1 - (t - dur) / 0.02, 0, 1))
    y = D.biquad(x * e * gain, 'lowpass', 3000, 1.0)
    out = np.zeros(n)
    m = min(len(y), n - n_at)
    out[n_at:n_at + m] += y[:m]
    return out


def place(buf, y, at, g):
    s = int(at * SR)
    m = min(len(y), len(buf) - s)
    buf[s:s + m] += y[:m] * g


def demos(out, listen, lift_office):
    rng = np.random.default_rng(1986)
    pick = {}

    def var(kind):
        files = sorted(n for n in out if n.startswith('ui_%s_' % kind))
        last = pick.get(kind)
        choice = [f for f in files if f != last] or files
        pick[kind] = choice[rng.integers(len(choice))]
        return out[pick[kind]]

    g = lambda kind: D.db(in_game(kind)[0])  # noqa: E731
    # 1: typing on the receiver PC with tab changes (the terminal's own chip answering each key, as kept)
    n = int(6.5 * SR)
    a = np.zeros(n)
    t = 0.3
    for step in range(26):
        kind = 'tab' if step in (6, 13, 20) else 'detent' if step in (9, 10, 11) else 'action' if step == 24 else 'key'
        k = 'action' if kind == 'action' else kind
        place(a, var(k), t, g(k))
        if kind == 'key':
            a += square(1480, 0.016, 0.030, int((t + 0.01) * SR), n)
        elif kind == 'tab':
            a += square(990, 0.020, 0.030, int((t + 0.01) * SR), n)
        elif kind == 'action':
            a += square(1320, 0.030, 0.040, int((t + 0.01) * SR), n) + square(1760, 0.045, 0.040, int((t + 0.055) * SR), n)
        t += 0.09 if kind == 'detent' else rng.uniform(0.13, 0.28)
    # 2: the reel deck: PLAY, MARK IN, MARK OUT, RETAIN
    b = np.zeros(int(6.0 * SR))
    place(b, var('transport'), 0.3, g('transport'))
    place(b, var('mark'), 2.0, g('mark'))
    place(b, var('mark'), 3.6, g('mark'))
    place(b, var('action'), 4.8, g('retain'))
    # 3: a TV switched on, 8 s, a channel change, off: from 2 m in the room (the office's chain, the panner at 2 m), and the same
    # from the next room (its wall: a 650 Hz low-pass and 0.4)
    def tv(wall):
        n = int(12.5 * SR)
        c = np.zeros(n)
        dist = 0.9 / (0.9 + 1.2 * (2.0 - 0.9))  # the inverse distance model, ref .9, rolloff 1.2, at 2 m
        place(c, out['tv_power_on'], 0.3, 0.4)
        bed = np.tile(out['tv_static'], 4)[:int(9.4 * SR)]
        bed = chain(bed, CHAINS['office and room 6']) * 0.09 * D.db(lift_office)
        e = np.ones(len(bed))
        u = np.arange(len(bed)) / SR
        e *= np.clip((u - 0.35) / 0.45, 0, 1)            # the tube coming up (WARM_S .45)
        e *= np.where((u > 4.4) & (u < 4.55), 0.25, 1.0)  # the channel change: the picture drops a moment
        e *= np.clip((9.4 - u) / 0.5, 0, 1)              # the tube going down (FADE_S .5)
        place(c, bed * e, 0.3, 1.0)
        place(c, 0.0016 * np.sin(2 * np.pi * 15734 * np.arange(int(9.4 * SR)) / SR) * e, 0.3, 1.0)  # the kept line whine (synth)
        place(c, D.biquad(out['tv_thunk'], 'lowpass', 2400, 1.0), 4.7, 0.35)
        place(c, out['tv_power_off'], 9.2, 0.4)
        c *= dist
        if wall:
            c = D.biquad(c, 'lowpass', 650, 1.0) * 0.4
        return c
    files = {'demo_1_typing_and_tabs': a, 'demo_2_reel_deck_play_mark_mark_retain': b, 'demo_3_tv_on_8s_channel_off_in_room': tv(False),
             'demo_3b_tv_same_from_next_room': tv(True)}
    for name, y in files.items():
        if np.abs(y).max() >= 1:
            sys.exit('S47 uisound: demo %s clips' % name)
        write_wav(y, os.path.join(listen, name + '.wav'))
    return {k: (round(peak(v), 1), round(rms50(v), 1)) for k, v in files.items()}


def readme(listen, rows, demo_levels):
    L = ['FIX10-7 (14 Nov 2026): UI clicks and TV static from the bought bundle (Ultimate SFX for Games). Made by',
         'Tools/uisound/build_uisound.py; not built into the game yet, not heard by anyone. Levels: dBFS before the master volume.',
         'Each kind\'s files play in turn (never the same one twice running) at the gain in brackets.', '']
    for kind, (files, length, hp, lp, target, ref, where) in list(KINDS.items()) + [('retain', ([], 0, 0, 0, RETAIN[0], RETAIN[1], RETAIN[2]))]:
        g = in_game(kind)[0]
        L.append('%s (%+.1f dB in the game; at most %s %s): %s' % (kind.upper(), g, ref or 'nothing before', REF.get(ref, ''), where))
        for n, r in sorted(rows.items()):
            if r['kind'] == (kind if kind != 'retain' else 'action'):
                L.append('  %s.wav  <- %s, rate %.2f, %.0f-%.0f Hz, %.3f s; file peak %.1f, loudest 50 ms %.1f; in the game %.1f / %.1f' % (
                    n, ' + '.join(r['sources']), r['rate'], r['highpass'], r['lowpass'], r['secs'], r['peak_dbfs'], r['loudest_50ms_dbfs'],
                    r['peak_dbfs'] + g, r['loudest_50ms_dbfs'] + g))
        L.append('')
    for n in ('tv_static', 'tv_thunk', 'tv_power_on', 'tv_power_off'):
        r = rows[n]
        L.append('%s.wav  <- %s: %s. %.2f s, peak %.1f%s' % (n, ' + '.join(r['sources']), r['done'], r['secs'], r['peak_dbfs'],
                 ', against the white noise it replaces in each TV chain (dB): %s, lifted in the game by %s to the same RMS; band shares '
                 '0-1/1-2/2-4/4-8/8-24 kHz %% after the chain: %s; loop seam %.2f, slope %.2f (under 3: no click)' % (
                     r['against_white_noise_db'], r['lift_db'], r['band_shares_pct_0_1_2_4_8_24k'], r['seam'], r['slope'])
                 if n == 'tv_static' else ', loudest 50 ms %.1f' % r['loudest_50ms_dbfs']))
    L += ['', 'Demos (peak, loudest 50 ms):']
    for k, v in demo_levels.items():
        L.append('  %s.wav  %s' % (k, v))
    L += ['  1: the receiver PC: keys, three tab changes, three TUNE detents, RETAIN TAKE; the terminal chip beeps are the kept synth.',
          '  2: the reel deck: PLAY, MARK IN, MARK OUT, RETAIN (keys only; the tape itself is not in the demo).',
          '  3: Dale\'s office TV at 2 m: power on, 8 s of snow through the set\'s own filters at its own level (.09), a channel change',
          '     at 4.7 s, power off; the 15.7 kHz line whine is the game\'s own (synth, kept, left out of the bed). 3b: from the next room.',
          '', 'The raw bundle files are not here and never in Git; only these processed forms.']
    with open(os.path.join(listen, 'README.txt'), 'w', encoding='utf-8') as f:
        f.write('\n'.join(L) + '\n')


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--bundle', default=BUNDLE)
    ap.add_argument('--content', default=MAIN_CONTENT if os.path.isdir(os.path.join(MAIN_CONTENT, 'Audio', 'Raw')) else os.path.join(PROJECT, 'Content'))
    ap.add_argument('--check', action='store_true')
    ap.add_argument('--verify', action='store_true')
    ap.add_argument('--listen')
    args = ap.parse_args()
    stage = os.path.join(args.content, SUB)
    if args.verify:
        bad = ['tracked by Git: ' + t for t in tracked(args.content)] + same_gains()
        try:
            man = json.load(open(os.path.join(stage, 'uisound.json'), encoding='utf-8'))
        except OSError:
            sys.exit('S47 uisound: nothing staged in %s (the game plays today\'s sounds)' % stage)
        rows = {}
        for n, r in man['sounds'].items():
            y = read_wav(os.path.join(stage, n + '.wav'))
            r = dict(r, peak_dbfs=round(peak(y), 1))
            if 'loudest_50ms_dbfs' in r:
                r['loudest_50ms_dbfs'] = round(rms50(y), 1)
            if n == 'tv_static':
                r['seam'], r['slope'] = (round(v, 2) for v in seam(y))
            rows[n] = r
        bad += check_levels(rows)
        print('S47 uisound: %d files read back from %s; %s' % (len(rows), stage, 'all within bounds, none tracked by Git' if not bad else 'FAIL'))
        for b in bad:
            print('  FAIL ' + b)
        sys.exit(1 if bad else 0)
    if not os.path.isdir(os.path.join(args.bundle, 'Ui Clicks')):
        print('S47 uisound: no bought bundle at %s: the game keeps today\'s sounds' % args.bundle)
        return
    out, rows = bake_all(args.bundle)
    bad = check_levels(rows)
    for n, r in rows.items():
        print('S47 uisound: %-16s %-10s %5.3f s  peak %6.1f%s' % (n, r['kind'], r['secs'], r['peak_dbfs'],
              '  loudest 50 ms %6.1f' % r['loudest_50ms_dbfs'] if 'loudest_50ms_dbfs' in r else '  rms %6.1f  vs noise %s  lift %s  bands %s  seam %.2f/%.2f' % (
                  r['rms_dbfs'], r['against_white_noise_db'], r['lift_db'], r['band_shares_pct_0_1_2_4_8_24k'], r['seam'], r['slope'])))
    if bad:
        sys.exit('S47 uisound: FAIL\n  ' + '\n  '.join(bad))
    if args.check:
        return
    if not os.path.isdir(os.path.join(args.content, 'Audio', 'Raw')):
        sys.exit('S47 uisound: no %s (run Scripts/import-audio.sh first)' % os.path.join(args.content, 'Audio', 'Raw'))
    if tracked(args.content):
        sys.exit('S47 uisound: %s is tracked by Git; refusing to stage there' % stage)
    os.makedirs(stage, exist_ok=True)
    for n, y in out.items():
        write_wav(y, os.path.join(stage, n + '.wav'))
    kinds = {k: sorted(n for n, r in rows.items() if r['kind'] == k) for k in KINDS}
    with open(os.path.join(stage, 'uisound.json'), 'w', encoding='utf-8') as f:
        json.dump(dict(made_by='Tools/uisound/build_uisound.py', bundle='Ultimate SFX for Games (Function Loops)', baked_rms_dbfs=BAKE_RMS,
                       baked_peak_dbfs=BAKE_PEAK, kinds=kinds, tv_lift_db=dict(office=rows['tv_static']['lift_db']['office and room 6'],
                       motel=rows['tv_static']['lift_db']['motel']), in_game_gain_db={k: in_game(k)[0] for k in list(KINDS) + ['retain']},
                       references=REF, sounds=rows), f, indent=1, sort_keys=True)
    total = sum(os.path.getsize(os.path.join(stage, n)) for n in os.listdir(stage))
    print('S47 uisound: %d files in %s (%.1f MB), none of them in Git' % (len(os.listdir(stage)), stage, total / 1e6))
    if args.listen:
        os.makedirs(args.listen, exist_ok=True)
        for n, y in out.items():
            write_wav(y, os.path.join(args.listen, n + '.wav'))
        readme(args.listen, rows, demos(out, args.listen, rows['tv_static']['lift_db']['office and room 6']))
        print('S47 uisound: the sounds, four demos and README.txt in %s' % args.listen)


if __name__ == '__main__':
    main()
