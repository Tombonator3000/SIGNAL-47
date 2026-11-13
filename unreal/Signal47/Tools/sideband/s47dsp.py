#!/usr/bin/env python3
"""Offline stand-ins for the nodes of S47WebAudio.h (48 kHz, quantum 128), and the four colours built from them.

Only what the engine has: gain, biquad (the Web Audio formulas), delay with a modulated delayTime (linear interpolation, as
FS47DelayNode::Read), waveshaper (linear interpolation in a curve, no oversampling, as FS47WaveShaperNode::Shape), stereo
panner (the specification's stereo-in law, as FS47StereoPannerNode::Process), oscillators and looped buffers as sources and
as modulators of parameters. No convolver, no pitch shifter, no splitter/merger. The one thing baked offline is marked BAKED.

Every colour takes the music (n, 2) and an intensity curve (n,) from 0 to 1 and is transparent at 0 (apart from a fixed
latency), so it can sit on the music bus all the time.
"""
import re
import subprocess

import numpy as np
from scipy.ndimage import minimum_filter1d, uniform_filter1d
from scipy.signal import fftconvolve, lfilter

SR = 48000
Q = 128
BLK = 256  # how often a gliding filter gets new coefficients here (the engine: every frame)


# ------------------------------------------------------------------ files and meters
def load(path, start=None, secs=None, extra=None):
    cmd = ['ffmpeg', '-v', 'error']
    if start is not None:
        cmd += ['-ss', '%.4f' % start, '-t', '%.4f' % secs]
    cmd += ['-i', path]
    if extra:
        cmd += ['-af', extra]
    cmd += ['-ac', '2', '-ar', str(SR), '-f', 'f32le', '-']
    r = subprocess.run(cmd, capture_output=True)
    if r.returncode != 0:
        raise SystemExit('ffmpeg: ' + r.stderr.decode('utf-8', 'replace')[-300:])
    return np.frombuffer(r.stdout, dtype=np.float32).astype(np.float64).reshape(-1, 2)


def measure(x):
    """Integrated loudness (LUFS) and true peak (dBTP) of an array."""
    r = subprocess.run(['ffmpeg', '-hide_banner', '-nostats', '-f', 'f32le', '-ar', str(SR), '-ac', '2', '-i', '-',
                        '-af', 'ebur128=peak=true', '-f', 'null', '-'], input=x.astype('<f4').tobytes(), capture_output=True)
    t = r.stderr.decode('utf-8', 'replace')
    t = t[t.rfind('Summary:'):]
    return float(re.search(r'I:\s+(-?[\d.]+) LUFS', t).group(1)), float(re.search(r'Peak:\s+(-?[\d.]+) dBFS', t).group(1))


def measure_file(path):
    r = subprocess.run(['ffmpeg', '-hide_banner', '-nostats', '-i', path, '-af', 'ebur128=peak=true', '-f', 'null', '-'], capture_output=True)
    t = r.stderr.decode('utf-8', 'replace')
    t = t[t.rfind('Summary:'):]
    return float(re.search(r'I:\s+(-?[\d.]+) LUFS', t).group(1)), float(re.search(r'Peak:\s+(-?[\d.]+) dBFS', t).group(1))


def limiter(x, ceiling_db=-2.0, win=0.02):
    """A look-ahead ceiling for the listening copies (not part of any colour)."""
    c = 10 ** (ceiling_db / 20)
    peak = np.abs(x).max(axis=1)
    g = np.minimum(1.0, c / np.maximum(peak, 1e-9))
    w = int(win * SR) | 1
    g = uniform_filter1d(minimum_filter1d(g, w, mode='nearest'), w, mode='nearest')
    return x * g[:, None], float(20 * np.log10(g.min()))


def write_mp3(x, path, lufs=-16.0, tp=-1.0):
    """To lufs integrated, under tp dBTP after the mp3 encoder (measured on the file itself); returns (LUFS, dBTP, limiter dB)."""
    i, _ = measure(x)
    y = x * 10 ** ((lufs - i) / 20)
    ceiling = -2.0
    for _ in range(6):
        z, red = limiter(y, ceiling)
        i2, _ = measure(z)
        z = z * 10 ** ((lufs - i2) / 20)
        z, red2 = limiter(z, ceiling)
        r = subprocess.run(['ffmpeg', '-v', 'error', '-y', '-f', 'f32le', '-ar', str(SR), '-ac', '2', '-i', '-', '-c:a', 'libmp3lame',
                            '-b:a', '192k', path], input=z.astype('<f4').tobytes(), capture_output=True)
        if r.returncode != 0:
            raise SystemExit('ffmpeg: ' + r.stderr.decode('utf-8', 'replace')[-300:])
        li, lp = measure_file(path)
        if lp <= tp:
            return li, lp, min(red, red2)
        ceiling -= (lp - tp) + 0.2
    raise SystemExit('could not get %s under %.1f dBTP' % (path, tp))


# ------------------------------------------------------------------ curves over time
def env(n, points, shape='smooth'):
    """A curve through (seconds, value) points; between them a smoothstep glide (the engine: setTargetAtTime or linear ramps)."""
    t = np.arange(n) / SR
    ts = np.array([p[0] for p in points], dtype=float)
    vs = np.array([p[1] for p in points], dtype=float)
    k = np.clip(np.searchsorted(ts, t, side='right') - 1, 0, len(ts) - 2)
    u = np.clip((t - ts[k]) / np.maximum(ts[k + 1] - ts[k], 1e-9), 0, 1)
    if shape == 'smooth':
        u = u * u * (3 - 2 * u)
    return vs[k] + (vs[k + 1] - vs[k]) * u


def sine(n, hz, phase=0.0):
    return np.sin(2 * np.pi * hz * np.arange(n) / SR + phase)


def db(v):
    return 10 ** (np.asarray(v) / 20.0)


# ------------------------------------------------------------------ nodes
def coefs(kind, f, q=0.0, g=0.0):
    """BiquadFilterNode's coefficients (Web Audio): q is dB for lowpass/highpass, plain Q otherwise; g is dB."""
    f = min(max(f, 1.0), SR / 2 - 1)
    w0 = 2 * np.pi * f / SR
    cs, sn = np.cos(w0), np.sin(w0)
    A = 10 ** (g / 40.0)
    if kind in ('lowpass', 'highpass'):
        al = sn / (2 * 10 ** (q / 20.0))
        a = [1 + al, -2 * cs, 1 - al]
        b = [(1 - cs) / 2, 1 - cs, (1 - cs) / 2] if kind == 'lowpass' else [(1 + cs) / 2, -(1 + cs), (1 + cs) / 2]
    elif kind == 'bandpass':
        al = sn / (2 * max(q, 1e-4))
        b, a = [al, 0, -al], [1 + al, -2 * cs, 1 - al]
    elif kind == 'peaking':
        al = sn / (2 * max(q, 1e-4))
        b, a = [1 + al * A, -2 * cs, 1 - al * A], [1 + al / A, -2 * cs, 1 - al / A]
    elif kind == 'highshelf':
        al = sn / 2 * np.sqrt(2.0)
        s = 2 * al * np.sqrt(A)
        b = [A * ((A + 1) + (A - 1) * cs + s), -2 * A * ((A - 1) + (A + 1) * cs), A * ((A + 1) + (A - 1) * cs - s)]
        a = [(A + 1) - (A - 1) * cs + s, 2 * ((A - 1) - (A + 1) * cs), (A + 1) - (A - 1) * cs - s]
    elif kind == 'lowshelf':
        al = sn / 2 * np.sqrt(2.0)
        s = 2 * al * np.sqrt(A)
        b = [A * ((A + 1) - (A - 1) * cs + s), 2 * A * ((A - 1) - (A + 1) * cs), A * ((A + 1) - (A - 1) * cs - s)]
        a = [(A + 1) + (A - 1) * cs + s, -2 * ((A - 1) + (A + 1) * cs), (A + 1) + (A - 1) * cs - s]
    else:
        raise ValueError(kind)
    b, a = np.array(b) / a[0], np.array(a) / a[0]
    return b, a


def biquad(x, kind, f, q=0.0, g=0.0):
    """One BiquadFilterNode. f, q, g: numbers, or curves of len(x) (then the coefficients follow them every BLK frames)."""
    n = len(x)
    if np.isscalar(f) and np.isscalar(q) and np.isscalar(g):
        b, a = coefs(kind, f, q, g)
        return lfilter(b, a, x, axis=0)
    fa = np.broadcast_to(f, (n,))
    qa = np.broadcast_to(q, (n,))
    ga = np.broadcast_to(g, (n,))
    y = np.empty_like(x)
    zi = np.zeros((2,) + x.shape[1:])
    last = None
    for s in range(0, n, BLK):
        e = min(n, s + BLK)
        m = (s + e) // 2
        key = (fa[m], qa[m], ga[m])
        if key != last:
            b, a = coefs(kind, *key)
            last = key
        y[s:e], zi = lfilter(b, a, x[s:e], axis=0, zi=zi)
    return y


def delay(x, secs):
    """DelayNode: secs a number or a curve; linear interpolation between frames."""
    n = len(x)
    t = np.arange(n, dtype=np.float64)
    src = t - np.broadcast_to(secs, (n,)) * SR
    if x.ndim == 1:
        return np.interp(src, t, x, left=0.0)
    return np.stack([np.interp(src, t, x[:, c], left=0.0) for c in range(x.shape[1])], axis=1)


def shaper(x, curve):
    """WaveShaperNode, oversample 'none': the curve spans -1..1, the input is clamped to it."""
    return np.interp(x, np.linspace(-1, 1, len(curve)), curve)


def panner(x, pan):
    """StereoPannerNode on a stereo input (pan a number or a curve)."""
    p = np.broadcast_to(pan, (len(x),))
    L, R = x[:, 0], x[:, 1]
    neg = p <= 0
    xa = np.where(neg, p + 1.0, p) * np.pi / 2
    outL = np.where(neg, L + R * np.cos(xa), L * np.cos(xa))
    outR = np.where(neg, R * np.sin(xa), R + L * np.sin(xa))
    return np.stack([outL, outR], axis=1)


def place(mono, pos):
    """A one-channel signal through a StereoPannerNode: pos -1..1, equal power."""
    a = (np.broadcast_to(pos, (len(mono),)) + 1) * np.pi / 4
    return np.stack([mono * np.cos(a), mono * np.sin(a)], axis=1)


def loop(buf, n, offset=0):
    """A looping AudioBufferSourceNode."""
    return buf[(np.arange(n) + offset) % len(buf)]


# ------------------------------------------------------------------ the buffers the colours need (built once, the same every run)
def buffers():
    rng = np.random.default_rng(47)
    out = {}
    # hiss: 4.0 s of white noise, two channels, RMS -20 dBFS
    h = rng.standard_normal((4 * SR, 2))
    out['hiss'] = h / np.sqrt((h ** 2).mean()) * 0.1
    # crackle: 7.3 s, about 2.5 single-frame ticks a second of random size and sign, the same in both channels but for a little
    n = int(7.3 * SR)
    c = np.zeros((n, 2))
    k = rng.random(n) < 2.5 / SR
    amp = rng.uniform(0.2, 1.0, k.sum()) ** 2 * rng.choice([-1, 1], k.sum())
    c[k, 0] = amp
    c[k, 1] = amp * rng.uniform(0.5, 1.0, k.sum())
    out['crackle'] = c
    # drift: 16.4 s of noise under 1.5 Hz, peak 1: the irregular part of the wow (a control signal, fed into delayTime)
    n = int(16.4 * SR)
    spec = np.fft.rfft(rng.standard_normal(n))
    fr = np.fft.rfftfreq(n, 1 / SR)
    spec[(fr > 1.5) | (fr < 0.08)] = 0
    d = np.fft.irfft(spec, n)
    out['drift'] = d / np.abs(d).max()
    # gate: 5.9 s, 1 with six holes to 0 of 25 to 110 ms (about one a second), 3 ms slopes: the signal dropping out (a control
    # signal: it goes into gain parameters)
    n = int(5.9 * SR)
    gte = np.ones(n)
    t = 0.3
    while t < 5.7:
        t += rng.exponential(1 / 1.6)
        ln = rng.uniform(0.025, 0.11)
        s, e = int(t * SR), int((t + ln) * SR)
        if e < n - int(0.05 * SR):
            gte[s:e] = 0.0
        t += ln + 0.06
    gte = uniform_filter1d(gte, int(0.003 * SR), mode='wrap')
    out['gate'] = gte
    return out


BUF = buffers()

# the waveshaper curves (4097 points)
_X = np.linspace(-1, 1, 4097)
CURVE_TAPE = np.tanh(1.6 * _X) / 1.6                                  # worn: the peaks round off, the body is left alone
CURVE_CLIP = np.where(_X >= 0, np.tanh(2.5 * _X) / np.tanh(2.5), 0.75 * np.tanh(3.3 * _X) / np.tanh(3.3))  # broken: uneven, hard
STEPS = 6
CURVE_STEP = np.round(_X * STEPS) / STEPS                               # broken: 13 levels (about 3.7 bits), mid-tread


# ------------------------------------------------------------------ WORN
WORN = dict(base=0.012, wow1=(0.41, 0.0035), wow2=(0.93, 0.0015), flutter=(6.7, 0.0010), drift=0.00035,
            sat_pre=8.0, sat_mix=0.8, lp=(18000.0, 2600.0), hp=(20.0, 70.0), wobble=(0.31, 0.10),
            hiss_db=-47.0, crackle_db=-36.0, makeup_db=1.4)


def worn(x, w, P=WORN):
    """Bus -> Delay(12 ms, delayTime moved by three oscillators and a drift buffer) -> [Gain dry | Gain pre -> WaveShaper tape ->
    Gain post] -> Biquad lowpass (18 k -> 2.6 k) -> Biquad highpass (20 -> 70) -> Gain (level wobble by an oscillator)
    + hiss buffer -> highpass 1.2 k -> lowpass 8 k -> Gain + crackle buffer -> bandpass 2.8 k -> Gain."""
    n = len(x)
    # pitch deviation of a sine in delayTime of depth D at f Hz is 2*pi*f*D: the depths below are set from the wanted deviation
    mod = np.zeros(n)
    for hz, dev, ph in ((P['wow1'][0], P['wow1'][1], 0.0), (P['wow2'][0], P['wow2'][1], 1.3), (P['flutter'][0], P['flutter'][1], 0.4)):
        mod += dev / (2 * np.pi * hz) * sine(n, hz, ph)
    mod += P['drift'] * loop(BUF['drift'], n)
    y = delay(x, P['base'] + w * mod)
    pre = db(P['sat_pre'])
    m = (P['sat_mix'] * w)[:, None]
    y = y * (1 - m) + shaper(y * pre, CURVE_TAPE) / pre * m
    y = biquad(y, 'lowpass', P['lp'][0] * (P['lp'][1] / P['lp'][0]) ** w, -3.0)
    y = biquad(y, 'highpass', P['hp'][0] * (P['hp'][1] / P['hp'][0]) ** w, -3.0)
    y = y * (1 + w * P['wobble'][1] * sine(n, P['wobble'][0], 0.7))[:, None]
    y = y * db(P['makeup_db'] * w)[:, None]
    hiss = biquad(biquad(loop(BUF['hiss'], n), 'highpass', 1200.0, -3.0), 'lowpass', 8000.0, -3.0)
    hiss *= db(P['hiss_db']) / np.sqrt((hiss ** 2).mean())
    crk = biquad(loop(BUF['crackle'], n), 'bandpass', 2800.0, 0.9)
    crk *= db(P['crackle_db']) / np.abs(crk).max()
    return y + hiss * (w ** 1.5)[:, None] + crk * (w ** 2)[:, None]


# ------------------------------------------------------------------ BROKEN
BROKEN = dict(hp=(20.0, 320.0), lp=(18000.0, 2800.0), lp_q=4.0, peak=(1500.0, 1.0, 7.0), drive=(4.0, 18.0),
              step_from=0.25, step_span=0.6, ring=(9000.0, 3700.0, 0.5), gate_from=0.4, gate_span=0.5, gate_depth=0.85,
              static_db=-46.0, static_hole_db=-30.0, dirty_db=(-8.5, -21.5), makeup_db=0.0)


def saw(freq):
    """A sawtooth oscillator whose frequency glides, band-limited (the engine's is PolyBLEP): harmonics under 20 kHz only."""
    ph = 2 * np.pi * np.cumsum(freq) / SR
    y = np.zeros(len(freq))
    for k in range(1, 6):
        y += np.where(freq * k < 20000, np.sin(k * ph) / k, 0.0)
    return y * 2 / np.pi


def hit_curves(n, at):
    """What one hard hit at `at` seconds adds to the automation: (push 0..1, extra drive dB, level dB, static 0..1, hole 0..1)."""
    t = np.arange(n) / SR - at
    on = t >= 0
    push = np.where(on, np.minimum(1.0, t / 0.008) * np.exp(-np.maximum(t - 0.25, 0) / 0.25), 0.0)
    burst = np.where(on, np.minimum(1.0, t / 0.003) * np.exp(-np.maximum(t - 0.11, 0) / 0.04), 0.0)
    static = np.where(on, np.minimum(1.0, t / 0.003) * np.exp(-np.maximum(t - 0.05, 0) / 0.07), 0.0)
    hole = np.clip((t - 0.13) / 0.004, 0, 1) * np.clip((0.25 - t) / 0.004, 0, 1)
    return np.clip(push, 0, 1), 10.0 * burst, 3.0 * burst, static, hole


def broken(x, b, hits=(), P=BROKEN):
    """Bus -> Biquad highpass (20 -> 320) -> [Gain clean | Gain drive -> WaveShaper clip -> (Gain | WaveShaper steps -> Gain) ->
    Gain (ring: gain moved at audio rate by a sawtooth oscillator) -> highpass 60 -> Gain] -> Gain gate (moved by the gate buffer)
    -> two Biquad lowpasses (18 k -> 2.8 k, the first resonant) -> Biquad peaking 1.5 k + static: hiss buffer -> bandpass 2.4 k -> Gain (moved
    by the gate buffer, inverted). A hit is automation on the same nodes."""
    n = len(x)
    b = np.clip(b, 0, 1)
    drive_x = np.zeros(n)
    level_x = np.zeros(n)
    static_x = np.zeros(n)
    hole = np.zeros(n)
    for at in hits:
        push, d, l, s, h = hit_curves(n, at)
        b = np.maximum(b, push)
        drive_x += d
        level_x += l
        static_x = np.maximum(static_x, s)
        hole = np.maximum(hole, h)
    y = biquad(x, 'highpass', P['hp'][0] * (P['hp'][1] / P['hp'][0]) ** b, 0.0)
    m = b ** 0.7
    c = np.clip((b - P['step_from']) / P['step_span'], 0, 1)
    d = shaper(y * db(P['drive'][0] + (P['drive'][1] - P['drive'][0]) * b + drive_x)[:, None], CURVE_CLIP)
    d = d * (1 - c)[:, None] + shaper(d, CURVE_STEP) * c[:, None]
    d = d * (1 + P['ring'][2] * c * saw(P['ring'][0] * (P['ring'][1] / P['ring'][0]) ** b))[:, None]
    d = biquad(d, 'highpass', 60.0, -3.0)
    y = y * (1 - m)[:, None] + d * (m * db(P['dirty_db'][0] + (P['dirty_db'][1] - P['dirty_db'][0]) * b + level_x))[:, None]
    gd = np.clip((b - P['gate_from']) / P['gate_span'], 0, 1) * P['gate_depth']
    gate = loop(BUF['gate'], n)
    y = y * ((1 - gd * (1 - gate)) * (1 - hole))[:, None]
    cut = P['lp'][0] * (P['lp'][1] / P['lp'][0]) ** b
    y = biquad(biquad(y, 'lowpass', cut, P['lp_q']), 'lowpass', cut, -3.0)
    y = biquad(y, 'peaking', P['peak'][0], P['peak'][1], P['peak'][2] * b)
    y = y * db(P['makeup_db'] * b)[:, None]
    st = biquad(loop(BUF['hiss'], n, 12345), 'bandpass', 2400.0, 0.6)
    st /= np.sqrt((st ** 2).mean())
    lvl = b ** 2 * db(P['static_db']) + (1 - gate) * gd * db(P['static_hole_db']) + static_x * db(-24.0)
    return y + st * lvl[:, None]


# ------------------------------------------------------------------ WIDE
WIDE = dict(v1=(0.018, 0.0035, 0.19, -0.55), v2=(0.027, 0.0045, 0.27, 0.55), voice=0.6, voice_hp=250.0, dry_drop=0.15,
            sweep=(900.0, 1680.0, 0.083, 0.9, 6.0), pan=(0.12, 0.4), makeup_db=-2.8)


def wide(x, a, P=WIDE):
    """Bus -> [Gain dry | Delay 18 ms (moved 3.5 ms by an oscillator at 0.19 Hz) -> highpass 250 -> StereoPanner (-0.55, moved
    +-0.4 by an oscillator at 0.12 Hz) -> Gain | Delay 27 ms (4.5 ms at 0.27 Hz) -> highpass 250 -> StereoPanner (+0.55, moved
    by the same oscillator) -> Gain] -> Biquad peaking (centre swept by an oscillator into detune, +-1680 cents round 900 Hz
    at 0.083 Hz). The body stays where it is; the halo round it drifts from side to side."""
    n = len(x)
    y = x * (1 - P['dry_drop'] * a)[:, None]
    sway = P['pan'][1] * sine(n, P['pan'][0])
    for k, (base, depth, hz, pos) in enumerate((P['v1'], P['v2'])):
        v = biquad(delay(x, base + depth * sine(n, hz, 2.1 * k)), 'highpass', P['voice_hp'], -3.0)
        y = y + panner(v, pos + sway) * (P['voice'] * a)[:, None]
    f0, cents, hz, q, gain = P['sweep']
    y = biquad(y, 'peaking', f0 * 2 ** (cents / 1200.0 * sine(n, hz)), q, gain * a)
    return y * db(P['makeup_db'] * a)[:, None]


# ------------------------------------------------------------------ FAR
FAR = dict(dry_left=0.10, dry_lp=(18000.0, 1500.0), dry_pan=(0.07, 0.6), send_hp=200.0, send_lp=3000.0,
           pp=(0.56, 0.56, 0.55, 1600.0, 0.8, 0.5), pp_to_rev=0.6, predelay=0.030, ap=((0.0079, 0.62), (0.0127, 0.62)),
           lines=(0.0613, 0.0739, 0.0891, 0.1017, 0.1273, 0.1439, 0.1631, 0.1877), rt60=7.0, shelf=(1800.0, 14.0),
           mod=((2, 0.0004, 0.11), (6, 0.0004, 0.14)), rev_lp=4500.0, rev=0.5, ghost=0.5, makeup_db=0.0)


def allpass(x, secs, g):
    """A Schroeder diffuser: Gain sum -> Delay -> out, with Gain g back into the sum and Gain -g forward (ideal here: in the
    engine the loop is one quantum longer than the forward path, see NOTES)."""
    M = int(round(secs * SR))
    n = len(x)
    v = np.zeros(n + M)
    y = np.empty(n)
    for s in range(0, n, M):
        e = min(n, s + M)
        vd = v[s:e]
        vn = x[s:e] + g * vd
        y[s:e] = -g * vn + vd
        v[s + M:e + M] = vn
    return y


def pingpong(send, tA, tB, fb, lp_hz):
    """send -> Delay A -> Delay B -> Biquad lowpass -> Gain fb -> back into Delay A. Returns what A and B give."""
    dA, dB = int(round(tA * SR)), int(round(tB * SR))
    n = len(send)
    per = dA + dB
    ain = np.zeros(n + per)  # ain[k + per] is what goes into A at frame k
    b, a = coefs('lowpass', lp_hz, -3.0)
    zi = np.zeros(2)
    for s in range(0, n, per):
        e = min(n, s + per)
        back, zi = lfilter(b, a, ain[s:e] * fb, zi=zi)  # B's output at s..e is A's input a period earlier
        ain[s + per:e + per] = send[s:e] + back
    a_in = ain[per:]
    A = np.concatenate([np.zeros(dA), a_in])[:n]
    B = np.concatenate([np.zeros(per), a_in])[:n]
    return A, B


def fdn(x, lines, rt60, shelf, mod=()):
    """Eight Delay lines, each back into itself through Gain (its share of rt60) and a Biquad highshelf (the highs die faster),
    all mixed by one shared Gain of -2/8 fed by every line (a Householder reflection: lossless before the gains).
    Returns the eight outputs (8, n)."""
    N = len(lines)
    d = [int(round(v * SR)) for v in lines]
    n = len(x)
    blk = min(d) - 64
    pad = max(d) + 64
    buf = np.zeros((N, n + pad + blk))
    outs = np.zeros((N, n))
    g = [10 ** (-3.0 * v / rt60) for v in lines]
    flt = [coefs('highshelf', shelf[0], 0.0, -shelf[1] * v) for v in lines]
    zi = [np.zeros(2) for _ in lines]
    sign = np.array([1, -1, 1, -1, 1, -1, 1, -1][:N]) / np.sqrt(N)
    modmap = {m[0]: m for m in mod}
    for s in range(0, n, blk):
        e = min(n, s + blk)
        o = np.empty((N, e - s))
        for i in range(N):
            if i in modmap:
                _, depth, hz = modmap[i]
                t = np.arange(s, e)
                pos = pad + t - d[i] - depth * SR * np.sin(2 * np.pi * hz * t / SR)
                k = np.floor(pos).astype(int)
                fr = pos - k
                o[i] = buf[i, k] * (1 - fr) + buf[i, k + 1] * fr
            else:
                o[i] = buf[i, pad + s - d[i]:pad + e - d[i]]
        outs[:, s:e] = o
        mixed = o - (2.0 / N) * o.sum(axis=0)
        for i in range(N):
            back, zi[i] = lfilter(flt[i][0], flt[i][1], mixed[i] * g[i], zi=zi[i])
            buf[i, pad + s:pad + e] = sign[i] * x[s:e] + back
    return outs


def far(x, f, ghost=None, trim_db=0.0, P=FAR):
    """Bus -> [Gain dry (1 -> 0.10, as the square of the intensity: it leaves later than the wash arrives) -> Biquad lowpass (18 k -> 1.5 k) -> StereoPanner (moved at 0.07 Hz)]
    | [Gain send (one channel) -> highpass 200 -> lowpass 3 k -> ping-pong (two Delays of 0.56 s, lowpass 1.6 k and Gain 0.55
    round the loop, a StereoPanner each) and -> Delay 30 ms -> two diffusers -> eight-line network -> two StereoPanners ->
    lowpass 4.5 k -> Gain]. ghost: the BAKED layer for this piece, brought up with f."""
    n = len(x)
    dry = biquad(x * (1 - (1 - P['dry_left']) * f ** 2)[:, None], 'lowpass', P['dry_lp'][0] * (P['dry_lp'][1] / P['dry_lp'][0]) ** f, -3.0)
    dry = panner(dry, P['dry_pan'][1] * f * sine(n, P['dry_pan'][0]))
    send = x.mean(axis=1) * f
    send = biquad(biquad(send, 'highpass', P['send_hp'], -3.0), 'lowpass', P['send_lp'], -3.0)
    tA, tB, fb, lp, pos, lvl = P['pp']
    A, B = pingpong(send, tA, tB, fb, lp)
    wet = (place(A, -pos) + place(B, pos)) * lvl
    r = delay(send + P['pp_to_rev'] * (A + B), P['predelay'])
    for secs, g in P['ap']:
        r = allpass(r, secs, g)
    o = fdn(r, P['lines'], P['rt60'], P['shelf'], P['mod'])
    sg = np.array([1, 1, -1, -1, 1, 1, -1, -1])[:, None]
    left = (o[0::2] * sg[0::2]).sum(axis=0)
    right = (o[1::2] * sg[1::2]).sum(axis=0)
    rev = place(left, -0.8) + place(right, 0.8)
    rev = biquad(rev, 'lowpass', P['rev_lp'], -3.0) * P['rev']
    y = dry + (wet + rev) * db(trim_db)
    if ghost is not None:
        y = y + ghost[:n] * (P['ghost'] * f)[:, None]
    return y * db(P['makeup_db'] * f)[:, None]


# ------------------------------------------------------------------ BAKED: what the nodes cannot do, rendered beforehand per piece
def bake_ghost(x, path=None, start=None, secs=None):
    """The reverse atmosphere and the shimmer of a passage, as one layer to start in step with the piece (a buffer or a second
    media deck): the passage backwards through a long room and turned round again (it swells into each note), plus the passage an
    octave up (rubberband, offline) through the same kind of room. Convolution and pitch shifting are fine here: this is baked."""
    rng = np.random.default_rng(4747)

    def room(secs_, lo, hi):
        m = int(secs_ * SR)
        t = np.arange(m) / SR
        ir = rng.standard_normal((m, 2)) * np.exp(-6.9 * t / (secs_ * 0.8))[:, None]
        ir = biquad(biquad(ir, 'highpass', lo, -3.0), 'lowpass', hi, -3.0)
        return ir / np.sqrt((ir ** 2).sum(axis=0))

    band = biquad(biquad(x, 'highpass', 250.0, -3.0), 'lowpass', 4000.0, -3.0)
    ir = room(3.5, 250.0, 3500.0)
    back = band[::-1]
    sw = np.stack([fftconvolve(back[:, c], ir[:, c])[:len(x)] for c in (0, 1)], axis=1)[::-1]
    ref = np.sqrt((x ** 2).mean())
    sw *= db(-7.0) * ref / np.sqrt((sw ** 2).mean())
    out = sw
    if path is not None:
        up = load(path, start, secs, 'rubberband=pitch=2.0')
        up = np.concatenate([up, np.zeros((max(0, len(x) - len(up)), 2))])[:len(x)]
        up = biquad(biquad(up, 'highpass', 800.0, -3.0), 'lowpass', 6000.0, -3.0)
        ir2 = room(5.0, 700.0, 6000.0)
        sh = np.stack([fftconvolve(up[:, c], ir2[:, c])[:len(x)] for c in (0, 1)], axis=1)
        sh *= db(-14.0) * ref / np.sqrt((sh ** 2).mean())
        out = out + sh
    return out


# the far colour's trim per piece (dB on everything wet): a long room rings up far more on a held pad than on a plucked guitar,
# so one number per piece, measured (tune.py), as the score's table already has a gain per piece
FAR_TRIM = {'cold_dish': -10.0, 'steelguitar': -6.5, 'night_freight': -3.0}
