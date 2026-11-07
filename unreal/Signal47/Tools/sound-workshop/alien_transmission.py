#!/usr/bin/env python3
"""Original distorted radio transmission for the alien encounter (ALIEN-CLIP-20261106-06).

Replaces the private film clip alien_interference.wav (7.000 s, mono, 48 kHz, PCM16) with a file that is
synthesised here from nothing: no recording, no human voice, no film or game sound is read or sampled.

Signal chain (all Python standard library, deterministic for a seed):
  1. Message: granular synthesis of short grains (25-140 ms, Hann window). Each grain is a pulse train
     (pitch 55-210 Hz with a glide) pushed through three resonators set to synthetic "formants" that are
     drawn per burst and drift. The pulses are not speech: the formant sets mix vowel-like and impossible
     values (extra low and extra high), and a part of the grains is played backwards.
  2. AM carrier: y = (0.28 + message) * cos(2 pi fc t), fc = 1.65 kHz with a slow random drift, i.e. a faint
     carrier with double sideband. A little of the baseband message is added so the formants stay readable.
  3. Band-pass 300 Hz to 3.4 kHz (two RBJ biquads), as a small set's speaker and IF stage would give.
  4. Static: white noise, high-passed crackle, clicks, a heterodyne whistle that sweeps while the set
     "hunts", and squelch dropouts. The static swells between the bursts and ducks under them.
  5. Soft clip (tanh) and a small sample-and-hold downsample for grit, 12 ms fades, peak set to PEAK.

Usage, from unreal/Signal47:
  python3 Tools/sound-workshop/alien_transmission.py                # writes Content/Audio/Raw/sound_design/radio_transmission_original.wav
  python3 Tools/sound-workshop/alien_transmission.py --out /tmp/x.wav --seed 47
  python3 Tools/sound-workshop/alien_transmission.py --check        # builds twice, verifies identical bytes and the numbers below
Prints duration, RMS, peak, spectral centroid, band shares and the SHA-256 of the file.
"""
import argparse
import cmath
import hashlib
import math
import random
import struct
import wave
from pathlib import Path

PROJECT = Path(__file__).resolve().parents[2]
DEFAULT_OUT = PROJECT / 'Content/Audio/Raw/sound_design/radio_transmission_original.wav'
RATE = 48000
SECONDS = 7.0
FRAMES = int(RATE * SECONDS)  # 336000, as the film clip had
PEAK = 0.62
DEFAULT_SEED = 47


# ---------- filters ----------

def biquad(kind, f0, q):
    w0 = 2 * math.pi * f0 / RATE
    alpha = math.sin(w0) / (2 * q)
    c = math.cos(w0)
    if kind == 'hp':
        b = [(1 + c) / 2, -(1 + c), (1 + c) / 2]
    elif kind == 'lp':
        b = [(1 - c) / 2, 1 - c, (1 - c) / 2]
    else:
        raise ValueError(kind)
    a = [1 + alpha, -2 * c, 1 - alpha]
    return [b[0] / a[0], b[1] / a[0], b[2] / a[0]], [a[1] / a[0], a[2] / a[0]]


def run_biquad(x, coef):
    (b0, b1, b2), (a1, a2) = coef
    y = [0.0] * len(x)
    x1 = x2 = y1 = y2 = 0.0
    for i, v in enumerate(x):
        o = b0 * v + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2
        y[i] = o
        x2, x1 = x1, v
        y2, y1 = y1, o
    return y


def resonate(x, freq, bandwidth):
    """Two-pole resonator with unity gain at the centre, for one formant."""
    r = math.exp(-math.pi * bandwidth / RATE)
    a1 = -2 * r * math.cos(2 * math.pi * freq / RATE)
    a2 = r * r
    g = 1 - r
    y = [0.0] * len(x)
    y1 = y2 = 0.0
    for i, v in enumerate(x):
        o = g * v - a1 * y1 - a2 * y2
        y[i] = o
        y2, y1 = y1, o
    return y


# ---------- message: granular synthetic formants ----------

def formant_set(rng, mood):
    """Three formant centres in Hz. mood 0: vowel-like, 1: stretched, 2: impossible (outside any mouth)."""
    if mood == 0:
        return [rng.uniform(300, 850), rng.uniform(900, 2300), rng.uniform(2300, 3200)]
    if mood == 1:
        return [rng.uniform(180, 450), rng.uniform(1300, 3000), rng.uniform(3000, 4200)]
    return [rng.uniform(110, 260), rng.uniform(600, 1100), rng.uniform(3300, 5200)]


def grain(rng, formants, length):
    f0a = rng.uniform(55, 210)
    f0b = f0a * rng.uniform(0.6, 1.5)
    pulses = [0.0] * length
    phase = 0.0
    for i in range(length):
        f0 = f0a + (f0b - f0a) * i / length
        phase += f0 / RATE
        if phase >= 1.0:
            phase -= 1.0
            pulses[i] = 1.0 - 0.4 * rng.random()
    out = [0.0] * length
    for k, f in enumerate(formants):
        bw = (60, 90, 140)[k] * rng.uniform(0.7, 1.8)
        res = resonate(pulses, f, bw)
        weight = (1.0, 0.7, 0.45)[k]
        for i in range(length):
            out[i] += res[i] * weight
    if rng.random() < 0.4:
        out.reverse()
    peak = max(abs(v) for v in out) or 1.0
    win = [0.5 - 0.5 * math.cos(2 * math.pi * i / (length - 1)) for i in range(length)]
    return [out[i] / peak * win[i] for i in range(length)]


def message(rng, bursts):
    m = [0.0] * FRAMES
    for start, end, mood in bursts:
        formants = formant_set(rng, mood)
        t = start
        while t < end:
            length = int(RATE * rng.uniform(0.025, 0.14))
            pos = int(t * RATE)
            if pos + length >= FRAMES:
                break
            # the formants wander a little between grains: the "speaker" never settles
            f = [v * rng.uniform(0.9, 1.12) for v in formants]
            if rng.random() < 0.18:
                f = formant_set(rng, rng.randrange(3))
            g = grain(rng, f, length)
            amp = rng.uniform(0.45, 1.0)
            for i in range(length):
                m[pos + i] += g[i] * amp
            t += rng.uniform(0.012, 0.075)  # overlapping grains
    peak = max(abs(v) for v in m) or 1.0
    return [v / peak for v in m]


# ---------- assembly ----------

def envelope(bursts, fade=0.03):
    """1 inside a burst, 0 outside, with short ramps."""
    e = [0.0] * FRAMES
    for start, end, _ in bursts:
        a, b, r = int(start * RATE), int(end * RATE), int(fade * RATE)
        for i in range(max(0, a), min(FRAMES, b)):
            k = min(1.0, (i - a) / r, (b - i) / r)
            e[i] = max(e[i], k)
    return e


def build(seed=DEFAULT_SEED):
    rng = random.Random(seed)
    bursts = [(0.62, 1.95, 0), (2.35, 3.30, 2), (3.80, 5.25, 1), (5.55, 6.45, 2)]
    env = envelope(bursts)
    msg = message(rng, bursts)

    # AM: faint carrier plus double sideband, carrier drifts slowly
    out = [0.0] * FRAMES
    phase = 0.0
    drift = 0.0
    for i in range(FRAMES):
        drift += (rng.gauss(0, 1) * 0.6 - drift * 0.0004)
        fc = 1650.0 + 55.0 * math.sin(2 * math.pi * 0.37 * i / RATE) + drift
        phase += 2 * math.pi * fc / RATE
        out[i] = (0.28 + 0.8 * msg[i]) * math.cos(phase) + 0.45 * msg[i]

    # band-pass 300 Hz - 3.4 kHz
    out = run_biquad(out, biquad('hp', 300, 0.8))
    out = run_biquad(out, biquad('lp', 3400, 0.8))

    # static: hiss swells between the bursts, ducks under them
    white = [rng.gauss(0, 1) for _ in range(FRAMES)]
    hiss = run_biquad(white, biquad('hp', 900, 0.7))
    hiss = run_biquad(hiss, biquad('lp', 5200, 0.7))
    # heterodyne whistle hunting for the carrier
    whistle = [0.0] * FRAMES
    wp = 0.0
    for i in range(FRAMES):
        t = i / RATE
        f = 1250 + 900 * math.sin(2 * math.pi * 0.23 * t) + 350 * math.sin(2 * math.pi * 1.7 * t + 1.3)
        wp += 2 * math.pi * f / RATE
        whistle[i] = math.sin(wp)
    # clicks and crackle
    crackle = [0.0] * FRAMES
    for _ in range(int(SECONDS * 55)):
        pos = rng.randrange(FRAMES - 40)
        amp = rng.uniform(0.3, 1.0) * (1 if rng.random() < 0.5 else -1)
        for k in range(rng.randrange(3, 30)):
            crackle[pos + k] += amp * math.exp(-k / 6.0) * (1 if k % 2 == 0 else -1)
    # squelch dropouts: the audio falls away for a few hundredths of a second
    gate = [1.0] * FRAMES
    for _ in range(9):
        a = rng.randrange(int(0.3 * RATE), FRAMES - 3000)
        n = int(RATE * rng.uniform(0.012, 0.09))
        for i in range(a, min(FRAMES, a + n)):
            gate[i] = 0.08

    mix = [0.0] * FRAMES
    for i in range(FRAMES):
        t = i / RATE
        swell = 0.55 + 0.45 * (1 - env[i]) + 0.25 * math.sin(2 * math.pi * 0.31 * t) ** 2
        tail = 1.0 + 1.6 * max(0.0, (t - 6.55) / 0.45)  # the set loses the station at the end
        sig = out[i] * gate[i] * (0.30 + 0.70 * env[i])
        noise = 0.18 * hiss[i] * swell * tail + 0.05 * whistle[i] * (1 - 0.6 * env[i]) + 0.22 * crackle[i]
        mix[i] = sig * 1.3 + noise

    # grit: soft clip, then a coarse sample-and-hold (about 12 kHz) and the fades
    held = 0.0
    for i in range(FRAMES):
        if i % 4 == 0:
            held = math.tanh(mix[i] * 1.1)
        mix[i] = 0.7 * math.tanh(mix[i] * 1.1) + 0.3 * held
    fade = int(0.012 * RATE)
    for i in range(fade):
        mix[i] *= i / fade
        mix[FRAMES - 1 - i] *= i / fade
    top = max(abs(v) for v in mix) or 1.0
    return [v / top * PEAK for v in mix]


def to_pcm16(samples):
    return b''.join(struct.pack('<h', max(-32768, min(32767, int(round(v * 32767))))) for v in samples)


def write_wav(path, samples):
    path.parent.mkdir(parents=True, exist_ok=True)
    with wave.open(str(path), 'wb') as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(RATE)
        w.writeframes(to_pcm16(samples))


# ---------- measurement (no ears available: numbers only) ----------

def fft(x):
    n = len(x)
    if n == 1:
        return x
    even, odd = fft(x[0::2]), fft(x[1::2])
    t = [cmath.exp(-2j * math.pi * k / n) * odd[k] for k in range(n // 2)]
    return [even[k] + t[k] for k in range(n // 2)] + [even[k] - t[k] for k in range(n // 2)]


def measure(samples):
    n = len(samples)
    rms = math.sqrt(sum(v * v for v in samples) / n)
    peak = max(abs(v) for v in samples)
    size = 4096
    power = [0.0] * (size // 2)
    count = 0
    for a in range(0, n - size, size):
        win = [samples[a + i] * (0.5 - 0.5 * math.cos(2 * math.pi * i / size)) for i in range(size)]
        spec = fft(win)
        for k in range(size // 2):
            power[k] += abs(spec[k]) ** 2
        count += 1
    freqs = [k * RATE / size for k in range(size // 2)]
    total = sum(power) or 1.0
    centroid = sum(f * p for f, p in zip(freqs, power)) / total

    def share(lo, hi):
        return sum(p for f, p in zip(freqs, power) if lo <= f < hi) / total

    # the quietest and loudest half-second by RMS, as a crude gap/burst contrast
    step = RATE // 2
    blocks = [math.sqrt(sum(v * v for v in samples[a:a + step]) / step) for a in range(0, n - step + 1, step)]
    return {
        'seconds': n / RATE, 'rms': rms, 'peak': peak, 'centroid_hz': centroid,
        'share_below_300': share(0, 300), 'share_300_3400': share(300, 3400), 'share_above_3400': share(3400, RATE / 2),
        'quiet_block_rms': min(blocks), 'loud_block_rms': max(blocks),
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--out', type=Path, default=DEFAULT_OUT)
    parser.add_argument('--seed', type=int, default=DEFAULT_SEED)
    parser.add_argument('--check', action='store_true', help='build twice and verify identical bytes and sane numbers; writes nothing')
    args = parser.parse_args()
    samples = build(args.seed)
    pcm = to_pcm16(samples)
    digest = hashlib.sha256(pcm).hexdigest()
    stats = measure(samples)
    if args.check:
        again = hashlib.sha256(to_pcm16(build(args.seed))).hexdigest()
        ok = (again == digest and len(samples) == FRAMES and abs(stats['seconds'] - 7.0) < 1e-9 and 0.3 < stats['peak'] < 0.99
              and 0.02 < stats['rms'] < 0.4 and 600 < stats['centroid_hz'] < 3500 and stats['loud_block_rms'] > 1.5 * stats['quiet_block_rms'])
        print(('PASS' if ok else 'FAIL'), 'alien transmission: deterministic, 7.000 s, 48 kHz mono, levels and spectrum in range')
    else:
        write_wav(args.out, samples)
        print('wrote', args.out)
    print('seed %d, pcm sha256 %s' % (args.seed, digest))
    print('seconds %.3f rms %.4f peak %.4f centroid %.0f Hz' % (stats['seconds'], stats['rms'], stats['peak'], stats['centroid_hz']))
    print('energy below 300 Hz %.3f, 300-3400 Hz %.3f, above 3400 Hz %.3f' % (stats['share_below_300'], stats['share_300_3400'], stats['share_above_3400']))
    print('quietest half-second rms %.4f, loudest %.4f' % (stats['quiet_block_rms'], stats['loud_block_rms']))
    if args.check and not ok:
        raise SystemExit(1)


if __name__ == '__main__':
    main()
