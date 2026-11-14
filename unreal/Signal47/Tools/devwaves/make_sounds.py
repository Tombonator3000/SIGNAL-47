#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""DEV-WAVES: lydsettet til utviklerskytingen (F10), 100 % syntetisert her.

Kjør:  python3 make_sounds.py (krever numpy) [utmappe]
Uten argument skrives filene til Content/DevWaves/Audio. Bare numpy trengs.

- Fast frø per lydnavn: samme filer hver gang, og én lyd kan endres uten at de andre flytter seg.
- Alle filer: mono, 44,1 kHz, 16-bit PCM, 3 ms forløp med innfading før anslaget. Lagene i
  skuddet (shot_transient + shot_body + shot_tail_*) ligger derfor likt når de startes samtidig.
- Skriptet leser filene tilbake og kontrollerer lengde, topp, start, DC og format (avslutter med 1 ved feil).
- Originalt arbeid: ingen opptak, ingen etterligning av kjente spill eller virkelige våpen.
"""
import math
import os
import sys
import wave
import zlib

import numpy as np

SR = 44100
PRE = 0.003      # forløp før anslaget (s)
SEED = 1986
WARM = 4410      # innsvingning for farget støy (kastes)

DEFAULT_OUT = os.path.normpath(os.path.join(
    os.path.dirname(os.path.abspath(__file__)), "..", "..", "Content", "DevWaves", "Audio"))


# ---------- byggeklosser ----------

def tsec(n):
    return np.arange(n) / SR


def unit(x):
    m = float(np.max(np.abs(x))) if len(x) else 0.0
    return x / m if m > 0 else x


def sat(x, drive=2.0):
    """Myk klipping (metning)."""
    return np.tanh(x * drive) / math.tanh(drive)


def _rbj(kind, f, q=0.7071):
    f = min(max(float(f), 5.0), SR * 0.49)
    w0 = 2 * math.pi * f / SR
    cw, sw = math.cos(w0), math.sin(w0)
    al = sw / (2 * q)
    if kind == "lp":
        b = ((1 - cw) / 2, 1 - cw, (1 - cw) / 2)
    elif kind == "hp":
        b = ((1 + cw) / 2, -(1 + cw), (1 + cw) / 2)
    elif kind == "bp":
        b = (al, 0.0, -al)
    else:
        raise ValueError(kind)
    return b, (1 + al, -2 * cw, 1 - al)


def _biquad(x, b, a, pad=0.25):
    """Kausal biquad regnet i frekvensplanet (nullfylt, så ingen forringing)."""
    n = len(x)
    nfft = 1 << int(math.ceil(math.log2(n + int(pad * SR))))
    spec = np.fft.rfft(x, nfft)
    z = np.exp(-2j * np.pi * np.arange(len(spec)) / nfft)
    h = (b[0] + b[1] * z + b[2] * z * z) / (a[0] + a[1] * z + a[2] * z * z)
    return np.fft.irfft(spec * h, nfft)[:n]


def filt(x, *stages):
    for st in stages:
        q = st[2] if len(st) > 2 else 0.7071
        x = _biquad(x, *_rbj(st[0], st[1], q))
    return x


def cn(r, n, *stages):
    """Farget støy uten innsvingning i starten."""
    return filt(r.standard_normal(n + WARM), *stages)[WARM:]


def svf(x, fc, q=0.7071, mode="lp"):
    """Tilstandsvariabelt filter med bevegelig grensefrekvens (resonant, «analogt»)."""
    n = len(x)
    g = np.tan(np.pi * np.clip(np.broadcast_to(np.asarray(fc, dtype=float), (n,)), 10.0, SR * 0.45) / SR)
    k = 1.0 / np.broadcast_to(np.asarray(q, dtype=float), (n,))
    a1 = 1.0 / (1.0 + g * (g + k))
    a2 = g * a1
    a3 = g * a2
    xl, a1l, a2l, a3l, kl = np.asarray(x, dtype=float).tolist(), a1.tolist(), a2.tolist(), a3.tolist(), k.tolist()
    out = [0.0] * n
    ic1 = ic2 = 0.0
    m = {"lp": 0, "bp": 1, "hp": 2}[mode]
    for i in range(n):
        v0 = xl[i]
        v3 = v0 - ic2
        v1 = a1l[i] * ic1 + a2l[i] * v3
        v2 = ic2 + a2l[i] * ic1 + a3l[i] * v3
        ic1 = 2.0 * v1 - ic1
        ic2 = 2.0 * v2 - ic2
        if m == 0:
            out[i] = v2
        elif m == 1:
            out[i] = v1
        else:
            out[i] = v0 - kl[i] * v1 - v2
    return np.asarray(out)


def ph_of(f):
    """Fase fra en frekvenskurve (Hz per sample)."""
    return 2 * np.pi * np.cumsum(np.asarray(f, dtype=float)) / SR


def sweep(n, f0, f1, tau, phase=0.0):
    """Sinus som faller (eller stiger) eksponentielt fra f0 mot f1."""
    return np.sin(ph_of(f1 + (f0 - f1) * np.exp(-tsec(n) / tau)) + phase)


def dec(n, tau, att=0.0):
    t = tsec(n)
    e = np.exp(-t / tau)
    if att > 0:
        e = e * np.minimum(1.0, t / att)
    return e


def env_trap(n, a, rel):
    t = tsec(n)
    return np.minimum(1.0, t / a) * np.minimum(1.0, (n / SR - t) / rel)


def add(buf, x, t0=0.0, g=1.0):
    i = int(round(t0 * SR))
    j = min(len(buf), i + len(x))
    if j > i >= 0:
        buf[i:j] += g * x[: j - i]


def click(r, n, f, tau, q=0.8):
    """Kort støyklikk rundt f."""
    return unit(cn(r, n, ("bp", f, q)) * dec(n, tau))


def metal(r, n, freqs, tau0, tilt=0.6, bend=0.0, amps=None):
    """Uharmonisk stabel av deltoner; de høye dør raskest. bend = tonefall i anslaget."""
    t = tsec(n)
    out = np.zeros(n)
    for i, f in enumerate(freqs):
        a = (amps[i] if amps is not None else 1.0 / (1.0 + 0.3 * i)) * r.uniform(0.7, 1.0)
        tau = tau0 * (freqs[0] / f) ** tilt * r.uniform(0.85, 1.15)
        phase = ph_of(f * (1.0 + bend * np.exp(-t / 0.02))) if bend else 2 * np.pi * f * t
        out += a * np.sin(phase + r.uniform(0, 2 * np.pi)) * np.exp(-t / tau)
    return unit(out * np.minimum(1.0, t / 0.0002))


def saw(ph, fmax, top=16000.0):
    out = np.zeros(len(ph))
    for h in range(1, max(1, int(top / fmax)) + 1):
        out += np.sin(h * ph) / h
    return out * (2 / np.pi)


def square(ph, fmax, top=6000.0):
    out = np.zeros(len(ph))
    for h in range(1, max(1, int(top / fmax)) + 1, 2):
        out += np.sin(h * ph) / h
    return out * (4 / np.pi)


def tri(ph, fmax, top=8000.0):
    out = np.zeros(len(ph))
    s = 1.0
    for h in range(1, max(1, int(top / fmax)) + 1, 2):
        out += s * np.sin(h * ph) / (h * h)
        s = -s
    return out * (8 / np.pi ** 2)


def grains(r, n, count, t0, spread, amp_tau, fl, fh, gl, gh, noisy=False):
    """Småkorn (grus, flis, statisk): tetthet og styrke avtar eksponentielt."""
    x = np.zeros(n)
    for _ in range(count):
        tt = t0 + r.exponential(spread)
        gt = r.uniform(gl, gh)
        m = max(8, int(gt * 5 * SR))
        tm = tsec(m)
        f = math.exp(r.uniform(math.log(fl), math.log(fh)))
        if noisy:
            g = r.standard_normal(m) * np.exp(-tm / gt)
        else:
            g = np.sin(2 * np.pi * f * tm + r.uniform(0, 6.283)) * np.exp(-tm / gt)
        add(x, g, tt, r.uniform(0.3, 1.0) * math.exp(-(tt - t0) / amp_tau))
    return x


def scrape(r, n, f0, f1, q=2.0, grain_hz=140.0):
    """Friksjon: støy gjennom et bevegelig båndpass, hakkete som metall mot metall."""
    t = tsec(n)
    fc = f0 * (f1 / f0) ** (t / max(t[-1], 1e-6))
    s = svf(r.standard_normal(n), fc, q, "bp")
    g = unit(np.abs(cn(r, n, ("lp", grain_hz))))
    return unit(s * (0.35 + 0.65 * g ** 1.5))


def voice(f, ratio, index, formants, shift=1.0):
    """FM-kilde gjennom smale, høye formanter: en stemme uten menneskelig strupe."""
    ph = ph_of(f)
    src = np.sin(ph + index * np.sin(ratio * ph)) + 0.4 * np.sin(2.0 * ph + 0.6 * index * np.sin(ratio * 1.007 * ph))
    out = np.zeros(len(ph))
    for ff, q, g in formants:
        out += g * svf(src, ff * shift, q, "bp") / q
    return out


def note(n, f, tau, bright=2600.0):
    """Myk synthtone: to svevende trekanter + litt sag gjennom et lavpass som lukker seg."""
    t = tsec(n)
    ph1, ph2 = ph_of(np.full(n, f * 1.003)), ph_of(np.full(n, f * 0.997))
    osc = tri(ph1, f) + tri(ph2, f) + 0.25 * saw(ph1, f, 9000.0)
    y = svf(osc, 900.0 + (bright - 900.0) * np.exp(-t / 0.15), 1.2, "lp")
    return y * dec(n, tau, 0.006)


# ---------- skuddet (lag som spilles samtidig) ----------

def g_shot_transient(r, n):
    t = tsec(n)
    crack = cn(r, n, ("hp", 1500), ("lp", 9000)) * np.exp(-t / 0.0035)
    snap = cn(r, n, ("bp", 1400, 0.9)) * np.exp(-t / 0.008)
    nw = int(0.0007 * SR)
    nwave = np.zeros(n)
    nwave[:nw] = np.linspace(1.0, -1.0, nw)            # N-bølge: én kort trykkpuls
    knock = sweep(n, 700, 320, 0.004, math.pi / 2) * np.exp(-t / 0.007)
    air = cn(r, n, ("hp", 6000)) * np.exp(-t / 0.014)
    x = unit(crack) + 0.7 * unit(snap) + 0.9 * nwave + 0.28 * knock + 0.15 * unit(air)
    return sat(x, 1.8)


def g_shot_body(r, n):
    t = tsec(n)
    thump = sweep(n, 180, 55, 0.032, math.pi / 2) * dec(n, 0.030, 0.0008)   # 180 -> 55 Hz; dør før den når bunnen
    knock = sweep(n, 270, 115, 0.020, math.pi / 2) * np.exp(-t / 0.032)
    burst = cn(r, n, ("lp", 4500), ("hp", 160)) * (np.exp(-t / 0.022) + 0.28 * np.exp(-t / 0.075))
    punch = cn(r, n, ("bp", 240, 0.8)) * np.exp(-t / 0.050)
    crack = cn(r, n, ("hp", 2600)) * np.exp(-t / 0.005)
    x = unit(thump) + 0.8 * knock + 0.9 * unit(burst) + 0.7 * unit(punch) + 0.7 * unit(crack)
    return filt(sat(x, 2.2), ("lp", 9000))


def g_shot_tail_yard(r, n):
    t = tsec(n)
    hi = cn(r, n, ("hp", 1600)) * np.exp(-t / 0.085)           # det lyse dør først: halen blir mørkere
    mid = cn(r, n, ("bp", 650, 0.6)) * np.exp(-t / 0.20)
    roll = unit(cn(r, n, ("lp", 9.0)))                          # rulling i bunnen
    low = cn(r, n, ("hp", 55), ("lp", 280), ("lp", 280)) * np.exp(-t / 0.38) * (1.0 + 0.45 * roll)
    roll2 = unit(cn(r, n, ("lp", 5.0)))
    sub = cn(r, n, ("lp", 95), ("lp", 95)) * np.exp(-t / 0.50) * (1.0 + 0.5 * roll2)
    x = 0.35 * unit(hi) + 0.9 * unit(mid) + unit(low) + 0.4 * unit(sub)
    nh = int(0.30 * SR)
    head = x[:nh] * dec(nh, 0.09)
    add(x, filt(head, ("lp", 1100)), 0.29, 0.30)               # to svake, mørke svar fra terrenget langt unna
    add(x, filt(head, ("lp", 700)), 0.61, 0.18)
    return sat(x * np.minimum(1.0, t / 0.002), 1.2)


def g_shot_tail_walls(r, n):
    t = tsec(n)
    m = int(0.09 * SR)
    tm = tsec(m)

    def slap(cut):
        b = cn(r, m, ("lp", cut), ("hp", 220)) * np.exp(-tm / 0.011)
        th = sweep(m, 210, 90, 0.012, math.pi / 2) * dec(m, 0.020, 0.001)
        return unit(unit(b) + 0.5 * th)

    x = np.zeros(n)
    add(x, unit(cn(r, n, ("bp", 900, 0.5)) * np.exp(-t / 0.030)), 0.0, 0.22)   # svakt direkte lag fra første øyeblikk
    for dt, g, cut in ((0.045, 1.0, 5200), (0.095, 0.72, 3600), (0.160, 0.50, 2500)):
        add(x, slap(cut), dt, g)                                # tre tydelige slag fra veggene
    for k in range(1, 7):                                       # flagreekko mellom parallelle vegger
        add(x, slap(1900 * 0.86 ** k), 0.160 + 0.052 * k, 0.30 * 0.62 ** k)
    te = np.maximum(t - 0.04, 0.0)
    diff = cn(r, n, ("bp", 700, 0.5)) * (1.0 - np.exp(-te / 0.05)) * np.exp(-te / 0.14)
    add(x, unit(diff), 0.0, 0.33)                               # kort diffus utklang
    return sat(x, 1.3)


def g_shot_mech(r, n):
    m = int(0.05 * SR)
    c1 = 0.8 * click(r, m, 3400, 0.0018) + metal(r, m, (1180, 2130, 2960, 4310, 5770, 7240), 0.012)
    c2 = (0.9 * click(r, m, 2600, 0.0022) + metal(r, m, (960, 1740, 2610, 3890, 5210, 6630), 0.018)
          + 0.5 * sweep(m, 300, 170, 0.006, math.pi / 2) * dec(m, 0.010, 0.0005))
    x = np.zeros(n)
    add(x, unit(c1), 0.0, 0.7)                                  # sluttstykket bak ...
    add(x, unit(c2), 0.036, 1.0)                                # ... og fram igjen
    return x


# ---------- håndtering ----------

def g_dry_click(r, n):
    t = tsec(n)
    x = (0.9 * click(r, n, 3200, 0.0016) + 0.8 * metal(r, n, (2150, 3480, 4950, 7100), 0.010)
         + 0.35 * sweep(n, 620, 450, 0.004, math.pi / 2) * dec(n, 0.008, 0.0004))
    spring = np.sin(2 * np.pi * 910 * t + 2.5 * np.sin(2 * np.pi * 61 * t)) * dec(n, 0.022, 0.002)
    return x + 0.12 * spring


def g_mag_out(r, n):
    x = np.zeros(n)
    m = int(0.05 * SR)
    add(x, unit(click(r, m, 3100, 0.0015) + 0.7 * metal(r, m, (1420, 2380, 3650, 5100), 0.007)), 0.0, 0.9)  # utløseren
    ns = int(0.21 * SR)
    add(x, scrape(r, ns, 1900, 950) * env_trap(ns, 0.02, 0.04), 0.035, 0.55)                 # magasinet glir ut
    add(x, scrape(r, ns, 4200, 3100, 1.2, 220.0) * env_trap(ns, 0.03, 0.05), 0.035, 0.15)
    k = int(0.07 * SR)
    tok = metal(r, k, (310, 540, 890, 1460), 0.018, tilt=0.4) + 0.5 * click(r, k, 1800, 0.002)
    add(x, unit(tok), 0.252, 0.8)                                                            # hult «tok» når det slipper
    return x


def g_mag_in(r, n):
    x = np.zeros(n)
    m = int(0.05 * SR)
    add(x, unit(click(r, m, 2400, 0.0015) + 0.6 * metal(r, m, (640, 1130, 1990), 0.008, tilt=0.4)), 0.0, 0.45)  # leppa treffer brønnen
    ns = int(0.165 * SR)
    add(x, scrape(r, ns, 950, 1900) * env_trap(ns, 0.03, 0.015), 0.012, 0.5)
    add(x, scrape(r, ns, 3000, 4300, 1.2, 220.0) * env_trap(ns, 0.04, 0.015), 0.012, 0.14)
    k = int(0.10 * SR)
    seat = (0.6 * sweep(k, 300, 190, 0.008, math.pi / 2) * dec(k, 0.018, 0.0005)
            + 0.8 * metal(r, k, (880, 1510, 2470, 3820, 5300), 0.016)
            + 0.8 * click(r, k, 2800, 0.002))
    add(x, unit(seat), 0.185, 1.0)                                                           # setter seg med et fast slag
    add(x, unit(click(r, m, 3600, 0.0012) + 0.6 * metal(r, m, (2100, 3400, 4900), 0.006)), 0.228, 0.4)  # sperren
    return x


def g_bolt_pull(r, n):
    x = np.zeros(n)
    ns = int(0.17 * SR)
    add(x, scrape(r, ns, 2100, 2900, 1.6, 300.0) * env_trap(ns, 0.012, 0.02), 0.0, 0.45)
    m = int(0.02 * SR)
    tt = 0.0
    for i in range(10):                                                                      # hakk etter hakk
        tick = unit(click(r, m, 3200 + 150 * i, 0.0009) + 0.8 * metal(r, m, (2400, 3700, 5200), 0.003))
        add(x, tick, tt, 0.45 + 0.03 * i)
        tt += 0.019 - 0.0006 * i + r.uniform(-0.0015, 0.0015)
    ts = tsec(ns)
    spring = np.sin(ph_of(620.0 * (900.0 / 620.0) ** (ts / ts[-1]))) * env_trap(ns, 0.03, 0.02)
    add(x, spring, 0.0, 0.06)                                                                # fjæra spennes
    k = int(0.06 * SR)
    stop = (0.9 * sweep(k, 300, 200, 0.006, math.pi / 2) * dec(k, 0.014, 0.0005)
            + metal(r, k, (1050, 1870, 2760, 4150, 5600), 0.016) + 0.7 * click(r, k, 2300, 0.002))
    add(x, unit(stop), 0.176, 1.0)                                                           # stopp i bakre stilling
    return x


def g_bolt_release(r, n):
    x = np.zeros(n)
    m = int(0.03 * SR)
    add(x, unit(click(r, m, 3800, 0.0012) + 0.6 * metal(r, m, (2300, 3900, 5600), 0.004)), 0.0, 0.4)  # sperren slipper
    ns = int(0.024 * SR)
    add(x, scrape(r, ns, 1500, 4000, 1.5, 400.0) * env_trap(ns, 0.004, 0.003), 0.002, 0.22)
    k = n - int(0.024 * SR)
    tk = tsec(k)
    slam = (metal(r, k, (1320, 2210, 3470, 4980, 6650, 8200), 0.045, tilt=0.7, bend=0.004, amps=(0.6, 0.8, 1.0, 1.0, 0.8, 0.6))
            + 0.4 * sweep(k, 320, 210, 0.006, math.pi / 2) * dec(k, 0.014, 0.0004)
            + 1.1 * click(r, k, 4200, 0.0022, 0.6))
    twang = np.sin(ph_of(790.0 * (1 - 0.03 * (1 - np.exp(-tk / 0.03)))) + 1.8 * np.sin(2 * np.pi * 47 * tk)) * dec(k, 0.045, 0.004)
    add(x, unit(slam) + 0.13 * twang, 0.024, 1.0)                                            # slår fram: lyst metall + fjærsang
    return x


def casing(r, n, partials, bounces, grit=0.0, ring=0.022, rattle=None):
    """Liten messinghylse: 4 høye deltoner (3-7 kHz) som slås an på nytt for hvert sprett."""
    x = np.zeros(n)
    m = int(0.12 * SR)
    tm = tsec(m)
    hits = list(bounces)
    if rattle:                                                   # ruller ut: tettere og svakere sprett
        tt, gap, g = rattle
        while gap > 0.004 and tt < n / SR:
            hits.append((tt, g))
            tt += gap
            gap *= 0.80
            g *= 0.78
    for bt, bg in hits:
        det = 1.0 + r.uniform(-0.004, 0.004)
        tone = np.zeros(m)
        for f in partials:                                       # hvert sprett treffer et nytt sted på hylsa
            tau = ring * r.uniform(0.7, 1.3) * (partials[0] / f) ** 0.6
            tone += r.uniform(0.4, 1.0) * np.sin(2 * np.pi * f * det * tm + r.uniform(0, 6.283)) * np.exp(-tm / tau)
        b = unit(tone * np.minimum(1.0, tm / 0.0002)) + 0.5 * click(r, m, 5200, 0.0007, 0.7)
        if grit:
            b = b + grit * unit(cn(r, m, ("bp", 3200, 0.6)) * np.exp(-tm / 0.006))
        add(x, b, bt, bg)
    return x


def g_casing_1(r, n):   # ren betong
    return casing(r, n, (3350, 4720, 5890, 6830),
                  ((0.0, 1.0), (0.092, 0.55), (0.151, 0.32), (0.189, 0.18), (0.212, 0.10)))


def g_casing_2(r, n):   # grus: kortere klang, mer knas
    return casing(r, n, (3080, 4390, 5560, 6410), ((0.0, 1.0), (0.071, 0.42), (0.118, 0.20)), grit=0.6, ring=0.014)


def g_casing_3(r, n):   # to sprett, så ruller den ut
    return casing(r, n, (3720, 5010, 6240, 6980), ((0.0, 0.9), (0.105, 0.6), (0.170, 0.36)),
                  ring=0.026, rattle=(0.205, 0.022, 0.22))


# ---------- treff ----------

def g_impact_dirt(r, n):
    t = tsec(n)
    thup = sweep(n, 230, 105, 0.014, math.pi / 2) * dec(n, 0.022, 0.0015)
    dull = cn(r, n, ("lp", 900), ("lp", 900)) * np.exp(-t / 0.018)
    spray = cn(r, n, ("bp", 2800, 0.5)) * np.minimum(1.0, t / 0.012) * np.exp(-t / 0.065)
    grit = filt(grains(r, n, 90, 0.012, 0.075, 0.11, 1800, 7000, 0.0006, 0.0025, noisy=True), ("hp", 1500), ("lp", 6000))
    peb = grains(r, n, 9, 0.04, 0.09, 0.12, 700, 1600, 0.002, 0.005)                         # småstein som lander
    x = unit(thup) + 0.9 * unit(dull) + 0.2 * unit(spray) + 0.3 * unit(grit) + 0.18 * unit(peb)
    return sat(x, 1.4)


def g_impact_metal(r, n):
    t = tsec(n)
    base = (412, 733, 1180, 1562, 2104, 2790, 3475, 4310, 5230, 6480)
    amps = (0.5, 0.7, 0.9, 0.8, 1.0, 1.0, 0.9, 0.7, 0.5, 0.35)
    plate = metal(r, n, base, 0.11, tilt=0.45, bend=0.012, amps=amps)                        # tynn plate: tonen faller litt
    beat = metal(r, n, tuple(f * 1.007 for f in base[2:8]), 0.09, tilt=0.45, bend=0.012, amps=amps[2:8])  # svevning = klang
    ping = np.sin(2 * np.pi * 3120 * t) * dec(n, 0.075, 0.0003)
    bang = cn(r, n, ("bp", 850, 0.7)) * np.exp(-t / 0.008)
    tick = cn(r, n, ("hp", 3000)) * np.exp(-t / 0.0015)
    x = plate + 0.6 * beat + 0.45 * ping + 0.7 * unit(bang) + 0.8 * unit(tick)
    return sat(x, 1.3)


def g_impact_concrete(r, n):
    t = tsec(n)
    crack = cn(r, n, ("hp", 800)) * np.exp(-t / 0.005)
    tok = metal(r, n, (950, 1900, 2750), 0.006, tilt=0.3)
    thud = sweep(n, 160, 95, 0.008, math.pi / 2) * dec(n, 0.014, 0.0006)
    chips = grains(r, n, 38, 0.02, 0.07, 0.10, 1500, 6000, 0.0008, 0.003)                    # flis som spretter
    dust = cn(r, n, ("hp", 3000)) * np.minimum(1.0, t / 0.012) * np.exp(-t / 0.06)
    x = unit(crack) + 0.7 * tok + 0.5 * thud + 0.42 * unit(chips) + 0.12 * unit(dust)
    return sat(x, 1.6)


# ---------- vesenene (ikke blodig, ikke menneskelig) ----------

def g_alien_hit(r, n):
    t = tsec(n)
    thud = sweep(n, 170, 88, 0.012, math.pi / 2) * dec(n, 0.028, 0.0015)
    soft = cn(r, n, ("lp", 520), ("lp", 520)) * np.exp(-t / 0.018)
    m = n - int(0.012 * SR)
    tm = tsec(m)
    f = 1150 + 600 * np.exp(-tm / 0.06) - 800 * np.exp(-tm / 0.012)                           # kort hyl: opp, så ned
    f = f * (1 + 0.02 * np.sin(2 * np.pi * 31 * tm))
    v = voice(f, 1.41, 2.2 * np.exp(-tm / 0.09) + 0.3, ((1450, 9, 1.0), (2900, 10, 0.8), (4700, 9, 0.45)))
    v = v * (1 + 0.35 * np.sin(2 * np.pi * 43 * tm)) * dec(m, 0.065, 0.006)
    x = np.zeros(n)
    add(x, unit(thud) + 0.6 * unit(soft), 0.0, 0.85)
    add(x, unit(v), 0.012, 1.0)
    add(x, metal(r, m, (3100, 4630, 6220), 0.035), 0.014, 0.12)                              # glassaktig etterklang
    return sat(x, 1.2)


def g_alien_death(r, n):
    t = tsec(n)
    u = t / (n / SR)
    f = (420 + 1080 * np.exp(-t / 0.33)) * (1 - 0.18 * np.exp(-t / 0.025))                   # lite rykk opp, så lang vei ned
    vib = 8 + 7 * u
    f = f * (1 + (0.010 + 0.022 * u) * np.sin(ph_of(vib)))
    idx = 2.6 * np.exp(-t / 0.40) + 0.25                                                     # færre overtoner mot slutten
    sh = 1.0 - 0.42 * u                                                                      # formantene synker med tonen
    v1 = voice(f, 2.01, idx, ((1700, 8, 1.0), (3200, 10, 0.7), (5200, 9, 0.35)), sh)
    v2 = voice(f * (1.5 - 0.24 * u), 1.41, 0.7 * idx, ((2100, 9, 1.0), (3900, 10, 0.6)), sh)   # to stemmer som glir fra hverandre
    breath = svf(r.standard_normal(n), 3200 * sh, 3.0, "bp") * (0.15 + 0.85 * u)
    x = unit(v1) + 0.45 * unit(v2) + 0.10 * unit(breath)
    x = svf(x, 200.0 * (1400.0 / 200.0) ** u, 0.9, "hp")                                     # tynnes ut: bunnen forsvinner
    env = (np.minimum(1.0, t / 0.008) * np.exp(-t / 0.42) * (1 - u ** 3)
           * (1 + 0.25 * u * np.sin(ph_of(vib * 2.0))))
    return sat(unit(x * env), 1.2)


def g_alien_dissolve(r, n):
    t = tsec(n)
    u = t / (n / SR)
    fizz = svf(r.standard_normal(n), 1200.0 * (3800.0 / 1200.0) ** u, 0.8, "hp")             # lysere og lysere
    air = cn(r, n, ("bp", 6000, 1.0)) * (0.4 + 0.6 * unit(np.abs(cn(r, n, ("lp", 18.0)))))
    sand = cn(r, n, ("bp", 3400, 0.7)) * unit(np.abs(cn(r, n, ("lp", 600.0))) ** 3)           # fin sand
    pops = grains(r, n, 140, 0.0, 0.30, 0.45, 3000, 11000, 0.00015, 0.0006)                  # statisk knitring
    tone = np.zeros(n)
    for f0 in (5200, 6900, 8300, 10100):                                                     # svakt skimmer som stiger
        trem = 0.5 + 0.5 * unit(cn(r, n, ("lp", 14.0)))
        tone += np.sin(ph_of(f0 * (1 + 0.06 * u)) + r.uniform(0, 6.283)) * trem
    x = 0.45 * unit(fizz) + 0.4 * unit(air) + 0.9 * unit(sand) + 0.5 * unit(pops) + 0.12 * unit(tone)
    return filt(x, ("lp", 8500)) * np.minimum(1.0, t / 0.004) * np.exp(-t / 0.36) * (1 - u ** 2)


def g_tall_stagger(r, n):
    t = tsec(n)
    u = t / (n / SR)
    whomp = sweep(n, 120, 52, 0.06) * dec(n, 0.11, 0.012)
    f = (72 - 18 * u) * (1 + 0.03 * unit(cn(r, n, ("lp", 25.0))))                            # ustø, dyp grunntone
    ph = ph_of(f)
    src = saw(ph, 70, 3000.0) * (1 + 0.6 * np.sin(0.5 * ph))                                 # undertone gir rasp
    sh = 1.0 - 0.2 * u
    growl = svf(src, 250 * sh, 4.0, "bp") / 4 + 0.7 * svf(src, 640 * sh, 5.0, "bp") / 5 + 0.3 * svf(src, 1150 * sh, 4.0, "bp") / 4
    growl = growl * np.minimum(1.0, t / 0.03) * np.exp(-t / 0.13)
    thud = cn(r, n, ("lp", 320), ("lp", 320)) * np.exp(-t / 0.05)
    huff = cn(r, n, ("bp", 900, 0.7)) * np.minimum(1.0, t / 0.04) * np.exp(-t / 0.10)
    x = 0.55 * unit(whomp) + 1.0 * unit(growl) + 0.5 * unit(thud) + 0.12 * unit(huff)
    return sat(x, 1.8)


# ---------- bølgene, grensesnittet, spilleren ----------

def g_wave_sting(r, n):
    t = tsec(n)
    total = n / SR
    u = t / total
    hit = sweep(n, 140, 55, 0.06, math.pi / 2) * dec(n, 0.22, 0.001)                         # lavt slag
    thud = cn(r, n, ("lp", 500), ("lp", 500)) * np.exp(-t / 0.05)
    drift = 1 + 0.002 * unit(cn(r, n, ("lp", 3.0)))                                          # analog ustøhet
    vib = 1 + (0.004 * u ** 2) * np.sin(2 * np.pi * 6.3 * t)
    fa = 110.0 * (155.56 / 110.0) ** (u ** 1.6) * drift * vib                                # A stiger til tritonus
    a = saw(ph_of(fa * 1.004), 160.0) + saw(ph_of(fa * 0.996), 160.0)                        # to svevende sagtenner
    sub = square(ph_of(55.0 * drift), 56.0, 3000.0)                                          # bassen blir liggende
    rise = np.clip((u - 0.45) / 0.55, 0.0, 1.0)
    c = saw(ph_of(440.0 * (466.16 / 440.0) ** rise * drift), 470.0) * rise ** 1.5            # tynn topp som klemmer seg inn
    tone = svf(0.5 * a + 0.45 * sub + 0.3 * c, 280.0 * (3600.0 / 280.0) ** (u ** 1.2), 3.2, "lp")  # filteret åpner seg
    tone = tone * np.minimum(1.0, t / 0.03) * (0.22 + 0.78 * u ** 1.4) * np.minimum(1.0, (total - t) / 0.035)
    x = unit(hit) + 0.5 * unit(thud) + 0.8 * sat(unit(tone), 1.5)
    return sat(x, 1.1)


def g_wave_clear(r, n):
    x = np.zeros(n)
    n1 = int(0.24 * SR)
    n2 = n - int(0.22 * SR)
    add(x, note(n1, 220.0, 0.11) * np.minimum(1.0, (n1 / SR - tsec(n1)) / 0.03), 0.0, 0.8)   # kvint ...
    add(x, note(n2, 293.66, 0.26), 0.21, 1.0)                                                # ... til grunntone
    add(x, note(n2, 146.83, 0.30, 1400.0), 0.21, 0.35)
    add(x, note(n2, 369.99, 0.22), 0.215, 0.22)                                              # svak ters og kvint: oppløst
    add(x, note(n2, 440.0, 0.20), 0.22, 0.16)
    return x


def g_heartbeat(r, n):
    x = np.zeros(n)
    m = int(0.25 * SR)

    def beat(f0, f1, tau):
        b = sweep(m, f0, f1, 0.03) * dec(m, tau, 0.008)
        nz = cn(r, m, ("lp", 110), ("lp", 110)) * dec(m, tau * 0.7, 0.006)
        return unit(b) + 0.25 * unit(nz)

    add(x, beat(66, 46, 0.050), 0.0, 1.0)      # lub
    add(x, beat(82, 56, 0.038), 0.27, 0.72)    # dub
    return sat(x, 1.25)


def g_hit_marker(r, n):
    t = tsec(n)
    return (np.sin(2 * np.pi * 2600 * t) * dec(n, 0.004, 0.0002)
            + 0.35 * np.sin(2 * np.pi * 3900 * t) * dec(n, 0.0025, 0.0002)
            + 0.3 * click(r, n, 2800, 0.0008, 0.6))


def g_kill_marker(r, n):
    t = tsec(n)
    x = (np.sin(2 * np.pi * 1150 * t) * dec(n, 0.009, 0.0003)
         + 0.8 * np.sin(2 * np.pi * 620 * t) * dec(n, 0.014, 0.0004)
         + 0.4 * np.sin(2 * np.pi * 1730 * t) * dec(n, 0.006, 0.0002)
         + 0.6 * sweep(n, 300, 210, 0.006) * dec(n, 0.013, 0.0005)
         + 0.35 * click(r, n, 2000, 0.001, 0.6))
    return sat(x, 1.3)


def g_pickup(r, n):
    x = np.zeros(n)
    m = int(0.06 * SR)
    latch = (click(r, m, 2900, 0.0014) + 0.8 * metal(r, m, (1900, 3100, 4400), 0.008)
             + 0.6 * sweep(m, 360, 290, 0.005, math.pi / 2) * dec(m, 0.012, 0.0005))
    add(x, unit(latch), 0.0, 0.8)                                                            # låsen
    k = int(0.12 * SR)
    rustle = cn(r, k, ("bp", 1300, 0.6)) * env_trap(k, 0.03, 0.06) * (0.5 + 0.5 * unit(np.abs(cn(r, k, ("lp", 60.0)))))
    add(x, unit(rustle), 0.02, 0.10)
    add(x, note(n - int(0.09 * SR), 587.33, 0.085, 3200.0), 0.09, 0.55)                      # myk bekreftelse oppover
    add(x, note(n - int(0.20 * SR), 880.0, 0.11, 3600.0), 0.20, 0.6)
    return x


def g_player_hurt(r, n):
    t = tsec(n)
    thump = sweep(n, 150, 78, 0.025, math.pi / 2) * dec(n, 0.040, 0.003)
    body = cn(r, n, ("lp", 380), ("lp", 380)) * np.exp(-t / 0.030)
    smack = cn(r, n, ("bp", 520, 0.8)) * np.exp(-t / 0.014)
    cloth = (cn(r, n, ("bp", 2400, 0.5)) * np.minimum(1.0, t / 0.010) * np.exp(-t / 0.05)
             * (0.5 + 0.5 * unit(np.abs(cn(r, n, ("lp", 70.0))))))                           # litt tøy
    x = unit(thump) + 0.7 * unit(body) + 0.6 * unit(smack) + 0.16 * unit(cloth)
    return sat(x, 1.7)


def g_ads_in(r, n):
    t = tsec(n)
    sw = np.sin(np.pi * np.clip(t / (n / SR), 0, 1)) ** 1.5                                  # stoffet rører seg og faller til ro
    rustle = cn(r, n, ("bp", 1200, 0.5), ("lp", 4500)) * sw * (0.45 + 0.55 * unit(np.abs(cn(r, n, ("lp", 90.0)))))
    low = cn(r, n, ("bp", 380, 0.7)) * sw
    x = unit(rustle) + 0.35 * unit(low)
    m = int(0.02 * SR)
    for tt, g in ((0.0, 0.24), (0.038, 0.15), (0.061, 0.21), (0.097, 0.12)):                 # stroppefestet skrangler så vidt
        add(x, unit(click(r, m, 4200, 0.0008) + 0.8 * metal(r, m, (2800, 4400, 6100), 0.003)), tt, g)
    return x


# navn, min ms, maks ms, lengde s, topp dBFS, utfading s, generator (rekkefølgen er også lytterekkefølgen)
SPEC = (
    ("shot_transient", 40, 70, 0.060, -1.0, 0.015, g_shot_transient),
    ("shot_body", 220, 320, 0.280, -1.0, 0.060, g_shot_body),
    ("shot_tail_yard", 1000, 1600, 1.450, -4.0, 0.300, g_shot_tail_yard),
    ("shot_tail_walls", 600, 1000, 0.800, -3.0, 0.200, g_shot_tail_walls),
    ("shot_mech", 60, 100, 0.085, -5.5, 0.015, g_shot_mech),
    ("dry_click", 50, 90, 0.070, -1.0, 0.020, g_dry_click),
    ("mag_out", 250, 400, 0.340, -1.0, 0.020, g_mag_out),
    ("mag_in", 250, 400, 0.340, -1.0, 0.030, g_mag_in),
    ("bolt_pull", 180, 260, 0.240, -1.0, 0.020, g_bolt_pull),
    ("bolt_release", 120, 200, 0.170, -1.0, 0.040, g_bolt_release),
    ("casing_1", 200, 350, 0.300, -5.0, 0.030, g_casing_1),
    ("casing_2", 200, 350, 0.250, -5.0, 0.030, g_casing_2),
    ("casing_3", 200, 350, 0.330, -5.0, 0.030, g_casing_3),
    ("impact_dirt", 300, 500, 0.400, -1.0, 0.080, g_impact_dirt),
    ("impact_metal", 300, 500, 0.450, -1.0, 0.100, g_impact_metal),
    ("impact_concrete", 300, 500, 0.380, -1.0, 0.080, g_impact_concrete),
    ("alien_hit", 250, 400, 0.330, -1.0, 0.060, g_alien_hit),
    ("alien_death", 800, 1200, 1.050, -1.0, 0.120, g_alien_death),
    ("alien_dissolve", 1000, 1400, 1.250, -1.0, 0.200, g_alien_dissolve),
    ("tall_stagger", 400, 600, 0.520, -1.0, 0.120, g_tall_stagger),
    ("wave_sting", 1300, 1800, 1.600, -1.0, 0.035, g_wave_sting),
    ("wave_clear", 800, 1200, 1.000, -3.0, 0.250, g_wave_clear),
    ("heartbeat", 750, 850, 0.800, -1.0, 0.050, g_heartbeat),
    ("hit_marker", 30, 50, 0.040, -1.0, 0.010, g_hit_marker),
    ("kill_marker", 70, 110, 0.095, -1.0, 0.025, g_kill_marker),
    ("pickup", 350, 550, 0.480, -1.0, 0.100, g_pickup),
    ("player_hurt", 250, 350, 0.300, -1.0, 0.070, g_player_hurt),
    ("ads_in", 120, 180, 0.160, -1.0, 0.030, g_ads_in),
)
NAMES = tuple(s[0] for s in SPEC)


def render(name, length, peak_db, fade_out, gen):
    """Lag én lyd: forløp, DC-sperre, fading og topp."""
    r = np.random.default_rng([SEED, zlib.crc32(name.encode("ascii"))])
    total = int(round(length * SR))
    pre = int(PRE * SR)
    x = np.concatenate([np.zeros(pre), np.asarray(gen(r, total - pre), dtype=float)])
    x = _biquad(x, *_rbj("hp", 20.0), pad=0.5)                 # ingen DC
    nfi = int(0.003 * SR)
    x[:nfi] *= np.linspace(0.0, 1.0, nfi, endpoint=False)
    nfo = max(8, int(fade_out * SR))
    x[-nfo:] *= 0.5 * (1.0 + np.cos(np.linspace(0.0, np.pi, nfo)))
    if not np.all(np.isfinite(x)):
        raise RuntimeError("NaN/inf i " + name)
    return x * (10 ** (peak_db / 20.0) / float(np.max(np.abs(x))))


def write_wav(path, x):
    pcm = np.clip(np.round(x * 32767.0), -32768, 32767).astype("<i2")
    with wave.open(path, "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(SR)
        w.writeframes(pcm.tobytes())


def check(path, lo_ms, hi_ms):
    """Les fila tilbake og mål: lengde, topp, start (første sample over -30 dB av toppen), DC, format."""
    with wave.open(path, "rb") as w:
        fmt_ok = (w.getnchannels(), w.getframerate(), w.getsampwidth()) == (1, SR, 2)
        x = np.frombuffer(w.readframes(w.getnframes()), dtype="<i2").astype(np.float64) / 32768.0
    ms = 1000.0 * len(x) / SR
    pk = float(np.max(np.abs(x)))
    pk_db = 20 * math.log10(pk) if pk > 0 else -200.0
    onset_ms = 1000.0 * int(np.argmax(np.abs(x) >= pk * 10 ** (-30 / 20.0))) / SR
    dc = float(np.mean(x))
    ok = (fmt_ok and bool(np.all(np.isfinite(x))) and lo_ms <= ms <= hi_ms and -6.0 <= pk_db <= -0.5
          and onset_ms <= 5.0 and abs(dc) < 0.002 and x[0] == 0.0 and abs(x[-1]) <= 2 / 32768.0)
    return ok, ms, pk_db, onset_ms, dc


def main(argv):
    out = argv[1] if len(argv) > 1 else DEFAULT_OUT
    os.makedirs(out, exist_ok=True)
    bad = 0
    size = 0
    print("%-16s %8s %8s %8s %9s" % ("navn", "ms", "topp dB", "start ms", "DC"))
    for name, lo, hi, length, peak_db, fade_out, gen in SPEC:
        path = os.path.join(out, name + ".wav")
        write_wav(path, render(name, length, peak_db, fade_out, gen))
        ok, ms, pk_db, onset_ms, dc = check(path, lo, hi)
        size += os.path.getsize(path)
        bad += 0 if ok else 1
        print("%-16s %8.1f %8.2f %8.2f %9.5f  %s" % (name, ms, pk_db, onset_ms, dc, "ok" if ok else "FEIL"))
    print("%d filer, %.2f MB, %d feil -> %s" % (len(SPEC), size / 1e6, bad, out))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
