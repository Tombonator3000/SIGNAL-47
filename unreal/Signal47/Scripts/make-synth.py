#!/usr/bin/env python3
"""The sounds the web version makes with oscillators (core/Audio.ts), rendered as WAV files for
Unreal: console beeps, the receiver carrier and static, the room tone, the dish motors, the
boom, the alarm. Pure Python (wave and math), no numpy. Usage: make-synth.py OUTDIR"""
import math
import os
import random
import struct
import sys
import wave

RATE = 44100
out = sys.argv[1]
os.makedirs(out, exist_ok=True)
random.seed(47)


def write(name, samples):
    with wave.open(os.path.join(out, name + '.wav'), 'wb') as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(RATE)
        w.writeframes(b''.join(struct.pack('<h', int(max(-1.0, min(1.0, s)) * 32767)) for s in samples))


def env(i, n, a=0.004, r=0.03):
    t = i / RATE
    d = (n - i) / RATE
    return min(1.0, t / a) * min(1.0, d / r)


def beep(name, freq, dur, gain=0.3, kind='square'):
    n = int(dur * RATE)
    s = []
    for i in range(n):
        ph = 2 * math.pi * freq * i / RATE
        v = (1.0 if math.sin(ph) >= 0 else -1.0) if kind == 'square' else math.sin(ph)
        s.append(v * gain * env(i, n))
    write(name, s)


# Beeps the prologue uses (Audio.beep(freq, dur, gain)).
beep('beep_1200', 1200, 0.08, 0.3)
beep('beep_900', 900, 0.03, 0.3)
beep('beep_1320', 1320, 0.07, 0.3)
beep('beep_1760', 1760, 0.09, 0.3)
beep('beep_1760_long', 1760, 0.18, 0.3)
beep('beep_2350', 2350, 0.4, 0.3)
beep('beep_1500', 1500, 0.06, 0.3)
beep('beep_1500_long', 1500, 0.12, 0.3)
beep('beep_1980', 1980, 0.09, 0.3)
beep('beep_1400_short', 1400, 0.02, 0.2)
for k, f in enumerate([600, 780, 960, 1140]):
    beep('solve_%d' % k, f, 0.025, 0.3)
beep('drone_52', 52, 2.8, 0.35, 'sine')

# The receiver's carrier: a low hum with a touch of static, looped (Audio.startCarrier).
n = RATE * 4
s = []
for i in range(n):
    t = i / RATE
    hum = 0.5 * math.sin(2 * math.pi * 120 * t) + 0.25 * math.sin(2 * math.pi * 240 * t) + 0.12 * math.sin(2 * math.pi * 60 * t)
    s.append(hum * 0.18)
write('carrier_hum', s)

# Static noise, looped: white noise through a crude one-pole lowpass.
s = []
y = 0.0
for i in range(RATE * 4):
    y += (random.uniform(-1, 1) - y) * 0.35
    s.append(y * 0.6)
write('static_noise', s)

# Room tone: a quiet 60 Hz with ventilation noise, looped.
s = []
y = 0.0
for i in range(RATE * 6):
    t = i / RATE
    y += (random.uniform(-1, 1) - y) * 0.05
    s.append(0.06 * math.sin(2 * math.pi * 60 * t) + y * 0.35)
write('room_tone', s)

# The dish motors: a slow grind, looped (Audio.startMotors).
s = []
y = 0.0
for i in range(RATE * 5):
    t = i / RATE
    y += (random.uniform(-1, 1) - y) * 0.12
    wob = 1 + 0.08 * math.sin(2 * math.pi * 0.9 * t)
    s.append(0.35 * math.sin(2 * math.pi * 38 * wob * t) + 0.2 * math.sin(2 * math.pi * 77 * t) + y * 0.25)
write('motors', s)

# The boom at the event (Audio.boom): a low thump with a long tail.
n = int(2.5 * RATE)
s = []
for i in range(n):
    t = i / RATE
    f = 90 * math.exp(-t * 2.5) + 28
    s.append(math.sin(2 * math.pi * f * t) * math.exp(-t * 1.8) * 0.9 + random.uniform(-1, 1) * math.exp(-t * 9) * 0.3)
write('boom', s)

# The pulse pattern of the anomaly, 4 then 7 pulses every 5.2 s, as a tone (Audio.signalFrame).
n = int(5.2 * RATE)
s = []
for i in range(n):
    t = i / RATE
    c = t % 5.2
    count = 4 if c < 1.6 else 7
    local = c if c < 1.6 else c - 2.1
    on = local >= 0 and local < count * 0.22 and (local % 0.22) < 0.11
    s.append(math.sin(2 * math.pi * 1420 * t) * (0.5 if on else 0.03))
write('signal_pulses', s)
print('synth: %d files' % len(os.listdir(out)))
