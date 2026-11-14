#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""RX-ACQUIRE-SKY-20261113: lydene til mottakerens himmelkart (ACQUIRE), 100 % syntetisert her.

Kjør:  python3 make_sounds.py [utmappe]      (krever bare numpy)
Uten argument skrives filene til Content/RxAcquire/Audio, der spillet leser dem fra disk ved første bruk
(FS47FreeformSession::LoadSkySounds; mappa stages med bygget, DefaultGame.ini). Ingen uasset og ingen import i editoren.
Mangler en fil, spiller spillet terminalens egne pip i stedet (FS47FreeformSession::SkySound).

Lydbildet: en terminal fra 1986. Firkant- og pulsbølger fra en liten piezo-høyttaler bak frontplata, et relé som klikker,
ingen romklang. Alt er lavt: disse høres på hver eneste jobb.

- Fast frø per lydnavn: samme filer hver gang, og én lyd kan endres uten at de andre flytter seg.
- Alle filer: mono, 44,1 kHz, 16-bit PCM, 3 ms stillhet før anslaget, myk ut-toning.
- Skriptet leser filene tilbake og kontrollerer format, lengde, topp, start, slutt og DC (avslutter med 1 ved feil).
- Originalt arbeid: ingen opptak og ingen etterligning av kjente maskiner eller spill.

Lydene (navn = filnavn uten .wav) og hvor spillet bruker dem:
  sweep_tick         søketikk mens siktet flyttes; spilles raskere og lysere jo nærmere krysset (Rate 0,8–1,35)
  converge           konvergenstonen under tikket nær krysset; stiger med nærheten (Rate 0,75–1,6)
  line_lock          en arms peilebånd låser; ett trinn opp per arm (Rate 1 / 1,122 / 1,244)
  lock_ring          en ring lukker seg i låsesekvensen; stiger ring for ring
  lock_confirm       fiksen er holdt: en avsluttet, lys treklang
  lock_confirm_open  det samme for en uvanlig kilde: slutter åpent, med en langsom svevning (sier ikke hvorfor)
  blurt              feil: låsen brutt, eller klikk på kartet uten peilinger
  key_blip           tastepip for styring og klikk på kartet (også brukt lavere/høyere for «arm mistet» og «lås starter»)
  hint               to myke pip når hintet kommer etter noen sekunder
"""
import math
import os
import sys
import wave
import zlib

import numpy as np

SR = 44100
PRE = 0.003
SEED = 1986

DEFAULT_OUT = os.path.normpath(os.path.join(
    os.path.dirname(os.path.abspath(__file__)), "..", "..", "Content", "RxAcquire", "Audio"))


# ---------- byggeklosser ----------

def tsec(n):
    return np.arange(n) / SR


def rng(name):
    return np.random.default_rng(SEED ^ zlib.crc32(name.encode("utf-8")))


def biquad(x, kind, f, q=0.7071):
    """RBJ-biquad i tidsplanet (lydene er korte)."""
    f = min(max(float(f), 10.0), SR * 0.45)
    w0 = 2 * math.pi * f / SR
    cw, sw = math.cos(w0), math.sin(w0)
    al = sw / (2 * q)
    if kind == "lp":
        b0, b1, b2 = (1 - cw) / 2, 1 - cw, (1 - cw) / 2
    elif kind == "hp":
        b0, b1, b2 = (1 + cw) / 2, -(1 + cw), (1 + cw) / 2
    elif kind == "bp":
        b0, b1, b2 = al, 0.0, -al
    else:
        raise ValueError(kind)
    a0, a1, a2 = 1 + al, -2 * cw, 1 - al
    b0, b1, b2, a1, a2 = b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0
    y = np.zeros(len(x))
    x1 = x2 = y1 = y2 = 0.0
    for i, v in enumerate(x.tolist()):
        o = b0 * v + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2
        x2, x1, y2, y1 = x1, v, y1, o
        y[i] = o
    return y


def speaker(x):
    """Den lille høyttaleren bak frontplata: ingen bass, en topp rundt 2,4 kHz, myk diskant."""
    x = biquad(x, "hp", 320, 0.8)
    x = x + 0.55 * biquad(x, "bp", 2400, 1.4)
    return biquad(x, "lp", 5200, 0.7)


def pulse(freq, n, duty=0.5, top=9000.0):
    """Båndbegrenset pulsbølge (sum av deltoner), frekvensen kan være en kurve."""
    f = np.broadcast_to(np.asarray(freq, dtype=float), (n,))
    ph = 2 * np.pi * np.cumsum(f) / SR
    out = np.zeros(n)
    fmax = float(np.max(f))
    for h in range(1, max(1, int(top / fmax)) + 1):
        out += (2.0 / (h * np.pi)) * np.sin(h * np.pi * duty) * np.cos(h * ph - h * np.pi * duty)
    return out * 2.0


def sine(freq, n, phase=0.0):
    f = np.broadcast_to(np.asarray(freq, dtype=float), (n,))
    return np.sin(2 * np.pi * np.cumsum(f) / SR + phase)


def env(n, attack, decay_tau=None, release=0.004):
    """Rask innsvingning, valgfritt eksponentielt fall, og en kort uttoning som alltid ender i null."""
    t = tsec(n)
    e = np.minimum(1.0, t / max(attack, 1e-5))
    if decay_tau:
        e = e * np.exp(-t / decay_tau)
    return e * np.minimum(1.0, (n / SR - t) / max(release, 1e-5))


def relay(r, n, f=3200.0, tau=0.0012):
    """Et lite reléklikk: båndpasset støy som dør på et par millisekunder."""
    x = biquad(r.standard_normal(n), "bp", f, 1.1) * np.exp(-tsec(n) / tau)
    m = float(np.max(np.abs(x)))
    return x / m if m > 0 else x


def place(buf, x, at, gain=1.0):
    i = int(round(at * SR))
    j = min(len(buf), i + len(x))
    if j > i >= 0:
        buf[i:j] += gain * x[: j - i]


def finish(x, peak):
    """Høyttaleren, topp på ønsket nivå, 3 ms stillhet foran og en ren slutt."""
    x = speaker(x)
    tail = int(0.006 * SR)
    x[-tail:] *= np.linspace(1.0, 0.0, tail)
    m = float(np.max(np.abs(x)))
    if m > 0:
        x = x * (peak / m)
    return np.concatenate([np.zeros(int(PRE * SR)), x])


# ---------- lydene ----------

def sweep_tick(r):
    n = int(0.024 * SR)
    x = np.zeros(n)
    place(x, relay(r, int(0.006 * SR), 3000, 0.0009), 0.0, 0.8)
    m = int(0.012 * SR)
    place(x, pulse(1900.0, m, 0.5) * env(m, 0.0004, 0.0035), 0.0005, 0.7)
    return finish(x, 0.34)


def converge(r):
    n = int(0.095 * SR)
    f = 660.0 * (1.0 + 0.004 * np.sin(2 * np.pi * 31.0 * tsec(n)))
    x = (sine(f, n) + 0.22 * sine(2 * f, n, 0.6) + 0.10 * pulse(f, n, 0.5, 5000)) * env(n, 0.004, 0.040, 0.010)
    return finish(x, 0.30)


def line_lock(r):
    n = int(0.150 * SR)
    x = np.zeros(n)
    place(x, relay(r, int(0.008 * SR), 2600, 0.0014), 0.0, 0.9)
    m = int(0.038 * SR)
    place(x, pulse(988.0, m, 0.25) * env(m, 0.001, 0.030), 0.006, 0.8)
    m2 = int(0.085 * SR)
    place(x, pulse(1319.0, m2, 0.25) * env(m2, 0.001, 0.034), 0.052, 0.8)
    return finish(x, 0.42)


def lock_ring(r):
    n = int(0.130 * SR)
    t = tsec(n)
    f = 880.0 * (1.0 + 0.059 * np.minimum(1.0, t / 0.030))  # en halvtone opp mens ringen lukker seg
    x = (sine(f, n) + 0.30 * pulse(f, n, 0.5, 6000) + 0.15 * sine(2.76 * f, n) * np.exp(-t / 0.012)) * env(n, 0.002, 0.045, 0.010)
    return finish(x, 0.38)


def _notes(n_total, notes, duty=0.25):
    x = np.zeros(n_total)
    for at, freq, length, tau, gain in notes:
        m = int(length * SR)
        place(x, pulse(freq, m, duty) * env(m, 0.0015, tau, 0.010), at, gain)
    return x


def lock_confirm(r):
    # C6, E6, G6 og en holdt C7: kommer hjem
    n = int(0.560 * SR)
    x = _notes(n, [(0.000, 1046.5, 0.075, 0.060, 0.8), (0.075, 1318.5, 0.075, 0.060, 0.8), (0.150, 1568.0, 0.075, 0.060, 0.8),
                   (0.225, 2093.0, 0.320, 0.110, 0.9)])
    m = int(0.320 * SR)
    place(x, sine(1046.5, m) * env(m, 0.004, 0.120, 0.020), 0.225, 0.35)
    place(x, relay(r, int(0.008 * SR), 2600, 0.0014), 0.0, 0.5)
    return finish(x, 0.50)


def lock_confirm_open(r):
    # C6, E6 og så en holdt F#6 med en nabo ni hertz over: blir stående åpent og svever langsomt
    n = int(0.640 * SR)
    x = _notes(n, [(0.000, 1046.5, 0.075, 0.060, 0.8), (0.075, 1318.5, 0.075, 0.060, 0.8)])
    m = int(0.480 * SR)
    hold = (pulse(1480.0, m, 0.25) + 0.9 * pulse(1489.0, m, 0.25)) * env(m, 0.002, 0.170, 0.030)
    place(x, hold, 0.150, 0.55)
    place(x, sine(1046.5 / 2, m) * env(m, 0.004, 0.150, 0.030), 0.150, 0.30)
    place(x, relay(r, int(0.008 * SR), 2600, 0.0014), 0.0, 0.5)
    return finish(x, 0.48)


def blurt(r):
    n = int(0.230 * SR)
    t = tsec(n)
    raw = pulse(155.0, n, 0.5, 2400) + 0.9 * pulse(164.0, n, 0.5, 2400)
    gate = 0.65 + 0.35 * np.sign(np.sin(2 * np.pi * 27.0 * t))  # den hakkete summeren
    x = biquad(raw * gate, "lp", 1500, 0.8) * env(n, 0.003, None, 0.030)
    # (høyttaleren tar bort grunntonen; det som står igjen er den ru overtonerekka)
    return finish(x, 0.40)


def key_blip(r):
    n = int(0.032 * SR)
    x = np.zeros(n)
    place(x, relay(r, int(0.005 * SR), 3600, 0.0008), 0.0, 0.9)
    m = int(0.016 * SR)
    place(x, pulse(1480.0, m, 0.5) * env(m, 0.0005, 0.006), 0.001, 0.6)
    return finish(x, 0.30)


def hint(r):
    n = int(0.310 * SR)
    x = np.zeros(n)
    m = int(0.070 * SR)
    pip = (sine(990.0, m) + 0.18 * pulse(990.0, m, 0.5, 5000)) * env(m, 0.004, 0.030, 0.012)
    place(x, pip, 0.0, 1.0)
    place(x, pip, 0.150, 0.85)
    return finish(x, 0.34)


SOUNDS = [("sweep_tick", sweep_tick), ("converge", converge), ("line_lock", line_lock), ("lock_ring", lock_ring),
          ("lock_confirm", lock_confirm), ("lock_confirm_open", lock_confirm_open), ("blurt", blurt), ("key_blip", key_blip), ("hint", hint)]


def write_wav(path, x):
    pcm = np.clip(np.round(x * 32767.0), -32767, 32767).astype("<i2")
    with wave.open(path, "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(SR)
        w.writeframes(pcm.tobytes())


def check(path, name):
    with wave.open(path, "rb") as w:
        ok_format = w.getnchannels() == 1 and w.getsampwidth() == 2 and w.getframerate() == SR
        data = np.frombuffer(w.readframes(w.getnframes()), dtype="<i2").astype(float) / 32767.0
    seconds = len(data) / SR
    peak = float(np.max(np.abs(data)))
    lead = float(np.max(np.abs(data[: int(PRE * SR) - 1])))
    end = float(np.max(np.abs(data[-8:])))
    dc = abs(float(np.mean(data)))
    rms = float(np.sqrt(np.mean(data ** 2)))
    ok = ok_format and 0.02 <= seconds <= 0.8 and 0.2 <= peak <= 0.6 and lead == 0.0 and end < 0.01 and dc < 0.01
    print("%-4s %-18s %5.0f ms  topp %.2f  rms %.3f  dc %.4f" % ("ok" if ok else "FEIL", name, seconds * 1000, peak, rms, dc))
    return ok


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_OUT
    os.makedirs(out, exist_ok=True)
    good = True
    for name, make in SOUNDS:
        path = os.path.join(out, name + ".wav")
        write_wav(path, make(rng(name)))
        good &= check(path, name)
    print("%d lyder i %s" % (len(SOUNDS), out))
    return 0 if good else 1


if __name__ == "__main__":
    sys.exit(main())
